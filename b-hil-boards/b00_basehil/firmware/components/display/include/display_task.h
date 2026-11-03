#pragma once

#include "display_types.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t display_task_start(const display_config_t *config);
void display_task_stop(void);
esp_err_t display_task_post_set_state(const display_state_t *state);
esp_err_t display_task_post_set_layout(const display_layout_t *layout);
esp_err_t display_task_post_set_content(const display_layout_t *layout);
esp_err_t display_task_post_clear(void);
esp_err_t display_task_post_refresh(void);

#ifdef __cplusplus
}
#endif
