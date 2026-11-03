idf_component_register(
    SRCS "device_discovery.c"
    INCLUDE_DIRS "include"
    REQUIRES device_identity esp_netif esp_wifi mdns
)
