#ifndef WIFI_H
#define WIFI_H

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
//#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_smartconfig.h"
#include "esp_netif.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;
const int ESPTOUCH_DONE_BIT = BIT1;
const char *WIFI = "smartconfig_wifi";

void smartconfig_task(void * parm);

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void initialise_wifi(void);



#endif