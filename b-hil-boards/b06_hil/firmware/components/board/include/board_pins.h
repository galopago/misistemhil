#pragma once

#include "driver/gpio.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B06_HIL_PIN_UNASSIGNED GPIO_NUM_NC

#define BOARD_I2C_SCL_GPIO GPIO_NUM_0
#define BOARD_I2C_SDA_GPIO GPIO_NUM_1
#define BOARD_ERROR_LED_GPIO GPIO_NUM_8
#define BOARD_ERROR_LED_ACTIVE_LEVEL 0
#define BOARD_FACTORY_RESET_GPIO GPIO_NUM_7
#define BOARD_FACTORY_RESET_ACTIVE_LEVEL 0

#define BOARD_INA219_ADDRESS_0 0x40
#define BOARD_INA219_ADDRESS_1 0x41
#define BOARD_INA219_ADDRESS_2 0x42
#define BOARD_OLED_ADDRESS_0 0x3C
#define BOARD_OLED_ADDRESS_1 0x3D

typedef struct {
    gpio_num_t status_led;
    gpio_num_t i2c_sda;
    gpio_num_t i2c_scl;
    gpio_num_t error_led;
    gpio_num_t factory_reset_switch;
} b06_hil_board_pins_t;

const char *board_name(void);
const b06_hil_board_pins_t *board_pins(void);
void board_i2c_bus_config(i2c_bus_config_t *out_config);

#ifdef __cplusplus
}
#endif
