#include "board_ctrl.h"

#include "board_pins.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "board_ctrl";

static SemaphoreHandle_t s_mutex = NULL;
static bool s_initialized = false;
static bool s_shadow[BOARD_CTRL_RELAY_CHANNEL_MAX];
static board_ctrl_changed_cb_t s_changed_cb = NULL;

static const gpio_num_t s_relay_ctr_gpios[BOARD_CTRL_RELAY_CHANNEL_MAX] = {
    BOARD_K1_CTR_GPIO,
    BOARD_K2_CTR_GPIO,
    BOARD_K3_CTR_GPIO,
    BOARD_K4_CTR_GPIO,
};

static const gpio_num_t s_boot_ctr_gpios[] = {
    BOARD_K1_CTR_GPIO,
    BOARD_K2_CTR_GPIO,
    BOARD_K3_CTR_GPIO,
    BOARD_K4_CTR_GPIO,
    BOARD_485_CTR_GPIO,
};

static int oe_inactive_level(void)
{
    return BOARD_CTRL_OE_ACTIVE_LEVEL == 0 ? 1 : 0;
}

static int ctr_inactive_level(void)
{
    return BOARD_RELAY_CTR_ACTIVE_LEVEL == 0 ? 1 : 0;
}

static bool channel_valid(int channel)
{
    return channel >= BOARD_CTRL_RELAY_CHANNEL_MIN && channel <= BOARD_CTRL_RELAY_CHANNEL_MAX;
}

esp_err_t board_ctrl_init(void)
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
     * B02_GPIO_BRINGUP_SAFE: latch OUT before enabling push-pull.
     * gpio_config(OUTPUT) first would drive default OUT=0 → /KOE active glitch.
     */
    const int oe_inactive = oe_inactive_level();
    const int ctr_inactive = ctr_inactive_level();

    /* Step 2: preload OE inactive, THEN claim as output. */
    gpio_set_level(BOARD_CTRL_OE_GPIO, oe_inactive);
    gpio_config_t oe_cfg = {
        .pin_bit_mask = 1ULL << BOARD_CTRL_OE_GPIO,
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

    /* Step 3: preload K1..K4 CTR and 485_CTR inactive, THEN enable as outputs. */
    for (size_t i = 0; i < sizeof(s_boot_ctr_gpios) / sizeof(s_boot_ctr_gpios[0]); ++i) {
        gpio_set_level(s_boot_ctr_gpios[i],
                       (s_boot_ctr_gpios[i] == BOARD_485_CTR_GPIO) ? BOARD_485_CTR_INACTIVE_LEVEL
                                                                   : ctr_inactive);
    }
    uint64_t ctr_mask = 0;
    for (size_t i = 0; i < sizeof(s_boot_ctr_gpios) / sizeof(s_boot_ctr_gpios[0]); ++i) {
        ctr_mask |= 1ULL << s_boot_ctr_gpios[i];
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

    /* Step 4: re-assert all five control outputs inactive; clear relay shadow. */
    for (size_t i = 0; i < sizeof(s_boot_ctr_gpios) / sizeof(s_boot_ctr_gpios[0]); ++i) {
        gpio_set_level(s_boot_ctr_gpios[i],
                       (s_boot_ctr_gpios[i] == BOARD_485_CTR_GPIO) ? BOARD_485_CTR_INACTIVE_LEVEL
                                                                   : ctr_inactive);
    }
    for (int i = 0; i < BOARD_CTRL_RELAY_CHANNEL_MAX; ++i) {
        s_shadow[i] = false;
    }

    /* Step 5: enable OE (active low) with actuators still cold. */
    gpio_set_level(BOARD_CTRL_OE_GPIO, BOARD_CTRL_OE_ACTIVE_LEVEL);

    /* Step 6: mark initialized. */
    s_initialized = true;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "OE enabled; K1..K4 and 485_CTR held inactive (cold)");
    return ESP_OK;
}

bool board_ctrl_is_ready(void)
{
    if (s_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    const bool ready = s_initialized;
    xSemaphoreGive(s_mutex);
    return ready;
}

esp_err_t board_ctrl_relay_set(int channel, bool on)
{
    if (!channel_valid(channel)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    const int idx = channel - 1;
    const int level = on ? BOARD_RELAY_CTR_ACTIVE_LEVEL : ctr_inactive_level();
    gpio_set_level(s_relay_ctr_gpios[idx], level);
    s_shadow[idx] = on;

    xSemaphoreGive(s_mutex);
    ESP_LOGD(TAG, "relay_set channel=%d on=%d", channel, on ? 1 : 0);
    if (s_changed_cb != NULL) {
        s_changed_cb();
    }
    return ESP_OK;
}

void board_ctrl_set_changed_callback(board_ctrl_changed_cb_t cb)
{
    s_changed_cb = cb;
}

esp_err_t board_ctrl_relay_get(int channel, bool *on_out)
{
    if (!channel_valid(channel) || on_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    *on_out = s_shadow[channel - 1];
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t board_ctrl_uart_path_set(board_uart_path_t path)
{
    if (path != BOARD_UART_PATH_TTL && path != BOARD_UART_PATH_RS485) {
        return ESP_ERR_INVALID_ARG;
    }
    return board_ctrl_relay_set(1, path == BOARD_UART_PATH_RS485);
}

esp_err_t board_ctrl_uart_path_get(board_uart_path_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    bool on = false;
    const esp_err_t err = board_ctrl_relay_get(1, &on);
    if (err != ESP_OK) {
        return err;
    }
    *out = on ? BOARD_UART_PATH_RS485 : BOARD_UART_PATH_TTL;
    return ESP_OK;
}

esp_err_t board_ctrl_uart_line_set(bool term, bool pullup, bool pulldown, bool has_term,
                                   bool has_pullup, bool has_pulldown)
{
    if (!has_term && !has_pullup && !has_pulldown) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    if (has_term) {
        err = board_ctrl_relay_set(2, term);
        if (err != ESP_OK) {
            return err;
        }
    }
    if (has_pulldown) {
        err = board_ctrl_relay_set(3, pulldown);
        if (err != ESP_OK) {
            return err;
        }
    }
    if (has_pullup) {
        err = board_ctrl_relay_set(4, pullup);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t board_ctrl_uart_line_get(bool *term, bool *pullup, bool *pulldown)
{
    if (term == NULL || pullup == NULL || pulldown == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = board_ctrl_relay_get(2, term);
    if (err != ESP_OK) {
        return err;
    }
    err = board_ctrl_relay_get(3, pulldown);
    if (err != ESP_OK) {
        return err;
    }
    return board_ctrl_relay_get(4, pullup);
}

esp_err_t board_ctrl_rs485_de_set(bool drive)
{
    if (s_mutex == NULL || !s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_initialized) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    const int level = drive ? BOARD_485_CTR_DRIVE_LEVEL : BOARD_485_CTR_INACTIVE_LEVEL;
    gpio_set_level(BOARD_485_CTR_GPIO, level);
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}
