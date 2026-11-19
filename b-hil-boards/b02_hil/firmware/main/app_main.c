#include "app_core.h"
#include "esp_log.h"

static const char *TAG = "b02_hil";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting b02_hil firmware");
    app_core_start();
}
