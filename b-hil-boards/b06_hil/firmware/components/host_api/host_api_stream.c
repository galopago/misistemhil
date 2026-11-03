#include "host_api_internal.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/base64.h"

static const char *TAG = "host_api_stream";

typedef struct {
    uint16_t len;
    uint8_t data[HOST_API_FRAME_MAX_BYTES];
} host_api_rx_frame_t;

static bool s_session_open = false;
static host_api_rx_frame_t s_rx[HOST_API_RX_DEPTH];
static uint8_t s_rx_head = 0;
static uint8_t s_rx_tail = 0;
static uint8_t s_rx_count = 0;
static SemaphoreHandle_t s_rx_notify = NULL;

static void ensure_rx_notify(void)
{
    if (s_rx_notify == NULL) {
        s_rx_notify = xSemaphoreCreateBinary();
    }
}

static void rx_clear(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    s_rx_count = 0;
    if (s_rx_notify != NULL) {
        (void)xSemaphoreTake(s_rx_notify, 0);
    }
}

void host_api_stream_reset(void)
{
    s_session_open = false;
    rx_clear();
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

static void rx_drop_oldest(void)
{
    if (s_rx_count == 0) {
        return;
    }
    s_rx_head = (uint8_t)((s_rx_head + 1U) % HOST_API_RX_DEPTH);
    s_rx_count--;
}

static void rx_enqueue(const uint8_t *data, size_t len)
{
    ensure_rx_notify();
    if (s_rx_count >= HOST_API_RX_DEPTH) {
        rx_drop_oldest();
    }

    host_api_rx_frame_t *slot = &s_rx[s_rx_tail];
    slot->len = (uint16_t)len;
    if (len > 0U && data != NULL) {
        memcpy(slot->data, data, len);
    }
    s_rx_tail = (uint8_t)((s_rx_tail + 1U) % HOST_API_RX_DEPTH);
    s_rx_count++;
    (void)xSemaphoreGive(s_rx_notify);
}

static bool rx_dequeue(host_api_rx_frame_t *out)
{
    if (s_rx_count == 0 || out == NULL) {
        return false;
    }
    *out = s_rx[s_rx_head];
    s_rx_head = (uint8_t)((s_rx_head + 1U) % HOST_API_RX_DEPTH);
    s_rx_count--;
    return true;
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

    if (strcmp(stream_id->valuestring, "loopback") != 0) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "unknown_stream", "unknown stream_id");
    }

    if (s_session_open) {
        cJSON_Delete(root);
        host_api_unlock();
        return host_api_reply_error(req, id_copy, "busy", "stream session already open");
    }

    ensure_rx_notify();
    rx_clear();
    s_session_open = true;
    cJSON_Delete(root);
    host_api_unlock();

    cJSON *result = cJSON_CreateObject();
    if (result == NULL) {
        s_session_open = false;
        return host_api_reply_error(req, id_copy, "internal", "out of memory");
    }
    cJSON_AddStringToObject(result, "stream_id", "loopback");
    cJSON_AddStringToObject(result, "session", HOST_API_SESSION_TOKEN);
    cJSON_AddNumberToObject(result, "max_frame_bytes", HOST_API_FRAME_MAX_BYTES);
    ESP_LOGI(TAG, "stream open stream_id=loopback session=%s", HOST_API_SESSION_TOKEN);
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
    rx_clear();
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

    rx_enqueue(decoded, decoded_len);
    const size_t bytes_written = decoded_len;
    free(decoded);
    host_api_unlock();

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

    ensure_rx_notify();

    /* Unlock while waiting so a concurrent write can enqueue (loopback / future HW). */
    if (s_rx_count == 0 && timeout_ms > 0) {
        host_api_unlock();
        (void)xSemaphoreTake(s_rx_notify, pdMS_TO_TICKS((uint32_t)timeout_ms));
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
    while (returned < max_frames && s_rx_count > 0) {
        host_api_rx_frame_t frame;
        if (!rx_dequeue(&frame)) {
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

    if (s_rx_count == 0 && s_rx_notify != NULL) {
        (void)xSemaphoreTake(s_rx_notify, 0);
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
