#pragma once

#include "display_canvas.h"
#include "display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void display_renderer_render(const display_state_t *state, const display_config_t *config,
                             display_canvas_t *canvas);
display_rect_t display_renderer_clip_rect(display_rect_t rect, display_rect_t bounds);
bool display_renderer_compute_line_bands(display_rect_t clipped_rect, int max_lines,
                                         display_rect_t *bands, size_t bands_capacity,
                                         size_t *band_count);

#ifdef __cplusplus
}
#endif
