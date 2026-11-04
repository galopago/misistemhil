idf_component_register(
    SRCS "relay_sched.c"
    INCLUDE_DIRS "include"
    REQUIRES relay_ctrl esp_timer freertos
)
