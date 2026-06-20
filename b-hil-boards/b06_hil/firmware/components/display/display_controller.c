#include "display_controller.h"

#include "display.h"
#include "display_layout_templates.h"

#include <string.h>

static bool s_initialized = false;
static display_layout_t s_controller_layout;

esp_err_t display_controller_init(void)
{
    s_initialized = true;
    return ESP_OK;
}

esp_err_t display_controller_show_layout(const display_layout_t *layout)
{
    if (!s_initialized || layout == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return display_set_content(layout);
}

esp_err_t display_controller_show_template(display_layout_template_t template_id,
                                           const char *const *lines, size_t line_count)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_controller_layout, 0, sizeof(s_controller_layout));
    switch (template_id) {
    case DISPLAY_TEMPLATE_FULL_FOUR_LINES:
        display_layout_template_full_four_lines(&s_controller_layout, lines, line_count);
        break;
    case DISPLAY_TEMPLATE_FULL_TWO_LINES:
        display_layout_template_full_two_lines(&s_controller_layout, lines, line_count);
        break;
    case DISPLAY_TEMPLATE_TOP_TWO_BOTTOM_ONE:
        display_layout_template_top_two_bottom_one(&s_controller_layout, lines, line_count);
        break;
    case DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT:
        display_layout_template_qr_left_text_right(&s_controller_layout, "http://192.168.4.1", lines,
                                                     line_count);
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    return display_controller_show_layout(&s_controller_layout);
}

esp_err_t display_controller_show_qr_setup(const char *payload, const char *const *text_lines,
                                           size_t text_line_count)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_controller_layout, 0, sizeof(s_controller_layout));
    display_layout_template_qr_left_text_right(&s_controller_layout, payload, text_lines,
                                               text_line_count);
    return display_controller_show_layout(&s_controller_layout);
}
