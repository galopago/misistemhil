#include "wifi_credentials.h"

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

static const char *NVS_NAMESPACE = "wifi_prov";
static const char *KEY_SSID = "ssid";
static const char *KEY_PASSWORD = "password";
static const char *KEY_PROVISIONED = "provisioned";

bool wifi_credentials_validate(const wifi_credentials_t *credentials)
{
    if (credentials == NULL) {
        return false;
    }

    const size_t ssid_len = strlen(credentials->ssid);
    if (ssid_len < 1U || ssid_len > WIFI_PROV_MAX_SSID_LEN) {
        return false;
    }

    const size_t password_len = strlen(credentials->password);
    if (password_len > WIFI_PROV_MAX_PASSWORD_LEN) {
        return false;
    }

    return true;
}

esp_err_t wifi_credentials_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

bool wifi_credentials_load(wifi_credentials_t *out)
{
    if (out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return false;
    }

    uint8_t provisioned = 0U;
    err = nvs_get_u8(handle, KEY_PROVISIONED, &provisioned);
    if (err != ESP_OK || provisioned != 1U) {
        nvs_close(handle);
        return false;
    }

    size_t ssid_len = sizeof(out->ssid);
    err = nvs_get_str(handle, KEY_SSID, out->ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    size_t password_len = sizeof(out->password);
    err = nvs_get_str(handle, KEY_PASSWORD, out->password, &password_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        out->password[0] = '\0';
        out->has_password = false;
    } else if (err != ESP_OK) {
        nvs_close(handle);
        return false;
    } else {
        out->has_password = out->password[0] != '\0';
    }

    nvs_close(handle);

    if (!wifi_credentials_validate(out)) {
        memset(out, 0, sizeof(*out));
        return false;
    }

    return true;
}

esp_err_t wifi_credentials_save(const wifi_credentials_t *credentials)
{
    if (!wifi_credentials_validate(credentials)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, KEY_SSID, credentials->ssid);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_set_str(handle, KEY_PASSWORD, credentials->password);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_set_u8(handle, KEY_PROVISIONED, 1U);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t wifi_credentials_erase(void)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
}
