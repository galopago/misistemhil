#include "host_api_internal.h"

#include "relay_ctrl.h"
#include "relay_sched.h"

#include <stdio.h>
#include <string.h>

static bool json_is_int(const cJSON *item)
{
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    return item->valuedouble == (double)item->valueint;
}

static bool args_keys_allowed(const cJSON *args, const char *const *allowed, size_t allowed_count)
{
    if (!cJSON_IsObject(args)) {
        return false;
    }
    for (const cJSON *child = args->child; child != NULL; child = child->next) {
        if (child->string == NULL) {
            return false;
        }
        bool found = false;
        for (size_t i = 0; i < allowed_count; ++i) {
            if (strcmp(child->string, allowed[i]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

static bool parse_channel(const cJSON *item, int *out_ch, const char **err_msg, const char *what)
{
    if (!json_is_int(item)) {
        *err_msg = what;
        return false;
    }
    const int ch = item->valueint;
    if (ch < RELAY_CTRL_CHANNEL_MIN || ch > RELAY_CTRL_CHANNEL_MAX) {
        *err_msg = "channel must be 1..6";
        return false;
    }
    *out_ch = ch;
    return true;
}

static bool parse_duration_ms(const cJSON *item, uint32_t min_ms, uint32_t max_ms, uint32_t *out_ms,
                              const char **err_msg)
{
    if (!json_is_int(item) || item->valueint < 0) {
        *err_msg = "duration_ms must be a non-negative int";
        return false;
    }
    const uint32_t ms = (uint32_t)item->valueint;
    if (ms < min_ms || ms > max_ms) {
        static char buf[96];
        snprintf(buf, sizeof(buf), "duration_ms must be %u..%u (min 20 ms floor)", (unsigned)min_ms,
                 (unsigned)max_ms);
        *err_msg = buf;
        return false;
    }
    *out_ms = ms;
    return true;
}

static esp_err_t arm_one_result(int channel, uint32_t duration_ms, cJSON **out_result,
                                const char **err_code, const char **err_msg)
{
    const esp_err_t err = relay_sched_hold(channel, duration_ms);
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "failed to arm release timer";
        return err;
    }

    bool on = false;
    (void)relay_ctrl_get(channel, &on);

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddNumberToObject(result, "channel", channel);
    cJSON_AddNumberToObject(result, "duration_ms", duration_ms);
    cJSON_AddBoolToObject(result, "armed", true);
    cJSON_AddBoolToObject(result, "on", on);
    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_handle_relay_pulse(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    static const char *const allowed[] = {"channel", "duration_ms"};
    if (!args_keys_allowed(args, allowed, 2U)) {
        *err_code = "bad_request";
        *err_msg = "relay_pulse unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    int channel = 0;
    uint32_t duration_ms = 0;
    if (!parse_channel(cJSON_GetObjectItemCaseSensitive(args, "channel"), &channel, err_msg,
                       "relay_pulse requires channel int") ||
        !parse_duration_ms(cJSON_GetObjectItemCaseSensitive(args, "duration_ms"), RELAY_MIN_DURATION_MS,
                           RELAY_MAX_PULSE_MS, &duration_ms, err_msg)) {
        *err_code = "bad_request";
        return ESP_ERR_INVALID_ARG;
    }

    return arm_one_result(channel, duration_ms, out_result, err_code, err_msg);
}

esp_err_t host_api_handle_relay_hold(const cJSON *args, cJSON **out_result, const char **err_code,
                                     const char **err_msg)
{
    static const char *const allowed[] = {"channel", "duration_ms"};
    if (!args_keys_allowed(args, allowed, 2U)) {
        *err_code = "bad_request";
        *err_msg = "relay_hold unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    int channel = 0;
    uint32_t duration_ms = 0;
    if (!parse_channel(cJSON_GetObjectItemCaseSensitive(args, "channel"), &channel, err_msg,
                       "relay_hold requires channel int") ||
        !parse_duration_ms(cJSON_GetObjectItemCaseSensitive(args, "duration_ms"), RELAY_MIN_DURATION_MS,
                           RELAY_MAX_HOLD_MS, &duration_ms, err_msg)) {
        *err_code = "bad_request";
        return ESP_ERR_INVALID_ARG;
    }

    return arm_one_result(channel, duration_ms, out_result, err_code, err_msg);
}

esp_err_t host_api_handle_relay_hold_many(const cJSON *args, cJSON **out_result,
                                          const char **err_code, const char **err_msg)
{
    static const char *const allowed[] = {"items"};
    if (!args_keys_allowed(args, allowed, 1U)) {
        *err_code = "bad_request";
        *err_msg = "relay_hold_many unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *items = cJSON_GetObjectItemCaseSensitive(args, "items");
    if (!cJSON_IsArray(items)) {
        *err_code = "bad_request";
        *err_msg = "relay_hold_many requires items array";
        return ESP_ERR_INVALID_ARG;
    }

    const int n = cJSON_GetArraySize(items);
    if (n < 1 || n > RELAY_CTRL_CHANNEL_MAX) {
        *err_code = "bad_request";
        *err_msg = "relay_hold_many items length must be 1..6";
        return ESP_ERR_INVALID_ARG;
    }

    relay_sched_item_t parsed[RELAY_CTRL_CHANNEL_MAX];
    bool seen[RELAY_CTRL_CHANNEL_MAX + 1] = {false};

    for (int i = 0; i < n; ++i) {
        const cJSON *el = cJSON_GetArrayItem(items, i);
        if (!cJSON_IsObject(el)) {
            *err_code = "bad_request";
            *err_msg = "relay_hold_many items must be objects";
            return ESP_ERR_INVALID_ARG;
        }
        static const char *const item_keys[] = {"channel", "duration_ms"};
        if (!args_keys_allowed(el, item_keys, 2U)) {
            *err_code = "bad_request";
            *err_msg = "relay_hold_many item unknown keys";
            return ESP_ERR_INVALID_ARG;
        }

        int channel = 0;
        uint32_t duration_ms = 0;
        if (!parse_channel(cJSON_GetObjectItemCaseSensitive(el, "channel"), &channel, err_msg,
                           "hold_many item requires channel int") ||
            !parse_duration_ms(cJSON_GetObjectItemCaseSensitive(el, "duration_ms"),
                               RELAY_MIN_DURATION_MS, RELAY_MAX_HOLD_MS, &duration_ms, err_msg)) {
            *err_code = "bad_request";
            return ESP_ERR_INVALID_ARG;
        }
        if (seen[channel]) {
            *err_code = "bad_request";
            *err_msg = "relay_hold_many duplicate channel";
            return ESP_ERR_INVALID_ARG;
        }
        seen[channel] = true;
        parsed[i].channel = channel;
        parsed[i].duration_ms = duration_ms;
    }

    const esp_err_t err = relay_sched_hold_many(parsed, (size_t)n);
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "failed to arm hold_many";
        return err;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *armed = cJSON_CreateArray();
    if (result == NULL || armed == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(armed);
        return ESP_ERR_NO_MEM;
    }
    for (int i = 0; i < n; ++i) {
        cJSON *row = cJSON_CreateObject();
        if (row == NULL) {
            cJSON_Delete(result);
            cJSON_Delete(armed);
            return ESP_ERR_NO_MEM;
        }
        cJSON_AddNumberToObject(row, "channel", parsed[i].channel);
        cJSON_AddNumberToObject(row, "duration_ms", parsed[i].duration_ms);
        if (!cJSON_AddItemToArray(armed, row)) {
            cJSON_Delete(row);
            cJSON_Delete(result);
            cJSON_Delete(armed);
            return ESP_ERR_NO_MEM;
        }
    }
    if (!cJSON_AddItemToObject(result, "armed", armed)) {
        cJSON_Delete(armed);
        cJSON_Delete(result);
        return ESP_ERR_NO_MEM;
    }
    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_handle_relay_hold_cancel(const cJSON *args, cJSON **out_result,
                                            const char **err_code, const char **err_msg)
{
    static const char *const allowed[] = {"channel", "all"};
    if (!args_keys_allowed(args, allowed, 2U)) {
        *err_code = "bad_request";
        *err_msg = "relay_hold_cancel unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *all_item = cJSON_GetObjectItemCaseSensitive(args, "all");
    const cJSON *channel_item = cJSON_GetObjectItemCaseSensitive(args, "channel");
    const bool has_all = (all_item != NULL);
    const bool has_channel = (channel_item != NULL);

    if (!has_all && !has_channel) {
        *err_code = "bad_request";
        *err_msg = "relay_hold_cancel requires channel or all";
        return ESP_ERR_INVALID_ARG;
    }

    bool cancelled = false;

    if (has_all) {
        if (!cJSON_IsBool(all_item)) {
            *err_code = "bad_request";
            *err_msg = "relay_hold_cancel all must be bool";
            return ESP_ERR_INVALID_ARG;
        }
        if (cJSON_IsTrue(all_item)) {
            bool any_armed = false;
            for (int ch = RELAY_CTRL_CHANNEL_MIN; ch <= RELAY_CTRL_CHANNEL_MAX; ++ch) {
                if (relay_sched_is_armed(ch)) {
                    any_armed = true;
                    break;
                }
            }
            const bool enc_active = relay_sched_encoder_active();
            (void)relay_sched_cancel_all();
            cancelled = any_armed || enc_active;
        } else if (!has_channel) {
            *err_code = "bad_request";
            *err_msg = "relay_hold_cancel all:false requires channel";
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (!(has_all && cJSON_IsTrue(all_item))) {
        int channel = 0;
        if (!parse_channel(channel_item, &channel, err_msg, "relay_hold_cancel requires channel int")) {
            *err_code = "bad_request";
            return ESP_ERR_INVALID_ARG;
        }
        const bool was = relay_sched_is_armed(channel);
        const esp_err_t err = relay_sched_cancel(channel);
        if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
            *err_code = "internal";
            *err_msg = "relay_hold_cancel failed";
            return err;
        }
        cancelled = was;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "cancelled", cancelled);
    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_handle_relay_release_all(const cJSON *args, cJSON **out_result,
                                            const char **err_code, const char **err_msg)
{
    if (!args_keys_allowed(args, NULL, 0U)) {
        *err_code = "bad_request";
        *err_msg = "relay_release_all takes no args";
        return ESP_ERR_INVALID_ARG;
    }

    (void)relay_sched_cancel_all();
    const esp_err_t err = relay_ctrl_set_mask(0);
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "relay_release_all driver fault";
        return err;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *states = cJSON_CreateArray();
    if (result == NULL || states == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(states);
        return ESP_ERR_NO_MEM;
    }
    for (int i = 0; i < RELAY_CTRL_CHANNEL_MAX; ++i) {
        cJSON *b = cJSON_CreateBool(false);
        if (b == NULL || !cJSON_AddItemToArray(states, b)) {
            cJSON_Delete(b);
            cJSON_Delete(states);
            cJSON_Delete(result);
            return ESP_ERR_NO_MEM;
        }
    }
    cJSON_AddNumberToObject(result, "mask", 0);
    if (!cJSON_AddItemToObject(result, "states", states)) {
        cJSON_Delete(states);
        cJSON_Delete(result);
        return ESP_ERR_NO_MEM;
    }
    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_handle_encoder_step(const cJSON *args, cJSON **out_result, const char **err_code,
                                       const char **err_msg)
{
    static const char *const allowed[] = {"channel_a", "channel_b", "steps", "direction", "period_ms"};
    if (!args_keys_allowed(args, allowed, 5U)) {
        *err_code = "bad_request";
        *err_msg = "encoder_step unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    int channel_a = 0;
    int channel_b = 0;
    if (!parse_channel(cJSON_GetObjectItemCaseSensitive(args, "channel_a"), &channel_a, err_msg,
                       "encoder_step requires channel_a int") ||
        !parse_channel(cJSON_GetObjectItemCaseSensitive(args, "channel_b"), &channel_b, err_msg,
                       "encoder_step requires channel_b int")) {
        *err_code = "bad_request";
        return ESP_ERR_INVALID_ARG;
    }
    if (channel_a == channel_b) {
        *err_code = "bad_request";
        *err_msg = "encoder_step channel_a and channel_b must be distinct";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *steps_item = cJSON_GetObjectItemCaseSensitive(args, "steps");
    const cJSON *dir_item = cJSON_GetObjectItemCaseSensitive(args, "direction");
    const cJSON *period_item = cJSON_GetObjectItemCaseSensitive(args, "period_ms");

    if (!json_is_int(steps_item) || steps_item->valueint < 1) {
        *err_code = "bad_request";
        *err_msg = "encoder_step steps must be int >= 1";
        return ESP_ERR_INVALID_ARG;
    }
    if (!cJSON_IsString(dir_item) || dir_item->valuestring == NULL) {
        *err_code = "bad_request";
        *err_msg = "encoder_step direction must be \"cw\" or \"ccw\"";
        return ESP_ERR_INVALID_ARG;
    }
    const bool cw = (strcmp(dir_item->valuestring, "cw") == 0);
    if (!cw && strcmp(dir_item->valuestring, "ccw") != 0) {
        *err_code = "bad_request";
        *err_msg = "encoder_step direction must be \"cw\" or \"ccw\"";
        return ESP_ERR_INVALID_ARG;
    }
    if (!json_is_int(period_item) || period_item->valueint < 0) {
        *err_code = "bad_request";
        *err_msg = "encoder_step period_ms must be int";
        return ESP_ERR_INVALID_ARG;
    }
    const uint32_t period_ms = (uint32_t)period_item->valueint;
    if (period_ms < RELAY_MIN_PHASE_MS || period_ms > RELAY_MAX_PHASE_MS) {
        *err_code = "bad_request";
        *err_msg = "encoder_step period_ms must be 20..1000";
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t steps = (uint32_t)steps_item->valueint;
    const uint64_t total_ms =
        (uint64_t)steps * (uint64_t)RELAY_QUAD_STATES_PER_STEP * (uint64_t)period_ms;
    if (total_ms > (uint64_t)RELAY_MAX_SEQUENCE_MS) {
        *err_code = "bad_request";
        *err_msg = "encoder_step total time exceeds max_sequence_ms 60000";
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err =
        relay_sched_encoder_step(channel_a, channel_b, steps, cw, period_ms);
    if (err == ESP_ERR_INVALID_STATE) {
        *err_code = "busy";
        *err_msg = "encoder already active";
        return err;
    }
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "encoder_step failed to start";
        return err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "started", true);
    cJSON_AddNumberToObject(result, "channel_a", channel_a);
    cJSON_AddNumberToObject(result, "channel_b", channel_b);
    cJSON_AddNumberToObject(result, "steps", steps);
    cJSON_AddStringToObject(result, "direction", dir_item->valuestring);
    cJSON_AddNumberToObject(result, "period_ms", period_ms);
    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_handle_encoder_pulse_train(const cJSON *args, cJSON **out_result,
                                              const char **err_code, const char **err_msg)
{
    /* Reject direction if present (even before allowed-keys) for clear error. */
    if (cJSON_GetObjectItemCaseSensitive(args, "direction") != NULL) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train must not include direction";
        return ESP_ERR_INVALID_ARG;
    }

    static const char *const allowed[] = {"channel", "pulses", "period_ms", "pulse_ms"};
    if (!args_keys_allowed(args, allowed, 4U)) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    int channel = 0;
    if (!parse_channel(cJSON_GetObjectItemCaseSensitive(args, "channel"), &channel, err_msg,
                       "encoder_pulse_train requires channel int")) {
        *err_code = "bad_request";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *pulses_item = cJSON_GetObjectItemCaseSensitive(args, "pulses");
    const cJSON *period_item = cJSON_GetObjectItemCaseSensitive(args, "period_ms");
    const cJSON *pulse_item = cJSON_GetObjectItemCaseSensitive(args, "pulse_ms");

    if (!json_is_int(pulses_item) || pulses_item->valueint < 1) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train pulses must be int >= 1";
        return ESP_ERR_INVALID_ARG;
    }
    if (!json_is_int(period_item) || period_item->valueint < 0 || !json_is_int(pulse_item) ||
        pulse_item->valueint < 0) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train period_ms/pulse_ms must be int";
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t pulses = (uint32_t)pulses_item->valueint;
    const uint32_t period_ms = (uint32_t)period_item->valueint;
    const uint32_t pulse_ms = (uint32_t)pulse_item->valueint;

    if (period_ms < RELAY_MIN_TRAIN_PERIOD_MS || period_ms > RELAY_MAX_PHASE_MS) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train period_ms must be 40..1000";
        return ESP_ERR_INVALID_ARG;
    }
    if (pulse_ms < RELAY_MIN_DURATION_MS || pulse_ms > (period_ms - RELAY_MIN_DURATION_MS)) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train pulse_ms must be 20..(period_ms-20)";
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint64_t)pulses * (uint64_t)period_ms > (uint64_t)RELAY_MAX_SEQUENCE_MS) {
        *err_code = "bad_request";
        *err_msg = "encoder_pulse_train total time exceeds max_sequence_ms 60000";
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err = relay_sched_encoder_pulse_train(channel, pulses, period_ms, pulse_ms);
    if (err == ESP_ERR_INVALID_STATE) {
        *err_code = "busy";
        *err_msg = "encoder already active";
        return err;
    }
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "encoder_pulse_train failed to start";
        return err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "started", true);
    cJSON_AddNumberToObject(result, "channel", channel);
    cJSON_AddNumberToObject(result, "pulses", pulses);
    cJSON_AddNumberToObject(result, "period_ms", period_ms);
    cJSON_AddNumberToObject(result, "pulse_ms", pulse_ms);
    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_handle_encoder_cancel(const cJSON *args, cJSON **out_result,
                                         const char **err_code, const char **err_msg)
{
    if (!args_keys_allowed(args, NULL, 0U)) {
        *err_code = "bad_request";
        *err_msg = "encoder_cancel takes no args";
        return ESP_ERR_INVALID_ARG;
    }

    const bool was = relay_sched_encoder_active();
    const esp_err_t err = relay_sched_encoder_cancel();
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        *err_code = "internal";
        *err_msg = "encoder_cancel failed";
        return err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "cancelled", was);
    *out_result = result;
    return ESP_OK;
}
