#include "app_core_wifi.h"

#include "app_core.h"
#include "board_pins.h"
#include "device_discovery.h"
#include "display_types.h"
#include "wifi_credentials.h"
#include "wifi_provisioning.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_core_wifi";

#define FACTORY_RESET_HOLD_MS 10000U
#define FACTORY_RESET_SAMPLE_MS 50U
#define FACTORY_RESET_MONITOR_STACK 3072U
#define FACTORY_RESET_MONITOR_PRIORITY 6
#define PROV_SETUP_LINE_JOIN "1 JOIN"
#define PROV_SETUP_LINE_SCAN_QR "2 SCAN QR"
#define PROV_SETUP_SSID_PREFIX_LEN 7
#define PROV_SETUP_FALLBACK_URL "http://192.168.4.1"
#define WIFI_OK_LINE_STATUS "WIFI OK"
#define WIFI_OK_MAC_PREFIX_LEN 8
#define WIFI_OK_MAC_SUFFIX_OFFSET 9
#define PROV_FAILURE_FLASH_MS 3000U
#define PROV_SETUP_URL_MAX_LEN 32U
#define PROV_AP_SSID_MAX_LEN 33U

static bool s_wifi_started = false;
static wifi_link_status_t s_last_link_status = WIFI_LINK_STATUS_UNPROVISIONED;
static char s_cached_setup_url[PROV_SETUP_URL_MAX_LEN];
static char s_cached_ap_ssid[PROV_AP_SSID_MAX_LEN];
static bool s_portal_display_cached = false;
static esp_timer_handle_t s_prov_failure_restore_timer = NULL;

static void apply_wifi_link_status_led(wifi_link_status_t status)
{
    error_led_pattern_t pattern;
    switch (status) {
    case WIFI_LINK_STATUS_UNPROVISIONED:
        pattern = ERROR_LED_PATTERN_ON;
        break;
    case WIFI_LINK_STATUS_CONNECTING:
        pattern = ERROR_LED_PATTERN_SLOW_BLINK;
        break;
    case WIFI_LINK_STATUS_CONNECTING_ALERT:
        pattern = ERROR_LED_PATTERN_FAST_BLINK;
        break;
    case WIFI_LINK_STATUS_CONNECTED:
        pattern = ERROR_LED_PATTERN_OFF;
        break;
    default:
        pattern = ERROR_LED_PATTERN_ON;
        break;
    }
    (void)app_core_error_led_set_pattern(pattern);
}

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

static void cache_portal_display_context(const wifi_prov_event_info_t *info)
{
    if (info == NULL) {
        return;
    }

    const char *url =
        info->setup_url != NULL ? info->setup_url : PROV_SETUP_FALLBACK_URL;
    strncpy(s_cached_setup_url, url, sizeof(s_cached_setup_url) - 1U);
    s_cached_setup_url[sizeof(s_cached_setup_url) - 1U] = '\0';

    if (info->ssid != NULL && strlen(info->ssid) >= 8U) {
        strncpy(s_cached_ap_ssid, info->ssid, sizeof(s_cached_ap_ssid) - 1U);
        s_cached_ap_ssid[sizeof(s_cached_ap_ssid) - 1U] = '\0';
        s_portal_display_cached = true;
    }
}

static void restore_provisioning_setup_from_cache(void)
{
    if (!s_portal_display_cached || s_cached_setup_url[0] == '\0' ||
        strlen(s_cached_ap_ssid) < 8U) {
        const char *fallback[] = {"WIFI", "SETUP"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, fallback, 2);
        return;
    }

    const wifi_prov_event_info_t cached_info = {
        .setup_url = s_cached_setup_url,
        .ssid = s_cached_ap_ssid,
    };
    show_provisioning_setup_display(&cached_info);
}

static void prov_failure_restore_timer_cb(void *arg)
{
    (void)arg;
    restore_provisioning_setup_from_cache();
}

static void cancel_provisioning_setup_restore(void)
{
    if (s_prov_failure_restore_timer != NULL) {
        (void)esp_timer_stop(s_prov_failure_restore_timer);
    }
}

static void schedule_provisioning_setup_restore(void)
{
    if (s_prov_failure_restore_timer == NULL) {
        return;
    }

    cancel_provisioning_setup_restore();
    (void)esp_timer_start_once(s_prov_failure_restore_timer,
                                (uint64_t)PROV_FAILURE_FLASH_MS * 1000ULL);
}

static esp_err_t ensure_prov_failure_restore_timer(void)
{
    if (s_prov_failure_restore_timer != NULL) {
        return ESP_OK;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = prov_failure_restore_timer_cb,
        .name = "prov_fail_rst",
    };
    return esp_timer_create(&timer_args, &s_prov_failure_restore_timer);
}

static void sync_device_discovery(const wifi_prov_event_info_t *info)
{
    if (info == NULL) {
        return;
    }

    switch (info->event) {
    case WIFI_PROV_EVENT_AP_STARTED:
    case WIFI_PROV_EVENT_PORTAL_READY:
        (void)device_discovery_stop("portal");
        break;
    default:
        break;
    }

    if (info->link_status == WIFI_LINK_STATUS_CONNECTED) {
        (void)device_discovery_start();
    } else if (s_last_link_status == WIFI_LINK_STATUS_CONNECTED) {
        (void)device_discovery_stop("link_lost");
    }

    s_last_link_status = info->link_status;
}

static esp_err_t factory_reset_gpio_configure(void)
{
    const b06_hil_board_pins_t *pins = board_pins();
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << pins->factory_reset_switch,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&config) == ESP_OK ? ESP_OK : ESP_FAIL;
}

static bool factory_reset_button_pressed(void)
{
    const b06_hil_board_pins_t *pins = board_pins();
    return gpio_get_level(pins->factory_reset_switch) == BOARD_FACTORY_RESET_ACTIVE_LEVEL;
}

static bool factory_reset_hold_confirmed(void)
{
    int64_t pressed_ms = 0;
    while (pressed_ms < (int64_t)FACTORY_RESET_HOLD_MS) {
        if (!factory_reset_button_pressed()) {
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(FACTORY_RESET_SAMPLE_MS));
        pressed_ms += (int64_t)FACTORY_RESET_SAMPLE_MS;
    }

    return true;
}

static bool factory_reset_requested_at_boot(void)
{
    if (factory_reset_gpio_configure() != ESP_OK) {
        return false;
    }

    if (!factory_reset_button_pressed()) {
        return false;
    }

    if (!factory_reset_hold_confirmed()) {
        return false;
    }

    ESP_LOGW(TAG, "factory reset requested");
    return true;
}

static void factory_reset_wait_for_release(void)
{
    while (factory_reset_button_pressed()) {
        vTaskDelay(pdMS_TO_TICKS(FACTORY_RESET_SAMPLE_MS));
    }
}

static void factory_reset_runtime_sequence(void)
{
    ESP_LOGW(TAG, "factory reset requested (runtime)");

    (void)device_discovery_stop("factory_reset");

    esp_err_t err = wifi_credentials_erase();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "factory reset erase failed err=%s", esp_err_to_name(err));
        return;
    }

    err = wifi_provisioning_factory_reset_to_portal();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "factory reset portal transition failed err=%s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "factory reset complete portal_active=true");
}

static void factory_reset_monitor_task(void *arg)
{
    (void)arg;

    if (factory_reset_gpio_configure() != ESP_OK) {
        ESP_LOGE(TAG, "factory reset monitor gpio init failed");
        vTaskDelete(NULL);
        return;
    }

    int64_t pressed_ms = 0;
    while (true) {
        if (factory_reset_button_pressed()) {
            vTaskDelay(pdMS_TO_TICKS(FACTORY_RESET_SAMPLE_MS));
            if (factory_reset_button_pressed()) {
                pressed_ms += (int64_t)FACTORY_RESET_SAMPLE_MS;
                if (pressed_ms >= (int64_t)FACTORY_RESET_HOLD_MS) {
                    factory_reset_runtime_sequence();
                    factory_reset_wait_for_release();
                    pressed_ms = 0;
                }
            } else {
                pressed_ms = 0;
            }
        } else {
            pressed_ms = 0;
            vTaskDelay(pdMS_TO_TICKS(FACTORY_RESET_SAMPLE_MS));
        }
    }
}

static esp_err_t factory_reset_monitor_start(void)
{
    const BaseType_t ok = xTaskCreate(factory_reset_monitor_task, "wifi_fr_mon",
                                      FACTORY_RESET_MONITOR_STACK, NULL,
                                      FACTORY_RESET_MONITOR_PRIORITY, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "factory reset monitor task create failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
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
        cache_portal_display_context(info);
        show_provisioning_setup_display(info);
        break;
    case WIFI_PROV_EVENT_SUBMITTED_CONNECTING:
        cancel_provisioning_setup_restore();
        {
            const char *lines[] = {"WIFI", "CONNECTING"};
            (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        }
        break;
    case WIFI_PROV_EVENT_SUBMITTED_SUCCESS:
        cancel_provisioning_setup_restore();
        show_wifi_connected_display(info);
        break;
    case WIFI_PROV_EVENT_SUBMITTED_FAILURE: {
        const char *lines[] = {"WIFI", "FAILED"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        schedule_provisioning_setup_restore();
        break;
    }
    case WIFI_PROV_EVENT_SAVED_CONNECTING:
    case WIFI_PROV_EVENT_RUNTIME_RECONNECTING:
    case WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE:
    case WIFI_PROV_EVENT_CONNECT_ALERT_PHASE: {
        const char *lines[] = {"WIFI", "CONNECTING"};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        break;
    }
    case WIFI_PROV_EVENT_SAVED_SUCCESS:
    case WIFI_PROV_EVENT_RUNTIME_RESTORED:
    case WIFI_PROV_EVENT_CONNECT_RESTORED:
        show_wifi_connected_display(info);
        break;
    case WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED:
        break;
    case WIFI_PROV_EVENT_ERROR: {
        const char *line2 = info->setup_url == NULL ? "AP FAIL" : "WEB FAIL";
        const char *lines[] = {"WIFI", line2};
        (void)app_core_display_show_template(DISPLAY_TEMPLATE_FULL_TWO_LINES, lines, 2);
        break;
    }
    case WIFI_PROV_EVENT_LINK_STATUS_CHANGED:
        break;
    default:
        break;
    }

    apply_wifi_link_status_led(info->link_status);
    sync_device_discovery(info);
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
    if (factory_reset_requested_at_boot()) {
        err = wifi_credentials_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "factory reset erase failed err=%s", esp_err_to_name(err));
            return err;
        }
        credentials_erased = true;
    }

    ESP_RETURN_ON_ERROR(wifi_provisioning_init(on_wifi_prov_event, NULL), TAG,
                        "wifi provisioning init failed");

    ESP_RETURN_ON_ERROR(device_discovery_init(), TAG, "device discovery init failed");

    ESP_RETURN_ON_ERROR(ensure_prov_failure_restore_timer(), TAG,
                        "prov failure restore timer create failed");

    ESP_RETURN_ON_ERROR(factory_reset_monitor_start(), TAG, "factory reset monitor start failed");

    wifi_credentials_t credentials = {0};
    const bool has_credentials = !credentials_erased && wifi_credentials_load(&credentials);

    if (!has_credentials) {
        apply_wifi_link_status_led(WIFI_LINK_STATUS_UNPROVISIONED);
    } else {
        apply_wifi_link_status_led(WIFI_LINK_STATUS_CONNECTING);
    }

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
