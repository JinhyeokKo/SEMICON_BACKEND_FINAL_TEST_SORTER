#pragma once

#include <stdint.h>
#include "esp_err.h"

#define SENSOR_MODBUS_REGISTER_COUNT 8U
#define SENSOR_MODBUS_PORT 502U
#define SENSOR_MODBUS_UNIT_ID 1U

/* 호출 한 번에 8워드를 같은 상태의 스냅샷으로 채워야 합니다.
 * 콜백은 다른 작업에서 호출되므로 상태 모듈에서 동기화해야 합니다.
 * 콜백 내부에서 Modbus API를 호출하지 마세요. */
typedef esp_err_t (*sensor_regs_provider_t)(uint16_t registers[SENSOR_MODBUS_REGISTER_COUNT]);

/* sensor_wifi_start 성공 후 app_main에서 한 번만 호출합니다.
 * IP 취득 후 TCP 502 서버 시작, Wi-Fi 끊김 시 서버 정리, 재연결 시 재시작.
 * FC04 Input Registers offset 0..23을 제공합니다. 기존 offset 0..7의
 * 원시 8워드는 유지하고 offset 8..23에 발행 번호를 묶은 DWORD 8개를
 * 추가합니다. 센서 작업을 기다리지 않습니다. */
esp_err_t sensor_modbus_start(sensor_regs_provider_t provider);
