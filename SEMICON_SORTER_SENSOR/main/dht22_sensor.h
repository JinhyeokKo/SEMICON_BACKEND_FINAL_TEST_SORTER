#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h" // IWYU pragma: keep
#include "esp_err.h"

/* SEN0137의 S 단자에 연결할 ESP32 GPIO 번호. */
#define DHT22_SENSOR_GPIO GPIO_NUM_4
#define DHT22_SENSOR_STARTUP_DELAY_MS 2500U
#define DHT22_SENSOR_READ_INTERVAL_MS 2500U

/* 프로젝트의 SENSOR_STATUS 규격. 1은 초기 대기, 5는 상태 모듈의 미갱신 판정이다. */
typedef enum {
  DHT22_STATUS_OK = 0,
  DHT22_STATUS_WAITING = 1,
  DHT22_STATUS_READ_ERROR = 2,
  DHT22_STATUS_CHECKSUM_ERROR = 3,
  DHT22_STATUS_RANGE_ERROR = 4,
  DHT22_STATUS_STALE = 5,
} dht22_status_t;

typedef struct {
  int16_t temp_x100;
  uint16_t rh_x100;
  bool valid;
  uint16_t status;
} dht22_reading_t;

#define DHT22_READING_INITIALIZER                                              \
  {.temp_x100 = 0, .rh_x100 = 0, .valid = false, .status = DHT22_STATUS_WAITING}

/*
 * 실제 센서에서 온습도 한 쌍을 읽는다. 호출 간격은 호출부가 관리한다.
 * reading은 최초 호출 전에 DHT22_READING_INITIALIZER로 초기화한다.
 * 성공: 숫자 필드에 100배 정수를 기록하고 valid=true, status=0.
 * 실패: 숫자 필드는 보존하고 valid=false 및 오류 status를 기록한다.
 * 숫자를 새 측정으로 사용할 때 반드시 valid와 반환값을 확인한다.
 * NULL: ESP_ERR_INVALID_ARG. 센서를 읽거나 메모리를 변경하지 않는다.
 * 동일한 구조체를 여러 task에서 공유할 때에는 호출부에서 접근을 보호한다.
 */
esp_err_t dht22_sensor_read(dht22_reading_t *reading);
