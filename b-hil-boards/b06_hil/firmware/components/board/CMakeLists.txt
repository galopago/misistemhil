idf_component_register(
    SRCS "board_pins.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio i2c_bus
)
