#include "relay_ctrl.h"

#include "board_pins.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "relay_ctrl";

static SemaphoreHandle_t s_mutex = NULL;
static bool s_initialized = false;
static bool s_oe_enabled = false;
static uint8_t s_shadow_mask = 0;

static const gpio_num_t s_ctr_gpios[BOARD_RELAY_CHANNEL_COUNT] = {
    BOARD_RELAY1_CTR_GPIO, BOARD_RELAY2_CTR_GPIO, BOARD_RELAY3_CTR_GPIO,
    BOARD_RELAY4_CTR_GPIO, BOARD_RELAY5_CTR_GPIO, BOARD_RELAY6_CTR_GPIO,
};

static int oe_inactive_level(void)
{
    return BOARD_RELAY_OE_ACTIVE_LEVEL == 0 ? 1 : 0;
}

static int ctr_inactive_level(void)
{
    return BOARD_RELAY_CTR_ACTIVE_LEVEL == 0 ? 1 : 0;
}

static bool channel_valid(int channel)
{
    return channel >= RELAY_CTRL_CHANNEL_MIN && channel <= RELAY_CTRL_CHANNEL_MAX;
}

static void apply_ctr_levels_unlocked(uint8_t mask)
{
    for (int i = 0; i < BOARD_RELAY_CHANNEL_COUNT; ++i) {
        const bool on = ((mask >> i) & 0x1U) != 0U;
        const int level = on ? BOARD_RELAY_CTR_ACTIVE_LEVEL : ctr_inactive_level();
        gpio_set_level(s_ctr_gpios[i], level);
    }
    s_shadow_mask = (uint8_t)(mask & RELAY_CTRL_MASK_ALL);
}

esp_err_t relay_ctrl_init(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    if (s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }

    /*
     * B03_RELAY_OE_LEVEL_BEFORE_DIR: latch OUT before enabling push-pull.
     * gpio_config(OUTPUT) first would drive default OUT=0 → /KOE active glitch.
     */
    const int oe_inactive = oe_inactive_level();
    const int ctr_inactive = ctr_inactive_level();

    /* Step 2: preload OE inactive, THEN claim as output. */
    gpio_set_level(BOARD_RELAY_OE_GPIO, oe_inactive);
    gpio_config_t oe_cfg = {
        .pin_bit_mask = 1ULL << BOARD_RELAY_OE_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&oe_cfg);
    if (err != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return err;
    }
    s_oe_enabled = false;

    /* Step 3: preload all CTR inactive, THEN enable as outputs. */
    for (int i = 0; i < BOARD_RELAY_CHANNEL_COUNT; ++i) {
        gpio_set_level(s_ctr_gpios[i], ctr_inactive);
    }
    uint64_t ctr_mask = 0;
    for (int i = 0; i < BOARD_RELAY_CHANNEL_COUNT; ++i) {
        ctr_mask |= 1ULL << s_ctr_gpios[i];
    }
    gpio_config_t ctr_cfg = {
        .pin_bit_mask = ctr_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&ctr_cfg);
    if (err != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return err;
    }

    /* Step 4: clear shadow; re-assert CTR lows. */
    s_shadow_mask = 0;
    for (int i = 0; i < BOARD_RELAY_CHANNEL_COUNT; ++i) {
        gpio_set_level(s_ctr_gpios[i], ctr_inactive);
    }

    /* Step 5: enable OE (active low) only with all CTR cold. */
    gpio_set_level(BOARD_RELAY_OE_GPIO, BOARD_RELAY_OE_ACTIVE_LEVEL);
    s_oe_enabled = true;

    /* Step 6: mark initialized. */
    s_initialized = true;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "OE enabled, mask=0x00");
    return ESP_OK;
}

esp_err_t relay_ctrl_set(int channel, bool on)
{
    if (!channel_valid(channel)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    const int idx = channel - 1;
    const uint8_t bit = (uint8_t)(1U << idx);
    uint8_t mask = s_shadow_mask;
    if (on) {
        mask = (uint8_t)(mask | bit);
    } else {
        mask = (uint8_t)(mask & (uint8_t)~bit);
    }
    apply_ctr_levels_unlocked(mask);

    xSemaphoreGive(s_mutex);
    ESP_LOGD(TAG, "set channel=%d on=%d", channel, on ? 1 : 0);
    return ESP_OK;
}

esp_err_t relay_ctrl_set_mask(uint8_t mask)
{
    if ((mask & (uint8_t)~RELAY_CTRL_MASK_ALL) != 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    apply_ctr_levels_unlocked(mask);
    xSemaphoreGive(s_mutex);
    ESP_LOGD(TAG, "set_mask=0x%02X", mask);
    return ESP_OK;
}

esp_err_t relay_ctrl_get(int channel, bool *on_out)
{
    if (!channel_valid(channel) || on_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    *on_out = ((s_shadow_mask >> (channel - 1)) & 0x1U) != 0U;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

uint8_t relay_ctrl_get_mask(void)
{
    if (s_mutex == NULL) {
        return 0;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }
    const uint8_t mask = s_initialized ? s_shadow_mask : 0;
    xSemaphoreGive(s_mutex);
    return mask;
}

bool relay_ctrl_is_output_enabled(void)
{
    if (s_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    const bool enabled = s_initialized && s_oe_enabled;
    xSemaphoreGive(s_mutex);
    return enabled;
}
