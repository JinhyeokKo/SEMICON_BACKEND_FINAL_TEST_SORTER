#include "dht22_sensor.h"

#include <stddef.h>

#include "dht.h"

esp_err_t dht22_sensor_read(dht22_reading_t *reading) {
  if (reading == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  reading->valid = false;

  int16_t humidity_x10 = 0;
  int16_t temperature_x10 = 0;

  /* DHT 드라이버의 출력은 습도→온도이며 단위는10배다. */
  const esp_err_t result = dht_read_data(DHT_TYPE_AM2301, DHT22_SENSOR_GPIO,
                                         &humidity_x10, &temperature_x10);

  if (result != ESP_OK) {
    reading->status = (result == ESP_ERR_INVALID_CRC)
                          ? DHT22_STATUS_CHECKSUM_ERROR
                          : DHT22_STATUS_READ_ERROR;
    return result;
  }

  // humidity_x10 -= 100; /* 비오는날 TEST ONLY: 습도 -10 %RH */

  /* 센서 물리 범위다. FX의 운전 허용 범위와 구별한다. */
  if (temperature_x10 < -400 || temperature_x10 > 800 || humidity_x10 < 0 ||
      humidity_x10 > 1000) {
    reading->status = DHT22_STATUS_RANGE_ERROR;
    return ESP_ERR_INVALID_RESPONSE;
  }

  /* 검사에 통과한 숫자만 100배 단위로 함께 공개한다. */
  reading->temp_x100 = (int16_t)(temperature_x10 * 10);
  reading->rh_x100 = (uint16_t)(humidity_x10 * 10);
  reading->status = DHT22_STATUS_OK;
  reading->valid = true;

  return ESP_OK;
}
