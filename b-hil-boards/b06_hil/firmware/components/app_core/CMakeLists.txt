idf_component_register(
    SRCS "app_core.c" "app_core_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES board display error_led i2c_bus setup_url wifi_credentials wifi_provisioning driver
)
