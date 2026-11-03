#include "app_core_meter.h"

#include "board_pins.h"
#include "i2c_bus.h"
#include "ina219.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "app_core_meter";

#define METER_CHANNEL_COUNT 3
#define METER_LOCK_TIMEOUT_MS 100U

static const uint8_t k_addrs[METER_CHANNEL_COUNT] = {
    BOARD_INA219_OUT1_ADDR,
    BOARD_INA219_OUT2_ADDR,
    BOARD_INA219_OUT3_ADDR,
};

static i2c_bus_device_t s_devices[METER_CHANNEL_COUNT];
static bool s_channel_ready[METER_CHANNEL_COUNT];
static SemaphoreHandle_t s_lock = NULL;
static bool s_inited = false;

static int channel_to_index(int channel)
{
    if (channel < 1 || channel > METER_CHANNEL_COUNT) {
        return -1;
    }
    return channel - 1;
}

static bool lock_meter(void)
{
    if (s_lock == NULL) {
        return false;
    }
    return xSemaphoreTake(s_lock, pdMS_TO_TICKS(METER_LOCK_TIMEOUT_MS)) == pdTRUE;
}

static void unlock_meter(void)
{
    if (s_lock != NULL) {
        xSemaphoreGive(s_lock);
    }
}

static void fill_failed(app_core_meter_sample_t *out, int channel)
{
    out->channel = channel;
    out->ok = false;
    out->voltage_v = 0.0f;
    out->current_a = 0.0f;
    out->power_w = 0.0f;
}

static void sample_channel_locked(int index, int channel, app_core_meter_sample_t *out)
{
    fill_failed(out, channel);

    if (index < 0 || index >= METER_CHANNEL_COUNT) {
        return;
    }
    if (!s_channel_ready[index] || s_devices[index] == NULL) {
        return;
    }

    float volts = 0.0f;
    float amps = 0.0f;
    const esp_err_t v_err = ina219_read_bus_voltage_v(s_devices[index], &volts);
    const esp_err_t i_err = ina219_read_current_a(s_devices[index], &amps);
    if (v_err != ESP_OK || i_err != ESP_OK) {
        return;
    }

    out->ok = true;
    out->voltage_v = volts;
    out->current_a = amps;
    out->power_w = volts * amps;
}

esp_err_t app_core_meter_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < METER_CHANNEL_COUNT; ++i) {
        const int silk = i + 1;
        s_devices[i] = NULL;
        s_channel_ready[i] = false;

        bool present = false;
        esp_err_t err = i2c_bus_probe(k_addrs[i], &present);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "probe OUT %d addr=0x%02X failed: %s", silk, k_addrs[i],
                     esp_err_to_name(err));
            continue;
        }
        if (!present) {
            ESP_LOGW(TAG, "INA219 OUT %d not present at 0x%02X", silk, k_addrs[i]);
            continue;
        }

        i2c_bus_device_t device = NULL;
        err = i2c_bus_add_device(k_addrs[i], &device);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "add device OUT %d failed: %s", silk, esp_err_to_name(err));
            continue;
        }

        err = ina219_init(device, BOARD_INA219_SHUNT_OHMS);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ina219_init OUT %d failed: %s", silk, esp_err_to_name(err));
            continue;
        }

        s_devices[i] = device;
        s_channel_ready[i] = true;
        ESP_LOGI(TAG, "INA219 OUT %d ready at 0x%02X", silk, k_addrs[i]);
    }

    s_inited = true;
    return ESP_OK;
}

esp_err_t app_core_meter_read_channel(int channel, app_core_meter_sample_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const int index = channel_to_index(channel);
    if (index < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_inited) {
        fill_failed(out, channel);
        return ESP_OK;
    }

    if (!lock_meter()) {
        ESP_LOGW(TAG, "meter lock timeout channel=%d", channel);
        fill_failed(out, channel);
        return ESP_OK;
    }

    sample_channel_locked(index, channel, out);
    unlock_meter();
    return ESP_OK;
}

esp_err_t app_core_meter_read_all(app_core_meter_sample_t out[3])
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_inited) {
        for (int i = 0; i < METER_CHANNEL_COUNT; ++i) {
            fill_failed(&out[i], i + 1);
        }
        return ESP_OK;
    }

    if (!lock_meter()) {
        ESP_LOGW(TAG, "meter lock timeout read_all");
        for (int i = 0; i < METER_CHANNEL_COUNT; ++i) {
            fill_failed(&out[i], i + 1);
        }
        return ESP_OK;
    }

    for (int i = 0; i < METER_CHANNEL_COUNT; ++i) {
        sample_channel_locked(i, i + 1, &out[i]);
    }
    unlock_meter();
    return ESP_OK;
}
