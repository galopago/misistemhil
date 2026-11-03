#include "setup_url.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "setup_url";

static const char SETUP_URL_PREFIX[] = "http://";

static bool octet_in_range(unsigned value)
{
    return value <= 255U;
}

static bool parse_octet(const char *start, const char *end, unsigned *out)
{
    if (start == NULL || end == NULL || out == NULL || start >= end) {
        return false;
    }

    unsigned value = 0U;
    for (const char *cursor = start; cursor < end; ++cursor) {
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
        value = (value * 10U) + (unsigned)(*cursor - '0');
        if (value > 255U) {
            return false;
        }
    }

    *out = value;
    return true;
}

static bool parse_ipv4_host(const char *host_start, const char *host_end, unsigned octets[4])
{
    if (host_start == NULL || host_end == NULL || host_start >= host_end) {
        return false;
    }

    size_t segment = 0U;
    const char *segment_start = host_start;

    for (const char *cursor = host_start; cursor <= host_end; ++cursor) {
        if (cursor < host_end && *cursor != '.') {
            continue;
        }

        if (segment >= 4U) {
            return false;
        }

        if (!parse_octet(segment_start, cursor, &octets[segment])) {
            return false;
        }

        ++segment;
        segment_start = cursor + 1;
    }

    return segment == 4U;
}

bool setup_url_format_ipv4(unsigned a, unsigned b, unsigned c, unsigned d, char *out,
                           size_t out_len)
{
    if (out == NULL || out_len < 15U) {
        return false;
    }

    if (!octet_in_range(a) || !octet_in_range(b) || !octet_in_range(c) || !octet_in_range(d)) {
        return false;
    }

    const int written = snprintf(out, out_len, "%s%u.%u.%u.%u", SETUP_URL_PREFIX, a, b, c, d);
    return written > 0 && (size_t)written < out_len;
}

bool setup_url_validate(const char *url)
{
    if (url == NULL) {
        return false;
    }

    const size_t prefix_len = sizeof(SETUP_URL_PREFIX) - 1U;
    if (strncmp(url, SETUP_URL_PREFIX, prefix_len) != 0) {
        return false;
    }

    const char *host_start = url + prefix_len;
    if (host_start[0] == '\0') {
        return false;
    }

    const char *cursor = host_start;
    while (*cursor != '\0') {
        if ((*cursor < '0' || *cursor > '9') && *cursor != '.') {
            return false;
        }
        ++cursor;
    }

    unsigned octets[4] = {0};
    if (!parse_ipv4_host(host_start, cursor, octets)) {
        return false;
    }

    return true;
}

static esp_err_t expect_validate(const char *url, bool expected)
{
    const bool actual = setup_url_validate(url);
    if (actual != expected) {
        ESP_LOGE(TAG, "validate(%s): got %d expected %d", url != NULL ? url : "(null)",
                 (int)actual, (int)expected);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t setup_url_self_test(void)
{
    if (expect_validate("http://192.168.4.1", true) != ESP_OK) {
        return ESP_FAIL;
    }
    if (expect_validate("http://255.255.255.255", true) != ESP_OK) {
        return ESP_FAIL;
    }
    if (expect_validate("https://192.168.4.1", false) != ESP_OK) {
        return ESP_FAIL;
    }
    if (expect_validate("http://192.168.1.1/setup", false) != ESP_OK) {
        return ESP_FAIL;
    }
    if (expect_validate(NULL, false) != ESP_OK) {
        return ESP_FAIL;
    }

    char buf[32] = {0};
    if (!setup_url_format_ipv4(192U, 168U, 4U, 1U, buf, sizeof(buf))) {
        ESP_LOGE(TAG, "format_ipv4 failed");
        return ESP_FAIL;
    }
    if (strcmp(buf, "http://192.168.4.1") != 0) {
        ESP_LOGE(TAG, "format_ipv4 wrote '%s'", buf);
        return ESP_FAIL;
    }

    if (setup_url_format_ipv4(256U, 0U, 0U, 0U, buf, sizeof(buf))) {
        ESP_LOGE(TAG, "format_ipv4 accepted invalid octet");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "self-test passed");
    return ESP_OK;
}
