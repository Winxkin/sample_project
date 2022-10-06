#include <stdio.h>
#include "gpioconfig.h"
#include "sx1278.h"
#include "http.h"
#include "nvs_flash.h"

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
