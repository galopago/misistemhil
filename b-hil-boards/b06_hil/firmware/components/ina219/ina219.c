#include "ina219.h"

#include <math.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "ina219";

#define INA219_REG_CONFIG 0x00
#define INA219_REG_BUS_VOLTAGE 0x02
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

/* BRNG=32V, PG=/8, 12-bit bus/shunt ADC, continuous shunt+bus. */
#define INA219_CONFIG_DEFAULT 0x399FU

#define INA219_I2C_TIMEOUT_MS 20U
#define INA219_MAX_DEVICES 3
#define INA219_BUS_VOLTAGE_LSB_V 0.004f
/* Assume ~3.2 A full-scale for USB 5 V rails with provisional 0.1 ohm shunt. */
#define INA219_MAX_EXPECTED_CURRENT_A 3.2f

typedef struct {
    i2c_bus_device_t device;
    float current_lsb;
    bool in_use;
} ina219_slot_t;

static ina219_slot_t s_slots[INA219_MAX_DEVICES];

static ina219_slot_t *find_slot(i2c_bus_device_t device)
{
    if (device == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < INA219_MAX_DEVICES; ++i) {
        if (s_slots[i].in_use && s_slots[i].device == device) {
            return &s_slots[i];
        }
    }
    return NULL;
}

static ina219_slot_t *alloc_slot(i2c_bus_device_t device)
{
    ina219_slot_t *existing = find_slot(device);
    if (existing != NULL) {
        return existing;
    }
    for (size_t i = 0; i < INA219_MAX_DEVICES; ++i) {
        if (!s_slots[i].in_use) {
            s_slots[i].device = device;
            s_slots[i].current_lsb = 0.0f;
            s_slots[i].in_use = true;
            return &s_slots[i];
        }
    }
    return NULL;
}

static esp_err_t ina219_write_u16(i2c_bus_device_t device, uint8_t reg, uint16_t value)
{
    const uint8_t tx[3] = {reg, (uint8_t)((value >> 8) & 0xFFU), (uint8_t)(value & 0xFFU)};
    const i2c_transaction_t txn = {
        .device = device,
        .tx = tx,
        .tx_len = sizeof(tx),
        .rx = NULL,
        .rx_len = 0,
        .timeout_ms = INA219_I2C_TIMEOUT_MS,
    };
    return i2c_bus_transceive(&txn);
}

static esp_err_t ina219_read_u16(i2c_bus_device_t device, uint8_t reg, uint16_t *out_value)
{
    if (out_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t rx[2] = {0};
    const i2c_transaction_t txn = {
        .device = device,
        .tx = &reg,
        .tx_len = 1,
        .rx = rx,
        .rx_len = sizeof(rx),
        .timeout_ms = INA219_I2C_TIMEOUT_MS,
    };
    esp_err_t err = i2c_bus_transceive(&txn);
    if (err != ESP_OK) {
        return err;
    }

    *out_value = ((uint16_t)rx[0] << 8) | (uint16_t)rx[1];
    return ESP_OK;
}

esp_err_t ina219_init(i2c_bus_device_t device, float shunt_ohms)
{
    if (device == NULL || !(shunt_ohms > 0.0f) || !isfinite(shunt_ohms)) {
        return ESP_ERR_INVALID_ARG;
    }

    ina219_slot_t *slot = alloc_slot(device);
    if (slot == NULL) {
        ESP_LOGE(TAG, "no free INA219 slot");
        return ESP_ERR_NO_MEM;
    }

    const float current_lsb = INA219_MAX_EXPECTED_CURRENT_A / 32768.0f;
    const float cal_f = 0.04096f / (current_lsb * shunt_ohms);
    if (!isfinite(cal_f) || cal_f < 1.0f || cal_f > 65535.0f) {
        ESP_LOGE(TAG, "invalid calibration for shunt=%.4f", shunt_ohms);
        return ESP_ERR_INVALID_ARG;
    }
    const uint16_t cal = (uint16_t)(cal_f + 0.5f);

    ESP_RETURN_ON_ERROR(ina219_write_u16(device, INA219_REG_CONFIG, INA219_CONFIG_DEFAULT), TAG,
                        "config write failed");
    ESP_RETURN_ON_ERROR(ina219_write_u16(device, INA219_REG_CALIBRATION, cal), TAG,
                        "calibration write failed");

    slot->current_lsb = current_lsb;
    ESP_LOGI(TAG, "init ok cal=%u current_lsb=%.6f shunt=%.4f", (unsigned)cal, current_lsb,
             shunt_ohms);
    return ESP_OK;
}

esp_err_t ina219_read_bus_voltage_v(i2c_bus_device_t device, float *out_v)
{
    if (device == NULL || out_v == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t raw = 0;
    ESP_RETURN_ON_ERROR(ina219_read_u16(device, INA219_REG_BUS_VOLTAGE, &raw), TAG,
                        "bus voltage read failed");
    *out_v = (float)(raw >> 3) * INA219_BUS_VOLTAGE_LSB_V;
    return ESP_OK;
}

esp_err_t ina219_read_current_a(i2c_bus_device_t device, float *out_a)
{
    if (device == NULL || out_a == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const ina219_slot_t *slot = find_slot(device);
    if (slot == NULL || !(slot->current_lsb > 0.0f)) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t raw = 0;
    ESP_RETURN_ON_ERROR(ina219_read_u16(device, INA219_REG_CURRENT, &raw), TAG,
                        "current read failed");
    const int16_t signed_raw = (int16_t)raw;
    *out_a = (float)signed_raw * slot->current_lsb;
    return ESP_OK;
}
