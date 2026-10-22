#include "sensor_modbus.h"
#include "sensor_service.h"
#include "sensor_wifi.h"

#include "esp_log.h"

static const char *TAG = "APP";

_Static_assert(SENSOR_REGISTER_COUNT == SENSOR_MODBUS_REGISTER_COUNT,
               "State map and Modbus register count must match");

void app_main(void) {
  /* BOOT_ID를 먼저 저장하고 센서 작업을 시작한다. */
  esp_err_t error = sensor_service_start();
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "Sensor service initialization failed: %s",
             esp_err_to_name(error));
    return;
  }
  error = sensor_wifi_start();
  if (error == ESP_ERR_NOT_FOUND) {
    /* SSID 미설정 상태에서도 센서 코드와 빌드는 사용할 수 있다. */
    return;
  }
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "Wi-Fi unavailable: %s; sensor acquisition continues",
             esp_err_to_name(error));
    return;
  }
  error = sensor_modbus_start(sensor_service_get_registers);
  if (error != ESP_OK) {
    ESP_LOGE(TAG,
             "Modbus initialization failed: %s; sensor acquisition continues",
             esp_err_to_name(error));
  }
}
