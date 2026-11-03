#pragma once

#include "display_canvas.h"
#include "display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISPLAY_FONT_CLASS_SMALL = 0,
    DISPLAY_FONT_CLASS_MEDIUM = 1,
    DISPLAY_FONT_CLASS_LARGE = 2,
    DISPLAY_FONT_CLASS_COUNT = 3,
} display_font_class_t;

typedef struct {
    display_font_class_t class_id;
    int glyph_width;
    int glyph_height;
    int visual_height;
} display_font_t;

const display_font_t *display_font_get(display_font_class_t class_id);
const display_font_t *display_font_select(display_font_policy_t policy, int band_height);
display_text_metrics_t display_font_measure_text(const char *text, const display_font_t *font);
void display_font_draw_text(display_canvas_t *canvas, int x, int y, const char *text,
                            const display_font_t *font, uint8_t color);
void display_font_truncate_to_width(char *text, size_t text_capacity, const display_font_t *font,
                                    int max_width);
char display_font_sanitize_char(char ch);

#ifdef __cplusplus
}
#endif
