#include "wifi_provisioning_pages.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"

static const char SETUP_CHUNK_HEAD[] =
    "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>b06_hil WiFi Setup</title></head><body><main>"
    "<h1>b06_hil WiFi Setup</h1>"
    "<p>Enter the WiFi network for this device.</p>";

static const char SETUP_CHUNK_FORM_START[] =
    "<form method=\"post\" action=\"/provision\">"
    "<label for=\"ssid\">SSID</label>"
    "<input id=\"ssid\" name=\"ssid\" type=\"text\" maxlength=\"32\" required";

static const char SETUP_CHUNK_FORM_END[] =
    ">"
    "<label for=\"password\">Password</label>"
    "<input id=\"password\" name=\"password\" type=\"password\" maxlength=\"63\">"
    "<button type=\"submit\">Connect</button>"
    "</form></main></body></html>";

static const char FAILURE_CHUNK_HEAD[] =
    "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>b06_hil WiFi Setup</title></head><body><main>"
    "<h1>WiFi setup failed</h1><p>";

static const char FAILURE_CHUNK_FORM_MID[] =
    "</p>"
    "<form method=\"post\" action=\"/provision\">"
    "<label for=\"ssid\">SSID</label>"
    "<input id=\"ssid\" name=\"ssid\" type=\"text\" maxlength=\"32\" required";

static esp_err_t wifi_prov_send_html_begin(httpd_req_t *req, int status_code)
{
    const char *status = status_code == 400 ? "400 Bad Request" : "200 OK";
    ESP_RETURN_ON_FALSE(httpd_resp_set_status(req, status) == ESP_OK, ESP_FAIL, "wifi_prov",
                        "set status failed");
    ESP_RETURN_ON_FALSE(httpd_resp_set_type(req, "text/html; charset=utf-8") == ESP_OK, ESP_FAIL,
                        "wifi_prov", "set type failed");
    ESP_RETURN_ON_FALSE(httpd_resp_set_hdr(req, "Cache-Control", "no-store") == ESP_OK, ESP_FAIL,
                        "wifi_prov", "set cache header failed");
    return ESP_OK;
}

static esp_err_t wifi_prov_send_chunk(httpd_req_t *req, const char *chunk)
{
    return httpd_resp_send_chunk(req, chunk, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_prov_send_html_end(httpd_req_t *req)
{
    return httpd_resp_send_chunk(req, NULL, 0);
}

static size_t html_escape(const char *input, char *out, size_t out_size)
{
    if (input == NULL || out == NULL || out_size == 0U) {
        return 0U;
    }

    size_t written = 0U;
    for (size_t i = 0U; input[i] != '\0'; ++i) {
        const char *replacement = NULL;
        char single[2] = {input[i], '\0'};

        switch (input[i]) {
        case '&':
            replacement = "&amp;";
            break;
        case '<':
            replacement = "&lt;";
            break;
        case '>':
            replacement = "&gt;";
            break;
        case '"':
            replacement = "&quot;";
            break;
        case '\'':
            replacement = "&#39;";
            break;
        default:
            replacement = single;
            break;
        }

        const size_t rep_len = strlen(replacement);
        if (written + rep_len + 1U >= out_size) {
            break;
        }
        memcpy(out + written, replacement, rep_len);
        written += rep_len;
    }

    out[written] = '\0';
    return written;
}

static esp_err_t send_setup_form_chunks(httpd_req_t *req, const char *ssid_value_attr)
{
    ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, SETUP_CHUNK_FORM_START), "wifi_prov",
                        "setup form start chunk failed");
    if (ssid_value_attr != NULL && ssid_value_attr[0] != '\0') {
        ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, ssid_value_attr), "wifi_prov",
                            "setup ssid attr chunk failed");
    }
    return wifi_prov_send_chunk(req, SETUP_CHUNK_FORM_END);
}

esp_err_t wifi_prov_send_setup_page(httpd_req_t *req, const char *message)
{
    ESP_RETURN_ON_ERROR(wifi_prov_send_html_begin(req, 200), "wifi_prov", "setup begin failed");
    ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, SETUP_CHUNK_HEAD), "wifi_prov",
                        "setup head chunk failed");

    if (message != NULL && message[0] != '\0') {
        char escaped[128];
        char status_para[128];
        html_escape(message, escaped, sizeof(escaped));
        const int written = snprintf(status_para, sizeof(status_para), "<p>%s</p>", escaped);
        if (written > 0 && (size_t)written < sizeof(status_para)) {
            ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, status_para), "wifi_prov",
                                "setup status chunk failed");
        }
    }

    ESP_RETURN_ON_ERROR(send_setup_form_chunks(req, NULL), "wifi_prov", "setup form failed");
    return wifi_prov_send_html_end(req);
}

esp_err_t wifi_prov_send_success_page(httpd_req_t *req)
{
    const char *html =
        "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>b06_hil WiFi Setup</title></head><body><main>"
        "<h1>WiFi connected</h1>"
        "<p>WiFi connected. You can close this page.</p>"
        "</main></body></html>";

    ESP_RETURN_ON_ERROR(wifi_prov_send_html_begin(req, 200), "wifi_prov", "success begin failed");
    ESP_RETURN_ON_ERROR(httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN), "wifi_prov",
                        "success send failed");
    return ESP_OK;
}

esp_err_t wifi_prov_send_failure_page(httpd_req_t *req, const char *message, const char *prefill_ssid,
                                      int status_code)
{
    char escaped_message[128];
    char ssid_value_attr[128] = {0};

    if (message != NULL && message[0] != '\0') {
        html_escape(message, escaped_message, sizeof(escaped_message));
    } else {
        strncpy(escaped_message, "WiFi connection failed. Check SSID and password.",
                sizeof(escaped_message) - 1U);
    }

    if (prefill_ssid != NULL && prefill_ssid[0] != '\0') {
        char escaped_ssid[96];
        html_escape(prefill_ssid, escaped_ssid, sizeof(escaped_ssid));
        snprintf(ssid_value_attr, sizeof(ssid_value_attr), " value=\"%s\"", escaped_ssid);
    }

    ESP_RETURN_ON_ERROR(wifi_prov_send_html_begin(req, status_code), "wifi_prov",
                        "failure begin failed");
    ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, FAILURE_CHUNK_HEAD), "wifi_prov",
                        "failure head chunk failed");
    ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, escaped_message), "wifi_prov",
                        "failure message chunk failed");
    ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, FAILURE_CHUNK_FORM_MID), "wifi_prov",
                        "failure form mid chunk failed");
    if (ssid_value_attr[0] != '\0') {
        ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, ssid_value_attr), "wifi_prov",
                            "failure ssid attr chunk failed");
    }
    ESP_RETURN_ON_ERROR(wifi_prov_send_chunk(req, SETUP_CHUNK_FORM_END), "wifi_prov",
                        "failure form end chunk failed");
    return wifi_prov_send_html_end(req);
}
