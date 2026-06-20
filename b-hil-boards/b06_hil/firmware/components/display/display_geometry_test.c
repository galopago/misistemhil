#include "display_geometry_test.h"

#include "display_renderer.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "display_geom";

static bool rect_eq(display_rect_t a, display_rect_t b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

static esp_err_t expect_rect(const char *label, display_rect_t actual, display_rect_t expected)
{
    if (!rect_eq(actual, expected)) {
        ESP_LOGE(TAG, "%s mismatch: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)", label, actual.x,
                 actual.y, actual.width, actual.height, expected.x, expected.y, expected.width,
                 expected.height);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t display_geometry_self_test(void)
{
    display_rect_t bands[8] = {0};
    size_t band_count = 0;

    const display_rect_t full = {.x = 0, .y = 0, .width = 128, .height = 64};
    if (!display_renderer_compute_line_bands(full, 4, bands, 8, &band_count) || band_count != 4) {
        ESP_LOGE(TAG, "Golden 1 band count failed");
        return ESP_FAIL;
    }

    const display_rect_t golden1_bands[4] = {
        {.x = 2, .y = 0, .width = 124, .height = 16},
        {.x = 2, .y = 16, .width = 124, .height = 16},
        {.x = 2, .y = 32, .width = 124, .height = 16},
        {.x = 2, .y = 48, .width = 124, .height = 16},
    };
    for (size_t i = 0; i < 4; ++i) {
        if (expect_rect("Golden1", bands[i], golden1_bands[i]) != ESP_OK) {
            return ESP_FAIL;
        }
    }

    const display_rect_t top = {.x = 0, .y = 0, .width = 128, .height = 32};
    if (!display_renderer_compute_line_bands(top, 2, bands, 8, &band_count) || band_count != 2) {
        ESP_LOGE(TAG, "Golden 2 top bands failed");
        return ESP_FAIL;
    }
    if (expect_rect("Golden2 top0", bands[0], ((display_rect_t){2, 0, 124, 16})) != ESP_OK) {
        return ESP_FAIL;
    }
    if (expect_rect("Golden2 top1", bands[1], ((display_rect_t){2, 16, 124, 16})) != ESP_OK) {
        return ESP_FAIL;
    }

    const display_rect_t bottom = {.x = 0, .y = 32, .width = 128, .height = 32};
    if (!display_renderer_compute_line_bands(bottom, 1, bands, 8, &band_count) || band_count != 1) {
        ESP_LOGE(TAG, "Golden 2 bottom band failed");
        return ESP_FAIL;
    }
    if (expect_rect("Golden2 bottom", bands[0], ((display_rect_t){2, 32, 124, 32})) != ESP_OK) {
        return ESP_FAIL;
    }

    const display_rect_t alert = {.x = 0, .y = 0, .width = 128, .height = 16};
    if (!display_renderer_compute_line_bands(alert, 1, bands, 8, &band_count) || band_count != 1) {
        ESP_LOGE(TAG, "Golden 3 band failed");
        return ESP_FAIL;
    }
    if (expect_rect("Golden3", bands[0], ((display_rect_t){2, 0, 124, 16})) != ESP_OK) {
        return ESP_FAIL;
    }

    const display_rect_t partial = {.x = -10, .y = 48, .width = 80, .height = 32};
    const display_rect_t clipped = display_renderer_clip_rect(partial, full);
    if (expect_rect("Golden4 clip", clipped, ((display_rect_t){0, 48, 70, 16})) != ESP_OK) {
        return ESP_FAIL;
    }
    if (!display_renderer_compute_line_bands(clipped, 1, bands, 8, &band_count) || band_count != 1) {
        ESP_LOGE(TAG, "Golden 4 band failed");
        return ESP_FAIL;
    }
    if (expect_rect("Golden4 band", bands[0], ((display_rect_t){2, 48, 66, 16})) != ESP_OK) {
        return ESP_FAIL;
    }

    const display_rect_t qr_rect = display_renderer_clip_rect(
        ((display_rect_t){.x = 0, .y = 0, .width = 64, .height = 64}), full);
    const display_rect_t text_rect = display_renderer_clip_rect(
        ((display_rect_t){.x = 64, .y = 0, .width = 64, .height = 64}), full);
    if (expect_rect("Golden5 qr", qr_rect, ((display_rect_t){0, 0, 64, 64})) != ESP_OK) {
        return ESP_FAIL;
    }
    if (expect_rect("Golden5 text", text_rect, ((display_rect_t){64, 0, 64, 64})) != ESP_OK) {
        return ESP_FAIL;
    }

    if (!display_renderer_compute_line_bands(text_rect, 4, bands, 8, &band_count) ||
        band_count != 4) {
        ESP_LOGE(TAG, "Golden 5 text bands failed");
        return ESP_FAIL;
    }
    if (expect_rect("Golden5 text0", bands[0], ((display_rect_t){66, 0, 60, 16})) != ESP_OK) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Display geometry self-test passed");
    return ESP_OK;
}
