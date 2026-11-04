idf_component_register(
    SRCS "host_api.c" "host_api_command.c" "host_api_relay_v2.c" "host_api_stream.c"
         "host_api_capabilities.c"
    INCLUDE_DIRS "include"
    REQUIRES device_identity esp_http_server esp_netif freertos json mbedtls relay_ctrl relay_sched
)
