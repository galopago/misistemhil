idf_component_register(
    SRCS "app_core.c"
    INCLUDE_DIRS "include"
    REQUIRES board display i2c_bus
)
