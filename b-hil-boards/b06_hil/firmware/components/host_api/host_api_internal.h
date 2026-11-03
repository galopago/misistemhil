#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#include "cJSON.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOST_API_VERSION 1
#define HOST_API_BOARD_ID "b06_hil"
#define HOST_API_BOARD_NUMBER "06"
#define HOST_API_LOCK_TIMEOUT_MS 100U
#define HOST_API_MAX_BODY 4096U
#define HOST_API_MAX_ID_LEN 64U
#define HOST_API_MAX_NAME_LEN 64U
#define HOST_API_ECHO_MAX_LEN 128U
#define HOST_API_FRAME_MAX_BYTES 1024U
#define HOST_API_RX_DEPTH 16U
#define HOST_API_SESSION_TOKEN "1"
#define HOST_API_MAX_READ_FRAMES 16U
#define HOST_API_MAX_READ_TIMEOUT_MS 5000U

esp_err_t host_api_lock(void);
void host_api_unlock(void);

esp_err_t host_api_recv_json(httpd_req_t *req, cJSON **out_root);
esp_err_t host_api_send_cjson(httpd_req_t *req, cJSON *root);
esp_err_t host_api_reply_ok(httpd_req_t *req, const char *id, cJSON *result);
esp_err_t host_api_reply_error(httpd_req_t *req, const char *id, const char *code,
                               const char *message);

const char *host_api_json_id(const cJSON *root);

esp_err_t host_api_command_handler(httpd_req_t *req);
esp_err_t host_api_stream_open_handler(httpd_req_t *req);
esp_err_t host_api_stream_close_handler(httpd_req_t *req);
esp_err_t host_api_stream_write_handler(httpd_req_t *req);
esp_err_t host_api_stream_read_handler(httpd_req_t *req);

void host_api_stream_reset(void);
cJSON *host_api_capabilities_build_result(void);

#ifdef __cplusplus
}
#endif
