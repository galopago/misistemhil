#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_ID_MAX_LEN 32

esp_err_t device_identity_init(void);
esp_err_t device_identity_get(char *out, size_t out_len);

#ifdef __cplusplus
}
#endif
