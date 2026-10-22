#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_netif.h"

/* app_main에서 NVS 초기화 후 한 번만 호출합니다. 연결 완료를 기다리지 않습니다.
 * SSID가 비어 있으면 ESP_ERR_NOT_FOUND: 센서 읽기는 계속 실행할 수 있습니다.
 * 이 모듈은 기본 STA 네트워크 인터페이스와 Wi-Fi 드라이버를 소유합니다. */
esp_err_t sensor_wifi_start(void);

/* IP 주소를 받은 상태에서만 true. 다른 작업에서 호출할 수 있습니다. */
bool sensor_wifi_is_connected(void);

/* 같은 임계 구역에서 IP 보유 여부와 연결 세대를 읽습니다.
 * 세대는 IP 취득/상실, 무선 끊김/정지마다 바뀝니다.
 * 짧은 끊김 후 즉시 재연결되어도 Modbus가 이전 소켓을 정리할 수 있습니다. */
bool sensor_wifi_get_link_state(uint32_t *generation);

/* 시작 성공 후부터 유효하며 애플리케이션 실행 중 유지됩니다. */
esp_netif_t *sensor_wifi_get_netif(void);
