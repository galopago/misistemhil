idf_component_register(
    SRCS "host_api.c" "host_api_command.c" "host_api_stream.c" "host_api_capabilities.c"
    INCLUDE_DIRS "include"
    REQUIRES board_ctrl device_identity dut_uart esp_http_server esp_netif freertos json mbedtls
)
