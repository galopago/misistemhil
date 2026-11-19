#include "host_api_internal.h"

#include "board_ctrl.h"
#include "device_identity.h"
#include "dut_uart.h"

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
    if (allowed_count == 0U) {
        return args->child == NULL;
    }
    if (allowed == NULL) {
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
    cJSON_AddBoolToObject(result, "streams_supported", true);

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
    if (ch < BOARD_CTRL_RELAY_CHANNEL_MIN || ch > BOARD_CTRL_RELAY_CHANNEL_MAX) {
        *err_code = "bad_request";
        *err_msg = "relay_set channel must be 1..4";
        return ESP_ERR_INVALID_ARG;
    }

    const bool on_val = cJSON_IsTrue(on);
    const esp_err_t set_err = board_ctrl_relay_set(ch, on_val);
    if (set_err == ESP_ERR_INVALID_STATE) {
        *err_code = "internal";
        *err_msg = "relay_set board_ctrl not ready";
        return set_err;
    }
    if (set_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "relay_set driver fault";
        return set_err;
    }

    bool applied = false;
    const esp_err_t get_err = board_ctrl_relay_get(ch, &applied);
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
    if (ch < BOARD_CTRL_RELAY_CHANNEL_MIN || ch > BOARD_CTRL_RELAY_CHANNEL_MAX) {
        *err_code = "bad_request";
        *err_msg = "relay_get channel must be 1..4";
        return ESP_ERR_INVALID_ARG;
    }

    bool on = false;
    const esp_err_t get_err = board_ctrl_relay_get(ch, &on);
    if (get_err == ESP_ERR_INVALID_STATE) {
        *err_code = "internal";
        *err_msg = "relay_get board_ctrl not ready";
        return get_err;
    }
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
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_uart_path_set(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    static const char *const allowed[] = {"path"};
    if (!args_keys_allowed(args, allowed, 1U)) {
        *err_code = "bad_request";
        *err_msg = "uart_path_set unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *path = cJSON_GetObjectItemCaseSensitive(args, "path");
    if (!cJSON_IsString(path) || path->valuestring == NULL) {
        *err_code = "bad_request";
        *err_msg = "uart_path_set requires path string";
        return ESP_ERR_INVALID_ARG;
    }

    board_uart_path_t p;
    if (strcmp(path->valuestring, "ttl") == 0) {
        p = BOARD_UART_PATH_TTL;
    } else if (strcmp(path->valuestring, "rs485") == 0) {
        p = BOARD_UART_PATH_RS485;
    } else {
        *err_code = "bad_request";
        *err_msg = "uart_path_set path must be ttl or rs485";
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t set_err = board_ctrl_uart_path_set(p);
    if (set_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_path_set driver fault";
        return set_err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(result, "path", path->valuestring);
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_uart_path_get(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    if (!args_keys_allowed(args, NULL, 0U)) {
        *err_code = "bad_request";
        *err_msg = "uart_path_get unknown args keys";
        return ESP_ERR_INVALID_ARG;
    }

    board_uart_path_t p = BOARD_UART_PATH_TTL;
    const esp_err_t get_err = board_ctrl_uart_path_get(&p);
    if (get_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_path_get driver fault";
        return get_err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(result, "path", p == BOARD_UART_PATH_RS485 ? "rs485" : "ttl");
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_uart_line_set(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    static const char *const allowed[] = {"term", "pullup", "pulldown"};
    if (!args_keys_allowed(args, allowed, 3U)) {
        *err_code = "bad_request";
        *err_msg = "uart_line_set unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *term = cJSON_GetObjectItemCaseSensitive(args, "term");
    const cJSON *pullup = cJSON_GetObjectItemCaseSensitive(args, "pullup");
    const cJSON *pulldown = cJSON_GetObjectItemCaseSensitive(args, "pulldown");
    const bool has_term = (term != NULL);
    const bool has_pullup = (pullup != NULL);
    const bool has_pulldown = (pulldown != NULL);

    if (!has_term && !has_pullup && !has_pulldown) {
        *err_code = "bad_request";
        *err_msg = "uart_line_set requires at least one of term/pullup/pulldown";
        return ESP_ERR_INVALID_ARG;
    }
    if ((has_term && !cJSON_IsBool(term)) || (has_pullup && !cJSON_IsBool(pullup)) ||
        (has_pulldown && !cJSON_IsBool(pulldown))) {
        *err_code = "bad_request";
        *err_msg = "uart_line_set flags must be bool";
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t set_err = board_ctrl_uart_line_set(
        has_term ? cJSON_IsTrue(term) : false, has_pullup ? cJSON_IsTrue(pullup) : false,
        has_pulldown ? cJSON_IsTrue(pulldown) : false, has_term, has_pullup, has_pulldown);
    if (set_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_line_set driver fault";
        return set_err;
    }

    bool t = false;
    bool pu = false;
    bool pd = false;
    const esp_err_t get_err = board_ctrl_uart_line_get(&t, &pu, &pd);
    if (get_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_line_set readback fault";
        return get_err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "term", t);
    cJSON_AddBoolToObject(result, "pullup", pu);
    cJSON_AddBoolToObject(result, "pulldown", pd);
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_uart_line_get(const cJSON *args, cJSON **out_result, const char **err_code,
                                      const char **err_msg)
{
    if (!args_keys_allowed(args, NULL, 0U)) {
        *err_code = "bad_request";
        *err_msg = "uart_line_get unknown args keys";
        return ESP_ERR_INVALID_ARG;
    }

    bool t = false;
    bool pu = false;
    bool pd = false;
    const esp_err_t get_err = board_ctrl_uart_line_get(&t, &pu, &pd);
    if (get_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_line_get driver fault";
        return get_err;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddBoolToObject(result, "term", t);
    cJSON_AddBoolToObject(result, "pullup", pu);
    cJSON_AddBoolToObject(result, "pulldown", pd);
    *out_result = result;
    return ESP_OK;
}

static cJSON *config_to_result(const dut_uart_config_t *cfg)
{
    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return NULL;
    }
    cJSON_AddNumberToObject(result, "baud", cfg->baud);
    cJSON_AddNumberToObject(result, "data_bits", cfg->data_bits);
    cJSON_AddStringToObject(result, "parity", cfg->parity);
    cJSON_AddNumberToObject(result, "stop_bits", cfg->stop_bits);
    return result;
}

static esp_err_t handle_uart_config_set(const cJSON *args, cJSON **out_result, const char **err_code,
                                        const char **err_msg)
{
    static const char *const allowed[] = {"baud", "data_bits", "parity", "stop_bits"};
    if (!args_keys_allowed(args, allowed, 4U)) {
        *err_code = "bad_request";
        *err_msg = "uart_config_set unknown or invalid args keys";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *baud = cJSON_GetObjectItemCaseSensitive(args, "baud");
    const cJSON *data_bits = cJSON_GetObjectItemCaseSensitive(args, "data_bits");
    const cJSON *parity = cJSON_GetObjectItemCaseSensitive(args, "parity");
    const cJSON *stop_bits = cJSON_GetObjectItemCaseSensitive(args, "stop_bits");
    if (baud == NULL && data_bits == NULL && parity == NULL && stop_bits == NULL) {
        *err_code = "bad_request";
        *err_msg = "uart_config_set requires at least one field";
        return ESP_ERR_INVALID_ARG;
    }

    if (dut_uart_is_open()) {
        *err_code = "busy";
        *err_msg = "uart_config_set busy while dut_uart stream open";
        return ESP_ERR_INVALID_STATE;
    }

    dut_uart_config_t cfg;
    esp_err_t err = dut_uart_config_get(&cfg);
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_config_set get current failed";
        return err;
    }

    if (baud != NULL) {
        if (!json_is_int(baud) || !dut_uart_baud_allowed(baud->valueint)) {
            *err_code = "bad_request";
            *err_msg = "uart_config_set baud not allowed";
            return ESP_ERR_INVALID_ARG;
        }
        cfg.baud = baud->valueint;
    }
    if (data_bits != NULL) {
        if (!json_is_int(data_bits) || data_bits->valueint < 5 || data_bits->valueint > 8) {
            *err_code = "bad_request";
            *err_msg = "uart_config_set data_bits must be 5..8";
            return ESP_ERR_INVALID_ARG;
        }
        cfg.data_bits = data_bits->valueint;
    }
    if (parity != NULL) {
        if (!cJSON_IsString(parity) || parity->valuestring == NULL ||
            (strcmp(parity->valuestring, "none") != 0 && strcmp(parity->valuestring, "even") != 0 &&
             strcmp(parity->valuestring, "odd") != 0)) {
            *err_code = "bad_request";
            *err_msg = "uart_config_set parity must be none|even|odd";
            return ESP_ERR_INVALID_ARG;
        }
        strncpy(cfg.parity, parity->valuestring, sizeof(cfg.parity) - 1U);
        cfg.parity[sizeof(cfg.parity) - 1U] = '\0';
    }
    if (stop_bits != NULL) {
        if (!cJSON_IsNumber(stop_bits)) {
            *err_code = "bad_request";
            *err_msg = "uart_config_set stop_bits must be number 1|1.5|2";
            return ESP_ERR_INVALID_ARG;
        }
        const double stop_v = stop_bits->valuedouble;
        const bool stop_ok = (stop_v > 0.95 && stop_v < 1.05) || (stop_v > 1.45 && stop_v < 1.55) ||
                             (stop_v > 1.95 && stop_v < 2.05);
        if (!stop_ok) {
            *err_code = "bad_request";
            *err_msg = "uart_config_set stop_bits must be 1|1.5|2";
            return ESP_ERR_INVALID_ARG;
        }
        cfg.stop_bits = (float)stop_v;
    }

    if (!dut_uart_framing_allowed(&cfg)) {
        *err_code = "bad_request";
        *err_msg = "uart_config_set invalid framing combination";
        return ESP_ERR_INVALID_ARG;
    }

    err = dut_uart_config_set(&cfg);
    if (err == ESP_ERR_INVALID_STATE) {
        *err_code = "busy";
        *err_msg = "uart_config_set busy while dut_uart stream open";
        return err;
    }
    if (err == ESP_ERR_INVALID_ARG) {
        *err_code = "bad_request";
        *err_msg = "uart_config_set invalid framing";
        return err;
    }
    if (err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_config_set driver fault";
        return err;
    }

    cJSON *result = config_to_result(&cfg);
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_uart_config_get(const cJSON *args, cJSON **out_result, const char **err_code,
                                        const char **err_msg)
{
    if (!args_keys_allowed(args, NULL, 0U)) {
        *err_code = "bad_request";
        *err_msg = "uart_config_get unknown args keys";
        return ESP_ERR_INVALID_ARG;
    }

    dut_uart_config_t cfg;
    const esp_err_t get_err = dut_uart_config_get(&cfg);
    if (get_err != ESP_OK) {
        *err_code = "internal";
        *err_msg = "uart_config_get driver fault";
        return get_err;
    }

    cJSON *result = config_to_result(&cfg);
    if (result == NULL) {
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
    } else if (strcmp(name_copy, "relay_get") == 0) {
        cmd_err = handle_relay_get(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "uart_path_set") == 0) {
        cmd_err = handle_uart_path_set(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "uart_path_get") == 0) {
        cmd_err = handle_uart_path_get(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "uart_line_set") == 0) {
        cmd_err = handle_uart_line_set(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "uart_line_get") == 0) {
        cmd_err = handle_uart_line_get(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "uart_config_set") == 0) {
        cmd_err = handle_uart_config_set(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "uart_config_get") == 0) {
        cmd_err = handle_uart_config_get(args, &result, &err_code, &err_msg);
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
        if (cmd_err == ESP_ERR_INVALID_ARG) {
            return host_api_reply_error(req, id_copy, err_code, err_msg);
        }
        return host_api_reply_error(req, id_copy, err_code, err_msg);
    }

    return host_api_reply_ok(req, id_copy, result);
}
