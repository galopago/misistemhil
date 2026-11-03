#include "app_core.h"
#include "esp_log.h"

static const char *TAG = "b00_basehil";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting b00_basehil firmware");
    app_core_start();
}
