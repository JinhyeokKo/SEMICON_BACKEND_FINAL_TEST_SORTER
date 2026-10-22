#include "sensor_frame_pack.h"

#include <stddef.h>

esp_err_t sensor_frame_next_generation(uint16_t previous, uint16_t *next)
{
    if (next == NULL || previous > SENSOR_FRAME_GENERATION_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    *next = previous >= SENSOR_FRAME_GENERATION_MAX
        ? 1U : (uint16_t)(previous + 1U);
    return ESP_OK;
}

esp_err_t sensor_frame_pack(
    const uint16_t raw[SENSOR_FRAME_RAW_REGISTER_COUNT],
    uint16_t generation,
    uint16_t registers[SENSOR_FRAME_REGISTER_COUNT])
{
    if (raw == NULL || registers == NULL
            || generation == 0 || generation > SENSOR_FRAME_GENERATION_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 입력과 출력이 겹쳐도 아직 읽지 않은 입력을 덮어쓰지 않습니다. */
    uint16_t snapshot[SENSOR_FRAME_RAW_REGISTER_COUNT];
    for (unsigned i = 0; i < SENSOR_FRAME_RAW_REGISTER_COUNT; ++i) {
        snapshot[i] = raw[i];
    }
    for (unsigned i = 0; i < SENSOR_FRAME_RAW_REGISTER_COUNT; ++i) {
        registers[i] = snapshot[i];
        registers[SENSOR_FRAME_PACKED_OFFSET + 2U * i] = generation;
        registers[SENSOR_FRAME_PACKED_OFFSET + 2U * i + 1U] = snapshot[i];
    }
    return ESP_OK;
}
