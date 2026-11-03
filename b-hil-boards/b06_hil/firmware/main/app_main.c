#include "app_core.h"
#include "esp_log.h"

static const char *TAG = "b06_hil";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting b06_hil firmware");
    app_core_start();
}
