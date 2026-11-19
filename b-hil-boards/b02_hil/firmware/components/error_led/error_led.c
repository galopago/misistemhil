#include "error_led.h"

#include "board_pins.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "error_led";

#define ERROR_LED_SLOW_ON_MS 500U
#define ERROR_LED_SLOW_OFF_MS 500U
#define ERROR_LED_FAST_ON_MS 125U
#define ERROR_LED_FAST_OFF_MS 125U

static error_led_pattern_t s_pattern = ERROR_LED_PATTERN_OFF;
static esp_timer_handle_t s_blink_timer = NULL;
static bool s_led_on = false;
static uint32_t s_blink_on_ms = 0U;
static uint32_t s_blink_off_ms = 0U;
static bool s_initialized = false;

static int inactive_gpio_level(void)
{
    return BOARD_ERROR_LED_ACTIVE_LEVEL == 0 ? 1 : 0;
}

static void apply_led_level(bool on)
{
    const int level = on ? BOARD_ERROR_LED_ACTIVE_LEVEL : inactive_gpio_level();
    gpio_set_level(BOARD_ERROR_LED_GPIO, level);
    s_led_on = on;
}

static void stop_blink_timer(void)
{
    if (s_blink_timer != NULL) {
        (void)esp_timer_stop(s_blink_timer);
    }
}

static void blink_timer_cb(void *arg)
{
    (void)arg;

    if (s_pattern != ERROR_LED_PATTERN_SLOW_BLINK &&
        s_pattern != ERROR_LED_PATTERN_FAST_BLINK) {
        stop_blink_timer();
        return;
    }

    if (s_led_on) {
        apply_led_level(false);
        (void)esp_timer_start_once(s_blink_timer, (uint64_t)s_blink_off_ms * 1000ULL);
        return;
    }

    apply_led_level(true);
    (void)esp_timer_start_once(s_blink_timer, (uint64_t)s_blink_on_ms * 1000ULL);
}

static void start_blink(uint32_t on_ms, uint32_t off_ms)
{
    s_blink_on_ms = on_ms;
    s_blink_off_ms = off_ms;
    stop_blink_timer();
    apply_led_level(true);
    ESP_RETURN_VOID_ON_FALSE(esp_timer_start_once(s_blink_timer, (uint64_t)on_ms * 1000ULL) ==
                                 ESP_OK,
                             TAG, "blink timer start failed");
}

static esp_err_t apply_pattern(error_led_pattern_t pattern)
{
    stop_blink_timer();

    switch (pattern) {
    case ERROR_LED_PATTERN_OFF:
        apply_led_level(false);
        break;
    case ERROR_LED_PATTERN_ON:
        apply_led_level(true);
        break;
    case ERROR_LED_PATTERN_SLOW_BLINK:
        start_blink(ERROR_LED_SLOW_ON_MS, ERROR_LED_SLOW_OFF_MS);
        break;
    case ERROR_LED_PATTERN_FAST_BLINK:
        start_blink(ERROR_LED_FAST_ON_MS, ERROR_LED_FAST_OFF_MS);
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    s_pattern = pattern;
    return ESP_OK;
}

esp_err_t error_led_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    gpio_config_t config = {
        .pin_bit_mask = 1ULL << BOARD_ERROR_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "gpio config failed");

    const esp_timer_create_args_t timer_args = {
        .callback = blink_timer_cb,
        .name = "error_led",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_blink_timer), TAG,
                        "blink timer create failed");

    ESP_RETURN_ON_ERROR(apply_pattern(ERROR_LED_PATTERN_OFF), TAG, "initial pattern failed");
    s_initialized = true;
    return ESP_OK;
}

esp_err_t error_led_set_pattern(error_led_pattern_t pattern)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    if (pattern == s_pattern) {
        return ESP_OK;
    }

    ESP_LOGD(TAG, "pattern %d -> %d", (int)s_pattern, (int)pattern);
    return apply_pattern(pattern);
}
