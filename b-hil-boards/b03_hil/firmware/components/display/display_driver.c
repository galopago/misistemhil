#include "display_driver.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "display_driver";

#define DISPLAY_I2C_TIMEOUT_MS 50

static uint8_t s_last_buffer[DISPLAY_FRAMEBUFFER_BYTES];
static bool s_initialized = false;
static bool s_hardware_ready = false;
static i2c_bus_device_t s_i2c_device = NULL;

static esp_err_t i2c_write(const uint8_t *data, size_t len)
{
    if (s_i2c_device == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    const i2c_transaction_t txn = {
        .device = s_i2c_device,
        .tx = data,
        .tx_len = len,
        .rx = NULL,
        .rx_len = 0,
        .timeout_ms = DISPLAY_I2C_TIMEOUT_MS,
    };
    return i2c_bus_transceive(&txn);
}

static esp_err_t ssd1306_write_commands(const uint8_t *cmds, size_t len)
{
    if (len == 0) {
        return ESP_OK;
    }

    uint8_t buffer[1 + 32];
    if (len > sizeof(buffer) - 1) {
        return ESP_ERR_INVALID_SIZE;
    }

    buffer[0] = 0x00;
    memcpy(&buffer[1], cmds, len);
    return i2c_write(buffer, len + 1);
}

static esp_err_t ssd1306_init_panel(void)
{
    static const uint8_t init_seq[] = {
        0xAE,
        0xD5, 0x80,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x00,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0xCF,
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4,
        0xA6,
        0xAF,
    };

    return ssd1306_write_commands(init_seq, sizeof(init_seq));
}

static esp_err_t ssd1306_flush_framebuffer(const uint8_t *framebuffer)
{
    ESP_RETURN_ON_ERROR(ssd1306_write_commands((const uint8_t[]){0x21, 0x00, 0x7F}, 3), TAG,
                        "set column address");
    ESP_RETURN_ON_ERROR(ssd1306_write_commands((const uint8_t[]){0x22, 0x00, 0x07}, 3), TAG,
                        "set page address");

    uint8_t page_buffer[1 + DISPLAY_WIDTH];
    page_buffer[0] = 0x40;

    for (int page = 0; page < DISPLAY_HEIGHT / 8; ++page) {
        memcpy(&page_buffer[1], &framebuffer[page * DISPLAY_WIDTH], DISPLAY_WIDTH);
        ESP_RETURN_ON_ERROR(i2c_write(page_buffer, sizeof(page_buffer)), TAG,
                            "write page %d", page);
    }

    return ESP_OK;
}

esp_err_t display_driver_init(const display_config_t *config)
{
    memset(s_last_buffer, 0, sizeof(s_last_buffer));
    s_i2c_device = NULL;
    s_hardware_ready = false;

    if (config != NULL && config->has_i2c_device) {
        s_i2c_device = config->i2c_device;
    }

    s_initialized = true;

    if (s_i2c_device != NULL && config != NULL && config->has_address) {
        const esp_err_t err = ssd1306_init_panel();
        if (err == ESP_OK) {
            s_hardware_ready = true;
            ESP_LOGI(TAG, "SSD1306 initialized at I2C address 0x%02X", config->address);
        } else {
            ESP_LOGW(TAG, "SSD1306 init failed at 0x%02X: %s; using framebuffer stub",
                     config->address, esp_err_to_name(err));
        }
    } else {
        ESP_LOGI(TAG, "Display driver initialized without I2C device (stub mode)");
    }

    return ESP_OK;
}

void display_driver_deinit(void)
{
    s_initialized = false;
    s_hardware_ready = false;
    s_i2c_device = NULL;
}

esp_err_t display_driver_flush(const display_canvas_t *canvas)
{
    if (!s_initialized || canvas == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(s_last_buffer, canvas->pixels, sizeof(s_last_buffer));

    if (s_hardware_ready) {
        const esp_err_t err = ssd1306_flush_framebuffer(canvas->pixels);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "SSD1306 flush failed: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGD(TAG, "Framebuffer flushed to SSD1306");
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Framebuffer flushed to driver stub");
    return ESP_OK;
}

const uint8_t *display_driver_get_last_buffer(void)
{
    return s_last_buffer;
}
