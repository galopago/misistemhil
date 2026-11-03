#include "display_renderer.h"

#include <string.h>

#include "display_fonts.h"
#include "display_qr.h"

static const display_rect_t FULL_SCREEN_RECT = {
    .x = 0,
    .y = 0,
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
};

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static display_h_align_t normalize_h_align(display_h_align_t align)
{
    if (align < DISPLAY_H_ALIGN_LEFT || align > DISPLAY_H_ALIGN_RIGHT) {
        return DISPLAY_H_ALIGN_LEFT;
    }
    return align;
}

static display_v_align_t normalize_v_align(display_v_align_t align)
{
    if (align < DISPLAY_V_ALIGN_TOP || align > DISPLAY_V_ALIGN_BOTTOM) {
        return DISPLAY_V_ALIGN_CENTER;
    }
    return align;
}

static display_emphasis_t normalize_emphasis(display_emphasis_t emphasis)
{
    if (emphasis < DISPLAY_EMPHASIS_NORMAL || emphasis > DISPLAY_EMPHASIS_BOLD) {
        return DISPLAY_EMPHASIS_NORMAL;
    }
    return emphasis;
}

static display_font_policy_t normalize_font_policy(display_font_policy_t policy)
{
    if (policy < DISPLAY_FONT_AUTO_FIT || policy > DISPLAY_FONT_CUSTOM) {
        return DISPLAY_FONT_AUTO_FIT;
    }
    return policy;
}

static void sanitize_ascii_copy(const char *input, char *output, size_t output_size)
{
    if (output_size == 0) {
        return;
    }
    if (input == NULL) {
        output[0] = '\0';
        return;
    }

    size_t out_index = 0;
    for (size_t i = 0; input[i] != '\0' && out_index + 1 < output_size; ++i) {
        output[out_index++] = display_font_sanitize_char(input[i]);
    }
    output[out_index] = '\0';
}

display_rect_t display_renderer_clip_rect(display_rect_t rect, display_rect_t bounds)
{
    const int x0 = rect.x > bounds.x ? rect.x : bounds.x;
    const int y0 = rect.y > bounds.y ? rect.y : bounds.y;
    const int x1 = (rect.x + rect.width) < (bounds.x + bounds.width) ? (rect.x + rect.width)
                                                                     : (bounds.x + bounds.width);
    const int y1 = (rect.y + rect.height) < (bounds.y + bounds.height) ? (rect.y + rect.height)
                                                                       : (bounds.y + bounds.height);

    display_rect_t clipped = {
        .x = x0,
        .y = y0,
        .width = x1 - x0,
        .height = y1 - y0,
    };
    return clipped;
}

bool display_renderer_compute_line_bands(display_rect_t clipped_rect, int max_lines,
                                         display_rect_t *bands, size_t bands_capacity,
                                         size_t *band_count)
{
    if (bands == NULL || band_count == NULL || max_lines <= 0) {
        return false;
    }

    const int usable_x = clipped_rect.x + DISPLAY_DEFAULT_MARGIN_X;
    const int usable_y = clipped_rect.y + DISPLAY_DEFAULT_MARGIN_Y;
    const int usable_width = clipped_rect.width - (DISPLAY_DEFAULT_MARGIN_X * 2);
    const int usable_height = clipped_rect.height - (DISPLAY_DEFAULT_MARGIN_Y * 2);

    if (usable_width <= 0 || usable_height <= 0) {
        *band_count = 0;
        return false;
    }

    const int clamped_max_lines = clamp_int(max_lines, 1, 8);
    const int band_height = usable_height / clamped_max_lines;
    if (band_height <= 0) {
        *band_count = 0;
        return false;
    }

    const size_t count = (size_t)clamped_max_lines;
    if (bands_capacity < count) {
        return false;
    }

    for (int i = 0; i < clamped_max_lines; ++i) {
        bands[i] = (display_rect_t){
            .x = usable_x,
            .y = usable_y + (i * band_height),
            .width = usable_width,
            .height = band_height,
        };
    }

    *band_count = count;
    return true;
}

static int compute_text_x(display_rect_t line_band, int text_width, display_h_align_t align)
{
    switch (align) {
    case DISPLAY_H_ALIGN_CENTER:
        return line_band.x + ((line_band.width - text_width) / 2);
    case DISPLAY_H_ALIGN_RIGHT:
        return line_band.x + line_band.width - text_width;
    case DISPLAY_H_ALIGN_LEFT:
    default:
        return line_band.x;
    }
}

static int compute_vertical_group_y(int y, int height, int group_height, display_v_align_t align)
{
    switch (align) {
    case DISPLAY_V_ALIGN_TOP:
        return y;
    case DISPLAY_V_ALIGN_BOTTOM:
        return y + height - group_height;
    case DISPLAY_V_ALIGN_CENTER:
    default:
        return y + ((height - group_height) / 2);
    }
}

static void render_text_region(display_canvas_t *canvas, display_rect_t rect,
                               const display_text_region_t *text_region)
{
    int max_lines = clamp_int(text_region->max_lines, 0, DISPLAY_MAX_LINES);
    if (max_lines == 0) {
        return;
    }

    const int usable_x = rect.x + DISPLAY_DEFAULT_MARGIN_X;
    const int usable_y = rect.y + DISPLAY_DEFAULT_MARGIN_Y;
    const int usable_width = rect.width - (DISPLAY_DEFAULT_MARGIN_X * 2);
    const int usable_height = rect.height - (DISPLAY_DEFAULT_MARGIN_Y * 2);

    if (usable_width <= 0 || usable_height <= 0) {
        return;
    }

    const int band_height = usable_height / max_lines;
    if (band_height <= 0) {
        return;
    }

    const display_font_policy_t font_policy = normalize_font_policy(text_region->font_policy);
    const display_font_t *font = display_font_select(font_policy, band_height);
    const size_t rendered_line_count =
        text_region->line_count < (size_t)max_lines ? text_region->line_count : (size_t)max_lines;
    const int group_height = band_height * max_lines;
    const display_v_align_t vertical_align = normalize_v_align(text_region->vertical_align);
    const int group_y = compute_vertical_group_y(usable_y, usable_height, group_height, vertical_align);
    const display_h_align_t region_h_align = normalize_h_align(text_region->horizontal_align);
    const display_emphasis_t region_emphasis = normalize_emphasis(text_region->emphasis);

    for (int i = 0; i < max_lines; ++i) {
        char line_text[DISPLAY_MAX_LINE_LEN];
        display_h_align_t line_align = region_h_align;
        display_emphasis_t line_emphasis = region_emphasis;

        if ((size_t)i < rendered_line_count) {
            const display_text_line_t *line = &text_region->lines[i];
            sanitize_ascii_copy(line->text, line_text, sizeof(line_text));
            if (line->has_align) {
                line_align = normalize_h_align(line->align);
            }
            if (line->has_emphasis) {
                line_emphasis = normalize_emphasis(line->emphasis);
            }
        } else {
            line_text[0] = '\0';
        }

        const display_rect_t line_band = {
            .x = usable_x,
            .y = group_y + (i * band_height),
            .width = usable_width,
            .height = band_height,
        };

        display_font_truncate_to_width(line_text, sizeof(line_text), font, usable_width);
        const display_text_metrics_t text_size = display_font_measure_text(line_text, font);
        const int text_x = compute_text_x(line_band, text_size.width, line_align);
        const int text_y = line_band.y + ((line_band.height - text_size.height) / 2);

        if (line_emphasis == DISPLAY_EMPHASIS_INVERTED) {
            display_canvas_fill_rect(canvas, line_band.x, line_band.y, line_band.width,
                                     line_band.height, DISPLAY_PIXEL_ON);
            display_canvas_draw_text(canvas, text_x, text_y, line_text, font, DISPLAY_PIXEL_OFF);
        } else {
            display_canvas_draw_text(canvas, text_x, text_y, line_text, font, DISPLAY_PIXEL_ON);
        }
    }
}

static void render_qr_region(display_canvas_t *canvas, display_rect_t rect,
                             const display_qr_region_t *qr_region)
{
    if (qr_region == NULL || qr_region->payload[0] == '\0') {
        return;
    }

    char payload[DISPLAY_MAX_LINE_LEN];
    sanitize_ascii_copy(qr_region->payload, payload, sizeof(payload));

    display_qr_matrix_t matrix = {0};
    if (!display_qr_generate(payload, &matrix)) {
        return;
    }

    const int module_count = matrix.width;
    int quiet_zone = qr_region->quiet_zone_modules > 0 ? qr_region->quiet_zone_modules
                                                     : DISPLAY_QR_QUIET_ZONE_MODULES;
    int scale = DISPLAY_QR_PREFERRED_SCALE;

    if (qr_region->scale == DISPLAY_QR_SCALE_1) {
        scale = 1;
    } else if (qr_region->scale == DISPLAY_QR_SCALE_2) {
        scale = 2;
    }

    int required_size = (module_count + quiet_zone * 2) * scale;
    if (required_size > rect.width || required_size > rect.height) {
        scale = DISPLAY_QR_FALLBACK_SCALE;
        required_size = (module_count + quiet_zone * 2) * scale;
    }

    if (required_size > rect.width || required_size > rect.height) {
        display_qr_release(&matrix);
        return;
    }

    const int origin_x =
        rect.x + ((rect.width - required_size) / 2) + (quiet_zone * scale);
    const int origin_y =
        rect.y + ((rect.height - required_size) / 2) + (quiet_zone * scale);

    for (int y = 0; y < module_count; ++y) {
        for (int x = 0; x < module_count; ++x) {
            if (!display_qr_module_at(&matrix, x, y)) {
                continue;
            }
            display_canvas_fill_rect(canvas, origin_x + (x * scale), origin_y + (y * scale), scale,
                                     scale, DISPLAY_PIXEL_ON);
        }
    }

    display_qr_release(&matrix);
}

static void render_separators(display_canvas_t *canvas, const display_state_t *state)
{
    if (state == NULL || state->layout.region_count == 0) {
        return;
    }

    for (size_t i = 0; i < state->layout.region_count; ++i) {
        const display_region_t *region = &state->layout.regions[i];
        if (region->content.type != DISPLAY_CONTENT_TEXT) {
            continue;
        }

        const display_rect_t clipped = display_renderer_clip_rect(region->rect, FULL_SCREEN_RECT);
        if (clipped.width <= 0 || clipped.height <= 0) {
            continue;
        }

        const int max_lines = clamp_int(region->content.text.max_lines, 1, DISPLAY_MAX_LINES);
        const int band_height = clipped.height / max_lines;
        if (band_height <= 1) {
            continue;
        }

        for (int line = 0; line < max_lines - 1; ++line) {
            const int y = clipped.y + ((line + 1) * band_height) - 1;
            display_canvas_draw_horizontal_line(canvas, clipped.x, y, clipped.width, DISPLAY_PIXEL_ON);
        }
    }
}

static void normalize_text_region(display_text_region_t *text_region)
{
    text_region->font_policy = normalize_font_policy(text_region->font_policy);
    text_region->horizontal_align = normalize_h_align(text_region->horizontal_align);
    text_region->vertical_align = normalize_v_align(text_region->vertical_align);
    text_region->emphasis = normalize_emphasis(text_region->emphasis);
    if (text_region->line_count > DISPLAY_MAX_LINES) {
        text_region->line_count = DISPLAY_MAX_LINES;
    }
}

static void normalize_region(display_region_t *region)
{
    if (region->content.type == DISPLAY_CONTENT_TEXT) {
        normalize_text_region(&region->content.text);
    }
}

static display_state_t s_render_state;

void display_renderer_render(const display_state_t *state, const display_config_t *config,
                             display_canvas_t *canvas)
{
    if (canvas == NULL) {
        return;
    }

    display_canvas_clear(canvas, DISPLAY_PIXEL_OFF);

    if (state == NULL) {
        return;
    }

    if (state->layout.region_count == 0) {
        return;
    }

    s_render_state = *state;
    for (size_t i = 0; i < s_render_state.layout.region_count; ++i) {
        display_region_t *region = &s_render_state.layout.regions[i];
        normalize_region(region);

        const display_rect_t clipped = display_renderer_clip_rect(region->rect, FULL_SCREEN_RECT);
        if (clipped.width <= 0 || clipped.height <= 0) {
            continue;
        }

        if (region->content.type == DISPLAY_CONTENT_TEXT) {
            render_text_region(canvas, clipped, &region->content.text);
        } else if (region->content.type == DISPLAY_CONTENT_QR) {
            render_qr_region(canvas, clipped, &region->content.qr);
        }
    }

    if (config != NULL && config->separators_enabled) {
        render_separators(canvas, state);
    }
}
