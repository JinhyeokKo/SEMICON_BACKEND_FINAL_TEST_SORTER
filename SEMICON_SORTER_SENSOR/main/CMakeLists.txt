idf_component_register(
    SRCS "main.c" "dht22_sensor.c" "sensor_state.c" "sensor_boot.c"
         "sensor_service.c" "sensor_wifi.c" "sensor_modbus.c" "sensor_frame_pack.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES freertos log esp_common esp-idf-lib__dht esp_driver_gpio
                  esp_timer nvs_flash esp_wifi esp_event esp_netif espressif__esp-modbus
)
