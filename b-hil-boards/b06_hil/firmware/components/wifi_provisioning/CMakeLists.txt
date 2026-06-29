idf_component_register(
    SRCS
        "wifi_provisioning.c"
        "wifi_provisioning_form.c"
        "wifi_provisioning_pages.c"
    INCLUDE_DIRS "include"
    REQUIRES device_identity esp_wifi esp_netif esp_event esp_http_server freertos wifi_credentials
)
