#include "wifi_provisioning.h"

#include "wifi_provisioning_form.h"
#include "wifi_provisioning_pages.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "wifi_credentials.h"
#include "board_pins.h"

static const char *TAG = "wifi_prov";

#define WIFI_PROV_SETUP_URL "http://192.168.4.1"
#define WIFI_PROV_STA_TIMEOUT_MS 30000U
#define WIFI_PROV_AP_START_TIMEOUT_MS 10000U
#define WIFI_PROV_SUCCESS_TEARDOWN_MS 2000U
#define WIFI_PROV_HTTPD_STACK_SIZE 8192U
#define WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS 5U
#define WIFI_PROV_SAVED_BOOT_PER_ATTEMPT_TIMEOUT_MS 12000U
#define WIFI_PROV_SAVED_BOOT_SETTLE_MS 1000U

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_AP_STARTED_BIT BIT1
#define WIFI_STA_DISCONNECTED_BIT BIT2

typedef enum {
    WIFI_PROV_STA_SOURCE_NONE = 0,
    WIFI_PROV_STA_SOURCE_SUBMITTED,
    WIFI_PROV_STA_SOURCE_SAVED,
} wifi_prov_sta_source_t;

static wifi_prov_event_cb_t s_event_cb = NULL;
static void *s_event_ctx = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;
static httpd_handle_t s_httpd = NULL;
static esp_netif_t *s_ap_netif = NULL;
static esp_netif_t *s_sta_netif = NULL;
static bool s_wifi_stack_ready = false;
static bool s_portal_active = false;
static bool s_locked_disconnected = false;
static char s_form_body[WIFI_PROV_MAX_FORM_BODY_LEN + 1];
static char s_ap_ssid[33] = {0};
static char s_sta_ipv4[16] = {0};
static char s_event_sta_ipv4[16] = {0};
static char s_event_sta_mac[18] = {0};
static wifi_prov_sta_source_t s_pending_sta_source = WIFI_PROV_STA_SOURCE_NONE;
static uint8_t s_saved_attempt = 0;
static int s_last_disconnect_reason = 0;
static bool s_saved_policy_exhausted = false;

static const uint32_t s_saved_boot_backoff_ms[WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS] = {
    0U, 1000U, 3000U, 5000U, 8000U,
};

static void emit_event(wifi_prov_event_t event, const char *setup_url, const char *ssid,
                       const char *sta_ipv4, const char *sta_mac, esp_err_t status)
{
    if (s_event_cb == NULL) {
        return;
    }

    wifi_prov_event_info_t info = {
        .event = event,
        .setup_url = setup_url,
        .ssid = ssid,
        .sta_ipv4 = sta_ipv4,
        .sta_mac = sta_mac,
        .status = status,
    };
    s_event_cb(&info, s_event_ctx);
}

static bool prepare_sta_success_event_payload(void)
{
    if (s_sta_ipv4[0] == '\0') {
        return false;
    }

    uint8_t mac[6] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK) {
        return false;
    }

    const int written = snprintf(s_event_sta_mac, sizeof(s_event_sta_mac),
                                 "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
                                 mac[3], mac[4], mac[5]);
    if (written != 17) {
        return false;
    }

    strncpy(s_event_sta_ipv4, s_sta_ipv4, sizeof(s_event_sta_ipv4) - 1U);
    s_event_sta_ipv4[sizeof(s_event_sta_ipv4) - 1U] = '\0';
    return true;
}

static void emit_sta_success_event(wifi_prov_event_t event, const char *ssid)
{
    const char *ipv4 = NULL;
    const char *mac = NULL;
    if (prepare_sta_success_event_payload()) {
        ipv4 = s_event_sta_ipv4;
        mac = s_event_sta_mac;
    }
    emit_event(event, NULL, ssid, ipv4, mac, ESP_OK);
}

static const char *sta_source_label(wifi_prov_sta_source_t source)
{
    switch (source) {
    case WIFI_PROV_STA_SOURCE_SUBMITTED:
        return "submitted";
    case WIFI_PROV_STA_SOURCE_SAVED:
        return "saved";
    default:
        return "unknown";
    }
}

static void log_sta_got_ip(const ip_event_got_ip_t *event, wifi_prov_sta_source_t source)
{
    char ip_str[16] = {0};
    if (event != NULL) {
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
    }
    if (source == WIFI_PROV_STA_SOURCE_SAVED && s_saved_attempt > 0U) {
        ESP_LOGI(TAG, "STA got IP ip=%s source=saved attempt=%u/%u", ip_str, s_saved_attempt,
                 WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS);
        return;
    }
    ESP_LOGI(TAG, "STA got IP ip=%s source=%s", ip_str, sta_source_label(source));
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "SoftAP started");
            xEventGroupSetBits(s_wifi_event_group, WIFI_AP_STARTED_BIT);
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "SoftAP stopped");
            break;
        case WIFI_EVENT_AP_STACONNECTED: {
            const wifi_event_ap_staconnected_t *event =
                (const wifi_event_ap_staconnected_t *)event_data;
            if (event != NULL) {
                ESP_LOGI(TAG,
                         "station joined mac=%02x:%02x:%02x:%02x:%02x:%02x aid=%d",
                         event->mac[0], event->mac[1], event->mac[2], event->mac[3],
                         event->mac[4], event->mac[5], event->aid);
            }
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            const wifi_event_ap_stadisconnected_t *event =
                (const wifi_event_ap_stadisconnected_t *)event_data;
            if (event != NULL) {
                ESP_LOGI(TAG,
                         "station left mac=%02x:%02x:%02x:%02x:%02x:%02x aid=%d",
                         event->mac[0], event->mac[1], event->mac[2], event->mac[3],
                         event->mac[4], event->mac[5], event->aid);
            }
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED: {
            const wifi_event_sta_disconnected_t *event =
                (const wifi_event_sta_disconnected_t *)event_data;
            if (s_pending_sta_source != WIFI_PROV_STA_SOURCE_NONE && event != NULL) {
                if (s_pending_sta_source == WIFI_PROV_STA_SOURCE_SAVED && s_saved_attempt > 0U) {
                    ESP_LOGW(TAG, "STA disconnected reason=%d source=saved attempt=%u/%u",
                             event->reason, s_saved_attempt, WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS);
                    s_last_disconnect_reason = (int)event->reason;
                    xEventGroupSetBits(s_wifi_event_group, WIFI_STA_DISCONNECTED_BIT);
                } else {
                    ESP_LOGW(TAG, "STA disconnected reason=%d source=%s", event->reason,
                             sta_source_label(s_pending_sta_source));
                }
            }
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        }
        default:
            break;
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (s_pending_sta_source == WIFI_PROV_STA_SOURCE_NONE) {
            if (s_saved_policy_exhausted) {
                const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
                char ip_str[16] = {0};
                if (event != NULL) {
                    esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
                }
                ESP_LOGW(TAG, "stale STA got IP ignored ip=%s source=saved", ip_str);
            }
            return;
        }

        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        if (event != NULL) {
            esp_ip4addr_ntoa(&event->ip_info.ip, s_sta_ipv4, sizeof(s_sta_ipv4));
        }
        log_sta_got_ip(event, s_pending_sta_source);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t dhcps_stop_tolerant(esp_netif_t *netif)
{
    const esp_err_t err = esp_netif_dhcps_stop(netif);
    if (err == ESP_OK || err == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        return ESP_OK;
    }
    return err;
}

static esp_err_t configure_ap_netif_ip(void)
{
    if (s_ap_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_RETURN_ON_ERROR(dhcps_stop_tolerant(s_ap_netif), TAG, "dhcps stop failed");

    esp_netif_ip_info_t ip_info = {0};
    ip_info.ip.addr = ESP_IP4TOADDR(192, 168, 4, 1);
    ip_info.gw.addr = ESP_IP4TOADDR(192, 168, 4, 1);
    ip_info.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
    ESP_RETURN_ON_ERROR(esp_netif_set_ip_info(s_ap_netif, &ip_info), TAG, "set ap ip failed");
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_start(s_ap_netif), TAG, "dhcps start failed");

    ESP_LOGI(TAG, "AP netif ip=192.168.4.1 gw=192.168.4.1 netmask=255.255.255.0");
    return ESP_OK;
}

static esp_err_t ensure_sta_netif(void)
{
    if (s_sta_netif != NULL) {
        return ESP_OK;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();
    return s_sta_netif != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t build_provisioning_ap_ssid(void)
{
    uint8_t mac[6] = {0};
    ESP_RETURN_ON_ERROR(esp_wifi_get_mac(WIFI_IF_AP, mac), TAG, "get softap mac failed");

    const int written = snprintf(s_ap_ssid, sizeof(s_ap_ssid), "HIL-%s-%02X%02X",
                                 BOARD_HIL_NUMBER_STRING, mac[4], mac[5]);
    if (written <= 0 || written >= (int)sizeof(s_ap_ssid)) {
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "provisioning AP SSID generated ssid=%s", s_ap_ssid);
    return ESP_OK;
}

static esp_err_t ensure_wifi_stack(void)
{
    if (s_wifi_stack_ready) {
        return ESP_OK;
    }

    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop failed: %s", esp_err_to_name(err));
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init failed");

    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   &wifi_event_handler, NULL),
                        TAG, "register wifi handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   &wifi_event_handler, NULL),
                        TAG, "register ip handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP,
                                                   &wifi_event_handler, NULL),
                        TAG, "register sta lost ip handler failed");

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_wifi_stack_ready = true;
    return ESP_OK;
}

static esp_err_t apply_sta_config(const wifi_credentials_t *credentials)
{
    wifi_config_t config = {0};
    const size_t ssid_len = strlen(credentials->ssid);
    const size_t password_len = strlen(credentials->password);

    if (ssid_len == 0U || ssid_len > WIFI_PROV_MAX_SSID_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    if (password_len > WIFI_PROV_MAX_PASSWORD_LEN) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(config.sta.ssid, credentials->ssid, ssid_len);
    memcpy(config.sta.password, credentials->password, password_len);
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
    config.sta.pmf_cfg.capable = true;
    config.sta.pmf_cfg.required = false;
    config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    config.sta.failure_retry_cnt = 0;

    ESP_LOGI(TAG,
             "STA config auth_profile=wpa2_wpa3_personal pmf_capable=true pmf_required=false");

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "set sta config failed");
    return ESP_OK;
}

static esp_err_t wait_for_sta_connected(wifi_prov_sta_source_t source, uint32_t timeout_ms)
{
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    s_sta_ipv4[0] = '\0';
    s_pending_sta_source = source;

    (void)esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        s_pending_sta_source = WIFI_PROV_STA_SOURCE_NONE;
        return err;
    }

    const EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdFALSE,
                            pdMS_TO_TICKS(timeout_ms));

    s_pending_sta_source = WIFI_PROV_STA_SOURCE_NONE;
    return (bits & WIFI_CONNECTED_BIT) != 0U ? ESP_OK : ESP_ERR_TIMEOUT;
}

static esp_err_t wait_for_saved_sta_attempt(uint8_t attempt, uint32_t timeout_ms)
{
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_STA_DISCONNECTED_BIT);
    s_sta_ipv4[0] = '\0';
    s_last_disconnect_reason = 0;
    s_saved_attempt = attempt;
    s_pending_sta_source = WIFI_PROV_STA_SOURCE_SAVED;

    (void)esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        s_saved_attempt = 0;
        s_pending_sta_source = WIFI_PROV_STA_SOURCE_NONE;
        return err;
    }

    const EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_STA_DISCONNECTED_BIT,
                            pdTRUE, pdFALSE, pdMS_TO_TICKS(timeout_ms));

    s_saved_attempt = 0;
    s_pending_sta_source = WIFI_PROV_STA_SOURCE_NONE;

    if ((bits & WIFI_CONNECTED_BIT) != 0U) {
        return ESP_OK;
    }
    if ((bits & WIFI_STA_DISCONNECTED_BIT) != 0U) {
        return ESP_FAIL;
    }
    return ESP_ERR_TIMEOUT;
}

static int saved_attempt_failure_reason(void)
{
    if (s_last_disconnect_reason != 0) {
        return s_last_disconnect_reason;
    }
    return WIFI_REASON_CONNECTION_FAIL;
}

static esp_err_t wait_for_ap_started(uint32_t timeout_ms)
{
    const EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_AP_STARTED_BIT, pdTRUE, pdFALSE,
                            pdMS_TO_TICKS(timeout_ms));
    return (bits & WIFI_AP_STARTED_BIT) != 0U ? ESP_OK : ESP_ERR_TIMEOUT;
}

static esp_err_t start_http_server(void)
{
    if (s_httpd != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = WIFI_PROV_HTTPD_STACK_SIZE;
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_httpd, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_httpd = NULL;
        return err;
    }

    ESP_LOGI(TAG, "HTTP server started on port 80");
    return ESP_OK;
}

static esp_err_t register_http_handlers(void);

static void success_teardown_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(WIFI_PROV_SUCCESS_TEARDOWN_MS));

    if (s_httpd != NULL) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
        ESP_LOGI(TAG, "HTTP server stopped after provisioning success");
    }

    (void)esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_LOGI(TAG, "SoftAP stopped after provisioning success");
    ESP_LOGI(TAG, "STA-only mode active after provisioning success");
    s_portal_active = false;
    vTaskDelete(NULL);
}

static esp_err_t schedule_success_teardown(void)
{
    ESP_LOGI(TAG, "success teardown scheduled delay_ms=%u", WIFI_PROV_SUCCESS_TEARDOWN_MS);
    const BaseType_t ok =
        xTaskCreate(success_teardown_task, "wifi_prov_td", 3072, NULL, 5, NULL);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET / from client");
    const esp_err_t err = wifi_prov_send_setup_page(req, NULL);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "GET / response sent");
    } else {
        ESP_LOGE(TAG, "GET / response failed: %s", esp_err_to_name(err));
    }
    return err;
}

static esp_err_t health_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /health from client");
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    const esp_err_t err = httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "GET /health response sent");
    } else {
        ESP_LOGE(TAG, "GET /health response failed: %s", esp_err_to_name(err));
    }
    return err;
}

static bool content_type_is_form_urlencoded(httpd_req_t *req)
{
    size_t content_type_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1U;
    if (content_type_len <= 1U) {
        return false;
    }

    char content_type[64];
    if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type,
                                    sizeof(content_type)) != ESP_OK) {
        return false;
    }

    return strstr(content_type, "application/x-www-form-urlencoded") != NULL;
}

static esp_err_t send_provision_failure_page(httpd_req_t *req, const char *message,
                                             const char *prefill_ssid, int status_code)
{
    const esp_err_t err = wifi_prov_send_failure_page(req, message, prefill_ssid, status_code);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "POST /provision failure response sent");
    } else {
        ESP_LOGE(TAG, "POST /provision failure response failed: %s", esp_err_to_name(err));
    }
    return err;
}

static esp_err_t provision_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /provision from client");

    if (!content_type_is_form_urlencoded(req)) {
        return send_provision_failure_page(req, "Invalid form submission.", NULL, 400);
    }

    if (req->content_len > WIFI_PROV_MAX_FORM_BODY_LEN) {
        return send_provision_failure_page(req, "Invalid form submission.", NULL, 400);
    }

    const int received = httpd_req_recv(req, s_form_body, sizeof(s_form_body) - 1U);
    if (received <= 0) {
        return send_provision_failure_page(req, "Invalid form submission.", NULL, 400);
    }
    s_form_body[received] = '\0';

    wifi_prov_form_t form = {0};
    if (!wifi_prov_form_parse(s_form_body, (size_t)received, &form)) {
        return send_provision_failure_page(req, "Invalid form submission.", NULL, 400);
    }

    wifi_credentials_t credentials = {0};
    strncpy(credentials.ssid, form.ssid, sizeof(credentials.ssid) - 1U);
    strncpy(credentials.password, form.password, sizeof(credentials.password) - 1U);
    credentials.has_password = form.password[0] != '\0';

    ESP_LOGI(TAG, "POST /provision parsed form");

    emit_event(WIFI_PROV_EVENT_SUBMITTED_CONNECTING, NULL, credentials.ssid, NULL, NULL,
               ESP_OK);

    esp_err_t sta_netif_err = ensure_sta_netif();
    if (sta_netif_err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_SUBMITTED_FAILURE, NULL, credentials.ssid, NULL, NULL,
                   sta_netif_err);
        return send_provision_failure_page(req, "WiFi connection failed. Check SSID and password.",
                                           credentials.ssid, 200);
    }

    esp_err_t mode_err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (mode_err != ESP_OK) {
        ESP_LOGE(TAG, "set apsta failed: %s", esp_err_to_name(mode_err));
        emit_event(WIFI_PROV_EVENT_SUBMITTED_FAILURE, NULL, credentials.ssid, NULL, NULL,
                   mode_err);
        return send_provision_failure_page(req, "WiFi connection failed. Check SSID and password.",
                                           credentials.ssid, 200);
    }

    esp_err_t cfg_err = apply_sta_config(&credentials);
    if (cfg_err != ESP_OK) {
        ESP_LOGE(TAG, "apply sta config failed: %s", esp_err_to_name(cfg_err));
        emit_event(WIFI_PROV_EVENT_SUBMITTED_FAILURE, NULL, credentials.ssid, NULL, NULL,
                   cfg_err);
        return send_provision_failure_page(req, "WiFi connection failed. Check SSID and password.",
                                           credentials.ssid, 200);
    }

    ESP_LOGI(TAG, "POST /provision attempting STA ssid=%s", credentials.ssid);

    const esp_err_t connect_err =
        wait_for_sta_connected(WIFI_PROV_STA_SOURCE_SUBMITTED, WIFI_PROV_STA_TIMEOUT_MS);
    if (connect_err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_SUBMITTED_FAILURE, NULL, credentials.ssid, NULL, NULL,
                   connect_err);
        return send_provision_failure_page(req, "WiFi connection failed. Check SSID and password.",
                                           credentials.ssid, 200);
    }

    const esp_err_t save_err = wifi_credentials_save(&credentials);
    if (save_err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, credentials.ssid, NULL, NULL, save_err);
        return send_provision_failure_page(req, "WiFi connection failed. Check SSID and password.",
                                           credentials.ssid, 200);
    }
    ESP_LOGI(TAG, "credentials saved source=submitted");

    const esp_err_t page_err = wifi_prov_send_success_page(req);
    if (page_err != ESP_OK) {
        ESP_LOGE(TAG, "POST /provision success response failed: %s", esp_err_to_name(page_err));
        ESP_LOGW(TAG, "credentials rollback after success page failure");
        (void)wifi_credentials_erase();
        emit_event(WIFI_PROV_EVENT_SUBMITTED_FAILURE, NULL, credentials.ssid, NULL, NULL,
                   page_err);
        return send_provision_failure_page(req, "WiFi connection failed. Check SSID and password.",
                                           credentials.ssid, 200);
    }
    ESP_LOGI(TAG, "POST /provision success response sent");

    emit_sta_success_event(WIFI_PROV_EVENT_SUBMITTED_SUCCESS, credentials.ssid);

    const esp_err_t teardown_err = schedule_success_teardown();
    if (teardown_err != ESP_OK) {
        ESP_LOGW(TAG, "success teardown schedule failed: %s", esp_err_to_name(teardown_err));
    }

    return ESP_OK;
}

static esp_err_t not_found_handler(httpd_req_t *req, httpd_err_code_t err)
{
    (void)err;
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_send(req, "Not Found", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t register_http_handlers(void)
{
    const httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
    };
    const httpd_uri_t health_uri = {
        .uri = "/health",
        .method = HTTP_GET,
        .handler = health_get_handler,
    };
    const httpd_uri_t provision_uri = {
        .uri = "/provision",
        .method = HTTP_POST,
        .handler = provision_post_handler,
    };

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &root_uri), TAG,
                        "register / failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &health_uri), TAG,
                        "register /health failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &provision_uri), TAG,
                        "register /provision failed");
    httpd_register_err_handler(s_httpd, HTTPD_404_NOT_FOUND, not_found_handler);
    ESP_LOGI(TAG, "HTTP handlers registered (GET /, GET /health, POST /provision)");
    return ESP_OK;
}

esp_err_t wifi_provisioning_init(wifi_prov_event_cb_t cb, void *ctx)
{
    if (s_wifi_stack_ready) {
        return ESP_OK;
    }

    esp_err_t err = ensure_wifi_stack();
    if (err != ESP_OK) {
        return err;
    }

    s_event_cb = cb;
    s_event_ctx = ctx;
    return ESP_OK;
}

esp_err_t wifi_provisioning_start_ap_portal(void)
{
    s_locked_disconnected = false;

    esp_err_t err = ensure_wifi_stack();
    if (err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, NULL, NULL, NULL, err);
        return err;
    }

    err = configure_ap_netif_ip();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AP netif setup failed: %s", esp_err_to_name(err));
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, NULL, NULL, NULL, err);
        return err;
    }

    err = build_provisioning_ap_ssid();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AP SSID generation failed: %s", esp_err_to_name(err));
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, NULL, NULL, NULL, err);
        return err;
    }

    wifi_config_t ap_config = {0};
    strncpy((char *)ap_config.ap.ssid, s_ap_ssid, sizeof(ap_config.ap.ssid) - 1U);
    ap_config.ap.ssid_len = (uint8_t)strlen(s_ap_ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
    ap_config.ap.max_connection = 4;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "set ap mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_config), TAG, "set ap config failed");

    ESP_LOGI(TAG, "starting SoftAP ssid=%s mode=AP", s_ap_ssid);

    xEventGroupClearBits(s_wifi_event_group, WIFI_AP_STARTED_BIT);
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    err = wait_for_ap_started(WIFI_PROV_AP_START_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SoftAP start timeout");
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, NULL, NULL, NULL, err);
        return err;
    }

    emit_event(WIFI_PROV_EVENT_AP_STARTED, WIFI_PROV_SETUP_URL, s_ap_ssid, NULL, NULL,
               ESP_OK);

    err = start_http_server();
    if (err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_ERROR, WIFI_PROV_SETUP_URL, NULL, NULL, NULL, err);
        return err;
    }

    err = register_http_handlers();
    if (err != ESP_OK) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
        emit_event(WIFI_PROV_EVENT_ERROR, WIFI_PROV_SETUP_URL, NULL, NULL, NULL, err);
        return err;
    }

    s_portal_active = true;
    ESP_LOGI(TAG, "provisioning portal ready at %s", WIFI_PROV_SETUP_URL);
    emit_event(WIFI_PROV_EVENT_PORTAL_READY, WIFI_PROV_SETUP_URL, s_ap_ssid, NULL, NULL,
               ESP_OK);
    return ESP_OK;
}

esp_err_t wifi_provisioning_connect_saved(const wifi_credentials_t *credentials)
{
    if (credentials == NULL || !wifi_credentials_validate(credentials)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_portal_active = false;
    s_locked_disconnected = false;
    s_saved_policy_exhausted = false;

    esp_err_t err = ensure_wifi_stack();
    if (err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, credentials->ssid, NULL, NULL, err);
        return err;
    }

    err = ensure_sta_netif();
    if (err != ESP_OK) {
        emit_event(WIFI_PROV_EVENT_ERROR, NULL, credentials->ssid, NULL, NULL, err);
        return err;
    }

    emit_event(WIFI_PROV_EVENT_SAVED_CONNECTING, NULL, credentials->ssid, NULL, NULL,
               ESP_OK);

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set sta mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
    ESP_RETURN_ON_ERROR(apply_sta_config(credentials), TAG, "apply sta config failed");

    ESP_LOGI(TAG, "saved boot connect policy attempts=%u per_attempt_timeout_ms=%u",
             WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS, WIFI_PROV_SAVED_BOOT_PER_ATTEMPT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(WIFI_PROV_SAVED_BOOT_SETTLE_MS));

    int last_reason = WIFI_REASON_CONNECTION_FAIL;

    for (uint8_t attempt = 1U; attempt <= WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS; ++attempt) {
        ESP_LOGI(TAG, "saved boot attempt %u/%u start", attempt, WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS);

        err = wait_for_saved_sta_attempt(attempt, WIFI_PROV_SAVED_BOOT_PER_ATTEMPT_TIMEOUT_MS);
        if (err == ESP_OK) {
            char ip_str[16] = {0};
            if (s_sta_netif != NULL) {
                esp_netif_ip_info_t ip_info = {0};
                if (esp_netif_get_ip_info(s_sta_netif, &ip_info) == ESP_OK) {
                    esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
                }
            }
            ESP_LOGI(TAG, "saved boot connected attempt=%u/%u ip=%s", attempt,
                     WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS, ip_str);
            emit_sta_success_event(WIFI_PROV_EVENT_SAVED_SUCCESS, credentials->ssid);
            return ESP_OK;
        }

        last_reason = saved_attempt_failure_reason();
        const uint32_t backoff_ms = s_saved_boot_backoff_ms[attempt - 1U];
        ESP_LOGW(TAG, "saved boot attempt %u/%u failed reason=%d backoff_ms=%lu", attempt,
                 WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS, last_reason, (unsigned long)backoff_ms);

        if (attempt < WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS && backoff_ms > 0U) {
            vTaskDelay(pdMS_TO_TICKS(backoff_ms));
        }
    }

    s_saved_policy_exhausted = true;
    ESP_LOGE(TAG, "saved boot failed attempts=%u last_reason=%d",
             WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS, last_reason);
    s_locked_disconnected = true;
    emit_event(WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED, NULL, credentials->ssid, NULL, NULL,
               ESP_FAIL);
    return ESP_FAIL;
}

esp_err_t wifi_provisioning_stop(void)
{
    if (s_httpd != NULL) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }

    if (s_wifi_stack_ready) {
        esp_wifi_stop();
    }

    s_portal_active = false;
    return ESP_OK;
}

bool wifi_provisioning_portal_active(void)
{
    return s_portal_active;
}

bool wifi_provisioning_locked_disconnected(void)
{
    return s_locked_disconnected;
}
