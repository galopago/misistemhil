#include "host_api_internal.h"

#include "device_identity.h"
#include "relay_ctrl.h"
#include "relay_sched.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"

static const char *TAG = "host_api_cmd";

static void fill_sta_ipv4(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }
    out[0] = '\0';

    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta == NULL) {
        return;
    }

    esp_netif_ip_info_t ip_info = {0};
    if (esp_netif_get_ip_info(sta, &ip_info) != ESP_OK) {
        return;
    }

    snprintf(out, out_len, IPSTR, IP2STR(&ip_info.ip));
}

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

static cJSON *build_states_array(uint8_t mask)
{
    cJSON *states = cJSON_CreateArray();
    if (states == NULL) {
        return NULL;
    }
    for (int i = 0; i < RELAY_CTRL_CHANNEL_MAX; ++i) {
        cJSON *b = cJSON_CreateBool(((mask >> i) & 0x1U) != 0U);
        if (b == NULL || !cJSON_AddItemToArray(states, b)) {
            cJSON_Delete(b);
            cJSON_Delete(states);
            return NULL;
        }
    }
    return states;
}

static cJSON *build_mask_states_result(uint8_t mask)
{
    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return NULL;
    }
    cJSON *states = build_states_array(mask);
    if (states == NULL) {
        cJSON_Delete(result);
        return NULL;
    }
    cJSON_AddNumberToObject(result, "mask", mask);
    if (!cJSON_AddItemToObject(result, "states", states)) {
        cJSON_Delete(states);
        cJSON_Delete(result);
        return NULL;
    }
    return result;
}

static esp_err_t handle_ping(cJSON **out_result)
{
    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "pong", true);
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_info(cJSON **out_result)
{
    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }

    char identity[DEVICE_ID_MAX_LEN] = {0};
    (void)device_identity_get(identity, sizeof(identity));

    char sta_ipv4[16] = {0};
    fill_sta_ipv4(sta_ipv4, sizeof(sta_ipv4));

    cJSON_AddNumberToObject(result, "api_version", HOST_API_VERSION);
    cJSON_AddStringToObject(result, "board_id", HOST_API_BOARD_ID);
    cJSON_AddStringToObject(result, "board_number", HOST_API_BOARD_NUMBER);
    cJSON_AddStringToObject(result, "identity", identity);
    cJSON_AddStringToObject(result, "sta_ipv4", sta_ipv4);
    cJSON_AddBoolToObject(result, "streams_supported", false);

    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_capabilities(cJSON **out_result)
{
    cJSON *result = host_api_capabilities_build_result();
    if (result == NULL) {
        ESP_LOGE(TAG, "capabilities build failed");
        return ESP_ERR_NO_MEM;
    }
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_relay_set(const cJSON *args, cJSON **out_result, const char **err_code,
                                  const char **err_msg)
{
    static const char *const allowed[] = {"channel", "on"};
    if (!args_keys_allowed(args, allowed, 2U)) {
        *err_code = "bad_request";
        *err_msg = "relay_set unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *channel = cJSON_GetObjectItemCaseSensitive(args, "channel");
    const cJSON *on = cJSON_GetObjectItemCaseSensitive(args, "on");
    if (!json_is_int(channel) || !cJSON_IsBool(on)) {
        *err_code = "bad_request";
        *err_msg = "relay_set requires channel int and on bool";
        return ESP_ERR_INVALID_ARG;
    }

    const int ch = channel->valueint;
    if (ch < RELAY_CTRL_CHANNEL_MIN || ch > RELAY_CTRL_CHANNEL_MAX) {
        *err_code = "bad_request";
        *err_msg = "relay_set channel must be 1..6";
        return ESP_ERR_INVALID_ARG;
    }

    const bool on_val = cJSON_IsTrue(on);
    (void)relay_sched_preempt_channel(ch);
    const esp_err_t set_err = relay_ctrl_set(ch, on_val);
    if (set_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "relay_set driver fault";
        return set_err;
    }

    bool applied = false;
    const esp_err_t get_err = relay_ctrl_get(ch, &applied);
    if (get_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "relay_set readback fault";
        return get_err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddNumberToObject(result, "channel", ch);
    cJSON_AddBoolToObject(result, "on", applied);
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_relay_set_all(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    static const char *const allowed[] = {"mask", "states"};
    if (!args_keys_allowed(args, allowed, 2U)) {
        *err_code = "bad_request";
        *err_msg = "relay_set_all unknown args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *mask_item = cJSON_GetObjectItemCaseSensitive(args, "mask");
    const cJSON *states_item = cJSON_GetObjectItemCaseSensitive(args, "states");
    const bool has_mask = (mask_item != NULL);
    const bool has_states = (states_item != NULL);

    if (has_mask == has_states) {
        *err_code = "bad_request";
        *err_msg = "relay_set_all requires exactly one of mask or states";
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t mask = 0;
    if (has_mask) {
        if (!json_is_int(mask_item)) {
            *err_code = "bad_request";
            *err_msg = "relay_set_all mask must be int";
            return ESP_ERR_INVALID_ARG;
        }
        const int m = mask_item->valueint;
        if (m < 0 || m > (int)RELAY_CTRL_MASK_ALL) {
            *err_code = "bad_request";
            *err_msg = "relay_set_all mask must be 0..63";
            return ESP_ERR_INVALID_ARG;
        }
        mask = (uint8_t)m;
    } else {
        if (!cJSON_IsArray(states_item) || cJSON_GetArraySize(states_item) != RELAY_CTRL_CHANNEL_MAX) {
            *err_code = "bad_request";
            *err_msg = "relay_set_all states must be array of 6 bools";
            return ESP_ERR_INVALID_ARG;
        }
        for (int i = 0; i < RELAY_CTRL_CHANNEL_MAX; ++i) {
            const cJSON *el = cJSON_GetArrayItem(states_item, i);
            if (!cJSON_IsBool(el)) {
                *err_code = "bad_request";
                *err_msg = "relay_set_all states elements must be bool";
                return ESP_ERR_INVALID_ARG;
            }
            if (cJSON_IsTrue(el)) {
                mask = (uint8_t)(mask | (uint8_t)(1U << i));
            }
        }
    }

    (void)relay_sched_preempt_all();
    const esp_err_t set_err = relay_ctrl_set_mask(mask);
    if (set_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "relay_set_all driver fault";
        return set_err;
    }

    const uint8_t applied = relay_ctrl_get_mask();
    cJSON *result = build_mask_states_result(applied);
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_relay_get(const cJSON *args, cJSON **out_result, const char **err_code,
                                  const char **err_msg)
{
    static const char *const allowed[] = {"channel"};
    if (!args_keys_allowed(args, allowed, 1U)) {
        *err_code = "bad_request";
        *err_msg = "relay_get unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *channel = cJSON_GetObjectItemCaseSensitive(args, "channel");
    if (!json_is_int(channel)) {
        *err_code = "bad_request";
        *err_msg = "relay_get requires channel int";
        return ESP_ERR_INVALID_ARG;
    }

    const int ch = channel->valueint;
    if (ch < RELAY_CTRL_CHANNEL_MIN || ch > RELAY_CTRL_CHANNEL_MAX) {
        *err_code = "bad_request";
        *err_msg = "relay_get channel must be 1..6";
        return ESP_ERR_INVALID_ARG;
    }

    bool on = false;
    const esp_err_t get_err = relay_ctrl_get(ch, &on);
    if (get_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "relay_get driver fault";
        return get_err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddNumberToObject(result, "channel", ch);
    cJSON_AddBoolToObject(result, "on", on);
    cJSON_AddNumberToObject(result, "hold_remaining_ms", relay_sched_remaining_ms(ch));
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_relay_get_all(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    if (!args_keys_allowed(args, NULL, 0U)) {
        *err_code = "bad_request";
        *err_msg = "relay_get_all takes no args";
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t mask = relay_ctrl_get_mask();
    cJSON *result = build_mask_states_result(mask);
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON *remaining = cJSON_CreateArray();
    if (remaining == NULL) {
        cJSON_Delete(result);
        return ESP_ERR_NO_MEM;
    }
    for (int ch = RELAY_CTRL_CHANNEL_MIN; ch <= RELAY_CTRL_CHANNEL_MAX; ++ch) {
        cJSON *n = cJSON_CreateNumber(relay_sched_remaining_ms(ch));
        if (n == NULL || !cJSON_AddItemToArray(remaining, n)) {
            cJSON_Delete(n);
            cJSON_Delete(remaining);
            cJSON_Delete(result);
            return ESP_ERR_NO_MEM;
        }
    }
    if (!cJSON_AddItemToObject(result, "hold_remaining_ms", remaining)) {
        cJSON_Delete(remaining);
        cJSON_Delete(result);
        return ESP_ERR_NO_MEM;
    }

    *out_result = result;
    return ESP_OK;
}

esp_err_t host_api_command_handler(httpd_req_t *req)
{
    const esp_err_t lock_err = host_api_lock();
    if (lock_err == ESP_ERR_TIMEOUT) {
        return host_api_reply_error(req, NULL, "busy", "host API busy");
    }
    if (lock_err != ESP_OK) {
        return host_api_reply_error(req, NULL, "internal", "host API lock failed");
    }

    cJSON *root = NULL;
    esp_err_t err = host_api_recv_json(req, &root);
    if (err != ESP_OK) {
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "invalid JSON body");
    }

    const char *id = host_api_json_id(root);
    if (id == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "missing or invalid id");
    }

    const cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    if (!cJSON_IsString(name) || name->valuestring == NULL || name->valuestring[0] == '\0' ||
        strlen(name->valuestring) > HOST_API_MAX_NAME_LEN || !cJSON_IsObject(args)) {
        char id_copy[HOST_API_MAX_ID_LEN + 1] = {0};
        strncpy(id_copy, id, sizeof(id_copy) - 1U);
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request",
                                    "missing name or args object");
    }

    char id_copy[HOST_API_MAX_ID_LEN + 1] = {0};
    char name_copy[HOST_API_MAX_NAME_LEN + 1] = {0};
    strncpy(id_copy, id, sizeof(id_copy) - 1U);
    strncpy(name_copy, name->valuestring, sizeof(name_copy) - 1U);

    cJSON *result = NULL;
    const char *err_code = "internal";
    const char *err_msg = "command failed";
    esp_err_t cmd_err = ESP_OK;

    if (strcmp(name_copy, "ping") == 0) {
        cmd_err = handle_ping(&result);
    } else if (strcmp(name_copy, "info") == 0) {
        cmd_err = handle_info(&result);
    } else if (strcmp(name_copy, "capabilities") == 0) {
        cmd_err = handle_capabilities(&result);
    } else if (strcmp(name_copy, "relay_set") == 0) {
        cmd_err = handle_relay_set(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_set_all") == 0) {
        cmd_err = handle_relay_set_all(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_get") == 0) {
        cmd_err = handle_relay_get(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_get_all") == 0) {
        cmd_err = handle_relay_get_all(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_pulse") == 0) {
        cmd_err = host_api_handle_relay_pulse(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_hold") == 0) {
        cmd_err = host_api_handle_relay_hold(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_hold_many") == 0) {
        cmd_err = host_api_handle_relay_hold_many(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_hold_cancel") == 0) {
        cmd_err = host_api_handle_relay_hold_cancel(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "relay_release_all") == 0) {
        cmd_err = host_api_handle_relay_release_all(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "encoder_step") == 0) {
        cmd_err = host_api_handle_encoder_step(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "encoder_pulse_train") == 0) {
        cmd_err = host_api_handle_encoder_pulse_train(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "encoder_cancel") == 0) {
        cmd_err = host_api_handle_encoder_cancel(args, &result, &err_code, &err_msg);
    } else {
        cJSON_Delete(root);
        host_api_unlock();
        char message[96];
        snprintf(message, sizeof(message), "unknown command name=%s", name_copy);
        return host_api_reply_error(req, id_copy, "unknown_command", message);
    }

    cJSON_Delete(root);
    host_api_unlock();

    if (cmd_err != ESP_OK) {
        cJSON_Delete(result);
        if (cmd_err == ESP_ERR_INVALID_ARG || cmd_err == ESP_ERR_INVALID_STATE) {
            return host_api_reply_error(req, id_copy, err_code, err_msg);
        }
        return host_api_reply_error(req, id_copy, "internal", "command handler failed");
    }

    return host_api_reply_ok(req, id_copy, result);
}
