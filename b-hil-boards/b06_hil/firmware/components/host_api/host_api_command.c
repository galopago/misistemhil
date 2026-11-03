#include "host_api_internal.h"

#include "app_core_meter.h"
#include "device_identity.h"

#include <math.h>
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

static cJSON *channel_sample_to_json(const app_core_meter_sample_t *sample)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL || sample == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }

    cJSON_AddNumberToObject(obj, "channel", sample->channel);
    cJSON_AddBoolToObject(obj, "ok", sample->ok);
    if (sample->ok) {
        cJSON_AddNumberToObject(obj, "voltage_v", sample->voltage_v);
        cJSON_AddNumberToObject(obj, "current_a", sample->current_a);
        cJSON_AddNumberToObject(obj, "power_w", sample->power_w);
    }
    return obj;
}

static bool args_object_is_empty(const cJSON *args)
{
    return args != NULL && cJSON_IsObject(args) && args->child == NULL;
}

static bool parse_silk_channel(const cJSON *channel_item, int *out_channel)
{
    if (out_channel == NULL || channel_item == NULL || !cJSON_IsNumber(channel_item)) {
        return false;
    }

    const double raw = channel_item->valuedouble;
    if (!isfinite(raw) || raw != floor(raw)) {
        return false;
    }

    const int channel = (int)raw;
    if (channel < 1 || channel > 3) {
        return false;
    }

    *out_channel = channel;
    return true;
}

static esp_err_t handle_meter_read(const cJSON *args, cJSON **out_result, const char **err_code,
                                   const char **err_msg)
{
    if (!cJSON_IsObject(args)) {
        *err_code = "bad_request";
        *err_msg = "meter_read requires args object";
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *channel_item = cJSON_GetObjectItemCaseSensitive(args, "channel");
    if (channel_item == NULL) {
        *err_code = "bad_request";
        *err_msg = "meter_read requires args.channel";
        return ESP_ERR_INVALID_ARG;
    }

    /* Reject unknown args keys (only channel is allowed). */
    for (const cJSON *child = args->child; child != NULL; child = child->next) {
        if (child->string == NULL || strcmp(child->string, "channel") != 0) {
            *err_code = "bad_request";
            *err_msg = "meter_read unknown args key";
            return ESP_ERR_INVALID_ARG;
        }
    }

    int channel = 0;
    if (!parse_silk_channel(channel_item, &channel)) {
        *err_code = "bad_request";
        *err_msg = "meter_read channel must be integer 1, 2, or 3";
        return ESP_ERR_INVALID_ARG;
    }

    app_core_meter_sample_t sample = {0};
    const esp_err_t err = app_core_meter_read_channel(channel, &sample);
    if (err == ESP_ERR_INVALID_ARG) {
        *err_code = "bad_request";
        *err_msg = "meter_read channel must be integer 1, 2, or 3";
        return ESP_ERR_INVALID_ARG;
    }
    if (err != ESP_OK) {
        return err;
    }

    cJSON *result = channel_sample_to_json(&sample);
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }
    *out_result = result;
    return ESP_OK;
}

static esp_err_t handle_meter_read_all(const cJSON *args, cJSON **out_result,
                                       const char **err_code, const char **err_msg)
{
    if (!args_object_is_empty(args)) {
        *err_code = "bad_request";
        *err_msg = "meter_read_all requires empty args object";
        return ESP_ERR_INVALID_ARG;
    }

    app_core_meter_sample_t samples[3];
    const esp_err_t err = app_core_meter_read_all(samples);
    if (err != ESP_OK) {
        return err;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *channels = cJSON_CreateArray();
    if (result == NULL || channels == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(channels);
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < 3; ++i) {
        cJSON *obj = channel_sample_to_json(&samples[i]);
        if (obj == NULL || !cJSON_AddItemToArray(channels, obj)) {
            cJSON_Delete(obj);
            cJSON_Delete(channels);
            cJSON_Delete(result);
            return ESP_ERR_NO_MEM;
        }
    }

    if (!cJSON_AddItemToObject(result, "channels", channels)) {
        cJSON_Delete(channels);
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
    } else if (strcmp(name_copy, "meter_read") == 0) {
        cmd_err = handle_meter_read(args, &result, &err_code, &err_msg);
    } else if (strcmp(name_copy, "meter_read_all") == 0) {
        cmd_err = handle_meter_read_all(args, &result, &err_code, &err_msg);
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
