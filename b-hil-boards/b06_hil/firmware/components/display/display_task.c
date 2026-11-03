#include "display_task.h"

#include <string.h>

#include "display_driver.h"
#include "display_renderer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "display_task";

#define DISPLAY_TASK_STACK_SIZE 8192

typedef enum {
    DISPLAY_MSG_SET_STATE = 0,
    DISPLAY_MSG_SET_LAYOUT,
    DISPLAY_MSG_SET_CONTENT,
    DISPLAY_MSG_CLEAR,
    DISPLAY_MSG_REFRESH,
} display_msg_type_t;

typedef struct {
    display_msg_type_t type;
} display_msg_t;

static QueueHandle_t s_msg_queue = NULL;
static TaskHandle_t s_task_handle = NULL;
static display_config_t s_config = DISPLAY_CONFIG_DEFAULT();
static display_state_t s_current_state = {0};
static display_canvas_t s_canvas = {0};
static bool s_has_state = false;
static bool s_force_refresh = false;
static int64_t s_last_flush_us = 0;
static display_state_t s_last_rendered_state = {0};
static bool s_has_rendered_state = false;

static display_state_t s_mailbox_state = {0};
static display_state_t s_post_scratch = {0};
static portMUX_TYPE s_mailbox_mux = portMUX_INITIALIZER_UNLOCKED;

static bool states_equal(const display_state_t *a, const display_state_t *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    return memcmp(a, b, sizeof(display_state_t)) == 0;
}

static void mailbox_store_state(const display_state_t *state)
{
    if (state == NULL) {
        return;
    }
    portENTER_CRITICAL(&s_mailbox_mux);
    s_mailbox_state = *state;
    portEXIT_CRITICAL(&s_mailbox_mux);
}

static void mailbox_load_state(display_state_t *dest)
{
    portENTER_CRITICAL(&s_mailbox_mux);
    *dest = s_mailbox_state;
    portEXIT_CRITICAL(&s_mailbox_mux);
}

static void render_and_flush(bool force)
{
    const int refresh_hz = s_config.refresh_limit_hz > 0 ? s_config.refresh_limit_hz
                                                         : DISPLAY_DEFAULT_REFRESH_LIMIT_HZ;
    const int64_t min_interval_us = 1000000LL / refresh_hz;
    const int64_t now_us = esp_timer_get_time();

    if (!force && !s_force_refresh && s_has_state && s_has_rendered_state &&
        states_equal(&s_current_state, &s_last_rendered_state)) {
        return;
    }

    if (!force && !s_force_refresh && s_last_flush_us > 0 &&
        (now_us - s_last_flush_us) < min_interval_us) {
        return;
    }

    if (!s_has_state) {
        display_canvas_clear(&s_canvas, DISPLAY_PIXEL_OFF);
    } else {
        display_renderer_render(&s_current_state, &s_config, &s_canvas);
    }

    display_driver_flush(&s_canvas);
    if (s_has_state) {
        s_last_rendered_state = s_current_state;
        s_has_rendered_state = true;
    } else {
        s_has_rendered_state = false;
    }
    s_last_flush_us = now_us;
    s_force_refresh = false;
}

static void apply_message(const display_msg_t *msg)
{
    switch (msg->type) {
    case DISPLAY_MSG_SET_STATE:
        mailbox_load_state(&s_current_state);
        s_has_state = true;
        break;
    case DISPLAY_MSG_SET_LAYOUT:
    case DISPLAY_MSG_SET_CONTENT:
        mailbox_load_state(&s_current_state);
        s_current_state.dirty = true;
        s_has_state = true;
        break;
    case DISPLAY_MSG_CLEAR:
        memset(&s_current_state, 0, sizeof(s_current_state));
        s_has_state = false;
        break;
    case DISPLAY_MSG_REFRESH:
        s_force_refresh = true;
        break;
    default:
        break;
    }
}

static void display_task_main(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Display task running");

    display_msg_t pending = {0};
    bool has_pending = false;

    while (true) {
        display_msg_t msg = {0};
        if (xQueueReceive(s_msg_queue, &msg, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (msg.type == DISPLAY_MSG_SET_STATE || msg.type == DISPLAY_MSG_SET_LAYOUT ||
                msg.type == DISPLAY_MSG_SET_CONTENT) {
                pending = msg;
                has_pending = true;

                display_msg_t newer = {0};
                while (xQueueReceive(s_msg_queue, &newer, 0) == pdTRUE) {
                    if (newer.type == DISPLAY_MSG_REFRESH) {
                        s_force_refresh = true;
                    } else if (newer.type == DISPLAY_MSG_CLEAR) {
                        pending.type = DISPLAY_MSG_CLEAR;
                        has_pending = true;
                    } else if (newer.type == DISPLAY_MSG_SET_STATE ||
                               newer.type == DISPLAY_MSG_SET_LAYOUT ||
                               newer.type == DISPLAY_MSG_SET_CONTENT) {
                        pending = newer;
                        has_pending = true;
                    }
                }
            } else {
                apply_message(&msg);
                render_and_flush(msg.type == DISPLAY_MSG_REFRESH);
                continue;
            }
        }

        if (has_pending) {
            apply_message(&pending);
            has_pending = false;
            render_and_flush(false);
        }
    }
}

static esp_err_t post_message(display_msg_type_t type, const display_state_t *state)
{
    if (s_msg_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (state != NULL && (type == DISPLAY_MSG_SET_STATE || type == DISPLAY_MSG_SET_LAYOUT ||
                          type == DISPLAY_MSG_SET_CONTENT)) {
        mailbox_store_state(state);
    }

    const display_msg_t msg = {.type = type};

    if (xQueueSend(s_msg_queue, &msg, 0) != pdTRUE) {
        display_msg_t dropped = {0};
        (void)xQueueReceive(s_msg_queue, &dropped, 0);
        if (xQueueSend(s_msg_queue, &msg, 0) != pdTRUE) {
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t display_task_start(const display_config_t *config)
{
    if (s_task_handle != NULL) {
        return ESP_OK;
    }

    if (config != NULL) {
        s_config = *config;
    } else {
        s_config = DISPLAY_CONFIG_DEFAULT();
    }

    ESP_ERROR_CHECK(display_driver_init(&s_config));
    display_canvas_init(&s_canvas);

    s_msg_queue = xQueueCreate(4, sizeof(display_msg_t));
    if (s_msg_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const BaseType_t created = xTaskCreate(display_task_main, "display_task", DISPLAY_TASK_STACK_SIZE,
                                           NULL, 5, &s_task_handle);
    if (created != pdPASS) {
        vQueueDelete(s_msg_queue);
        s_msg_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Display task started");
    return ESP_OK;
}

void display_task_stop(void)
{
    if (s_task_handle != NULL) {
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }
    if (s_msg_queue != NULL) {
        vQueueDelete(s_msg_queue);
        s_msg_queue = NULL;
    }
    display_driver_deinit();
}

esp_err_t display_task_post_set_state(const display_state_t *state)
{
    return post_message(DISPLAY_MSG_SET_STATE, state);
}

esp_err_t display_task_post_set_layout(const display_layout_t *layout)
{
    if (layout == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_post_scratch.layout = *layout;
    s_post_scratch.dirty = true;
    return post_message(DISPLAY_MSG_SET_LAYOUT, &s_post_scratch);
}

esp_err_t display_task_post_set_content(const display_layout_t *layout)
{
    return display_task_post_set_layout(layout);
}

esp_err_t display_task_post_clear(void)
{
    return post_message(DISPLAY_MSG_CLEAR, NULL);
}

esp_err_t display_task_post_refresh(void)
{
    const display_msg_t msg = {
        .type = DISPLAY_MSG_REFRESH,
    };
    if (s_msg_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_msg_queue, &msg, 0) != pdTRUE) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
