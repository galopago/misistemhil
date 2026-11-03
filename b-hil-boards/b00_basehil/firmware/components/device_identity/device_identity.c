#include "device_identity.h"

#include "board_pins.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"

static const char *TAG = "device_id";

static bool s_initialized = false;
static bool s_identity_logged = false;

esp_err_t device_identity_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    uint8_t mac[6] = {0};
    const esp_err_t err = esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (err != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    s_initialized = true;
    return ESP_OK;
}

esp_err_t device_identity_get(char *out, size_t out_len)
{
    if (out == NULL || out_len < 12U) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t init_err = device_identity_init();
    if (init_err != ESP_OK) {
        return init_err;
    }

    uint8_t mac[6] = {0};
    const esp_err_t mac_err = esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (mac_err != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    const int written =
        snprintf(out, out_len, "HIL-%s-%02X%02X", BOARD_HIL_NUMBER_STRING, mac[4], mac[5]);
    if (written <= 0 || (size_t)written >= out_len) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (!s_identity_logged) {
        ESP_LOGI(TAG, "identity %s", out);
        s_identity_logged = true;
    }
    return ESP_OK;
}
