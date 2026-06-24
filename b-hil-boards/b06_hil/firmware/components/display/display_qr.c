#include "display_qr.h"

#include "display_fonts.h"
#include "qrcodegen.h"
#include "setup_url.h"

#include <string.h>

#define DISPLAY_QR_MAX_PAYLOAD_BYTES 32
#define DISPLAY_QR_MAX_MODULE_COUNT 25

static uint8_t s_modules[DISPLAY_QR_MAX_MODULE_COUNT * DISPLAY_QR_MAX_MODULE_COUNT];
static uint8_t s_qrcode_buffer[qrcodegen_BUFFER_LEN_FOR_VERSION(2)];
static uint8_t s_encode_buffer[qrcodegen_BUFFER_LEN_FOR_VERSION(2)];

static void sanitize_payload_copy(const char *input, char *output, size_t output_size)
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

bool display_qr_generate(const char *payload, display_qr_matrix_t *matrix)
{
    if (payload == NULL || matrix == NULL || payload[0] == '\0') {
        if (matrix != NULL) {
            memset(matrix, 0, sizeof(*matrix));
        }
        return false;
    }

    if (!setup_url_validate(payload)) {
        memset(matrix, 0, sizeof(*matrix));
        return false;
    }

    char sanitized[DISPLAY_MAX_LINE_LEN];
    sanitize_payload_copy(payload, sanitized, sizeof(sanitized));
    const size_t payload_len = strlen(sanitized);
    if (payload_len == 0 || payload_len > DISPLAY_QR_MAX_PAYLOAD_BYTES) {
        memset(matrix, 0, sizeof(*matrix));
        return false;
    }

    memcpy(s_encode_buffer, sanitized, payload_len);

    if (!qrcodegen_encodeBinary(s_encode_buffer, payload_len, s_qrcode_buffer, qrcodegen_Ecc_LOW, 1,
                                2, qrcodegen_Mask_AUTO, false)) {
        memset(matrix, 0, sizeof(*matrix));
        return false;
    }

    const int module_count = qrcodegen_getSize(s_qrcode_buffer);
    if (module_count <= 0 || module_count > DISPLAY_QR_MAX_MODULE_COUNT) {
        memset(matrix, 0, sizeof(*matrix));
        return false;
    }

    memset(s_modules, 0, sizeof(s_modules));
    for (int y = 0; y < module_count; ++y) {
        for (int x = 0; x < module_count; ++x) {
            if (qrcodegen_getModule(s_qrcode_buffer, x, y)) {
                s_modules[(size_t)y * (size_t)module_count + (size_t)x] = 1U;
            }
        }
    }

    matrix->width = module_count;
    matrix->height = module_count;
    matrix->modules = s_modules;
    return true;
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
