#include "dut_uart.h"

#include "board_ctrl.h"
#include "board_pins.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "dut_uart";

#define DUT_UART_PORT UART_NUM_1
#define DUT_UART_RX_TASK_STACK 3072
#define DUT_UART_RX_TASK_PRIO 10
#define DUT_UART_EVENT_BUF 256
#define DUT_UART_TX_BUF 0

static SemaphoreHandle_t s_mutex = NULL;
static SemaphoreHandle_t s_rx_notify = NULL;
static bool s_inited = false;
static bool s_open = false;
static dut_uart_config_t s_cfg;
static TaskHandle_t s_rx_task = NULL;
static dut_uart_config_changed_cb_t s_config_changed_cb = NULL;

static dut_uart_frame_t s_rx_q[DUT_UART_RX_QUEUE_DEPTH];
static uint8_t s_rx_head = 0;
static uint8_t s_rx_tail = 0;
static uint8_t s_rx_count = 0;

static const int s_allowed_baud[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

void dut_uart_config_set_defaults(dut_uart_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    cfg->baud = 9600;
    cfg->data_bits = 8;
    cfg->stop_bits = 1.0f;
    strncpy(cfg->parity, "none", sizeof(cfg->parity) - 1U);
    cfg->parity[sizeof(cfg->parity) - 1U] = '\0';
}

bool dut_uart_baud_allowed(int baud)
{
    for (size_t i = 0; i < sizeof(s_allowed_baud) / sizeof(s_allowed_baud[0]); ++i) {
        if (s_allowed_baud[i] == baud) {
            return true;
        }
    }
    return false;
}

static bool stop_is_near(float value, float target)
{
    const float d = value - target;
    return d > -0.05f && d < 0.05f;
}

bool dut_uart_framing_allowed(const dut_uart_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    if (!dut_uart_baud_allowed(cfg->baud)) {
        return false;
    }
    if (cfg->data_bits < 5 || cfg->data_bits > 8) {
        return false;
    }
    if (strcmp(cfg->parity, "none") != 0 && strcmp(cfg->parity, "even") != 0 &&
        strcmp(cfg->parity, "odd") != 0) {
        return false;
    }
    if (stop_is_near(cfg->stop_bits, 1.5f)) {
        return cfg->data_bits == 5;
    }
    if (stop_is_near(cfg->stop_bits, 1.0f) || stop_is_near(cfg->stop_bits, 2.0f)) {
        return true;
    }
    return false;
}

static void rx_clear_unlocked(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    s_rx_count = 0;
    if (s_rx_notify != NULL) {
        (void)xSemaphoreTake(s_rx_notify, 0);
    }
}

static void rx_enqueue_unlocked(const uint8_t *data, size_t len)
{
    if (len > DUT_UART_FRAME_MAX_BYTES) {
        len = DUT_UART_FRAME_MAX_BYTES;
    }
    if (s_rx_count >= DUT_UART_RX_QUEUE_DEPTH) {
        s_rx_head = (uint8_t)((s_rx_head + 1U) % DUT_UART_RX_QUEUE_DEPTH);
        s_rx_count--;
    }
    dut_uart_frame_t *slot = &s_rx_q[s_rx_tail];
    slot->len = (uint16_t)len;
    if (len > 0U && data != NULL) {
        memcpy(slot->data, data, len);
    }
    s_rx_tail = (uint8_t)((s_rx_tail + 1U) % DUT_UART_RX_QUEUE_DEPTH);
    s_rx_count++;
    if (s_rx_notify != NULL) {
        (void)xSemaphoreGive(s_rx_notify);
    }
}

static uart_word_length_t map_data_bits(int data_bits)
{
    switch (data_bits) {
    case 5:
        return UART_DATA_5_BITS;
    case 6:
        return UART_DATA_6_BITS;
    case 7:
        return UART_DATA_7_BITS;
    case 8:
    default:
        return UART_DATA_8_BITS;
    }
}

static uart_parity_t map_parity(const char *parity)
{
    if (parity != NULL && strcmp(parity, "even") == 0) {
        return UART_PARITY_EVEN;
    }
    if (parity != NULL && strcmp(parity, "odd") == 0) {
        return UART_PARITY_ODD;
    }
    return UART_PARITY_DISABLE;
}

static uart_stop_bits_t map_stop_bits(float stop_bits)
{
    if (stop_is_near(stop_bits, 1.5f)) {
        return UART_STOP_BITS_1_5;
    }
    if (stop_is_near(stop_bits, 2.0f)) {
        return UART_STOP_BITS_2;
    }
    return UART_STOP_BITS_1;
}

static esp_err_t apply_hw_config_unlocked(const dut_uart_config_t *cfg)
{
    uart_config_t uc = {
        .baud_rate = cfg->baud,
        .data_bits = map_data_bits(cfg->data_bits),
        .parity = map_parity(cfg->parity),
        .stop_bits = map_stop_bits(cfg->stop_bits),
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(DUT_UART_PORT, &uc);
    if (err != ESP_OK) {
        return err;
    }
    return uart_set_pin(DUT_UART_PORT, BOARD_UART_TX_GPIO, BOARD_UART_RX_GPIO, UART_PIN_NO_CHANGE,
                        UART_PIN_NO_CHANGE);
}

static void rx_task(void *arg)
{
    (void)arg;
    uint8_t acc[DUT_UART_FRAME_MAX_BYTES];
    size_t acc_len = 0;

    for (;;) {
        if (!s_open) {
            acc_len = 0;
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        const TickType_t wait_ticks =
            (acc_len > 0U) ? pdMS_TO_TICKS(DUT_UART_RX_IDLE_GAP_MS) : pdMS_TO_TICKS(50);
        uint8_t byte = 0;
        const int n = uart_read_bytes(DUT_UART_PORT, &byte, 1, wait_ticks);
        if (n > 0) {
            if (acc_len < DUT_UART_FRAME_MAX_BYTES) {
                acc[acc_len++] = byte;
            }
            if (acc_len >= DUT_UART_FRAME_MAX_BYTES) {
                if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                    if (s_open) {
                        rx_enqueue_unlocked(acc, acc_len);
                    }
                    xSemaphoreGive(s_mutex);
                }
                acc_len = 0;
            }
        } else if (acc_len > 0U) {
            /* Idle gap after at least one byte → one frame. */
            if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                if (s_open) {
                    rx_enqueue_unlocked(acc, acc_len);
                }
                xSemaphoreGive(s_mutex);
            }
            acc_len = 0;
        }
    }
}

esp_err_t dut_uart_init(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_rx_notify == NULL) {
        s_rx_notify = xSemaphoreCreateBinary();
        if (s_rx_notify == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (s_inited) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }

    dut_uart_config_set_defaults(&s_cfg);

    esp_err_t err = uart_driver_install(DUT_UART_PORT, DUT_UART_EVENT_BUF * 2, DUT_UART_TX_BUF, 0,
                                        NULL, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        xSemaphoreGive(s_mutex);
        return err;
    }

    err = apply_hw_config_unlocked(&s_cfg);
    if (err != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return err;
    }

    (void)board_ctrl_rs485_de_set(false);

    if (s_rx_task == NULL) {
        BaseType_t ok = xTaskCreate(rx_task, "dut_uart_rx", DUT_UART_RX_TASK_STACK, NULL,
                                    DUT_UART_RX_TASK_PRIO, &s_rx_task);
        if (ok != pdPASS) {
            xSemaphoreGive(s_mutex);
            return ESP_ERR_NO_MEM;
        }
    }

    s_open = false;
    rx_clear_unlocked();
    s_inited = true;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "init 9600 8N1 TX=GPIO%d RX=GPIO%d idle_gap_ms=%u", (int)BOARD_UART_TX_GPIO,
             (int)BOARD_UART_RX_GPIO, (unsigned)DUT_UART_RX_IDLE_GAP_MS);
    return ESP_OK;
}

bool dut_uart_is_open(void)
{
    if (s_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    const bool open = s_inited && s_open;
    xSemaphoreGive(s_mutex);
    return open;
}

esp_err_t dut_uart_config_get(dut_uart_config_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    *out = s_cfg;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t dut_uart_config_set(const dut_uart_config_t *cfg)
{
    if (!dut_uart_framing_allowed(cfg)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (s_open) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = apply_hw_config_unlocked(cfg);
    if (err == ESP_OK) {
        s_cfg = *cfg;
        s_cfg.parity[sizeof(s_cfg.parity) - 1U] = '\0';
    }
    xSemaphoreGive(s_mutex);
    if (err == ESP_OK && s_config_changed_cb != NULL) {
        s_config_changed_cb();
    }
    return err;
}

void dut_uart_set_config_changed_callback(dut_uart_config_changed_cb_t cb)
{
    s_config_changed_cb = cb;
}

esp_err_t dut_uart_stream_open(const dut_uart_config_t *cfg_or_null)
{
    if (s_mutex == NULL || !s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (s_open) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    bool cfg_changed = false;
    if (cfg_or_null != NULL) {
        if (!dut_uart_framing_allowed(cfg_or_null)) {
            xSemaphoreGive(s_mutex);
            return ESP_ERR_INVALID_ARG;
        }
        esp_err_t err = apply_hw_config_unlocked(cfg_or_null);
        if (err != ESP_OK) {
            xSemaphoreGive(s_mutex);
            return err;
        }
        s_cfg = *cfg_or_null;
        s_cfg.parity[sizeof(s_cfg.parity) - 1U] = '\0';
        cfg_changed = true;
    }

    (void)uart_flush_input(DUT_UART_PORT);
    rx_clear_unlocked();
    (void)board_ctrl_rs485_de_set(false);
    s_open = true;
    xSemaphoreGive(s_mutex);

    if (cfg_changed && s_config_changed_cb != NULL) {
        s_config_changed_cb();
    }

    ESP_LOGI(TAG, "stream open baud=%d", s_cfg.baud);
    return ESP_OK;
}

esp_err_t dut_uart_stream_close(void)
{
    if (s_mutex == NULL) {
        return ESP_OK;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    s_open = false;
    rx_clear_unlocked();
    (void)board_ctrl_rs485_de_set(false);
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "stream closed; DE inactive");
    return ESP_OK;
}

esp_err_t dut_uart_stream_write(const uint8_t *data, size_t len, size_t *written)
{
    if (written != NULL) {
        *written = 0;
    }
    if (len > DUT_UART_FRAME_MAX_BYTES) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len > 0U && data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL || !s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (!s_open) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreGive(s_mutex);

    board_uart_path_t path = BOARD_UART_PATH_TTL;
    (void)board_ctrl_uart_path_get(&path);
    const bool use_de = (path == BOARD_UART_PATH_RS485) && (len > 0U);

    if (use_de) {
        const esp_err_t de_err = board_ctrl_rs485_de_set(true);
        if (de_err != ESP_OK) {
            return de_err;
        }
    } else {
        (void)board_ctrl_rs485_de_set(false);
    }

    int wrote = 0;
    if (len > 0U) {
        wrote = uart_write_bytes(DUT_UART_PORT, (const char *)data, len);
    }

    esp_err_t wait_err = ESP_OK;
    if (len > 0U && wrote >= 0) {
        wait_err = uart_wait_tx_done(DUT_UART_PORT, pdMS_TO_TICKS(DUT_UART_TX_DONE_TIMEOUT_MS));
    }

    /* Always force DE back to receive after RS-485 activity (incl. timeout). */
    if (path == BOARD_UART_PATH_RS485) {
        (void)board_ctrl_rs485_de_set(false);
    }

    if (wrote < 0) {
        return ESP_FAIL;
    }
    if (wait_err == ESP_ERR_TIMEOUT) {
        return ESP_ERR_TIMEOUT;
    }
    if (wait_err != ESP_OK) {
        return wait_err;
    }
    if (written != NULL) {
        *written = (size_t)wrote;
    }
    return ESP_OK;
}

esp_err_t dut_uart_rx_wait(uint32_t timeout_ms)
{
    if (s_rx_notify == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (timeout_ms == 0U) {
        return ESP_OK;
    }
    (void)xSemaphoreTake(s_rx_notify, pdMS_TO_TICKS(timeout_ms));
    return ESP_OK;
}

bool dut_uart_rx_try_pop(dut_uart_frame_t *out)
{
    if (out == NULL || s_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    if (s_rx_count == 0) {
        xSemaphoreGive(s_mutex);
        return false;
    }
    *out = s_rx_q[s_rx_head];
    s_rx_head = (uint8_t)((s_rx_head + 1U) % DUT_UART_RX_QUEUE_DEPTH);
    s_rx_count--;
    xSemaphoreGive(s_mutex);
    return true;
}

size_t dut_uart_rx_count(void)
{
    if (s_mutex == NULL) {
        return 0;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }
    const size_t n = s_rx_count;
    xSemaphoreGive(s_mutex);
    return n;
}
