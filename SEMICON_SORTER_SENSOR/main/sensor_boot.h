#pragma once

#include <stdint.h>
#include "esp_err.h"

/* 부팅마다 한 번만 호출한다. commit 성공 후에만 out_boot_id를 변경한다. */
esp_err_t sensor_boot_next_id(uint16_t *out_boot_id);
