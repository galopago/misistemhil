#include "i2c_bus.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "i2c_bus";

typedef struct i2c_bus_device {
    i2c_master_dev_handle_t handle;
    uint8_t address;
    bool in_use;
} i2c_bus_device_impl_t;

static i2c_master_bus_handle_t s_bus_handle = NULL;
static i2c_bus_device_impl_t s_devices[I2C_BUS_MAX_DEVICES];
static uint32_t s_bus_frequency_hz = I2C_BUS_DEFAULT_FREQUENCY_HZ;
static SemaphoreHandle_t s_bus_mutex = NULL;

static bool is_valid_address(uint8_t address)
{
    return address >= I2C_BUS_ADDRESS_MIN && address <= I2C_BUS_ADDRESS_MAX;
}

static bool is_registered_device(const i2c_bus_device_impl_t *device)
{
    if (device == NULL || !device->in_use) {
        return false;
    }

    for (size_t i = 0; i < I2C_BUS_MAX_DEVICES; ++i) {
        if (&s_devices[i] == device && s_devices[i].in_use) {
            return true;
        }
    }
    return false;
}

static i2c_bus_device_impl_t *find_device_by_address(uint8_t address)
{
    for (size_t i = 0; i < I2C_BUS_MAX_DEVICES; ++i) {
        if (s_devices[i].in_use && s_devices[i].address == address) {
            return &s_devices[i];
        }
    }
    return NULL;
}

static i2c_bus_device_impl_t *allocate_device_slot(void)
{
    for (size_t i = 0; i < I2C_BUS_MAX_DEVICES; ++i) {
        if (!s_devices[i].in_use) {
            return &s_devices[i];
        }
    }
    return NULL;
}

static esp_err_t execute_transaction(i2c_bus_device_impl_t *device, const i2c_transaction_t *txn)
{
    if (txn->rx_len > 0) {
        if (txn->tx_len > 0) {
            return i2c_master_transmit_receive(device->handle, txn->tx, txn->tx_len, txn->rx,
                                               txn->rx_len, txn->timeout_ms);
        }
        return i2c_master_receive(device->handle, txn->rx, txn->rx_len, txn->timeout_ms);
    }

    if (txn->tx_len > 0) {
        return i2c_master_transmit(device->handle, txn->tx, txn->tx_len, txn->timeout_ms);
    }

    return ESP_ERR_INVALID_ARG;
}

esp_err_t i2c_bus_init(const i2c_bus_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(s_bus_handle == NULL, ESP_ERR_INVALID_STATE, TAG, "bus already initialized");

    s_bus_frequency_hz = config->bus_frequency_hz > 0 ? config->bus_frequency_hz
                                                      : I2C_BUS_DEFAULT_FREQUENCY_HZ;

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = (gpio_num_t)config->sda_pin,
        .scl_io_num = (gpio_num_t)config->scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags =
            {
                .enable_internal_pullup = true,
            },
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_bus_handle), TAG,
                        "failed to create master bus");

    s_bus_mutex = xSemaphoreCreateMutex();
    if (s_bus_mutex == NULL) {
        i2c_del_master_bus(s_bus_handle);
        s_bus_handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    memset(s_devices, 0, sizeof(s_devices));
    ESP_LOGI(TAG, "I2C bus ready on SCL GPIO%d SDA GPIO%d at %lu Hz", config->scl_pin,
             config->sda_pin, (unsigned long)s_bus_frequency_hz);
    return ESP_OK;
}

esp_err_t i2c_bus_add_device(uint8_t address, i2c_bus_device_t *out_device)
{
    ESP_RETURN_ON_FALSE(out_device != NULL, ESP_ERR_INVALID_ARG, TAG, "out_device is NULL");
    ESP_RETURN_ON_FALSE(is_valid_address(address), ESP_ERR_INVALID_ARG, TAG,
                        "invalid address 0x%02X", address);
    ESP_RETURN_ON_FALSE(s_bus_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "bus not initialized");

    i2c_bus_device_impl_t *existing = find_device_by_address(address);
    if (existing != NULL) {
        *out_device = existing;
        return ESP_OK;
    }

    i2c_bus_device_impl_t *slot = allocate_device_slot();
    ESP_RETURN_ON_FALSE(slot != NULL, ESP_ERR_NO_MEM, TAG, "device table full");

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = s_bus_frequency_hz,
    };

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus_handle, &device_config, &slot->handle), TAG,
                        "failed to add device 0x%02X", address);

    slot->address = address;
    slot->in_use = true;
    *out_device = slot;
    ESP_LOGI(TAG, "Registered I2C device 0x%02X", address);
    return ESP_OK;
}

esp_err_t i2c_bus_probe(uint8_t address, bool *present)
{
    ESP_RETURN_ON_FALSE(present != NULL, ESP_ERR_INVALID_ARG, TAG, "present is NULL");
    ESP_RETURN_ON_FALSE(is_valid_address(address), ESP_ERR_INVALID_ARG, TAG,
                        "invalid address 0x%02X", address);
    ESP_RETURN_ON_FALSE(s_bus_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "bus not initialized");

    const esp_err_t err = i2c_master_probe(s_bus_handle, address, I2C_BUS_PROBE_TIMEOUT_MS);
    if (err == ESP_OK) {
        *present = true;
        return ESP_OK;
    }
    if (err == ESP_ERR_NOT_FOUND) {
        *present = false;
        return ESP_OK;
    }
    return err;
}

esp_err_t i2c_bus_transceive(const i2c_transaction_t *txn)
{
    ESP_RETURN_ON_FALSE(txn != NULL, ESP_ERR_INVALID_ARG, TAG, "txn is NULL");
    ESP_RETURN_ON_FALSE(txn->device != NULL, ESP_ERR_INVALID_ARG, TAG, "device is NULL");
    ESP_RETURN_ON_FALSE(txn->timeout_ms > 0, ESP_ERR_INVALID_ARG, TAG, "timeout_ms must be > 0");
    ESP_RETURN_ON_FALSE(s_bus_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "bus not initialized");
    ESP_RETURN_ON_FALSE(s_bus_mutex != NULL, ESP_ERR_INVALID_STATE, TAG, "bus mutex missing");

    i2c_bus_device_impl_t *device = (i2c_bus_device_impl_t *)txn->device;
    ESP_RETURN_ON_FALSE(is_registered_device(device), ESP_ERR_INVALID_ARG, TAG,
                        "unknown I2C device handle");

    if (txn->rx_len > 0) {
        ESP_RETURN_ON_FALSE(txn->rx != NULL, ESP_ERR_INVALID_ARG, TAG, "rx buffer is NULL");
        if (txn->tx_len > 0) {
            ESP_RETURN_ON_FALSE(txn->tx != NULL, ESP_ERR_INVALID_ARG, TAG, "tx buffer is NULL");
        }
    } else if (txn->tx_len > 0) {
        ESP_RETURN_ON_FALSE(txn->tx != NULL, ESP_ERR_INVALID_ARG, TAG, "tx buffer is NULL");
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_bus_mutex, pdMS_TO_TICKS(txn->timeout_ms)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    const esp_err_t err = execute_transaction(device, txn);
    xSemaphoreGive(s_bus_mutex);
    return err;
}

void i2c_bus_deinit(void)
{
    for (size_t i = 0; i < I2C_BUS_MAX_DEVICES; ++i) {
        if (s_devices[i].in_use && s_devices[i].handle != NULL) {
            i2c_master_bus_rm_device(s_devices[i].handle);
            s_devices[i].handle = NULL;
            s_devices[i].in_use = false;
            s_devices[i].address = 0;
        }
    }

    if (s_bus_handle != NULL) {
        i2c_del_master_bus(s_bus_handle);
        s_bus_handle = NULL;
    }

    if (s_bus_mutex != NULL) {
        vSemaphoreDelete(s_bus_mutex);
        s_bus_mutex = NULL;
    }
}
