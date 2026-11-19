idf_component_register(
    SRCS "dut_uart.c"
    INCLUDE_DIRS "include"
    REQUIRES board board_ctrl driver freertos
)
