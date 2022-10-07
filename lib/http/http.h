#ifndef HTTP_H
#define HTTP_H
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define BUFF_LEN 128

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "example.com"
#define WEB_PORT "80"
#define WEB_PATH "/"

const char *HTTP = "HTTP";

const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
                      "Host: " WEB_SERVER ":" WEB_PORT "\r\n"
                      "User-Agent: esp-idf/1.0 esp32\r\n"
                      "\r\n";

void http_get_task(void *pvParameters);

// ESP_ERROR_CHECK( nvs_flash_init() );
// ESP_ERROR_CHECK(esp_netif_init());
// ESP_ERROR_CHECK(esp_event_loop_create_default());

// xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);

#endif