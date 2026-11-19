#pragma once

#include "esp_err.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Start STA host API HTTP server on port 80. Idempotent if already running. */
esp_err_t host_api_start(void);

/**
 * Stop host API, close any stream session, free RX queue.
 * reason: short log tag only ("link_lost", "portal", "shutdown", …).
 * Idempotent if already stopped.
 */
esp_err_t host_api_stop(const char *reason);

bool host_api_is_running(void);

#ifdef __cplusplus
}
#endif
