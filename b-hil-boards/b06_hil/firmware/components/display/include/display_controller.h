#pragma once

#include "display_types.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t display_controller_init(void);
esp_err_t display_controller_show_layout(const display_layout_t *layout);
esp_err_t display_controller_show_template(display_layout_template_t template_id,
                                           const char *const *lines,
                                           size_t line_count);
esp_err_t display_controller_show_qr_setup(const char *payload,
                                           const char *const *text_lines,
                                           size_t text_line_count);

#ifdef __cplusplus
}
#endif
