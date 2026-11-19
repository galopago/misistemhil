idf_component_register(
    SRCS "board_ctrl.c"
    INCLUDE_DIRS "include"
    REQUIRES board esp_driver_gpio freertos
)
