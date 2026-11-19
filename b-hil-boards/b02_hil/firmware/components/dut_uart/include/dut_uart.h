#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DUT_UART_FRAME_MAX_BYTES 1024U
#define DUT_UART_RX_QUEUE_DEPTH 16U
/** RX idle gap that ends a frame (normative v1). */
#define DUT_UART_RX_IDLE_GAP_MS 5U
/** TX-done wait ceiling under host handler budget. */
#define DUT_UART_TX_DONE_TIMEOUT_MS 4000U

typedef struct {
    int baud;
    int data_bits;   /* 5..8 */
    float stop_bits; /* 1, 2, or 1.5 (1.5 only with data_bits==5) */
    char parity[8];  /* "none" | "even" | "odd" */
} dut_uart_config_t;

typedef struct {
    uint16_t len;
    uint8_t data[DUT_UART_FRAME_MAX_BYTES];
} dut_uart_frame_t;

/** Boot: install UART default 9600 8N1; DE inactive; no session. After board_ctrl_init. */
esp_err_t dut_uart_init(void);

bool dut_uart_is_open(void);

esp_err_t dut_uart_config_get(dut_uart_config_t *out);

/**
 * Apply framing. Fails with ESP_ERR_INVALID_STATE if stream open (host → busy).
 * Partial: pass a full merged config from the caller.
 */
esp_err_t dut_uart_config_set(const dut_uart_config_t *cfg);

/** Open session. cfg_or_null = keep stored config; else apply then arm RX. */
esp_err_t dut_uart_stream_open(const dut_uart_config_t *cfg_or_null);

/** Close session; discard RX queue; DE inactive. Path/line unchanged. Idempotent. */
esp_err_t dut_uart_stream_close(void);

esp_err_t dut_uart_stream_write(const uint8_t *data, size_t len, size_t *written);

/** Wait up to timeout_ms for at least one RX frame (0 = no wait). */
esp_err_t dut_uart_rx_wait(uint32_t timeout_ms);

/** Pop one queued frame; false if empty. */
bool dut_uart_rx_try_pop(dut_uart_frame_t *out);

size_t dut_uart_rx_count(void);

/** true if baud is in the allow-list. */
bool dut_uart_baud_allowed(int baud);

/** true if baud+data+parity+stop match HW matrix (27 profiles × baud list). */
bool dut_uart_framing_allowed(const dut_uart_config_t *cfg);

void dut_uart_config_set_defaults(dut_uart_config_t *cfg);

/** Optional notify after framing changes (config_set / open override) for OLED. */
typedef void (*dut_uart_config_changed_cb_t)(void);
void dut_uart_set_config_changed_callback(dut_uart_config_changed_cb_t cb);

#ifdef __cplusplus
}
#endif
