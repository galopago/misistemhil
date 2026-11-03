#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Register INA219 devices and start the idle metering task (display paused). */
esp_err_t app_core_metering_init(void);

/** Begin 500 ms STA-connected metering OLED updates. */
void app_core_metering_start(void);

/** Pause metering OLED updates (provisioning / link-down / reconnect UX). */
void app_core_metering_stop(void);

#ifdef __cplusplus
}
#endif
