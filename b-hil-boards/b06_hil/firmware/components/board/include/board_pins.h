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

#define BOARD_OLED_ADDRESS_0 0x3C
#define BOARD_OLED_ADDRESS_1 0x3D

/** Two-digit HIL board number for this project (`b06_hil` -> `06`). */
#define BOARD_HIL_NUMBER_STRING "06"

/** Silk OUT 1 / U3 */
#define BOARD_INA219_OUT1_ADDR 0x42
/** Silk OUT 2 / U2 */
#define BOARD_INA219_OUT2_ADDR 0x41
/** Silk OUT 3 / U1 */
#define BOARD_INA219_OUT3_ADDR 0x40
/**
 * Provisional shunt ohms for R5/R6/R8 until BOM confirms values.
 * TODO(b06-hil): replace when BOM shunt ohms are known.
 */
#define BOARD_INA219_SHUNT_OHMS 0.1f

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
