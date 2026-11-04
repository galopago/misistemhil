#pragma once

#include "display_controller.h"
#include "error_led.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_core_start(void);

esp_err_t app_core_error_led_set_pattern(error_led_pattern_t pattern);

esp_err_t app_core_display_show_template(display_layout_template_t template_id,
                                         const char *const *lines,
                                         size_t line_count);
esp_err_t app_core_display_show_qr_setup(const char *url,
                                         const char *const *text_lines,
                                         size_t text_line_count);

esp_err_t app_core_relay_set(int channel, bool on);
esp_err_t app_core_relay_set_mask(uint8_t mask);
esp_err_t app_core_relay_get(int channel, bool *on_out);
uint8_t app_core_relay_get_mask(void);
esp_err_t app_core_relay_pulse(int channel, uint32_t duration_ms);
esp_err_t app_core_relay_hold(int channel, uint32_t duration_ms);
esp_err_t app_core_relay_release_all(void);

/** STA-connected main relay OLED (`B03_MAIN_RELAY_OLED`). */
void app_core_relay_oled_register(void);
void app_core_relay_oled_set_active(bool active);
bool app_core_relay_oled_is_active(void);
void app_core_relay_oled_show(void);

#ifdef __cplusplus
}
#endif
