#include "sensor_wifi.h"

#include <stddef.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_RETRY_BIT BIT1
#define WIFI_READY_BIT BIT2
#define WIFI_RETRY_DELAY_MS 5000U

static const char *TAG = "sensor_wifi";
static EventGroupHandle_t s_events;
static esp_netif_t *s_sta_netif;
static TaskHandle_t s_retry_task;
static esp_event_handler_instance_t s_wifi_handler;
static esp_event_handler_instance_t s_ip_handler;
static portMUX_TYPE s_link_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_has_ip;
static uint32_t s_link_generation;

static void link_changed(bool has_ip) {
  portENTER_CRITICAL(&s_link_lock);
  s_has_ip = has_ip;
  ++s_link_generation;
  portEXIT_CRITICAL(&s_link_lock);
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id,
                               void *event_data) {
  (void)arg;
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
    xEventGroupSetBits(s_events, WIFI_READY_BIT | WIFI_RETRY_BIT);
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    const wifi_event_sta_disconnected_t *event = event_data;
    link_changed(false);
    xEventGroupClearBits(s_events, WIFI_CONNECTED_BIT);
    xEventGroupSetBits(s_events, WIFI_RETRY_BIT);
    ESP_LOGW(TAG, "Disconnected (reason=%u); retry in 5 s",
             event ? (unsigned)event->reason : 0U);
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_STOP) {
    link_changed(false);
    xEventGroupClearBits(s_events, WIFI_CONNECTED_BIT | WIFI_READY_BIT);
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    const ip_event_got_ip_t *event = event_data;
    link_changed(true);
    xEventGroupSetBits(s_events, WIFI_CONNECTED_BIT);
    ESP_LOGI(TAG, "IP address: " IPSTR, IP2STR(&event->ip_info.ip));
  } else if (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP) {
    /* 무선 연결을 유지하면서 DHCP 주소를 다시 받는 경우도 있습니다.
     * Modbus는 IP 재취득까지 멈추고 센서 읽기는 계속 실행합니다. */
    link_changed(false);
    xEventGroupClearBits(s_events, WIFI_CONNECTED_BIT);
  }
}

static void wifi_retry_task(void *arg) {
  (void)arg;
  for (;;) {
    (void)xEventGroupWaitBits(s_events, WIFI_RETRY_BIT, pdTRUE, pdFALSE,
                              portMAX_DELAY);
    /* 이벤트 콜백에서 연결을 반복하지 않습니다. 시작/끊김 후 5초 간격을 둡니다.
     */
    vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
    const EventBits_t bits = xEventGroupGetBits(s_events);
    if ((bits & WIFI_READY_BIT) == 0 || (bits & WIFI_CONNECTED_BIT) != 0) {
      continue;
    }
    const esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "Connection request failed: %s", esp_err_to_name(err));
      xEventGroupSetBits(s_events, WIFI_RETRY_BIT);
    }
  }
}

esp_err_t sensor_wifi_start(void) {
  /* app_main의 직렬 초기화 경로에서만 호출합니다. */
  if (s_events != NULL) {
    return ESP_ERR_INVALID_STATE;
  }
  const size_t ssid_len = strlen(CONFIG_SENSOR_WIFI_SSID);
  const size_t password_len = strlen(CONFIG_SENSOR_WIFI_PASSWORD);
  if (ssid_len == 0) {
    ESP_LOGI(TAG,
             "SSID is empty; Wi-Fi/Modbus disabled, sensor task continues");
    return ESP_ERR_NOT_FOUND;
  }
  wifi_config_t station = {0};
  if (ssid_len > sizeof(station.sta.ssid) ||
      password_len > sizeof(station.sta.password) ||
      (password_len > 0 && password_len < 8)) {
    ESP_LOGE(TAG,
             "Invalid SSID/password length; check Sensor network menuconfig");
    return ESP_ERR_INVALID_ARG;
  }
  memcpy(station.sta.ssid, CONFIG_SENSOR_WIFI_SSID, ssid_len);
  memcpy(station.sta.password, CONFIG_SENSOR_WIFI_PASSWORD, password_len);
  station.sta.threshold.authmode =
      password_len == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
  station.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
  station.sta.pmf_cfg.capable = true;
  station.sta.pmf_cfg.required = false;

  s_events = xEventGroupCreate();
  if (s_events == NULL) {
    return ESP_ERR_NO_MEM;
  }
  bool own_event_loop = false;
  bool driver_initialized = false;
  bool wifi_registered = false;
  bool ip_registered = false;
  bool driver_started = false;

  esp_err_t err = esp_netif_init();
  if (err != ESP_OK) {
    goto fail;
  }
  err = esp_event_loop_create_default();
  if (err == ESP_OK) {
    own_event_loop = true;
  } else if (err != ESP_ERR_INVALID_STATE) {
    goto fail;
  }
  s_sta_netif = esp_netif_create_default_wifi_sta();
  if (s_sta_netif == NULL) {
    err = ESP_ERR_NO_MEM;
    goto fail;
  }
  const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&init_config);
  if (err != ESP_OK) {
    goto fail;
  }
  driver_initialized = true;
  err = esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &s_wifi_handler);
  if (err != ESP_OK) {
    goto fail;
  }
  wifi_registered = true;
  err = esp_event_handler_instance_register(
      IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &s_ip_handler);
  if (err != ESP_OK) {
    goto fail;
  }
  ip_registered = true;
  /* Wi-Fi 설정은 RAM에 두고 BOOT_ID의 NVS 영역을 건드리지 않습니다. */
  err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  if (err != ESP_OK) {
    goto fail;
  }
  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK) {
    goto fail;
  }
  err = esp_wifi_set_config(WIFI_IF_STA, &station);
  if (err != ESP_OK) {
    goto fail;
  }
  err = esp_wifi_start();
  if (err != ESP_OK) {
    goto fail;
  }
  driver_started = true;
  err = esp_wifi_set_ps(WIFI_PS_NONE);
  if (err != ESP_OK) {
    goto fail;
  }
  if (xTaskCreate(wifi_retry_task, "wifi_retry", 4096, NULL, 4,
                  &s_retry_task) != pdPASS) {
    err = ESP_ERR_NO_MEM;
    goto fail;
  }
  ESP_LOGI(
      TAG,
      "Station started; initial connection in 5 s (credentials not logged)");
  return ESP_OK;

fail:
  link_changed(false);
  if (driver_started) {
    (void)esp_wifi_stop();
  }
  if (ip_registered) {
    (void)esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID,
                                                s_ip_handler);
  }
  if (wifi_registered) {
    (void)esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                s_wifi_handler);
  }
  if (driver_initialized) {
    (void)esp_wifi_deinit();
  }
  if (s_sta_netif != NULL) {
    esp_netif_destroy_default_wifi(s_sta_netif);
    s_sta_netif = NULL;
  }
  if (own_event_loop) {
    (void)esp_event_loop_delete_default();
  }
  vEventGroupDelete(s_events);
  s_events = NULL;
  ESP_LOGE(TAG, "Wi-Fi initialization failed: %s; sensor task continues",
           esp_err_to_name(err));
  return err;
}

bool sensor_wifi_is_connected(void) { return sensor_wifi_get_link_state(NULL); }

bool sensor_wifi_get_link_state(uint32_t *generation) {
  portENTER_CRITICAL(&s_link_lock);
  const bool has_ip = s_has_ip;
  if (generation != NULL) {
    *generation = s_link_generation;
  }
  portEXIT_CRITICAL(&s_link_lock);
  return has_ip;
}

esp_netif_t *sensor_wifi_get_netif(void) { return s_sta_netif; }
