#pragma once

#include "esp_err.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configure INA219 calibration for the given shunt resistance (ohms).
 * Uses active i2c_bus phase-2 path (`i2c_bus_transceive`).
 */
esp_err_t ina219_init(i2c_bus_device_t device, float shunt_ohms);

esp_err_t ina219_read_bus_voltage_v(i2c_bus_device_t device, float *out_v);

esp_err_t ina219_read_current_a(i2c_bus_device_t device, float *out_a);

#ifdef __cplusplus
}
#endif
