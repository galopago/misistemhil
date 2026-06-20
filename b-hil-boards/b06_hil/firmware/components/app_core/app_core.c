#include "app_core.h"

#include "board_pins.h"
#include "display.h"
#include "display_controller.h"
#include "display_geometry_test.h"
#include "esp_check.h"
#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "app_core";

static esp_err_t resolve_oled_config(display_config_t *cfg)
{
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!cfg->has_address) {
        static const uint8_t candidates[] = {BOARD_OLED_ADDRESS_0, BOARD_OLED_ADDRESS_1};

        for (size_t i = 0; i < sizeof(candidates); ++i) {
            bool present = false;
            ESP_RETURN_ON_ERROR(i2c_bus_probe(candidates[i], &present), TAG,
                                "OLED probe failed for 0x%02X", candidates[i]);
            if (present) {
                cfg->address = candidates[i];
                cfg->has_address = true;
                ESP_LOGI(TAG, "OLED detected at I2C address 0x%02X", cfg->address);
                break;
            }
        }

        if (!cfg->has_address) {
            ESP_LOGW(TAG, "OLED not detected at 0x%02X or 0x%02X; starting display stub",
                     BOARD_OLED_ADDRESS_0, BOARD_OLED_ADDRESS_1);
            return ESP_OK;
        }
    }

    i2c_bus_device_t device = NULL;
    ESP_RETURN_ON_ERROR(i2c_bus_add_device(cfg->address, &device), TAG,
                        "failed to register OLED device 0x%02X", cfg->address);
    cfg->i2c_device = device;
    cfg->has_i2c_device = true;
    return ESP_OK;
}

void app_core_start(void)
{
    ESP_LOGI(TAG, "Board: %s", board_name());

    const b06_hil_board_pins_t *pins = board_pins();
    ESP_LOGI(TAG, "I2C SCL GPIO%d SDA GPIO%d", pins->i2c_scl, pins->i2c_sda);

    i2c_bus_config_t bus_config = {0};
    board_i2c_bus_config(&bus_config);
    ESP_ERROR_CHECK(i2c_bus_init(&bus_config));

    ESP_ERROR_CHECK(display_geometry_self_test());

    display_config_t cfg = DISPLAY_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(resolve_oled_config(&cfg));
    ESP_ERROR_CHECK(display_start(&cfg));
    ESP_ERROR_CHECK(display_controller_init());

    const char *demo_lines[] = {"b06_hil", "READY", "v0.1", ""};
    ESP_ERROR_CHECK(display_controller_show_template(DISPLAY_TEMPLATE_FULL_FOUR_LINES, demo_lines,
                                                     4));
}
