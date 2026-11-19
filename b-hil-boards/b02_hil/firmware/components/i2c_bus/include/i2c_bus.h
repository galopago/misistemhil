#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_BUS_DEFAULT_FREQUENCY_HZ 400000
#define I2C_BUS_MAX_DEVICES 8
#define I2C_BUS_PROBE_TIMEOUT_MS 50
#define I2C_BUS_ADDRESS_MIN 0x08
#define I2C_BUS_ADDRESS_MAX 0x77

typedef struct {
    int scl_pin;
    int sda_pin;
    uint32_t bus_frequency_hz;
} i2c_bus_config_t;

typedef struct i2c_bus_device *i2c_bus_device_t;

typedef struct {
    i2c_bus_device_t device;
    const uint8_t *tx;
    size_t tx_len;
    uint8_t *rx;
    size_t rx_len;
    uint32_t timeout_ms;
} i2c_transaction_t;

esp_err_t i2c_bus_init(const i2c_bus_config_t *config);
esp_err_t i2c_bus_add_device(uint8_t address, i2c_bus_device_t *out_device);
esp_err_t i2c_bus_probe(uint8_t address, bool *present);
esp_err_t i2c_bus_transceive(const i2c_transaction_t *txn);
void i2c_bus_deinit(void);

#ifdef __cplusplus
}
#endif
