#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct i2c_bus_device *i2c_bus_device_t;

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_PIXEL_OFF 0
#define DISPLAY_PIXEL_ON 1

#define DISPLAY_DEFAULT_MARGIN_X 2
#define DISPLAY_DEFAULT_MARGIN_Y 0
#define DISPLAY_DEFAULT_REFRESH_LIMIT_HZ 10
#define DISPLAY_DEFAULT_SEPARATOR_ENABLED false

#define DISPLAY_ASCII_MIN 0x20
#define DISPLAY_ASCII_MAX 0x7E

#define DISPLAY_MAX_REGIONS 8
#define DISPLAY_MAX_LINES 8
#define DISPLAY_MAX_LINE_LEN 64
#define DISPLAY_MAX_REGION_ID_LEN 16

#define DISPLAY_QR_QUIET_ZONE_MODULES 1
#define DISPLAY_QR_PREFERRED_SCALE 2
#define DISPLAY_QR_FALLBACK_SCALE 1

#define DISPLAY_FRAMEBUFFER_BYTES (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)

typedef enum {
    DISPLAY_CONTENT_TEXT = 0,
    DISPLAY_CONTENT_QR = 1,
} display_content_type_t;

typedef enum {
    DISPLAY_FONT_AUTO_FIT = 0,
    DISPLAY_FONT_SMALL = 1,
    DISPLAY_FONT_MEDIUM = 2,
    DISPLAY_FONT_LARGE = 3,
    DISPLAY_FONT_CUSTOM = 4,
} display_font_policy_t;

typedef enum {
    DISPLAY_H_ALIGN_LEFT = 0,
    DISPLAY_H_ALIGN_CENTER = 1,
    DISPLAY_H_ALIGN_RIGHT = 2,
} display_h_align_t;

typedef enum {
    DISPLAY_V_ALIGN_TOP = 0,
    DISPLAY_V_ALIGN_CENTER = 1,
    DISPLAY_V_ALIGN_BOTTOM = 2,
} display_v_align_t;

typedef enum {
    DISPLAY_EMPHASIS_NORMAL = 0,
    DISPLAY_EMPHASIS_INVERTED = 1,
    DISPLAY_EMPHASIS_BOLD = 2,
} display_emphasis_t;

typedef enum {
    DISPLAY_ORIENTATION_NORMAL = 0,
    DISPLAY_ORIENTATION_ROTATED_180 = 1,
} display_orientation_t;

typedef enum {
    DISPLAY_TEMPLATE_FULL_FOUR_LINES = 0,
    DISPLAY_TEMPLATE_FULL_TWO_LINES = 1,
    DISPLAY_TEMPLATE_TOP_TWO_BOTTOM_ONE = 2,
    DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT = 3,
} display_layout_template_t;

typedef enum {
    DISPLAY_QR_SCALE_AUTO = 0,
    DISPLAY_QR_SCALE_1 = 1,
    DISPLAY_QR_SCALE_2 = 2,
} display_qr_scale_t;

typedef enum {
    DISPLAY_QR_EC_LOW = 0,
} display_qr_error_correction_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
} display_rect_t;

typedef struct {
    char text[DISPLAY_MAX_LINE_LEN];
    display_h_align_t align;
    bool has_align;
    display_emphasis_t emphasis;
    bool has_emphasis;
} display_text_line_t;

typedef struct {
    display_text_line_t lines[DISPLAY_MAX_LINES];
    size_t line_count;
    int max_lines;
    display_font_policy_t font_policy;
    display_h_align_t horizontal_align;
    display_v_align_t vertical_align;
    display_emphasis_t emphasis;
} display_text_region_t;

typedef struct {
    char payload[DISPLAY_MAX_LINE_LEN];
    display_qr_error_correction_t error_correction;
    display_qr_scale_t scale;
    int quiet_zone_modules;
} display_qr_region_t;

typedef struct {
    display_content_type_t type;
    display_text_region_t text;
    display_qr_region_t qr;
} display_region_content_t;

typedef struct {
    char id[DISPLAY_MAX_REGION_ID_LEN];
    display_rect_t rect;
    display_region_content_t content;
} display_region_t;

typedef struct {
    display_region_t regions[DISPLAY_MAX_REGIONS];
    size_t region_count;
} display_layout_t;

typedef struct {
    display_layout_t layout;
    bool dirty;
} display_state_t;

typedef struct {
    int width;
    int height;
    uint8_t address;
    bool has_address;
    i2c_bus_device_t i2c_device;
    bool has_i2c_device;
    display_orientation_t orientation;
    int contrast;
    bool has_contrast;
    bool separators_enabled;
    display_layout_template_t default_layout_template;
    int refresh_limit_hz;
} display_config_t;

typedef struct {
    int width;
    int height;
} display_text_metrics_t;

#define DISPLAY_CONFIG_DEFAULT()                                                                     \
    (display_config_t) {                                                                           \
        .width = DISPLAY_WIDTH,                                                                      \
        .height = DISPLAY_HEIGHT,                                                                    \
        .has_address = false,                                                                        \
        .i2c_device = NULL,                                                                          \
        .has_i2c_device = false,                                                                     \
        .orientation = DISPLAY_ORIENTATION_NORMAL,                                                   \
        .has_contrast = false,                                                                       \
        .separators_enabled = DISPLAY_DEFAULT_SEPARATOR_ENABLED,                                     \
        .default_layout_template = DISPLAY_TEMPLATE_FULL_FOUR_LINES,                                 \
        .refresh_limit_hz = DISPLAY_DEFAULT_REFRESH_LIMIT_HZ,                                        \
    }

#ifdef __cplusplus
}
#endif
