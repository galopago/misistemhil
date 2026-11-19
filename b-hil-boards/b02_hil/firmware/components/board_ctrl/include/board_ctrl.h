#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_CTRL_RELAY_CHANNEL_MIN 1
#define BOARD_CTRL_RELAY_CHANNEL_MAX 4

typedef enum {
    BOARD_UART_PATH_TTL = 0,
    BOARD_UART_PATH_RS485 = 1,
} board_uart_path_t;

/**
 * Cold bring-up for /KOE buffer + K1..K4 CTR + 485_CTR.
 * Idempotent. Level-before-direction; ends with actuators cold and OE enabled.
 */
esp_err_t board_ctrl_init(void);

/** true after a successful board_ctrl_init(). */
bool board_ctrl_is_ready(void);

/**
 * Energize or release exactly one relay coil (channel 1..4 = K1..K4).
 * Other channels unchanged. Does not toggle /KOE or 485_CTR.
 */
esp_err_t board_ctrl_relay_set(int channel, bool on);

/** Read software shadow for channel 1..4. */
esp_err_t board_ctrl_relay_get(int channel, bool *on_out);

/** Path ttl|rs485 via K1; shares coil shadow with relay_set channel 1. */
esp_err_t board_ctrl_uart_path_set(board_uart_path_t path);
esp_err_t board_ctrl_uart_path_get(board_uart_path_t *out);

/**
 * Partial line update: only flags with has_*=true change.
 * term→K2, pullup→K4, pulldown→K3. Shares shadow with relay_set.
 */
esp_err_t board_ctrl_uart_line_set(bool term, bool pullup, bool pulldown, bool has_term,
                                   bool has_pullup, bool has_pulldown);
esp_err_t board_ctrl_uart_line_get(bool *term, bool *pullup, bool *pulldown);

/**
 * RS-485 DE via 485_CTR (does not touch /KOE or K1..K4).
 * drive=true → BOARD_485_CTR_DRIVE_LEVEL; false → INACTIVE.
 */
esp_err_t board_ctrl_rs485_de_set(bool drive);

/** Optional notify after successful relay/path/line coil changes (for OLED). */
typedef void (*board_ctrl_changed_cb_t)(void);
void board_ctrl_set_changed_callback(board_ctrl_changed_cb_t cb);

#ifdef __cplusplus
}
#endif
