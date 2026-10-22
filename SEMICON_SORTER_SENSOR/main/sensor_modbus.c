#include "sensor_modbus.h"
#include "sensor_frame_pack.h"
#include "sensor_wifi.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "mbcontroller.h" // IWYU pragma: keep

#define REGISTER_REFRESH_MS 100U
#define SERVER_RETRY_MS 5000U
#define NOTIFICATION_DRAIN_LIMIT 32U

static const char *TAG = "sensor_modbus";
static TaskHandle_t s_server_task;
static sensor_regs_provider_t s_provider;
_Static_assert(SENSOR_MODBUS_REGISTER_COUNT == SENSOR_FRAME_RAW_REGISTER_COUNT,
               "The provider must keep its original eight-register contract");
/* 이 배열의 쓰기는 서버 작업만, 읽기는 esp-modbus 내부 작업만 수행합니다.
 * 실행 중 쓰기/읽기는 동일한 mbc_slave_lock 임계 구역으로 보호합니다. */
static uint16_t s_input_registers[SENSOR_FRAME_REGISTER_COUNT];

static esp_err_t keep_only_input_read(void *handle) {
  /* v2.1.3의 기본 함수 목록입니다. 공식 공개 API로 FC04만 남깁니다. */
  static const uint8_t unused_functions[] = {0x01, 0x02, 0x03, 0x05, 0x06,
                                             0x0f, 0x10, 0x11, 0x17};
  for (size_t i = 0; i < sizeof(unused_functions); ++i) {
    const esp_err_t err = mbc_delete_handler(handle, unused_functions[i]);
    /* 빌드 설정에 의해 해당 함수가 없는 경우도 정상입니다. */
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      return err;
    }
  }
  uint16_t count = 0;
  esp_err_t err = mbc_get_handler_count(handle, &count);
  if (err != ESP_OK) {
    return err;
  }
  mb_fn_handler_fp input_handler = NULL;
  err = mbc_get_handler(handle, 0x04, &input_handler);
  return err == ESP_OK && input_handler != NULL && count == 1
             ? ESP_OK
             : ESP_ERR_INVALID_STATE;
}

/* 호출 시 Modbus lock을 잡고 있으면 안 됩니다.
 * esp-modbus가 자체 작업과 소켓을 정리하고 controller 메모리를 해제합니다. */
static esp_err_t delete_server(void **handle) {
  if (*handle == NULL) {
    return ESP_OK;
  }
  const esp_err_t err = mbc_slave_delete(*handle);
  /* v2.1.3은 delete의 하위 정리가 실패해도 controller를 해제합니다. */
  *handle = NULL;
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Server cleanup failed: %s; automatic restart disabled",
             esp_err_to_name(err));
  }
  return err;
}

static esp_err_t create_server(void **handle, bool *constructor_failed) {
  *constructor_failed = false;
  mb_communication_info_t config = {
      .tcp_opts.port = SENSOR_MODBUS_PORT,
      .tcp_opts.mode = MB_TCP,
      .tcp_opts.addr_type = MB_IPV4,
      .tcp_opts.ip_addr_table = NULL,
      .tcp_opts.ip_netif_ptr = sensor_wifi_get_netif(),
      .tcp_opts.uid = SENSOR_MODBUS_UNIT_ID,
  };
  esp_err_t err = mbc_slave_create_tcp(&config, handle);
  if (err != ESP_OK || *handle == NULL) {
    /* v2.1.3 생성 실패는 내부 부분 생성 자원을 호출자에게 노출하지 않을 수
     * 있습니다. 이 경로에서 반복 생성하면 메모리가 누적될 수 있어 중단합니다.
     */
    *constructor_failed = true;
    return err == ESP_OK ? ESP_ERR_INVALID_STATE : err;
  }
  err = keep_only_input_read(*handle);
  if (err != ESP_OK) {
    return err;
  }
  const mb_register_area_descriptor_t area = {
      .start_offset = 0,
      .type = MB_PARAM_INPUT,
      .access = MB_ACCESS_RO,
      .address = s_input_registers,
      /* API는 워드 수가 아닌 바이트 수를 받습니다: 24워드 = 48바이트. */
      .size = sizeof(s_input_registers),
  };
  err = mbc_slave_set_descriptor(*handle, area);
  if (err != ESP_OK) {
    return err;
  }
  return mbc_slave_start(*handle);
}

static void drain_read_notifications(void *handle) {
  /* FC04마다 생성되는 접근 알림을 소비합니다. 화면/센서 상태는 변경하지
   * 않습니다. */
  mb_param_info_t info;
  for (unsigned i = 0; i < NOTIFICATION_DRAIN_LIMIT; ++i) {
    if (mbc_slave_get_param_info(handle, &info, 0) != ESP_OK) {
      break;
    }
  }
}

static void modbus_server_task(void *arg) {
  (void)arg;
  void *handle = NULL;
  uint32_t server_generation = 0;
  /* Wi-Fi 서버가 재생성되어도 같은 부팅의 발행 번호는 이어갑니다. */
  uint16_t publication_generation = 0;
  uint16_t snapshot[SENSOR_MODBUS_REGISTER_COUNT];
  uint16_t packed[SENSOR_FRAME_REGISTER_COUNT];
  for (;;) {
    uint32_t network_generation = 0;
    const bool connected = sensor_wifi_get_link_state(&network_generation);
    if (handle != NULL && server_generation != network_generation) {
      if (delete_server(&handle) != ESP_OK) {
        break;
      }
      ESP_LOGI(TAG, "Network generation changed; previous TCP server released");
    }
    if (!connected) {
      if (handle != NULL) {
        if (delete_server(&handle) != ESP_OK) {
          break;
        }
        ESP_LOGI(TAG, "Server stopped until Wi-Fi obtains an IP address");
      }
      vTaskDelay(pdMS_TO_TICKS(REGISTER_REFRESH_MS));
      continue;
    }

    /* 콜백은 상태 모듈의 mutex를 사용할 수 있습니다.
     * Modbus lock 밖에서 호출하여 두 lock 사이 교착을 피합니다. */
    esp_err_t err = s_provider(snapshot);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Register snapshot failed: %s; no old data served",
               esp_err_to_name(err));
      if (delete_server(&handle) != ESP_OK) {
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(SERVER_RETRY_MS));
      continue;
    }

    uint16_t next_publication_generation = 0;
    err = sensor_frame_next_generation(publication_generation,
                                       &next_publication_generation);
    if (err == ESP_OK) {
      err = sensor_frame_pack(snapshot, next_publication_generation, packed);
    }
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Frame packing failed: %s; no old data served",
               esp_err_to_name(err));
      if (delete_server(&handle) != ESP_OK) {
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(SERVER_RETRY_MS));
      continue;
    }

    if (handle == NULL) {
      /* 아직 서버 작업이 이 배열을 읽지 않으므로 잠금 없이 초기화합니다. */
      /* 발행 직전에 번호를 소비합니다. 이후 서버 생성/정리가 실패해도
       * 같은 번호로 다른 데이터가 다시 공개되지 않게 합니다. */
      publication_generation = next_publication_generation;
      memcpy(s_input_registers, packed, sizeof(s_input_registers));
      bool constructor_failed = false;
      err = create_server(&handle, &constructor_failed);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Server start failed: %s", esp_err_to_name(err));
        if (delete_server(&handle) != ESP_OK) {
          break;
        }
        if (constructor_failed) {
          ESP_LOGE(
              TAG,
              "Controller construction failed; automatic restart disabled");
          break;
        }
        ESP_LOGW(TAG, "Retry server start in 5 s");
        vTaskDelay(pdMS_TO_TICKS(SERVER_RETRY_MS));
        continue;
      }
      /* 생성 도중 네트워크 이벤트가 왔으면 다음 주기에 세대 차이로 정리합니다.
       */
      server_generation = network_generation;
      ESP_LOGI(TAG,
               "TCP %u ready: Unit ID %u, FC04 offset 0, count 24, PUB_GEN=%u; "
               "legacy offset 0/count 8 supported",
               (unsigned)SENSOR_MODBUS_PORT, (unsigned)SENSOR_MODBUS_UNIT_ID,
               (unsigned)publication_generation);
    } else {
      err = mbc_slave_lock(handle);
      if (err == ESP_OK) {
        /* 원시 8워드와 번호를 묶은 8개 값을 함께 발행합니다.
         * 임계 구역 안에는 48바이트 복사만 둡니다. */
        publication_generation = next_publication_generation;
        memcpy(s_input_registers, packed, sizeof(s_input_registers));
        err = mbc_slave_unlock(handle);
      }
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Register publication failed: %s", esp_err_to_name(err));
        if (delete_server(&handle) != ESP_OK) {
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(SERVER_RETRY_MS));
        continue;
      }
    }
    drain_read_notifications(handle);
    vTaskDelay(pdMS_TO_TICKS(REGISTER_REFRESH_MS));
  }
  /* 정리가 실패했을 때 중복 서버를 만들지 않습니다. 센서 읽기는 계속
   * 실행됩니다. */
  vTaskDelete(NULL);
}

esp_err_t sensor_modbus_start(sensor_regs_provider_t provider) {
  if (provider == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  /* app_main에서만 직렬로 초기화하며, 생성된 작업은 애플리케이션 동안
   * 유지합니다. */
  if (s_server_task != NULL || sensor_wifi_get_netif() == NULL) {
    return ESP_ERR_INVALID_STATE;
  }
  s_provider = provider;
  if (xTaskCreate(modbus_server_task, "sensor_modbus", 4096, NULL, 4,
                  &s_server_task) != pdPASS) {
    s_provider = NULL;
    s_server_task = NULL;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}
