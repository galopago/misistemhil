#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_PROV_MAX_SSID_LEN 32
#define WIFI_PROV_MAX_PASSWORD_LEN 64

typedef struct {
    char ssid[WIFI_PROV_MAX_SSID_LEN + 1];
    char password[WIFI_PROV_MAX_PASSWORD_LEN + 1];
    bool has_password;
} wifi_credentials_t;

esp_err_t wifi_credentials_init(void);
bool wifi_credentials_load(wifi_credentials_t *out);
esp_err_t wifi_credentials_save(const wifi_credentials_t *credentials);
esp_err_t wifi_credentials_erase(void);
bool wifi_credentials_validate(const wifi_credentials_t *credentials);

#ifdef __cplusplus
}
#endif
