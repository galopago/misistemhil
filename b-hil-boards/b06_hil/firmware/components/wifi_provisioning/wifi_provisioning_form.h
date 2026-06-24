#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "wifi_credentials.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ssid[WIFI_PROV_MAX_SSID_LEN + 1];
    char password[WIFI_PROV_MAX_PASSWORD_LEN + 1];
    bool has_ssid;
    bool valid;
} wifi_prov_form_t;

#define WIFI_PROV_MAX_FORM_BODY_LEN 256

bool wifi_prov_form_parse(const char *body, size_t body_len, wifi_prov_form_t *out);

#ifdef __cplusplus
}
#endif
