#include "display_canvas.h"

#include <string.h>

#include "display_fonts.h"

void display_canvas_init(display_canvas_t *canvas)
{
    if (canvas == NULL) {
        return;
    }
    memset(canvas->pixels, 0, sizeof(canvas->pixels));
}

void display_canvas_clear(display_canvas_t *canvas, uint8_t color)
{
    if (canvas == NULL) {
        return;
    }
    memset(canvas->pixels, color ? 0xFF : 0x00, sizeof(canvas->pixels));
}

void display_canvas_set_pixel(display_canvas_t *canvas, int x, int y, uint8_t color)
{
    if (canvas == NULL || x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }

    const int page = y / 8;
    const int bit = y % 8;
    const size_t index = (size_t)page * DISPLAY_WIDTH + (size_t)x;
    const uint8_t mask = (uint8_t)(1U << bit);

    if (color) {
        canvas->pixels[index] |= mask;
    } else {
        canvas->pixels[index] &= (uint8_t)~mask;
    }
}

void display_canvas_draw_horizontal_line(display_canvas_t *canvas, int x, int y, int width,
                                         uint8_t color)
{
    for (int i = 0; i < width; ++i) {
        display_canvas_set_pixel(canvas, x + i, y, color);
    }
}

void display_canvas_draw_vertical_line(display_canvas_t *canvas, int x, int y, int height,
                                       uint8_t color)
{
    for (int i = 0; i < height; ++i) {
        display_canvas_set_pixel(canvas, x, y + i, color);
    }
}

void display_canvas_draw_rect(display_canvas_t *canvas, int x, int y, int width, int height,
                              uint8_t color)
{
    display_canvas_draw_horizontal_line(canvas, x, y, width, color);
    display_canvas_draw_horizontal_line(canvas, x, y + height - 1, width, color);
    display_canvas_draw_vertical_line(canvas, x, y, height, color);
    display_canvas_draw_vertical_line(canvas, x + width - 1, y, height, color);
}

void display_canvas_fill_rect(display_canvas_t *canvas, int x, int y, int width, int height,
                              uint8_t color)
{
    for (int row = 0; row < height; ++row) {
        display_canvas_draw_horizontal_line(canvas, x, y + row, width, color);
    }
}

void display_canvas_draw_text(display_canvas_t *canvas, int x, int y, const char *text,
                              const void *font, uint8_t color)
{
    display_font_draw_text(canvas, x, y, text, (const display_font_t *)font, color);
}

display_text_metrics_t display_canvas_measure_text(const char *text, const void *font)
{
    return display_font_measure_text(text, (const display_font_t *)font);
}

const uint8_t *display_canvas_buffer(const display_canvas_t *canvas)
{
    return canvas == NULL ? NULL : canvas->pixels;
}
