#include "sensor_state.h"

#include <stddef.h>

esp_err_t sensor_state_init(sensor_state_t *state, uint16_t boot_id)
{
    if (state == NULL || boot_id == 0 || boot_id > SENSOR_COUNTER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    *state = (sensor_state_t){
        .boot_id = boot_id,
        .status = DHT22_STATUS_WAITING,
    };
    return ESP_OK;
}

esp_err_t sensor_state_apply_reading(sensor_state_t *state,
                                     const dht22_reading_t *reading,
                                     uint64_t now_us)
{
    if (state == NULL || reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (state->boot_id == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (state->have_good_sample && now_us < state->last_good_us) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!reading->valid) {
        /* 실패한 숫자나 시각을 복사하지 않는다. */
        state->status = reading->status >= DHT22_STATUS_READ_ERROR
                && reading->status <= DHT22_STATUS_RANGE_ERROR
            ? reading->status : DHT22_STATUS_READ_ERROR;
        return ESP_OK;
    }
    if (reading->status != DHT22_STATUS_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    if (reading->temp_x100 < -4000 || reading->temp_x100 > 8000
            || reading->rh_x100 > 10000) {
        state->status = DHT22_STATUS_RANGE_ERROR;
        return ESP_OK;
    }

    state->temp_x100 = reading->temp_x100;
    state->rh_x100 = reading->rh_x100;
    state->sample_seq = state->sample_seq >= SENSOR_COUNTER_MAX
        ? 1U : (uint16_t)(state->sample_seq + 1U);
    state->last_good_us = now_us;
    state->have_good_sample = true;
    state->status = DHT22_STATUS_OK;
    return ESP_OK;
}

esp_err_t sensor_state_get_registers(const sensor_state_t *state,
                                     uint64_t now_us,
                                     uint16_t registers[SENSOR_REGISTER_COUNT])
{
    if (state == NULL || registers == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (state->boot_id == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (state->have_good_sample && now_us < state->last_good_us) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t age = SENSOR_AGE_MAX;
    uint16_t status = state->status;
    bool valid = state->have_good_sample && status == DHT22_STATUS_OK;
    if (state->have_good_sample) {
        const uint64_t elapsed_us = now_us - state->last_good_us;
        const uint64_t units = elapsed_us / SENSOR_AGE_UNIT_US;
        age = units >= SENSOR_AGE_MAX ? SENSOR_AGE_MAX : (uint16_t)units;
        if (valid && elapsed_us >= SENSOR_STALE_AFTER_US) {
            valid = false;
            status = DHT22_STATUS_STALE;
        }
    }

    registers[SENSOR_REG_MAP_VERSION] = SENSOR_MAP_VERSION;
    registers[SENSOR_REG_BOOT_ID] = state->boot_id;
    registers[SENSOR_REG_SAMPLE_SEQ] = state->sample_seq;
    /* 음수 온도도 동일한 16비트 2의 보수 표현으로 공개한다. */
    registers[SENSOR_REG_TEMP_X100] = (uint16_t)state->temp_x100;
    registers[SENSOR_REG_RH_X100] = state->rh_x100;
    registers[SENSOR_REG_VALID] = valid ? 1U : 0U;
    registers[SENSOR_REG_STATUS] = status;
    registers[SENSOR_REG_AGE_100MS] = age;
    return ESP_OK;
}
