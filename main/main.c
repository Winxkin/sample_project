#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sx1278.h"

int lora_status;
uint8_t buffer[32];
static const char *LORA = "LORA";

void task_tx(void *p)
{
    while (1)
    {

        ESP_LOGI(LORA, "task tx");
        vTaskDelay(pdMS_TO_TICKS(5000));
        sx1278_send_packet((uint8_t *)"Hello", 5);
        printf("packet sent...\n");
        lora_status = 1;
    }
}

void task_rx(void *p)
{
    int x;
    ESP_LOGI(LORA, "task rx");
    while (1)
    {

        sx1278_receive(); // put into receive mode
        while (sx1278_received())
        {
            ESP_LOGI(LORA, "have info");
            x = sx1278_receive_packet(buffer, sizeof(buffer));
            buffer[x] = 0;
            printf("Received: %s\n", buffer);
            sx1278_receive();
        }
        vTaskDelay(1);
    }
}

void app_main(void)
{
    lora_status = 1;
    sx1278_init();
    sx1278_set_frequency(915e6);
    sx1278_enable_crc();
    // xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, NULL);
    xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
}