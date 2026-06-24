#pragma once

#include "wifi_credentials.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_LINK_STATUS_UNPROVISIONED = 0,
    WIFI_LINK_STATUS_CONNECTING,
    WIFI_LINK_STATUS_CONNECTING_ALERT,
    WIFI_LINK_STATUS_CONNECTED,
    WIFI_LINK_STATUS_DISCONNECTED, /* deprecated v2; do not set on creds path */
} wifi_link_status_t;

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
    WIFI_PROV_EVENT_LINK_STATUS_CHANGED,
    WIFI_PROV_EVENT_RUNTIME_RECONNECTING,
    WIFI_PROV_EVENT_RUNTIME_RESTORED,
    WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE,
    WIFI_PROV_EVENT_CONNECT_ALERT_PHASE,
    WIFI_PROV_EVENT_CONNECT_RESTORED,
} wifi_prov_event_t;

typedef struct {
    wifi_prov_event_t event;
    const char *setup_url;
    const char *ssid;
    const char *sta_ipv4;
    const char *sta_mac;
    esp_err_t status;
    wifi_link_status_t link_status;
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
