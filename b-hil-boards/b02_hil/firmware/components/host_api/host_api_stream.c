#include "host_api_internal.h"

#include "dut_uart.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/base64.h"

static const char *TAG = "host_api_stream";

static bool s_session_open = false;

void host_api_stream_reset(void)
{
    s_session_open = false;
    (void)dut_uart_stream_close();
}

static bool session_matches(const cJSON *session_item)
{
    if (!s_session_open) {
        return false;
    }
    if (!cJSON_IsString(session_item) || session_item->valuestring == NULL) {
        return false;
    }
    return strcmp(session_item->valuestring, HOST_API_SESSION_TOKEN) == 0;
}

static bool json_is_int(const cJSON *item)
{
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    return item->valuedouble == (double)item->valueint;
}

static bool params_keys_allowed(const cJSON *params)
{
    static const char *const allowed[] = {"baud", "data_bits", "parity", "stop_bits"};
    if (!cJSON_IsObject(params)) {
        return false;
    }
    for (const cJSON *child = params->child; child != NULL; child = child->next) {
        if (child->string == NULL) {
            return false;
        }
        bool found = false;
        for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i) {
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

static esp_err_t parse_open_params(const cJSON *params, dut_uart_config_t *cfg_out, bool *has_override)
{
    *has_override = false;
    if (!params_keys_allowed(params)) {
        return ESP_ERR_INVALID_ARG;
    }

    dut_uart_config_t cfg;
    esp_err_t err = dut_uart_config_get(&cfg);
    if (err != ESP_OK) {
        return err;
    }

    const cJSON *baud = cJSON_GetObjectItemCaseSensitive(params, "baud");
    const cJSON *data_bits = cJSON_GetObjectItemCaseSensitive(params, "data_bits");
    const cJSON *parity = cJSON_GetObjectItemCaseSensitive(params, "parity");
    const cJSON *stop_bits = cJSON_GetObjectItemCaseSensitive(params, "stop_bits");

    if (baud != NULL) {
        if (!json_is_int(baud) || !dut_uart_baud_allowed(baud->valueint)) {
            return ESP_ERR_INVALID_ARG;
        }
        cfg.baud = baud->valueint;
        *has_override = true;
    }
    if (data_bits != NULL) {
        if (!json_is_int(data_bits) || data_bits->valueint < 5 || data_bits->valueint > 8) {
            return ESP_ERR_INVALID_ARG;
        }
        cfg.data_bits = data_bits->valueint;
        *has_override = true;
    }
    if (parity != NULL) {
        if (!cJSON_IsString(parity) || parity->valuestring == NULL ||
            (strcmp(parity->valuestring, "none") != 0 && strcmp(parity->valuestring, "even") != 0 &&
             strcmp(parity->valuestring, "odd") != 0)) {
            return ESP_ERR_INVALID_ARG;
        }
        strncpy(cfg.parity, parity->valuestring, sizeof(cfg.parity) - 1U);
        cfg.parity[sizeof(cfg.parity) - 1U] = '\0';
        *has_override = true;
    }
    if (stop_bits != NULL) {
        if (!cJSON_IsNumber(stop_bits)) {
            return ESP_ERR_INVALID_ARG;
        }
        const double stop_v = stop_bits->valuedouble;
        const bool stop_ok = (stop_v > 0.95 && stop_v < 1.05) || (stop_v > 1.45 && stop_v < 1.55) ||
                             (stop_v > 1.95 && stop_v < 2.05);
        if (!stop_ok) {
            return ESP_ERR_INVALID_ARG;
        }
        cfg.stop_bits = (float)stop_v;
        *has_override = true;
    }

    if (*has_override && !dut_uart_framing_allowed(&cfg)) {
        return ESP_ERR_INVALID_ARG;
    }

    *cfg_out = cfg;
    return ESP_OK;
}

esp_err_t host_api_stream_open_handler(httpd_req_t *req)
{
    const esp_err_t lock_err = host_api_lock();
    if (lock_err == ESP_ERR_TIMEOUT) {
        return host_api_reply_error(req, NULL, "busy", "host API busy");
    }
    if (lock_err != ESP_OK) {
        return host_api_reply_error(req, NULL, "internal", "host API lock failed");
    }

    cJSON *root = NULL;
    if (host_api_recv_json(req, &root) != ESP_OK) {
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "invalid JSON body");
    }

    const char *id = host_api_json_id(root);
    if (id == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "missing or invalid id");
    }

    char id_copy[HOST_API_MAX_ID_LEN + 1] = {0};
    strncpy(id_copy, id, sizeof(id_copy) - 1U);

    const cJSON *stream_id = cJSON_GetObjectItemCaseSensitive(root, "stream_id");
    const cJSON *params = cJSON_GetObjectItemCaseSensitive(root, "params");
    if (!cJSON_IsString(stream_id) || stream_id->valuestring == NULL || !cJSON_IsObject(params)) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request",
                                    "missing stream_id or params object");
    }

    if (strcmp(stream_id->valuestring, "dut_uart") != 0) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "unknown_stream", "unknown stream_id");
    }

    if (s_session_open || dut_uart_is_open()) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "busy", "stream session already open");
    }

    dut_uart_config_t cfg;
    bool has_override = false;
    const esp_err_t parse_err = parse_open_params(params, &cfg, &has_override);
    if (parse_err != ESP_OK) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request", "invalid stream open params");
    }

    const esp_err_t open_err = dut_uart_stream_open(has_override ? &cfg : NULL);
    cJSON_Delete(root);
    if (open_err == ESP_ERR_INVALID_STATE) {
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "busy", "stream session already open");
    }
    if (open_err == ESP_ERR_INVALID_ARG) {
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request", "invalid stream open params");
    }
    if (open_err != ESP_OK) {
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "internal", "dut_uart open failed");
    }

    s_session_open = true;
    host_api_unlock();

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        s_session_open = false;
        (void)dut_uart_stream_close();
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }
    cJSON_AddStringToObject(result, "stream_id", "dut_uart");
    cJSON_AddStringToObject(result, "session", HOST_API_SESSION_TOKEN);
    cJSON_AddNumberToObject(result, "max_frame_bytes", HOST_API_FRAME_MAX_BYTES);
    ESP_LOGI(TAG, "stream open stream_id=dut_uart session=%s", HOST_API_SESSION_TOKEN);
    return host_api_reply_ok(req, id_copy, result);
}

esp_err_t host_api_stream_close_handler(httpd_req_t *req)
{
    const esp_err_t lock_err = host_api_lock();
    if (lock_err == ESP_ERR_TIMEOUT) {
        return host_api_reply_error(req, NULL, "busy", "host API busy");
    }
    if (lock_err != ESP_OK) {
        return host_api_reply_error(req, NULL, "internal", "host API lock failed");
    }

    cJSON *root = NULL;
    if (host_api_recv_json(req, &root) != ESP_OK) {
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "invalid JSON body");
    }

    const char *id = host_api_json_id(root);
    if (id == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "missing or invalid id");
    }

    char id_copy[HOST_API_MAX_ID_LEN + 1] = {0};
    strncpy(id_copy, id, sizeof(id_copy) - 1U);
    cJSON_Delete(root);

    s_session_open = false;
    (void)dut_uart_stream_close();
    host_api_unlock();

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }
    cJSON_AddBoolToObject(result, "closed", true);
    ESP_LOGI(TAG, "stream closed");
    return host_api_reply_ok(req, id_copy, result);
}

esp_err_t host_api_stream_write_handler(httpd_req_t *req)
{
    const esp_err_t lock_err = host_api_lock();
    if (lock_err == ESP_ERR_TIMEOUT) {
        return host_api_reply_error(req, NULL, "busy", "host API busy");
    }
    if (lock_err != ESP_OK) {
        return host_api_reply_error(req, NULL, "internal", "host API lock failed");
    }

    cJSON *root = NULL;
    if (host_api_recv_json(req, &root) != ESP_OK) {
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "invalid JSON body");
    }

    const char *id = host_api_json_id(root);
    if (id == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "missing or invalid id");
    }

    char id_copy[HOST_API_MAX_ID_LEN + 1] = {0};
    strncpy(id_copy, id, sizeof(id_copy) - 1U);

    const cJSON *session = cJSON_GetObjectItemCaseSensitive(root, "session");
    const cJSON *payload_b64 = cJSON_GetObjectItemCaseSensitive(root, "payload_b64");
    if (!session_matches(session)) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "unavailable", "no open stream session");
    }
    if (!cJSON_IsString(payload_b64) || payload_b64->valuestring == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request", "missing payload_b64");
    }

    const size_t b64_len = strlen(payload_b64->valuestring);
    uint8_t *decoded = calloc(1, HOST_API_FRAME_MAX_BYTES + 4U);
    if (decoded == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }

    size_t decoded_len = 0;
    const int b64_err =
        mbedtls_base64_decode(decoded, HOST_API_FRAME_MAX_BYTES, &decoded_len,
                              (const unsigned char *)payload_b64->valuestring, b64_len);
    cJSON_Delete(root);

    if (b64_err != 0 || decoded_len > HOST_API_FRAME_MAX_BYTES) {
        free(decoded);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request",
                                    "invalid Base64 or payload too large");
    }

    size_t bytes_written = 0;
    const esp_err_t wr_err = dut_uart_stream_write(decoded, decoded_len, &bytes_written);
    free(decoded);
    host_api_unlock();

    if (wr_err == ESP_ERR_INVALID_STATE) {
        return host_api_reply_error(req, id_copy, "unavailable", "no open stream session");
    }
    if (wr_err == ESP_ERR_TIMEOUT) {
        return host_api_reply_error(req, id_copy, "timeout", "uart TX wait timed out");
    }
    if (wr_err != ESP_OK) {
        return host_api_reply_error(req, id_copy, "internal", "dut_uart write failed");
    }

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }
    cJSON_AddNumberToObject(result, "bytes_written", (double)bytes_written);
    return host_api_reply_ok(req, id_copy, result);
}

esp_err_t host_api_stream_read_handler(httpd_req_t *req)
{
    const esp_err_t lock_err = host_api_lock();
    if (lock_err == ESP_ERR_TIMEOUT) {
        return host_api_reply_error(req, NULL, "busy", "host API busy");
    }
    if (lock_err != ESP_OK) {
        return host_api_reply_error(req, NULL, "internal", "host API lock failed");
    }

    cJSON *root = NULL;
    if (host_api_recv_json(req, &root) != ESP_OK) {
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "invalid JSON body");
    }

    const char *id = host_api_json_id(root);
    if (id == NULL) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, NULL, "bad_request", "missing or invalid id");
    }

    char id_copy[HOST_API_MAX_ID_LEN + 1] = {0};
    strncpy(id_copy, id, sizeof(id_copy) - 1U);

    const cJSON *session = cJSON_GetObjectItemCaseSensitive(root, "session");
    const cJSON *max_frames_item = cJSON_GetObjectItemCaseSensitive(root, "max_frames");
    const cJSON *timeout_item = cJSON_GetObjectItemCaseSensitive(root, "timeout_ms");

    if (!session_matches(session)) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "unavailable", "no open stream session");
    }
    if (!cJSON_IsNumber(max_frames_item) || !cJSON_IsNumber(timeout_item)) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request",
                                    "max_frames and timeout_ms required");
    }

    const int max_frames = max_frames_item->valueint;
    const int timeout_ms = timeout_item->valueint;
    cJSON_Delete(root);

    if (max_frames < 1 || max_frames > (int)HOST_API_MAX_READ_FRAMES || timeout_ms < 0 ||
        timeout_ms > (int)HOST_API_MAX_READ_TIMEOUT_MS) {
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "bad_request",
                                    "max_frames or timeout_ms out of range");
    }

    if (dut_uart_rx_count() == 0 && timeout_ms > 0) {
        host_api_unlock();
        (void)dut_uart_rx_wait((uint32_t)timeout_ms);
        if (host_api_lock() != ESP_OK) {
            return host_api_reply_error(req, id_copy, "busy", "host API busy");
        }
        if (!s_session_open) {
            host_api_unlock();
            return host_api_reply_error(req, id_copy, "unavailable", "no open stream session");
        }
    }

    cJSON *frames = cJSON_CreateArray();
    if (frames == NULL) {
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }

    int returned = 0;
    while (returned < max_frames) {
        dut_uart_frame_t frame;
        if (!dut_uart_rx_try_pop(&frame)) {
            break;
        }

        size_t b64_olen = 0;
        (void)mbedtls_base64_encode(NULL, 0, &b64_olen, frame.data, frame.len);
        if (b64_olen == 0U) {
            b64_olen = 1U;
        }
        char *b64 = calloc(1, b64_olen + 4U);
        if (b64 == NULL) {
            cJSON_Delete(frames);
            host_api_unlock();
            return host_api_reply_error(req, id_copy, "internal", "out of memory");
        }
        size_t written = 0;
        if (frame.len == 0U) {
            b64[0] = '\0';
        } else if (mbedtls_base64_encode((unsigned char *)b64, b64_olen + 1U, &written, frame.data,
                                         frame.len) != 0) {
            free(b64);
            cJSON_Delete(frames);
            host_api_unlock();
            return host_api_reply_error(req, id_copy, "internal", "base64 encode failed");
        }

        cJSON *item = cJSON_CreateObject();
        if (item == NULL) {
            free(b64);
            cJSON_Delete(frames);
            host_api_unlock();
            return host_api_reply_error(req, id_copy, "internal", "out of memory");
        }
        cJSON_AddStringToObject(item, "payload_b64", b64);
        cJSON_AddNumberToObject(item, "bytes", frame.len);
        cJSON_AddItemToArray(frames, item);
        free(b64);
        returned++;
    }

    host_api_unlock();

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        cJSON_Delete(frames);
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }
    cJSON_AddItemToObject(result, "frames", frames);
    return host_api_reply_ok(req, id_copy, result);
}
