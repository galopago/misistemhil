#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RELAY_CTRL_CHANNEL_MIN 1
#define RELAY_CTRL_CHANNEL_MAX 6
#define RELAY_CTRL_MASK_ALL 0x3Fu

typedef enum {
    RELAY_VIEW_STATIC_OFF = 0, /* token '0' */
    RELAY_VIEW_STATIC_ON = 1,  /* token '1' */
    RELAY_VIEW_PULSE = 2,      /* token 'P' — pulse / hold / one-shot */
    RELAY_VIEW_TRAIN = 3,      /* token 'T' — pulse train */
    RELAY_VIEW_ENCODER = 4,    /* token 'E' — quadrature encoder */
} relay_view_mode_t;

typedef void (*relay_ctrl_view_changed_cb_t)(void);

esp_err_t relay_ctrl_init(void);

/** Static product set: GPIO + shadow + view mode 0/1; may notify view listener. */
esp_err_t relay_ctrl_set(int channel, bool on);
esp_err_t relay_ctrl_set_mask(uint8_t mask);

/**
 * Electrical drive only (pulse/train/encoder edges). Updates GPIO + shadow;
 * does **not** change view mode or notify the OLED listener.
 */
esp_err_t relay_ctrl_drive(int channel, bool on);
esp_err_t relay_ctrl_drive_mask(uint8_t mask);

esp_err_t relay_ctrl_get(int channel, bool *on_out);
uint8_t relay_ctrl_get_mask(void);
bool relay_ctrl_is_output_enabled(void);

esp_err_t relay_ctrl_get_view_mode(int channel, relay_view_mode_t *mode_out);
/**
 * Set OLED/view mode for channel 1..6 without changing GPIO.
 * If mode differs from current, update storage and invoke the view-changed
 * callback (same callback used by relay_ctrl_set*).
 */
esp_err_t relay_ctrl_set_view_mode(int channel, relay_view_mode_t mode);
char relay_ctrl_view_token(relay_view_mode_t mode);

void relay_ctrl_set_view_changed_callback(relay_ctrl_view_changed_cb_t cb);

#ifdef __cplusplus
}
#endif
