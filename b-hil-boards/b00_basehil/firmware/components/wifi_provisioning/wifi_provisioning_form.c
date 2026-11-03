#include "wifi_provisioning_form.h"

#include <string.h>

static bool hex_value(char ch, unsigned *out)
{
    if (ch >= '0' && ch <= '9') {
        *out = (unsigned)(ch - '0');
        return true;
    }
    if (ch >= 'A' && ch <= 'F') {
        *out = (unsigned)(ch - 'A') + 10U;
        return true;
    }
    if (ch >= 'a' && ch <= 'f') {
        *out = (unsigned)(ch - 'a') + 10U;
        return true;
    }
    return false;
}

static bool append_decoded_char(char *dest, size_t dest_size, size_t *dest_len, char ch)
{
    if (*dest_len + 1U >= dest_size) {
        return false;
    }
    dest[*dest_len] = ch;
    (*dest_len)++;
    dest[*dest_len] = '\0';
    return true;
}

static bool decode_field_value_segment(const char *input, size_t input_len, char *out,
                                       size_t out_size)
{
    if (out == NULL || out_size == 0U) {
        return false;
    }

    out[0] = '\0';
    size_t out_len = 0U;

    for (size_t i = 0U; i < input_len; ++i) {
        if (input[i] == '%') {
            if (i + 2U >= input_len) {
                return false;
            }
            unsigned hi = 0U;
            unsigned lo = 0U;
            if (!hex_value(input[i + 1U], &hi) || !hex_value(input[i + 2U], &lo)) {
                return false;
            }
            if (!append_decoded_char(out, out_size, &out_len, (char)((hi << 4) | lo))) {
                return false;
            }
            i += 2U;
            continue;
        }

        if (input[i] == '+') {
            if (!append_decoded_char(out, out_size, &out_len, ' ')) {
                return false;
            }
            continue;
        }

        if (!append_decoded_char(out, out_size, &out_len, input[i])) {
            return false;
        }
    }

    return true;
}

static bool name_matches(const char *name, size_t name_len, const char *literal)
{
    const size_t literal_len = strlen(literal);
    return name_len == literal_len && memcmp(name, literal, literal_len) == 0;
}

static void set_field_segment(wifi_prov_form_t *form, const char *name, size_t name_len,
                              const char *value, size_t value_len)
{
    if (form == NULL || name == NULL || value == NULL) {
        return;
    }

    if (name_matches(name, name_len, "ssid")) {
        if (!decode_field_value_segment(value, value_len, form->ssid, sizeof(form->ssid))) {
            form->valid = false;
            return;
        }
        form->has_ssid = form->ssid[0] != '\0';
        return;
    }

    if (name_matches(name, name_len, "password")) {
        if (!decode_field_value_segment(value, value_len, form->password,
                                        sizeof(form->password))) {
            form->valid = false;
        }
    }
}

bool wifi_prov_form_parse(const char *body, size_t body_len, wifi_prov_form_t *out)
{
    if (body == NULL || out == NULL || body_len > WIFI_PROV_MAX_FORM_BODY_LEN) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->valid = true;

    size_t start = 0U;
    for (size_t i = 0U; i <= body_len; ++i) {
        if (i < body_len && body[i] != '&') {
            continue;
        }

        const size_t pair_len = i - start;
        if (pair_len == 0U) {
            start = i + 1U;
            continue;
        }

        const char *pair = body + start;
        const char *equals = memchr(pair, '=', pair_len);
        if (equals == NULL) {
            return false;
        }

        const size_t name_len = (size_t)(equals - pair);
        const char *value = equals + 1U;
        const size_t value_len = pair_len - name_len - 1U;

        set_field_segment(out, pair, name_len, value, value_len);
        if (!out->valid) {
            return false;
        }

        start = i + 1U;
    }

    if (!out->has_ssid) {
        return false;
    }

    wifi_credentials_t credentials = {0};
    strncpy(credentials.ssid, out->ssid, sizeof(credentials.ssid) - 1U);
    strncpy(credentials.password, out->password, sizeof(credentials.password) - 1U);
    credentials.has_password = out->password[0] != '\0';

    return wifi_credentials_validate(&credentials);
}
