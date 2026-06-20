#pragma once

#include "display_canvas.h"
#include "display_types.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t display_driver_init(const display_config_t *config);
void display_driver_deinit(void);
esp_err_t display_driver_flush(const display_canvas_t *canvas);
const uint8_t *display_driver_get_last_buffer(void);

#ifdef __cplusplus
}
#endif
