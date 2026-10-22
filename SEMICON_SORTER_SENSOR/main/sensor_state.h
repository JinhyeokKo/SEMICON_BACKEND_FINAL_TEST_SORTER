#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dht22_sensor.h"
#include "esp_err.h"

#define SENSOR_MAP_VERSION 1U
#define SENSOR_COUNTER_MAX 32000U
#define SENSOR_AGE_MAX 32000U
#define SENSOR_AGE_UNIT_US 100000ULL
#define SENSOR_STALE_AFTER_US 6000000ULL
#define SENSOR_REGISTER_COUNT 8U

typedef enum {
    SENSOR_REG_MAP_VERSION = 0,
    SENSOR_REG_BOOT_ID,
    SENSOR_REG_SAMPLE_SEQ,
    SENSOR_REG_TEMP_X100,
    SENSOR_REG_RH_X100,
    SENSOR_REG_VALID,
    SENSOR_REG_STATUS,
    SENSOR_REG_AGE_100MS,
} sensor_register_index_t;

/* 순수 상태 모델. 공유할 때 호출부가 같은 mutex로 모든 접근을 보호한다. */
typedef struct {
    uint16_t boot_id;
    uint16_t sample_seq;
    int16_t temp_x100;
    uint16_t rh_x100;
    uint16_t status;
    bool have_good_sample;
    uint64_t last_good_us;
} sensor_state_t;

esp_err_t sensor_state_init(sensor_state_t *state, uint16_t boot_id);
esp_err_t sensor_state_apply_reading(sensor_state_t *state,
                                     const dht22_reading_t *reading,
                                     uint64_t now_us);
esp_err_t sensor_state_get_registers(const sensor_state_t *state,
                                     uint64_t now_us,
                                     uint16_t registers[SENSOR_REGISTER_COUNT]);
