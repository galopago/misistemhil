#include "host_api.h"

#include "host_api_internal.h"

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "host_api";

static httpd_handle_t s_httpd = NULL;
static SemaphoreHandle_t s_api_mutex = NULL;
static bool s_running = false;

esp_err_t host_api_lock(void)
{
    if (s_api_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_api_mutex, pdMS_TO_TICKS(HOST_API_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void host_api_unlock(void)
{
    if (s_api_mutex != NULL) {
        xSemaphoreGive(s_api_mutex);
    }
}

const char *host_api_json_id(const cJSON *root)
{
    if (root == NULL) {
        return NULL;
    }
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    if (!cJSON_IsString(id) || id->valuestring == NULL || id->valuestring[0] == '\0') {
        return NULL;
    }
    if (strlen(id->valuestring) > HOST_API_MAX_ID_LEN) {
        return NULL;
    }
    return id->valuestring;
}

esp_err_t host_api_recv_json(httpd_req_t *req, cJSON **out_root)
{
    if (req == NULL || out_root == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_root = NULL;

    if (req->content_len <= 0 || req->content_len > (int)HOST_API_MAX_BODY) {
        return ESP_ERR_INVALID_SIZE;
    }

    char *body = calloc(1, (size_t)req->content_len + 1U);
    if (body == NULL) {
        return ESP_ERR_NO_MEM;
    }

    int received = 0;
    while (received < req->content_len) {
        const int chunk =
            httpd_req_recv(req, body + received, (size_t)(req->content_len - received));
        if (chunk <= 0) {
            free(body);
            return ESP_FAIL;
        }
        received += chunk;
    }

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (root == NULL || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    *out_root = root;
    return ESP_OK;
}

esp_err_t host_api_send_cjson(httpd_req_t *req, cJSON *root)
{
    if (req == NULL || root == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    char *printed = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (printed == NULL) {
        return ESP_ERR_NO_MEM;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    const esp_err_t err = httpd_resp_send(req, printed, HTTPD_RESP_USE_STRLEN);
    free(printed);
    return err;
}

esp_err_t host_api_reply_ok(httpd_req_t *req, const char *id, cJSON *result)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        cJSON_Delete(result);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddBoolToObject(root, "ok", true);
    if (id != NULL) {
        cJSON_AddStringToObject(root, "id", id);
    } else {
        cJSON_AddNullToObject(root, "id");
    }
    cJSON_AddNumberToObject(root, "api_version", HOST_API_VERSION);
    if (result != NULL) {
        cJSON_AddItemToObject(root, "result", result);
    } else {
        cJSON_AddItemToObject(root, "result", cJSON_CreateObject());
    }

    return host_api_send_cjson(req, root);
}

esp_err_t host_api_reply_error(httpd_req_t *req, const char *id, const char *code,
                               const char *message)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddBoolToObject(root, "ok", false);
    if (id != NULL) {
        cJSON_AddStringToObject(root, "id", id);
    } else {
        cJSON_AddNullToObject(root, "id");
    }

    cJSON *error = cJSON_CreateObject();
    if (error == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(error, "code", code != NULL ? code : "internal");
    cJSON_AddStringToObject(error, "message", message != NULL ? message : "error");
    cJSON_AddItemToObject(root, "error", error);

    return host_api_send_cjson(req, root);
}

static esp_err_t register_handlers(httpd_handle_t server)
{
    const httpd_uri_t command_uri = {
        .uri = "/v1/command",
        .method = HTTP_POST,
        .handler = host_api_command_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t open_uri = {
        .uri = "/v1/stream/open",
        .method = HTTP_POST,
        .handler = host_api_stream_open_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t close_uri = {
        .uri = "/v1/stream/close",
        .method = HTTP_POST,
        .handler = host_api_stream_close_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t write_uri = {
        .uri = "/v1/stream/write",
        .method = HTTP_POST,
        .handler = host_api_stream_write_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t read_uri = {
        .uri = "/v1/stream/read",
        .method = HTTP_POST,
        .handler = host_api_stream_read_handler,
        .user_ctx = NULL,
    };

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &command_uri), TAG,
                        "register /v1/command failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &open_uri), TAG,
                        "register /v1/stream/open failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &close_uri), TAG,
                        "register /v1/stream/close failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &write_uri), TAG,
                        "register /v1/stream/write failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &read_uri), TAG,
                        "register /v1/stream/read failed");
    return ESP_OK;
}

esp_err_t host_api_start(void)
{
    if (s_running) {
        return ESP_OK;
    }

    if (s_api_mutex == NULL) {
        s_api_mutex = xSemaphoreCreateMutex();
        if (s_api_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_httpd, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed err=%s", esp_err_to_name(err));
        s_httpd = NULL;
        return err;
    }

    err = register_handlers(s_httpd);
    if (err != ESP_OK) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
        return err;
    }

    s_running = true;
    ESP_LOGI(TAG, "host API started port=80");
    return ESP_OK;
}

esp_err_t host_api_stop(const char *reason)
{
    if (!s_running) {
        return ESP_OK;
    }

    host_api_stream_reset();

    if (s_httpd != NULL) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }

    s_running = false;
    ESP_LOGI(TAG, "host API stopped reason=%s", reason != NULL ? reason : "unknown");
    return ESP_OK;
}

bool host_api_is_running(void)
{
    return s_running;
}
