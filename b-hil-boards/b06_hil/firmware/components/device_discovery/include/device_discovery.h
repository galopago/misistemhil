#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t device_discovery_init(void);
esp_err_t device_discovery_start(void);
esp_err_t device_discovery_stop(const char *reason);
bool device_discovery_is_active(void);

#ifdef __cplusplus
}
#endif
