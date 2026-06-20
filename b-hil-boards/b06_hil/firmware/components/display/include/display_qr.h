#pragma once

#include "display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct display_canvas display_canvas_t;

typedef struct {
    int width;
    int height;
    const uint8_t *modules;
} display_qr_matrix_t;

bool display_qr_generate(const char *payload, display_qr_matrix_t *matrix);
bool display_qr_module_at(const display_qr_matrix_t *matrix, int x, int y);
void display_qr_release(display_qr_matrix_t *matrix);

#ifdef __cplusplus
}
#endif
