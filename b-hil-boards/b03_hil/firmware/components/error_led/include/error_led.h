#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ERROR_LED_PATTERN_OFF = 0,
    ERROR_LED_PATTERN_SLOW_BLINK,
    ERROR_LED_PATTERN_ON,
    ERROR_LED_PATTERN_FAST_BLINK,
} error_led_pattern_t;

esp_err_t error_led_init(void);
esp_err_t error_led_set_pattern(error_led_pattern_t pattern);

#ifdef __cplusplus
}
#endif
