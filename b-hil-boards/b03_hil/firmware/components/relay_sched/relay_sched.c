#include "relay_sched.h"

#include "relay_ctrl.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "relay_sched";

typedef struct {
    esp_timer_handle_t timer;
    int64_t deadline_us;
    bool armed;
} channel_arm_t;

typedef enum {
    ENC_NONE = 0,
    ENC_QUAD,
    ENC_TRAIN,
} encoder_kind_t;

typedef struct {
    encoder_kind_t kind;
    esp_timer_handle_t timer;
    int channel_a;
    int channel_b;
    int channel; /* train */
    uint32_t steps_total;
    uint32_t steps_done;
    uint32_t phase; /* 0..3 for quad; 0=ON 1=OFF for train within pulse */
    uint32_t pulses_total;
    uint32_t pulses_done;
    uint32_t period_ms;
    uint32_t pulse_ms;
    bool cw;
    bool active;
} encoder_run_t;

static SemaphoreHandle_t s_mutex = NULL;
static bool s_initialized = false;
static channel_arm_t s_ch[RELAY_CTRL_CHANNEL_MAX];
static encoder_run_t s_enc;

static bool channel_ok(int channel)
{
    return channel >= RELAY_CTRL_CHANNEL_MIN && channel <= RELAY_CTRL_CHANNEL_MAX;
}

static int idx(int channel)
{
    return channel - 1;
}

static void release_timer_cb(void *arg)
{
    const int channel = (int)(intptr_t)arg;
    if (!channel_ok(channel)) {
        return;
    }

    if (s_mutex != NULL && xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        channel_arm_t *arm = &s_ch[idx(channel)];
        arm->armed = false;
        arm->deadline_us = 0;
        xSemaphoreGive(s_mutex);
    }

    /* Natural one-shot end: drive OFF then clear OLED token to 0. */
    (void)relay_ctrl_drive(channel, false);
    (void)relay_ctrl_set_view_mode(channel, RELAY_VIEW_STATIC_OFF);
}

static void cancel_channel_unlocked(int channel, bool force_off)
{
    channel_arm_t *arm = &s_ch[idx(channel)];
    if (arm->timer != NULL && arm->armed) {
        (void)esp_timer_stop(arm->timer);
    }
    arm->armed = false;
    arm->deadline_us = 0;
    if (force_off) {
        (void)relay_ctrl_drive(channel, false);
    }
    /* Clear dynamic P (or leave STATIC_OFF before a following static set). */
    (void)relay_ctrl_set_view_mode(channel, RELAY_VIEW_STATIC_OFF);
}

static void encoder_abort_unlocked(bool leave_off)
{
    if (!s_enc.active) {
        return;
    }

    const encoder_kind_t kind = s_enc.kind;
    const int channel_a = s_enc.channel_a;
    const int channel_b = s_enc.channel_b;
    const int channel = s_enc.channel;
    esp_timer_handle_t timer = s_enc.timer;
    if (timer != NULL) {
        (void)esp_timer_stop(timer);
    }
    if (leave_off) {
        if (kind == ENC_QUAD) {
            (void)relay_ctrl_drive(channel_a, false);
            (void)relay_ctrl_drive(channel_b, false);
        } else if (kind == ENC_TRAIN) {
            (void)relay_ctrl_drive(channel, false);
        }
    }

    /* Always clear T/E tokens when the encoder run ends. */
    if (kind == ENC_QUAD) {
        (void)relay_ctrl_set_view_mode(channel_a, RELAY_VIEW_STATIC_OFF);
        (void)relay_ctrl_set_view_mode(channel_b, RELAY_VIEW_STATIC_OFF);
    } else if (kind == ENC_TRAIN) {
        (void)relay_ctrl_set_view_mode(channel, RELAY_VIEW_STATIC_OFF);
    }

    memset(&s_enc, 0, sizeof(s_enc));
    s_enc.timer = timer;
    s_enc.kind = ENC_NONE;
}

static bool encoder_uses_channel_unlocked(int channel)
{
    if (!s_enc.active) {
        return false;
    }
    if (s_enc.kind == ENC_QUAD) {
        return channel == s_enc.channel_a || channel == s_enc.channel_b;
    }
    if (s_enc.kind == ENC_TRAIN) {
        return channel == s_enc.channel;
    }
    return false;
}

static void apply_quad_phase_unlocked(void)
{
    /* Full-step CW: 00 → 10 → 11 → 01 */
    static const uint8_t a_levels[4] = {0, 1, 1, 0};
    static const uint8_t b_levels[4] = {0, 0, 1, 1};
    const uint32_t phase = s_enc.phase & 3U;
    (void)relay_ctrl_drive(s_enc.channel_a, a_levels[phase] != 0U);
    (void)relay_ctrl_drive(s_enc.channel_b, b_levels[phase] != 0U);
}

static void encoder_timer_cb(void *arg);

static void encoder_schedule_next_unlocked(uint64_t delay_ms)
{
    if (s_enc.timer == NULL || !s_enc.active) {
        return;
    }
    (void)esp_timer_stop(s_enc.timer);
    (void)esp_timer_start_once(s_enc.timer, delay_ms * 1000ULL);
}

static void encoder_timer_cb(void *arg)
{
    (void)arg;
    if (s_mutex == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    if (!s_enc.active) {
        xSemaphoreGive(s_mutex);
        return;
    }

    if (s_enc.kind == ENC_QUAD) {
        if (s_enc.cw) {
            s_enc.phase = (s_enc.phase + 1U) & 3U;
        } else {
            s_enc.phase = (s_enc.phase + 3U) & 3U; /* -1 mod 4 */
        }
        if (s_enc.phase == 0U) {
            /* Completed a 4-state cycle when returning to 0 after advance. */
            s_enc.steps_done++;
            if (s_enc.steps_done >= s_enc.steps_total) {
                (void)relay_ctrl_drive(s_enc.channel_a, false);
                (void)relay_ctrl_drive(s_enc.channel_b, false);
                encoder_abort_unlocked(false);
                xSemaphoreGive(s_mutex);
                return;
            }
        }
        apply_quad_phase_unlocked();
        encoder_schedule_next_unlocked(s_enc.period_ms);
    } else if (s_enc.kind == ENC_TRAIN) {
        if (s_enc.phase == 0U) {
            /* End of ON → go OFF for remainder of period */
            (void)relay_ctrl_drive(s_enc.channel, false);
            s_enc.phase = 1U;
            const uint32_t off_ms = s_enc.period_ms - s_enc.pulse_ms;
            encoder_schedule_next_unlocked(off_ms);
        } else {
            /* End of OFF → next pulse or done */
            s_enc.pulses_done++;
            if (s_enc.pulses_done >= s_enc.pulses_total) {
                encoder_abort_unlocked(false);
                xSemaphoreGive(s_mutex);
                return;
            }
            (void)relay_ctrl_drive(s_enc.channel, true);
            s_enc.phase = 0U;
            encoder_schedule_next_unlocked(s_enc.pulse_ms);
        }
    }

    xSemaphoreGive(s_mutex);
}

static esp_err_t ensure_channel_timer(int channel)
{
    static const char *const k_names[RELAY_CTRL_CHANNEL_MAX] = {
        "rly_rel_1", "rly_rel_2", "rly_rel_3", "rly_rel_4", "rly_rel_5", "rly_rel_6",
    };
    channel_arm_t *arm = &s_ch[idx(channel)];
    if (arm->timer != NULL) {
        return ESP_OK;
    }
    const esp_timer_create_args_t args = {
        .callback = release_timer_cb,
        .arg = (void *)(intptr_t)channel,
        .name = k_names[idx(channel)],
    };
    return esp_timer_create(&args, &arm->timer);
}

static esp_err_t arm_release_unlocked(int channel, uint32_t duration_ms)
{
    esp_err_t err = ensure_channel_timer(channel);
    if (err != ESP_OK) {
        return err;
    }

    if (encoder_uses_channel_unlocked(channel)) {
        encoder_abort_unlocked(true);
    }

    channel_arm_t *arm = &s_ch[idx(channel)];
    (void)esp_timer_stop(arm->timer);

    /* OLED: arm P before first drive ON (pulse / hold / hold_many). */
    (void)relay_ctrl_set_view_mode(channel, RELAY_VIEW_PULSE);

    err = relay_ctrl_drive(channel, true);
    if (err != ESP_OK) {
        (void)relay_ctrl_set_view_mode(channel, RELAY_VIEW_STATIC_OFF);
        return err;
    }

    arm->deadline_us = esp_timer_get_time() + ((int64_t)duration_ms * 1000LL);
    arm->armed = true;
    return esp_timer_start_once(arm->timer, (uint64_t)duration_ms * 1000ULL);
}

esp_err_t relay_sched_init(void)
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

    memset(s_ch, 0, sizeof(s_ch));
    memset(&s_enc, 0, sizeof(s_enc));

    const esp_timer_create_args_t enc_args = {
        .callback = encoder_timer_cb,
        .arg = NULL,
        .name = "rly_enc",
    };
    esp_err_t err = esp_timer_create(&enc_args, &s_enc.timer);
    if (err != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return err;
    }

    for (int ch = RELAY_CTRL_CHANNEL_MIN; ch <= RELAY_CTRL_CHANNEL_MAX; ++ch) {
        err = ensure_channel_timer(ch);
        if (err != ESP_OK) {
            xSemaphoreGive(s_mutex);
            return err;
        }
    }

    s_initialized = true;
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "relay_sched ready");
    return ESP_OK;
}

esp_err_t relay_sched_pulse(int channel, uint32_t duration_ms)
{
    return relay_sched_hold(channel, duration_ms);
}

esp_err_t relay_sched_hold(int channel, uint32_t duration_ms)
{
    if (!channel_ok(channel)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    const esp_err_t err = arm_release_unlocked(channel, duration_ms);
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relay_sched_hold_many(const relay_sched_item_t *items, size_t count)
{
    if (items == NULL || count == 0 || count > (size_t)RELAY_CTRL_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    esp_err_t err = ESP_OK;
    for (size_t i = 0; i < count; ++i) {
        err = arm_release_unlocked(items[i].channel, items[i].duration_ms);
        if (err != ESP_OK) {
            break;
        }
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relay_sched_cancel(int channel)
{
    if (!channel_ok(channel)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    const bool was = s_ch[idx(channel)].armed;
    cancel_channel_unlocked(channel, true);
    xSemaphoreGive(s_mutex);
    return was ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t relay_sched_cancel_all(void)
{
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    for (int ch = RELAY_CTRL_CHANNEL_MIN; ch <= RELAY_CTRL_CHANNEL_MAX; ++ch) {
        cancel_channel_unlocked(ch, true);
    }
    encoder_abort_unlocked(true);
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

bool relay_sched_is_armed(int channel)
{
    if (!channel_ok(channel) || s_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    const bool armed = s_ch[idx(channel)].armed;
    xSemaphoreGive(s_mutex);
    return armed;
}

uint32_t relay_sched_remaining_ms(int channel)
{
    if (!channel_ok(channel) || s_mutex == NULL) {
        return 0;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }
    uint32_t rem = 0;
    const channel_arm_t *arm = &s_ch[idx(channel)];
    if (arm->armed) {
        const int64_t now = esp_timer_get_time();
        if (arm->deadline_us > now) {
            rem = (uint32_t)((arm->deadline_us - now) / 1000LL);
        }
    }
    xSemaphoreGive(s_mutex);
    return rem;
}

esp_err_t relay_sched_preempt_channel(int channel)
{
    if (!channel_ok(channel)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (encoder_uses_channel_unlocked(channel)) {
        encoder_abort_unlocked(true);
    }
    cancel_channel_unlocked(channel, false); /* leave GPIO to caller */
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t relay_sched_preempt_all(void)
{
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    encoder_abort_unlocked(true);
    for (int ch = RELAY_CTRL_CHANNEL_MIN; ch <= RELAY_CTRL_CHANNEL_MAX; ++ch) {
        cancel_channel_unlocked(ch, false);
    }
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t relay_sched_encoder_step(int channel_a, int channel_b, uint32_t steps, bool cw,
                                   uint32_t period_ms)
{
    if (!channel_ok(channel_a) || !channel_ok(channel_b) || channel_a == channel_b || steps < 1U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (s_enc.active) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    cancel_channel_unlocked(channel_a, false);
    cancel_channel_unlocked(channel_b, false);

    s_enc.kind = ENC_QUAD;
    s_enc.channel_a = channel_a;
    s_enc.channel_b = channel_b;
    s_enc.steps_total = steps;
    s_enc.steps_done = 0;
    s_enc.phase = 0;
    s_enc.period_ms = period_ms;
    s_enc.cw = cw;
    s_enc.active = true;

    (void)relay_ctrl_set_view_mode(channel_a, RELAY_VIEW_ENCODER);
    (void)relay_ctrl_set_view_mode(channel_b, RELAY_VIEW_ENCODER);

    apply_quad_phase_unlocked();
    const esp_err_t err = esp_timer_start_once(s_enc.timer, (uint64_t)period_ms * 1000ULL);
    if (err != ESP_OK) {
        encoder_abort_unlocked(true);
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relay_sched_encoder_pulse_train(int channel, uint32_t pulses, uint32_t period_ms,
                                          uint32_t pulse_ms)
{
    if (!channel_ok(channel) || pulses < 1U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    if (s_enc.active) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    cancel_channel_unlocked(channel, false);

    s_enc.kind = ENC_TRAIN;
    s_enc.channel = channel;
    s_enc.pulses_total = pulses;
    s_enc.pulses_done = 0;
    s_enc.phase = 0;
    s_enc.period_ms = period_ms;
    s_enc.pulse_ms = pulse_ms;
    s_enc.active = true;

    (void)relay_ctrl_set_view_mode(channel, RELAY_VIEW_TRAIN);
    (void)relay_ctrl_drive(channel, true);
    const esp_err_t err = esp_timer_start_once(s_enc.timer, (uint64_t)pulse_ms * 1000ULL);
    if (err != ESP_OK) {
        encoder_abort_unlocked(true);
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relay_sched_encoder_cancel(void)
{
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    const bool was = s_enc.active;
    encoder_abort_unlocked(true);
    xSemaphoreGive(s_mutex);
    return was ? ESP_OK : ESP_ERR_NOT_FOUND;
}

bool relay_sched_encoder_active(void)
{
    if (s_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    const bool active = s_enc.active;
    xSemaphoreGive(s_mutex);
    return active;
}
