#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "sensor_state.h"

/* app_main에서 한 번만 호출한다. NVS, 상태, 읽기·감시 task를 준비한다.
 * 성공 후 서비스는 애플리케이션 수명 동안 유지한다. */
esp_err_t sensor_service_start(void);

/* 전체 8워드를 같은 시각·같은 mutex로 복사한다. GPIO를 직접 읽지 않는다. */
esp_err_t sensor_service_get_registers(uint16_t registers[SENSOR_REGISTER_COUNT]);
