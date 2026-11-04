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

esp_err_t relay_ctrl_init(void);

esp_err_t relay_ctrl_set(int channel, bool on);
esp_err_t relay_ctrl_set_mask(uint8_t mask);
esp_err_t relay_ctrl_get(int channel, bool *on_out);
uint8_t relay_ctrl_get_mask(void);
bool relay_ctrl_is_output_enabled(void);

#ifdef __cplusplus
}
#endif
