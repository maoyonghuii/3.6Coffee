#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_http_server.h"

#include "web_server.h"
#include "WIFI.h"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static const char *TAG = "WEB_SERVER";
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Send HTTP response with a run-time generated html consisting of
 * a list of all files and folders under the requested path.
 * In case of SPIFFS this returns empty list when path is any
 * string other than '/', since SPIFFS doesn't support directories */
static esp_err_t http_SendText_html(httpd_req_t *req)
{
    /* Get handle to embedded file upload script */

    const size_t upload_script_size = (index_html_end - index_html_start);
    const char TxBuffer[] = "<h1> SSID1 other WIFI</h1>";
    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_send_chunk(req, (const char *)index_html_start, upload_script_size);

    httpd_resp_send_chunk(req, (const char *)TxBuffer, sizeof(TxBuffer));

    return ESP_OK;
}
unsigned char CharToNum(unsigned char Data)
{
    if (Data >= '0' && Data <= '9')
    {
        return Data - '0';
    }
    else if (Data >= 'a' && Data <= 'f')
    {
        switch (Data)
        {
        case 'a':
            return 10;
        case 'b':
            return 11;
        case 'c':
            return 12;
        case 'd':
            return 13;
        case 'e':
            return 14;
        case 'f':
            return 15;
        default:
            break;
        }
    }
    else if (Data >= 'A' && Data <= 'F')
    {
        switch (Data)
        {
        case 'A':
            return 10;
        case 'B':
            return 11;
        case 'C':
            return 12;
        case 'D':
            return 13;
        case 'E':
            return 14;
        case 'F':
            return 15;
        default:
            break;
        }
    }
    return 0;
}
/* 强制门户访问时连接wifi后的第一次任意GET请求 */
static esp_err_t HTTP_FirstGet_handler(httpd_req_t *req)
{
    http_SendText_html(req);

    return ESP_OK;
}

static esp_err_t IO_Controll_POST_handler(httpd_req_t *req)
{
    ESP_LOGI("IO_Controll_POST_handler", "start");
    char content[100];
    httpd_resp_set_type(req, "text/plain");
    if (req->method == HTTP_GET)
    {
        httpd_resp_send_404(req);
        return ESP_OK;
    }
    if (httpd_req_get_url_query_str(req, content, sizeof(content)) == ESP_OK)
    {
        char param[32];
        if (httpd_query_key_value(content, "pin", param, sizeof(param)) == ESP_OK)
        {
            int pin = atoi(param); // 将字符串转换为整数
            if (pin < 1)
            {
                if (httpd_query_key_value(content, "state", param, sizeof(param)) == ESP_OK)
                {
                    int state = atoi(param);
                     ESP_LOGI("IO_Controll_POST_handler", "pin: %d  state:%d  ",pin,state);
                    gpio_reset_pin(pin);
                    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
                    gpio_set_level(pin, state);
                    httpd_resp_sendstr(req, "OK");
                    return ESP_OK;
                }
            }
        }
        // else if (httpd_query_key_value(content, "pwmpin", param, sizeof(param)) == ESP_OK)
        // {
        //     int pwmPin = atoi(param);
        //     if (pwmPin >= 0 && pwmPin <= 255)
        //     {
        //         if (httpd_query_key_value(content, "pwmValue", param, sizeof(param)) == ESP_OK)
        //         {
        //             int pwmValue = atoi(param);
        //             ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, pwmValue);
        //             ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        //             httpd_resp_sendstr(req, "OK");
        //             return ESP_OK;
        //         }
        //     }
    }
    // }

    httpd_resp_send_404(req);
    return ESP_OK;
}
/* 门户页面发回的，带有要连接的WIFI的名字和密码 */
static esp_err_t WIFI_Config_POST_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;
    ESP_LOGI(TAG, "get post handler");
    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* 如果发生超时，重新尝试接收数据 */
                continue;
            }
            return ESP_FAIL;
        }
        /* 发送相同的数据回去 */
        char WIFI_ConfigBackInformation[100] = "The WIFI To Connect :";
        strcat(WIFI_ConfigBackInformation, buf);
        httpd_resp_send_chunk(req, WIFI_ConfigBackInformation, sizeof(WIFI_ConfigBackInformation));
        remaining -= ret;
        /* 提取WiFi名称和密码信息 */
        char wifi_name[50];
        char wifi_password[50];
        char wifi_passwordTransformation[50] = {0};
        esp_err_t e = httpd_query_key_value(buf, "ssid", wifi_name, sizeof(wifi_name));
        if (e == ESP_OK)
        {
            printf("SSID = %s\r\n", wifi_name);
        }
        else
        {
            printf("error = %d\r\n", e);
        }

        e = httpd_query_key_value(buf, "passWord", wifi_password, sizeof(wifi_password));
        if (e == ESP_OK)
        {
            /*对传回来的数据进行处理*/
            unsigned char Len = strlen(wifi_password);
            char tempBuffer[2];
            char *temp;
            unsigned char Cnt = 0;
            temp = wifi_password;
            for (int i = 0; i < Len;)
            {
                if (*temp == '%')
                {
                    tempBuffer[0] = CharToNum(temp[1]);
                    tempBuffer[1] = CharToNum(temp[2]);
                    *temp = tempBuffer[0] * 16 + tempBuffer[1];
                    wifi_passwordTransformation[Cnt] = *temp;
                    temp += 3;
                    i += 3;
                    Cnt++;
                }
                else
                {
                    wifi_passwordTransformation[Cnt] = *temp;
                    temp++;
                    i++;
                    Cnt++;
                }
            }
            temp -= Len;
            printf("Len = %d\r\n", Len);
            printf("wifi_password = %s\r\n", wifi_password);
            printf("pswd = %s\r\n", wifi_passwordTransformation);
        }
        else
        {
            printf("error = %d\r\n", e);
        }
        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");

        NvsWriteDataToFlash("WIFI Config", wifi_name, wifi_passwordTransformation);
        esp_restart();
        // esp_wifi_stop();
        // esp_event_handler_unregister(WIFI_EVENT,
        //                                 ESP_EVENT_ANY_ID,
        //                                 &wifi_event_handler
        //                                 );
        // esp_netif_destroy_default_wifi(ap_netif);
        // esp_event_loop_delete_default();
        // esp_wifi_deinit();
        // esp_netif_deinit();
        // httpd_stop(server);
        // printf("hello \r\n");
        // ESP_LOGI(TAG,"led on");
        // wifi_init_sta(wifi_name,wifi_passwordTransformation);
    }
    return ESP_OK;
}

void web_server_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK)
    {
        // 启动 HTTP 服务器，将服务器句柄和配置作为参数传递给 httpd_start 函数，并检查返回值是否为 ESP_OK
        ESP_LOGE(TAG, "Failed to start file server!");
        return;
    }
    httpd_uri_t file_download = {
        .uri = "/",                       // 匹配形如 /path/to/file 的所有 URI
        .method = HTTP_GET,               // 处理 GET 请求
        .handler = HTTP_FirstGet_handler, // 处理函数为 HTTP_FirstGet_handler
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &file_download);
    httpd_uri_t configWifi = {
        .uri = "/configwifi",                // 匹配形如 /upload/path/to/file 的所有 URI
        .method = HTTP_POST,                 // 处理 POST 请求
        .handler = WIFI_Config_POST_handler, // 处理函数为 WIFI_Config_POST_handler
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &configWifi);
    httpd_uri_t controll_IO = {
        .uri = "/controll_io",               // 匹配形如 /upload/path/to/file 的所有 URI
        .method = HTTP_POST,                 // 处理 POST 请求
        .handler = IO_Controll_POST_handler, // 处理函数为 WIFI_Config_POST_handler
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &controll_IO);
}
