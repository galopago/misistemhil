#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

bool setup_url_format_ipv4(unsigned a, unsigned b, unsigned c, unsigned d, char *out,
                           size_t out_len);
bool setup_url_validate(const char *url);
esp_err_t setup_url_self_test(void);

#ifdef __cplusplus
}
#endif
