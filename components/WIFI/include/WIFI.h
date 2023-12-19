#ifndef _CONNECT_WIFI_H
#define _CONNECT_WIFI_H
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi_types.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"


#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sntp.h"
#include "ui.h"
extern struct DateTime
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    char weekday; // 0 (Sunday) to 6 (Saturday)
};
extern struct tm timeinfo;
extern struct DateTime NetDateTime;
extern char dateString[64];
void WIFI_AP_Init(void);
extern void getNetTime(void);
void wifi_init_sta(char *WIFI_Name, char *WIFI_PassWord);

void NvsWriteDataToFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord);
unsigned char NvsReadDataFromFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord);

#endif