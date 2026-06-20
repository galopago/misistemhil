#pragma once

#include "display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct display_canvas display_canvas_t;

struct display_canvas {
    uint8_t pixels[DISPLAY_FRAMEBUFFER_BYTES];
};

void display_canvas_init(display_canvas_t *canvas);
void display_canvas_clear(display_canvas_t *canvas, uint8_t color);
void display_canvas_set_pixel(display_canvas_t *canvas, int x, int y, uint8_t color);
void display_canvas_draw_horizontal_line(display_canvas_t *canvas, int x, int y, int width,
                                         uint8_t color);
void display_canvas_draw_vertical_line(display_canvas_t *canvas, int x, int y, int height,
                                       uint8_t color);
void display_canvas_draw_rect(display_canvas_t *canvas, int x, int y, int width, int height,
                              uint8_t color);
void display_canvas_fill_rect(display_canvas_t *canvas, int x, int y, int width, int height,
                              uint8_t color);
void display_canvas_draw_text(display_canvas_t *canvas, int x, int y, const char *text,
                              const void *font, uint8_t color);
display_text_metrics_t display_canvas_measure_text(const char *text, const void *font);
const uint8_t *display_canvas_buffer(const display_canvas_t *canvas);

#ifdef __cplusplus
}
#endif
