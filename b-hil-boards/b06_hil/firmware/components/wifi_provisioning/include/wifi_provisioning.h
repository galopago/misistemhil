#pragma once

#include "wifi_credentials.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_PROV_EVENT_AP_STARTED = 0,
    WIFI_PROV_EVENT_PORTAL_READY,
    WIFI_PROV_EVENT_SUBMITTED_CONNECTING,
    WIFI_PROV_EVENT_SUBMITTED_SUCCESS,
    WIFI_PROV_EVENT_SUBMITTED_FAILURE,
    WIFI_PROV_EVENT_SAVED_CONNECTING,
    WIFI_PROV_EVENT_SAVED_SUCCESS,
    WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED,
    WIFI_PROV_EVENT_ERROR,
} wifi_prov_event_t;

typedef struct {
    wifi_prov_event_t event;
    const char *setup_url;
    const char *ssid;
    const char *sta_ipv4;
    const char *sta_mac;
    esp_err_t status;
} wifi_prov_event_info_t;

typedef void (*wifi_prov_event_cb_t)(const wifi_prov_event_info_t *info, void *ctx);

esp_err_t wifi_provisioning_init(wifi_prov_event_cb_t cb, void *ctx);
esp_err_t wifi_provisioning_start_ap_portal(void);
esp_err_t wifi_provisioning_connect_saved(const wifi_credentials_t *credentials);
esp_err_t wifi_provisioning_stop(void);
bool wifi_provisioning_portal_active(void);
bool wifi_provisioning_locked_disconnected(void);

#ifdef __cplusplus
}
#endif
