#include "app_core.h"

#include "device_identity.h"
#include "display_types.h"
#include "relay_ctrl.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "app_core_oled";

#define RELAY_OLED_PAIR_LEN 13 /* "1:[0]  2:[1]" + NUL */

static bool s_sta_relay_oled_active = false;
static char s_line0[DEVICE_ID_MAX_LEN];
static char s_line1[RELAY_OLED_PAIR_LEN];
static char s_line2[RELAY_OLED_PAIR_LEN];
static char s_line3[RELAY_OLED_PAIR_LEN];

static void format_pair_line(char *out, size_t out_len, int left_ch, int right_ch)
{
    relay_view_mode_t left_mode = RELAY_VIEW_STATIC_OFF;
    relay_view_mode_t right_mode = RELAY_VIEW_STATIC_OFF;
    (void)relay_ctrl_get_view_mode(left_ch, &left_mode);
    (void)relay_ctrl_get_view_mode(right_ch, &right_mode);

    snprintf(out, out_len, "%d:[%c]  %d:[%c]", left_ch, relay_ctrl_view_token(left_mode), right_ch,
             relay_ctrl_view_token(right_mode));
}

void app_core_relay_oled_show(void)
{
    if (device_identity_get(s_line0, sizeof(s_line0)) != ESP_OK || s_line0[0] == '\0') {
        ESP_LOGW(TAG, "relay OLED skipped: identity unavailable");
        return;
    }

    format_pair_line(s_line1, sizeof(s_line1), 1, 2);
    format_pair_line(s_line2, sizeof(s_line2), 3, 4);
    format_pair_line(s_line3, sizeof(s_line3), 5, 6);

    const char *lines[] = {s_line0, s_line1, s_line2, s_line3};
    (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_FOUR_LINES, lines, 4);
}

void app_core_relay_oled_set_active(bool active)
{
    s_sta_relay_oled_active = active;
}

bool app_core_relay_oled_is_active(void)
{
    return s_sta_relay_oled_active;
}

static void on_relay_view_changed(void)
{
    if (!s_sta_relay_oled_active) {
        return;
    }
    app_core_relay_oled_show();
}

void app_core_relay_oled_register(void)
{
    relay_ctrl_set_view_changed_callback(on_relay_view_changed);
}
