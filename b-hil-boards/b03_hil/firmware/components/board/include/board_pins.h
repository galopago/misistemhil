#pragma once

#include "driver/gpio.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B03_HIL_PIN_UNASSIGNED GPIO_NUM_NC

#define BOARD_I2C_SCL_GPIO GPIO_NUM_0
#define BOARD_I2C_SDA_GPIO GPIO_NUM_1
#define BOARD_ERROR_LED_GPIO GPIO_NUM_8
#define BOARD_ERROR_LED_ACTIVE_LEVEL 0
#define BOARD_FACTORY_RESET_GPIO GPIO_NUM_7
#define BOARD_FACTORY_RESET_ACTIVE_LEVEL 0

#define BOARD_OLED_ADDRESS_0 0x3C
#define BOARD_OLED_ADDRESS_1 0x3D

#define BOARD_RELAY_OE_GPIO GPIO_NUM_2
#define BOARD_RELAY_OE_ACTIVE_LEVEL 0

#define BOARD_RELAY1_CTR_GPIO GPIO_NUM_21
#define BOARD_RELAY2_CTR_GPIO GPIO_NUM_9
#define BOARD_RELAY3_CTR_GPIO GPIO_NUM_3
#define BOARD_RELAY4_CTR_GPIO GPIO_NUM_4
#define BOARD_RELAY5_CTR_GPIO GPIO_NUM_10
#define BOARD_RELAY6_CTR_GPIO GPIO_NUM_20

#define BOARD_RELAY_CTR_ACTIVE_LEVEL 1
#define BOARD_RELAY_CHANNEL_COUNT 6

/** Two-digit HIL board number for this project (`b03_hil` -> `03`). */
#define BOARD_HIL_NUMBER_STRING "03"

typedef struct {
    gpio_num_t status_led;
    gpio_num_t i2c_sda;
    gpio_num_t i2c_scl;
    gpio_num_t error_led;
    gpio_num_t factory_reset_switch;
    gpio_num_t relay_oe;
    gpio_num_t relay_ctr[BOARD_RELAY_CHANNEL_COUNT];
} b03_hil_board_pins_t;

const char *board_name(void);
const b03_hil_board_pins_t *board_pins(void);
void board_i2c_bus_config(i2c_bus_config_t *out_config);

#ifdef __cplusplus
}
#endif
