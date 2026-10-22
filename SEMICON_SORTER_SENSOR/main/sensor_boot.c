#include "sensor_boot.h"

#include <stddef.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "sensor_state.h"

esp_err_t sensor_boot_next_id(uint16_t *out_boot_id)
{
    if (out_boot_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* BOOT_ID와 다른 NVS 설정을 자동으로 지우지 않는다. */
    esp_err_t error = nvs_flash_init();
    if (error != ESP_OK) {
        return error;
    }
    nvs_handle_t handle;
    error = nvs_open("sensor", NVS_READWRITE, &handle);
    if (error != ESP_OK) {
        return error;
    }

    uint16_t previous = 0;
    uint16_t next = 1;
    error = nvs_get_u16(handle, "boot_id", &previous);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        error = ESP_OK;
    } else if (error == ESP_OK) {
        if (previous == 0 || previous > SENSOR_COUNTER_MAX) {
            error = ESP_ERR_INVALID_STATE;
        } else {
            next = previous >= SENSOR_COUNTER_MAX ? 1U : (uint16_t)(previous + 1U);
        }
    }
    if (error == ESP_OK) {
        error = nvs_set_u16(handle, "boot_id", next);
    }
    if (error == ESP_OK) {
        error = nvs_commit(handle);
    }
    nvs_close(handle);
    if (error == ESP_OK) {
        *out_boot_id = next;
    }
    return error;
}
