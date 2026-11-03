#include "app_core_metering.h"

#include "app_core.h"
#include "app_core_meter.h"
#include "device_identity.h"
#include "display_types.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_core_metering";

#define METERING_PERIOD_MS 500U
#define METERING_TASK_STACK 4096U
#define METERING_TASK_PRIORITY 5
#define METERING_CHANNEL_COUNT 3
#define METERING_LINE_CAP 17

static const int k_silk[METERING_CHANNEL_COUNT] = {1, 2, 3};

static volatile bool s_active = false;
static bool s_task_started = false;

static float clamp_display_mag(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 9.99f) {
        return 9.99f;
    }
    return value;
}

static void format_channel_line(char *out, size_t out_len, int silk, bool ok, float volts,
                                float amps)
{
    if (out == NULL || out_len == 0) {
        return;
    }

    /* Fixed 14-char columns; current sign column keeps digits from shifting. */
    if (!ok) {
        (void)snprintf(out, out_len, "%d ----V  ----A", silk);
        return;
    }

    const char sign = (amps < 0.0f) ? '-' : ' ';
    const float v_mag = clamp_display_mag(volts);
    const float i_mag = clamp_display_mag(fabsf(amps));
    (void)snprintf(out, out_len, "%d %4.2fV %c%4.2fA", silk, v_mag, sign, i_mag);
}

static void metering_push_display(void)
{
    char hostname[DEVICE_ID_MAX_LEN] = {0};
    if (device_identity_get(hostname, sizeof(hostname)) != ESP_OK || hostname[0] == '\0') {
        ESP_LOGW(TAG, "identity unavailable; skip metering frame");
        return;
    }

    char line1[METERING_LINE_CAP];
    char line2[METERING_LINE_CAP];
    char line3[METERING_LINE_CAP];
    char *channel_lines[METERING_CHANNEL_COUNT] = {line1, line2, line3};

    app_core_meter_sample_t samples[METERING_CHANNEL_COUNT];
    if (app_core_meter_read_all(samples) != ESP_OK) {
        for (size_t i = 0; i < METERING_CHANNEL_COUNT; ++i) {
            format_channel_line(channel_lines[i], METERING_LINE_CAP, k_silk[i], false, 0.0f,
                                0.0f);
        }
    } else {
        for (size_t i = 0; i < METERING_CHANNEL_COUNT; ++i) {
            format_channel_line(channel_lines[i], METERING_LINE_CAP, samples[i].channel,
                                samples[i].ok, samples[i].voltage_v, samples[i].current_a);
        }
    }

    const char *lines[4] = {hostname, line1, line2, line3};
    (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_FOUR_LINES, lines, 4);
}

static void metering_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();

    while (true) {
        if (s_active) {
            metering_push_display();
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(METERING_PERIOD_MS));
    }
}

esp_err_t app_core_metering_init(void)
{
    esp_err_t err = app_core_meter_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "app_core_meter_init failed: %s", esp_err_to_name(err));
        return err;
    }

    if (!s_task_started) {
        const BaseType_t ok =
            xTaskCreate(metering_task, "metering", METERING_TASK_STACK, NULL,
                        METERING_TASK_PRIORITY, NULL);
        if (ok != pdPASS) {
            ESP_LOGE(TAG, "metering task create failed");
            return ESP_ERR_NO_MEM;
        }
        s_task_started = true;
    }

    return ESP_OK;
}

void app_core_metering_start(void)
{
    if (!s_task_started) {
        ESP_LOGW(TAG, "start requested before init");
        return;
    }
    s_active = true;
    metering_push_display();
}

void app_core_metering_stop(void)
{
    s_active = false;
}
