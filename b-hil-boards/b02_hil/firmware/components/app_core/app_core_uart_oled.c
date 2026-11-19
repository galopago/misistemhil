#include "app_core.h"

#include "board_ctrl.h"
#include "device_identity.h"
#include "display_types.h"
#include "dut_uart.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "app_core_oled";

#define UART_OLED_LINE_LEN 17 /* MEDIUM ≤16 chars + NUL */

static bool s_sta_uart_oled_active = false;
static char s_line0[DEVICE_ID_MAX_LEN];
static char s_line1[UART_OLED_LINE_LEN];
static char s_line2[UART_OLED_LINE_LEN];
static char s_line3[UART_OLED_LINE_LEN];

void app_core_uart_oled_show(void)
{
    if (device_identity_get(s_line0, sizeof(s_line0)) != ESP_OK || s_line0[0] == '\0') {
        ESP_LOGW(TAG, "uart OLED skipped: identity unavailable");
        return;
    }

    board_uart_path_t path = BOARD_UART_PATH_TTL;
    (void)board_ctrl_uart_path_get(&path);

    if (path == BOARD_UART_PATH_RS485) {
        strncpy(s_line1, "PATH RS485", sizeof(s_line1) - 1U);
        s_line1[sizeof(s_line1) - 1U] = '\0';

        bool term = false;
        bool pullup = false;
        bool pulldown = false;
        (void)board_ctrl_uart_line_get(&term, &pullup, &pulldown);
        snprintf(s_line2, sizeof(s_line2), "T:%d U:%d D:%d", term ? 1 : 0, pullup ? 1 : 0,
                 pulldown ? 1 : 0);
    } else {
        strncpy(s_line1, "PATH TTL", sizeof(s_line1) - 1U);
        s_line1[sizeof(s_line1) - 1U] = '\0';
        strncpy(s_line2, "T:- U:- D:-", sizeof(s_line2) - 1U);
        s_line2[sizeof(s_line2) - 1U] = '\0';
    }

    dut_uart_config_t cfg;
    dut_uart_config_set_defaults(&cfg);
    (void)dut_uart_config_get(&cfg);

    /* B02_UART_FRAMING_HW_FULL: BAUD <n> <d><p><s>; stop 1.5 → token "15". */
    char parity_char = 'N';
    if (strcmp(cfg.parity, "even") == 0) {
        parity_char = 'E';
    } else if (strcmp(cfg.parity, "odd") == 0) {
        parity_char = 'O';
    } else {
        parity_char = 'N';
    }
    const char *stop_tok = "1";
    if (cfg.stop_bits > 1.45f && cfg.stop_bits < 1.55f) {
        stop_tok = "15";
    } else if (cfg.stop_bits > 1.95f && cfg.stop_bits < 2.05f) {
        stop_tok = "2";
    }
    snprintf(s_line3, sizeof(s_line3), "BAUD %d %d%c%s", cfg.baud, cfg.data_bits, parity_char,
             stop_tok);

    const char *lines[] = {s_line0, s_line1, s_line2, s_line3};
    (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_FOUR_LINES, lines, 4);
}

void app_core_uart_oled_set_active(bool active)
{
    s_sta_uart_oled_active = active;
}

bool app_core_uart_oled_is_active(void)
{
    return s_sta_uart_oled_active;
}

static void on_shadow_changed(void)
{
    if (!s_sta_uart_oled_active) {
        return;
    }
    app_core_uart_oled_show();
}

void app_core_uart_oled_register(void)
{
    board_ctrl_set_changed_callback(on_shadow_changed);
    dut_uart_set_config_changed_callback(on_shadow_changed);
}
