#pragma once

#include "esp_err.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int channel; /* 1..3 */
    bool ok;
    float voltage_v;
    float current_a;
    float power_w; /* voltage_v * current_a when ok */
} app_core_meter_sample_t;

/** Probe/register three INA219 devices and create the sample mutex. */
esp_err_t app_core_meter_init(void);

/**
 * Fresh sample for silk channel 1..3.
 * Returns ESP_ERR_INVALID_ARG for bad channel/NULL; I2C soft-fail → out->ok=false.
 */
esp_err_t app_core_meter_read_channel(int channel, app_core_meter_sample_t *out);

/** Fresh samples for OUT 1..3 into out[0..2] (channel field set to 1..3). */
esp_err_t app_core_meter_read_all(app_core_meter_sample_t out[3]);

#ifdef __cplusplus
}
#endif
