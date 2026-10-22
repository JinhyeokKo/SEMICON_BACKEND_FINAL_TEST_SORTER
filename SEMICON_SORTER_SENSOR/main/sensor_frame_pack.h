#pragma once

#include <stdint.h>

#include "esp_err.h"

/* 기존 FC04 offset 0..7의 계약은 그대로 유지합니다. */
#define SENSOR_FRAME_RAW_REGISTER_COUNT 8U
#define SENSOR_FRAME_GENERATION_MAX 32000U
#define SENSOR_FRAME_PACKED_OFFSET SENSOR_FRAME_RAW_REGISTER_COUNT
#define SENSOR_FRAME_REGISTER_COUNT (SENSOR_FRAME_RAW_REGISTER_COUNT * 3U)

/* 이전 번호 0은 최초 발행 전입니다. 번호는 1..32000에서 순환합니다.
 * 잘못된 인수는 출력 값을 변경하지 않습니다. */
esp_err_t sensor_frame_next_generation(uint16_t previous, uint16_t *next);

/* 동일 스냅샷의 원시 8워드와 (generation << 16) | raw 8개를 만듭니다.
 * offset 8..23의 각 DWORD는 high-word first입니다:
 *   registers[8 + 2*i] = generation, registers[9 + 2*i] = raw[i].
 * Modbus 각 워드의 wire byte order는 프로토콜 라이브러리가 처리합니다.
 * generation은 1..32000이며 원시 워드는 부호 해석 없이 그대로 복사합니다.
 * 함수는 스레드나 공유 상태를 사용하지 않습니다. 입력/출력 배열의 겹침을
 * 허용하며, 잘못된 인수는 출력 배열을 변경하지 않습니다. */
esp_err_t sensor_frame_pack(
    const uint16_t raw[SENSOR_FRAME_RAW_REGISTER_COUNT],
    uint16_t generation,
    uint16_t registers[SENSOR_FRAME_REGISTER_COUNT]);
