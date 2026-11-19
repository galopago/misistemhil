#pragma once

#include "driver/gpio.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B02_HIL_PIN_UNASSIGNED GPIO_NUM_NC

#define BOARD_HIL_NUMBER_STRING        "02"

#define BOARD_I2C_SCL_GPIO             GPIO_NUM_0
#define BOARD_I2C_SDA_GPIO             GPIO_NUM_1
#define BOARD_ERROR_LED_GPIO           GPIO_NUM_8
#define BOARD_ERROR_LED_ACTIVE_LEVEL   0
#define BOARD_FACTORY_RESET_GPIO       GPIO_NUM_7
#define BOARD_FACTORY_RESET_ACTIVE_LEVEL 0
#define BOARD_OLED_ADDRESS_0           0x3C
#define BOARD_OLED_ADDRESS_1           0x3D

#define BOARD_CTRL_OE_GPIO             GPIO_NUM_2
#define BOARD_CTRL_OE_ACTIVE_LEVEL     0   /* /KOE active-low */

#define BOARD_UART_TX_GPIO             GPIO_NUM_5
#define BOARD_UART_RX_GPIO             GPIO_NUM_6
#define BOARD_TTL_IN_GPIO              GPIO_NUM_10

#define BOARD_K1_CTR_GPIO              GPIO_NUM_21  /* TTL/RS485 mux */
#define BOARD_K2_CTR_GPIO              GPIO_NUM_9   /* termination */
#define BOARD_K3_CTR_GPIO              GPIO_NUM_3   /* pull-down */
#define BOARD_K4_CTR_GPIO              GPIO_NUM_4   /* pull-up */
#define BOARD_485_CTR_GPIO             GPIO_NUM_20  /* MAX485 DE/~RE path */

#define BOARD_RELAY_CTR_ACTIVE_LEVEL   1
#define BOARD_485_CTR_INACTIVE_LEVEL   0  /* bootstrap receive / not driving */
/* Tentative: CTR high → MAX485 drive (confirm on bench; see B02_485_DE_POLARITY). */
#define BOARD_485_CTR_DRIVE_LEVEL      1

/* Optional alias kept identical to BOARD_CTRL_OE_GPIO. */
#define BOARD_KOE_GPIO                 BOARD_CTRL_OE_GPIO

typedef struct {
    gpio_num_t status_led;
    gpio_num_t i2c_sda;
    gpio_num_t i2c_scl;
    gpio_num_t error_led;
    gpio_num_t factory_reset_switch;
} b02_hil_board_pins_t;

const char *board_name(void);
const b02_hil_board_pins_t *board_pins(void);
void board_i2c_bus_config(i2c_bus_config_t *out_config);

#ifdef __cplusplus
}
#endif
