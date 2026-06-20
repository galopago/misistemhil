#include "display_qr.h"

#include <string.h>

bool display_qr_generate(const char *payload, display_qr_matrix_t *matrix)
{
    (void)payload;
    if (matrix != NULL) {
        memset(matrix, 0, sizeof(*matrix));
    }
    return false;
}

bool display_qr_module_at(const display_qr_matrix_t *matrix, int x, int y)
{
    if (matrix == NULL || matrix->modules == NULL || matrix->width <= 0) {
        return false;
    }
    if (x < 0 || y < 0 || x >= matrix->width || y >= matrix->width) {
        return false;
    }
    const size_t index = (size_t)y * (size_t)matrix->width + (size_t)x;
    return matrix->modules[index] != 0;
}

void display_qr_release(display_qr_matrix_t *matrix)
{
    if (matrix == NULL) {
        return;
    }
    matrix->width = 0;
    matrix->height = 0;
    matrix->modules = NULL;
}
