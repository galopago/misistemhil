#include "board_pins.h"

static const b06_hil_board_pins_t BOARD_PINS = {
    .status_led = B06_HIL_PIN_UNASSIGNED,
    .i2c_sda = BOARD_I2C_SDA_GPIO,
    .i2c_scl = BOARD_I2C_SCL_GPIO,
    .error_led = BOARD_ERROR_LED_GPIO,
    .factory_reset_switch = BOARD_FACTORY_RESET_GPIO,
};

const char *board_name(void)
{
    return "b06_hil";
}

const b06_hil_board_pins_t *board_pins(void)
{
    return &BOARD_PINS;
}

void board_i2c_bus_config(i2c_bus_config_t *out_config)
{
    if (out_config == NULL) {
        return;
    }

    out_config->scl_pin = (int)BOARD_PINS.i2c_scl;
    out_config->sda_pin = (int)BOARD_PINS.i2c_sda;
    out_config->bus_frequency_hz = I2C_BUS_DEFAULT_FREQUENCY_HZ;
}
