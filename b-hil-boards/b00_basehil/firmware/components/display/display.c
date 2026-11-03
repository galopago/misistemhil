#include "display.h"

#include "display_task.h"

esp_err_t display_start(const display_config_t *config)
{
    return display_task_start(config);
}

void display_stop(void)
{
    display_task_stop();
}

esp_err_t display_set_state(const display_state_t *state)
{
    return display_task_post_set_state(state);
}

esp_err_t display_set_layout(const display_layout_t *layout)
{
    return display_task_post_set_layout(layout);
}

esp_err_t display_set_content(const display_layout_t *layout)
{
    return display_task_post_set_content(layout);
}

esp_err_t display_clear(void)
{
    return display_task_post_clear();
}

esp_err_t display_force_refresh(void)
{
    return display_task_post_refresh();
}
