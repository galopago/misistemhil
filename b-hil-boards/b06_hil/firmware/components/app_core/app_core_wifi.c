#include "app_core_wifi.h"

#include "app_core.h"
#include "board_pins.h"
#include "display_types.h"
#include "wifi_credentials.h"
#include "wifi_provisioning.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_core_wifi";

#define FACTORY_RESET_HOLD_MS 2000U
#define FACTORY_RESET_SAMPLE_MS 50U
#define PROV_SETUP_LINE_JOIN "1 JOIN"
#define PROV_SETUP_LINE_SCAN_QR "2 SCAN QR"
#define PROV_SETUP_SSID_PREFIX_LEN 7
#define PROV_SETUP_FALLBACK_URL "http://192.168.4.1"
#define WIFI_OK_LINE_STATUS "WIFI OK"
#define WIFI_OK_MAC_PREFIX_LEN 8
#define WIFI_OK_MAC_SUFFIX_OFFSET 9

static bool s_wifi_started = false;

static void show_wifi_connected_display(const wifi_prov_event_info_t *info)
{
    if (info == NULL || info->sta_ipv4 == NULL || info->sta_mac == NULL ||
        strlen(info->sta_mac) != 17U) {
        const char *fallback[] = {"WIFI", "OK"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, fallback, 2);
        return;
    }

    char mac_line2[9];
    char mac_line3[9];
    strncpy(mac_line2, info->sta_mac, WIFI_OK_MAC_PREFIX_LEN);
    mac_line2[WIFI_OK_MAC_PREFIX_LEN] = '\0';
    strncpy(mac_line3, info->sta_mac + WIFI_OK_MAC_SUFFIX_OFFSET, 8);
    mac_line3[8] = '\0';

    const char *lines[] = {WIFI_OK_LINE_STATUS, info->sta_ipv4, mac_line2, mac_line3};
    (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_FOUR_LINES, lines, 4);
}

static void show_provisioning_setup_display(const wifi_prov_event_info_t *info)
{
    const char *url =
        (info != NULL && info->setup_url != NULL) ? info->setup_url : PROV_SETUP_FALLBACK_URL;
    const char *ap_ssid = info != NULL ? info->ssid : NULL;

    if (ap_ssid == NULL || strlen(ap_ssid) < 8U) {
        const char *fallback[] = {"WIFI", "SETUP"};
        (void)app_core_display_show_qr_setup(url, fallback, 2);
        return;
    }

    char line_ssid_prefix[PROV_SETUP_SSID_PREFIX_LEN + 1];
    char line_ssid_suffix[DISPLAY_MAX_LINE_LEN];

    strncpy(line_ssid_prefix, ap_ssid, PROV_SETUP_SSID_PREFIX_LEN);
    line_ssid_prefix[PROV_SETUP_SSID_PREFIX_LEN] = '\0';
    strncpy(line_ssid_suffix, ap_ssid + PROV_SETUP_SSID_PREFIX_LEN, DISPLAY_MAX_LINE_LEN - 1U);
    line_ssid_suffix[DISPLAY_MAX_LINE_LEN - 1U] = '\0';

    const char *lines[] = {PROV_SETUP_LINE_JOIN, line_ssid_prefix, line_ssid_suffix,
                           PROV_SETUP_LINE_SCAN_QR};
    (void)app_core_display_show_qr_setup(url, lines, 4);
}

static bool factory_reset_requested(void)
{
    const b06_hil_board_pins_t *pins = board_pins();
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << pins->factory_reset_switch,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_FALSE(gpio_config(&config) == ESP_OK, false, TAG, "gpio config failed");

    int64_t pressed_ms = 0;
    while (pressed_ms < (int64_t)FACTORY_RESET_HOLD_MS) {
        const int level = gpio_get_level(pins->factory_reset_switch);
        const bool pressed = level == BOARD_FACTORY_RESET_ACTIVE_LEVEL;
        if (!pressed) {
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(FACTORY_RESET_SAMPLE_MS));
        pressed_ms += (int64_t)FACTORY_RESET_SAMPLE_MS;
    }

    ESP_LOGW(TAG, "factory reset requested");
    return true;
}

static void on_wifi_prov_event(const wifi_prov_event_info_t *info, void *ctx)
{
    (void)ctx;
    if (info == NULL) {
        return;
    }

    switch (info->event) {
    case WIFI_PROV_EVENT_AP_STARTED:
    case WIFI_PROV_EVENT_PORTAL_READY:
        show_provisioning_setup_display(info);
        break;
    case WIFI_PROV_EVENT_SUBMITTED_CONNECTING: {
        const char *lines[] = {"WIFI", "CONNECTING"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        break;
    }
    case WIFI_PROV_EVENT_SUBMITTED_SUCCESS:
        show_wifi_connected_display(info);
        break;
    case WIFI_PROV_EVENT_SUBMITTED_FAILURE: {
        const char *lines[] = {"WIFI", "FAILED"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        break;
    }
    case WIFI_PROV_EVENT_SAVED_CONNECTING: {
        const char *lines[] = {"WIFI", "CONNECTING"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        break;
    }
    case WIFI_PROV_EVENT_SAVED_SUCCESS:
        show_wifi_connected_display(info);
        break;
    case WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED: {
        const char *lines[] = {"WIFI", "FAILED", "HOLD", "RESET"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_FOUR_LINES, lines, 4);
        break;
    }
    case WIFI_PROV_EVENT_ERROR: {
        const char *line2 = info->setup_url == NULL ? "AP FAIL" : "WEB FAIL";
        const char *lines[] = {"WIFI", line2};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        break;
    }
    default:
        break;
    }
}

esp_err_t app_core_wifi_start(void)
{
    if (s_wifi_started) {
        return ESP_OK;
    }

    esp_err_t err = wifi_credentials_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi credentials init failed: %s", esp_err_to_name(err));
        const char *lines[] = {"WIFI", "NVS ERR"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        return err;
    }

    bool credentials_erased = false;
    if (factory_reset_requested()) {
        err = wifi_credentials_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "factory reset erase failed: %s", esp_err_to_name(err));
            return err;
        }
        credentials_erased = true;
    }

    ESP_RETURN_ON_ERROR(wifi_provisioning_init(on_wifi_prov_event, NULL), TAG,
                        "wifi provisioning init failed");

    wifi_credentials_t credentials = {0};
    const bool has_credentials = !credentials_erased && wifi_credentials_load(&credentials);

    if (!has_credentials) {
        err = wifi_provisioning_start_ap_portal();
        if (err != ESP_OK) {
            return err;
        }
    } else {
        err = wifi_provisioning_connect_saved(&credentials);
    }

    s_wifi_started = true;
    return ESP_OK;
}
