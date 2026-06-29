#include "device_discovery.h"

#include "device_identity.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"

static const char *TAG = "device_discovery";

static bool s_mdns_inited = false;
static bool s_active = false;
static char s_active_hostname[DEVICE_ID_MAX_LEN] = {0};

esp_err_t device_discovery_init(void)
{
    if (s_mdns_inited) {
        return ESP_OK;
    }

    const esp_err_t err = mdns_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "mdns init failed err=%s", esp_err_to_name(err));
        return err;
    }

    s_mdns_inited = true;
    return ESP_OK;
}

esp_err_t device_discovery_start(void)
{
    ESP_RETURN_ON_ERROR(device_discovery_init(), TAG, "discovery init failed");

    char hostname[DEVICE_ID_MAX_LEN] = {0};
    const esp_err_t id_err = device_identity_get(hostname, sizeof(hostname));
    if (id_err != ESP_OK) {
        ESP_LOGE(TAG, "identity get failed err=%s", esp_err_to_name(id_err));
        return id_err;
    }

    if (s_active && strcmp(s_active_hostname, hostname) == 0) {
        return ESP_OK;
    }

    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "sta netif unavailable");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = mdns_hostname_set(hostname);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns hostname set failed err=%s", esp_err_to_name(err));
        return err;
    }

    err = mdns_instance_name_set(hostname);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns instance name set failed err=%s", esp_err_to_name(err));
        return err;
    }

    err = mdns_register_netif(sta_netif);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "mdns register netif failed err=%s", esp_err_to_name(err));
        return err;
    }

    strncpy(s_active_hostname, hostname, sizeof(s_active_hostname) - 1U);
    s_active_hostname[sizeof(s_active_hostname) - 1U] = '\0';
    s_active = true;
    ESP_LOGI(TAG, "mdns start hostname=%s", hostname);
    return ESP_OK;
}

esp_err_t device_discovery_stop(const char *reason)
{
    if (!s_active) {
        return ESP_OK;
    }

    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif != NULL) {
        (void)mdns_unregister_netif(sta_netif);
    }

    s_active = false;
    s_active_hostname[0] = '\0';
    ESP_LOGI(TAG, "mdns stop reason=%s", reason != NULL ? reason : "unknown");
    return ESP_OK;
}

bool device_discovery_is_active(void)
{
    return s_active;
}
