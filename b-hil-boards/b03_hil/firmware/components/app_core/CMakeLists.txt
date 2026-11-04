idf_component_register(
    SRCS "app_core.c" "app_core_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES board device_discovery device_identity display error_led esp_timer host_api i2c_bus relay_ctrl setup_url wifi_credentials wifi_provisioning driver
)
