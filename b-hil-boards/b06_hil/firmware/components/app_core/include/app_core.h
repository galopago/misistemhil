#pragma once

#include "display_controller.h"
#include "error_led.h"

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

#ifdef __cplusplus
}
#endif
