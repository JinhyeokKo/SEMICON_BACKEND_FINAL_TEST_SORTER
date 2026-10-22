#include "sensor_service.h"

#include <stddef.h>

#include "dht22_sensor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sensor_boot.h"

static const char *TAG = "SENSOR";
static sensor_state_t s_state;
static SemaphoreHandle_t s_lock;
static TaskHandle_t s_read_task;
static TaskHandle_t s_monitor_task;
static bool s_start_attempted;
static bool s_ready;

esp_err_t sensor_service_get_registers(uint16_t registers[SENSOR_REGISTER_COUNT])
{
    if (registers == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    const esp_err_t error = sensor_state_get_registers(
        &s_state, (uint64_t)esp_timer_get_time(), registers);
    xSemaphoreGive(s_lock);
    return error;
}

static void sensor_read_task(void *arg)
{
    (void)arg;
    dht22_reading_t reading = DHT22_READING_INITIALIZER;
    vTaskDelay(pdMS_TO_TICKS(DHT22_SENSOR_STARTUP_DELAY_MS));
    for (;;) {
        /* GPIO 응답 대기는 상태 mutex 밖에서 실행한다. */
        const esp_err_t read_error = dht22_sensor_read(&reading);
        uint16_t registers[SENSOR_REGISTER_COUNT];
        xSemaphoreTake(s_lock, portMAX_DELAY);
        const uint64_t now_us = (uint64_t)esp_timer_get_time();
        const esp_err_t apply_error = sensor_state_apply_reading(&s_state, &reading, now_us);
        const esp_err_t snapshot_error = sensor_state_get_registers(&s_state, now_us, registers);
        xSemaphoreGive(s_lock);
        if (apply_error != ESP_OK || snapshot_error != ESP_OK) {
            ESP_LOGE(TAG, "State update failed: apply=%s, snapshot=%s",
                     esp_err_to_name(apply_error), esp_err_to_name(snapshot_error));
        } else if (read_error == ESP_OK && registers[SENSOR_REG_VALID] == 1U) {
            ESP_LOGI(TAG, "BOOT=%u SEQ=%u temp=%.1f C rh=%.1f %%RH valid=%u status=%u age=%u",
                     (unsigned)registers[SENSOR_REG_BOOT_ID],
                     (unsigned)registers[SENSOR_REG_SAMPLE_SEQ],
                     reading.temp_x100 / 100.0, reading.rh_x100 / 100.0,
                     (unsigned)registers[SENSOR_REG_VALID],
                     (unsigned)registers[SENSOR_REG_STATUS],
                     (unsigned)registers[SENSOR_REG_AGE_100MS]);
        } else {
            ESP_LOGW(TAG, "Read failed: %s; BOOT=%u SEQ=%u valid=%u status=%u age=%u; values retained",
                     esp_err_to_name(read_error),
                     (unsigned)registers[SENSOR_REG_BOOT_ID],
                     (unsigned)registers[SENSOR_REG_SAMPLE_SEQ],
                     (unsigned)registers[SENSOR_REG_VALID],
                     (unsigned)registers[SENSOR_REG_STATUS],
                     (unsigned)registers[SENSOR_REG_AGE_100MS]);
        }
        vTaskDelay(pdMS_TO_TICKS(DHT22_SENSOR_READ_INTERVAL_MS));
    }
}

static void sensor_monitor_task(void *arg)
{
    (void)arg;
    uint16_t previous_status = DHT22_STATUS_WAITING;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        uint16_t registers[SENSOR_REGISTER_COUNT];
        if (sensor_service_get_registers(registers) == ESP_OK) {
            const uint16_t status = registers[SENSOR_REG_STATUS];
            if (status == DHT22_STATUS_STALE && previous_status != status) {
                ESP_LOGW(TAG, "No new good sample for 6 s: SEQ=%u valid=0 status=5 age=%u",
                         (unsigned)registers[SENSOR_REG_SAMPLE_SEQ],
                         (unsigned)registers[SENSOR_REG_AGE_100MS]);
            }
            previous_status = status;
        }
    }
}

esp_err_t sensor_service_start(void)
{
    if (s_start_attempted) {
        return ESP_ERR_INVALID_STATE;
    }
    s_start_attempted = true;
    uint16_t boot_id = 0;
    esp_err_t error = sensor_boot_next_id(&boot_id);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "BOOT_ID preparation failed: %s; NVS was not erased", esp_err_to_name(error));
        return error;
    }
    error = sensor_state_init(&s_state, boot_id);
    if (error != ESP_OK) {
        return error;
    }
    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }
    s_ready = true;
    if (xTaskCreate(sensor_monitor_task, "sensor_age", 3072, NULL, 3, &s_monitor_task) != pdPASS) {
        error = ESP_ERR_NO_MEM;
    } else if (xTaskCreate(sensor_read_task, "sensor_read", 4096, NULL, 5, &s_read_task) != pdPASS) {
        vTaskDelete(s_monitor_task);
        s_monitor_task = NULL;
        error = ESP_ERR_NO_MEM;
    }
    if (error != ESP_OK) {
        s_ready = false;
        vSemaphoreDelete(s_lock);
        s_lock = NULL;
        return error;
    }
    ESP_LOGI(TAG, "BOOT=%u GPIO=%d startup=%u ms interval=%u ms; map=1 words=8",
             (unsigned)boot_id, (int)DHT22_SENSOR_GPIO,
             (unsigned)DHT22_SENSOR_STARTUP_DELAY_MS, (unsigned)DHT22_SENSOR_READ_INTERVAL_MS);
    return ESP_OK;
}
