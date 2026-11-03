idf_component_register(
    SRCS "app_core_meter.c"
    INCLUDE_DIRS "include"
    REQUIRES board ina219 i2c_bus freertos
)
