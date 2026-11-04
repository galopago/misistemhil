#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RELAY_MIN_DURATION_MS 20U
#define RELAY_MAX_PULSE_MS 4000U
#define RELAY_MAX_HOLD_MS 60000U
#define RELAY_MIN_PHASE_MS 20U
#define RELAY_MIN_TRAIN_PERIOD_MS 40U
#define RELAY_MAX_PHASE_MS 1000U
#define RELAY_MAX_SEQUENCE_MS 60000U
#define RELAY_QUAD_STATES_PER_STEP 4U

typedef struct {
    int channel;
    uint32_t duration_ms;
} relay_sched_item_t;

esp_err_t relay_sched_init(void);

esp_err_t relay_sched_pulse(int channel, uint32_t duration_ms);
esp_err_t relay_sched_hold(int channel, uint32_t duration_ms);
esp_err_t relay_sched_hold_many(const relay_sched_item_t *items, size_t count);
esp_err_t relay_sched_cancel(int channel);
esp_err_t relay_sched_cancel_all(void);
bool relay_sched_is_armed(int channel);
uint32_t relay_sched_remaining_ms(int channel);

/** Cancel release timer on channel and abort encoder if it uses that channel. */
esp_err_t relay_sched_preempt_channel(int channel);
/** Cancel all timers and abort encoder (for set_all / release_all). */
esp_err_t relay_sched_preempt_all(void);

esp_err_t relay_sched_encoder_step(int channel_a, int channel_b, uint32_t steps, bool cw,
                                   uint32_t period_ms);
esp_err_t relay_sched_encoder_pulse_train(int channel, uint32_t pulses, uint32_t period_ms,
                                          uint32_t pulse_ms);
esp_err_t relay_sched_encoder_cancel(void);
bool relay_sched_encoder_active(void);

#ifdef __cplusplus
}
#endif
