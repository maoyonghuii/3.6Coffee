#include <stdio.h>
#include "WIFI.h"
#include "ui.h"

/*mqtt*/
#include "my_mqtt.h"

static const char *TAG = "wifi station";
#define EXAMPLE_ESP_WIFI_SSID "Connection"
#define EXAMPLE_ESP_WIFI_PASS "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL (10)
#define EXAMPLE_MAX_STA_CONN (5)

// #define EXAMPLE_ESP_WIFI_SSID      "espConnect"
// #define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

#define CONFIG_ESP_WIFI_AUTH_OPEN 1

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

struct DateTime NetDateTime;
// 星期几的字符串表示
const char *weekdayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void WIFI_AP_Init(void)
{
    ESP_LOGI(TAG, "WIFI AP Start Init");

    ESP_ERROR_CHECK(esp_netif_init());                /*初始化底层TCP/IP堆栈*/
    ESP_ERROR_CHECK(esp_event_loop_create_default()); /*创建默认事件循环*/
    esp_netif_create_default_wifi_ap();               /*创建默认WIFI AP,如果出现任何初始化错误,此API将中止*/

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); /*初始化WiFi为WiFi驱动程序分配资源，如WiFi控制结构、RX/TX缓冲区、WiFi NVS结构等。此WiFi还启动WiFi任务。*/

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
void getNetTime();
void getWifiTimeTask();
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static void initialize_sntp(void);
void printDateTime(const struct DateTime *datetime);
static int s_retry_num = 0;
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(char *WIFI_Name, char *WIFI_PassWord)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };

    strcpy((char *)&wifi_config.sta.ssid, WIFI_Name);
    strcpy((char *)&wifi_config.sta.password, WIFI_PassWord);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    //  xTaskCreatePinnedToCore(&getWifiTimeTask, "getWifiTimeTask", 2048 * 4, NULL, 11, NULL, 0);
    getNetTime();
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_Name, WIFI_PassWord);
        // xTaskCreate(tcp_client_task, "tcp_client", 1024 * 10, NULL, 5, NULL);/*TCP_client 连接TCP*/
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_Name, WIFI_PassWord);
        NvsWriteDataToFlash("", "", ""); /*超出最大重连次数后，退出连接，清楚保存的连接信息，重启*/
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void NvsWriteDataToFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord)
{
    nvs_handle handle;
    // 写入一个整形数据，一个字符串，WIFI信息以及版本信息
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA2 = "String";
    static const char *DATA3 = "blob_wifi";
    // static const char *DATA4 = "blob_version";

    // // 要写入的字符串
    // char str_for_store[50] = "WIFI Config Is OK!";
    // 要写入的WIFI信息
    wifi_config_t wifi_config_to_store;
    //  wifi_config_t wifi_config_to_store = {
    //     .sta = {
    //         .ssid = "store_ssid:hello_kitty",
    //         .password = "store_password:1234567890",
    //     },
    // };
    strcpy((char *)&wifi_config_to_store.sta.ssid, WIFI_Name);
    strcpy((char *)&wifi_config_to_store.sta.password, WIFI_PassWord);
    // // 要写入的版本号
    // uint8_t version_for_store[4] = {0x01, 0x01, 0x01, 0x00};

    printf("set size:%u\r\n", sizeof(wifi_config_to_store));
    ESP_ERROR_CHECK(nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_str(handle, DATA2, ConfirmString));
    ESP_ERROR_CHECK(nvs_set_blob(handle, DATA3, &wifi_config_to_store, sizeof(wifi_config_to_store)));
    // ESP_ERROR_CHECK( nvs_set_blob( handle, DATA4, version_for_store, 4) );

    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

unsigned char NvsReadDataFromFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord)
{
    nvs_handle handle;                                 // 定义一个NVS（非易失性存储）句柄
    static const char *NVS_CUSTOMER = "customer data"; // 定义NVS命名空间
    static const char *DATA2 = "String";               // 定义用于存储字符串数据的键名
    static const char *DATA3 = "blob_wifi";            // 定义用于存储WiFi配置数据的键名
    // static const char *DATA4 = "blob_version"; // 未被使用的键名

    uint32_t str_length = 50;         // 定义用于存储字符串数据的缓冲区长度
    char str_data[50] = {0};          // 用于存储字符串数据的缓冲区
    wifi_config_t wifi_config_stored; // 定义WiFi配置数据结构
    // uint8_t version[4] = {0}; // 未被使用的版本数据
    // uint32_t version_len = 4; // 未被使用的版本数据长度

    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored)); // 将WiFi配置数据结构清零
    uint32_t wifi_len = sizeof(wifi_config_stored);               // 定义WiFi配置数据结构长度

    ESP_ERROR_CHECK(nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle)); // 打开NVS存储，并获取句柄

    ESP_LOGI(TAG, "nvsgetstr:%d", nvs_get_str(handle, DATA2, str_data, &str_length)); // 从NVS中读取字符串数据
    nvs_get_blob(handle, DATA3, &wifi_config_stored, &wifi_len);                      // 从NVS中读取WiFi配置数据
    // ESP_ERROR_CHECK ( nvs_get_blob(handle, DATA4, version, &version_len) ); // 未被使用的版本数据读取

    printf("[data1]: %s len:%lu\r\n", str_data, str_length);                                                // 打印读取的字符串数据
    printf("[data3]: ssid:%s passwd:%s\r\n", wifi_config_stored.sta.ssid, wifi_config_stored.sta.password); // 打印读取的WiFi配置数据
    strcpy(WIFI_Name, (char *)&wifi_config_stored.sta.ssid);                                                // 将WiFi名称拷贝到输出参数中
    strcpy(WIFI_PassWord, (char *)&wifi_config_stored.sta.password);                                        // 将WiFi密码拷贝到输出参数中
    nvs_close(handle);                                                                                      // 关闭NVS存储句柄

    if (strcmp(ConfirmString, str_data) == 0) // 比较读取的字符串数据和输入的确认字符串
    {
        return 0x00; // 如果相等，返回成功标志
    }
    else
    {
        return 0xFF; // 如果不相等，返回失败标志
    }
}

void getWifiTimeTask()
{
    // sntp_init();
    //   initialize_sntp();
    while (1)
    {
        getNetTime();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
char dateString[64];
bool is_update_date = true;

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    // 获取系统时间戳
    time_t now = 0;
    time(&now);
    setenv("TZ", "CST-8", 1);
    tzset();
    struct tm timeinfo = {0};
    // 结合设置的时区，转换为tm结构体
    localtime_r(&now, &timeinfo);

    // 转为字符串（方法随意，不一定要用strftime）
    char str[64];
    strftime(str, sizeof(str), "%c", &timeinfo);
    ESP_LOGI("test", "strftime %s", str);
    char weekdayName[20];
    char monthName[20];
    strftime(weekdayName, sizeof(weekdayName), "%A", &timeinfo); // 获取星期几的英文名称
    strftime(monthName, sizeof(monthName), "%B", &timeinfo);     // 获取月份的英文名称
    snprintf(dateString, sizeof(dateString), "%s %02d, %s", weekdayName, timeinfo.tm_mday, monthName);

    // ClockTime = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    my_mqtt_init();
}
// 获取网络时间 将其写入本地时间
void getNetTime()
{
            sntp_stop();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp.aliyun.com");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}