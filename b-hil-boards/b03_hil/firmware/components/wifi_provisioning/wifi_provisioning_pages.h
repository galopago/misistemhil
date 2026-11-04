#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_prov_send_setup_page(httpd_req_t *req, const char *message);
esp_err_t wifi_prov_send_success_page(httpd_req_t *req);
esp_err_t wifi_prov_send_failure_page(httpd_req_t *req, const char *message, const char *prefill_ssid,
                                      int status_code);

#ifdef __cplusplus
}
#endif
