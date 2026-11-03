#include "host_api_internal.h"

#include "device_identity.h"

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

static esp_err_t handle_echo(const cJSON *args, cJSON **out_result, const char **err_code,
                             const char **err_msg)
{
    const cJSON *text = cJSON_GetObjectItemCaseSensitive(args, "text");
    if (!cJSON_IsString(text) || text->valuestring == NULL) {
        *err_code = "bad_request";
        *err_msg = "echo requires args.text string";
        return ESP_ERR_INVALID_ARG;
    }

    const size_t len = strlen(text->valuestring);
    if (len > HOST_API_ECHO_MAX_LEN) {
        *err_code = "bad_request";
        *err_msg = "echo text exceeds 128 bytes";
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(result, "text", text->valuestring);
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
    } else if (strcmp(name_copy, "echo") == 0) {
        cmd_err = handle_echo(args, &result, &err_code, &err_msg);
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
        return host_api_reply_error(req, id_copy, "internal", "command handler failed");
    }

    return host_api_reply_ok(req, id_copy, result);
}
