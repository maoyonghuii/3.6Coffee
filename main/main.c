/* SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "lv_demos.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_painter.h"
#include "esp_io_expander_tca9554.h"
#include "esp_jpeg_dec.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ppbuffer.h"
#include "usb_stream.h"
#include "iot_button.h"
#include "ui.h"
#include "lv_fs_if.h"
#include "sd_card.h"
#include "WIFI.h"
#include "web_server.h"
#include "dns_server.h"

/*mqtt*/
#include "my_mqtt.h"

/*二维码解码*/
#include "esp_code_scanner.h"

#define qr_width 480
#define qr_height 320
extern lv_obj_t *ui_LabMsgqr;
/*二维码解码*/
static uint8_t *qr_frame_buf1 = NULL;
static uint8_t *qr_frame_buf2 = NULL;
static PingPongBuffer_t *qr_ppbuffer_handle = NULL;
static bool if_qr_ppbuffer_init = false;

static const char *TAG = "uvc_camera_lcd_demo";
/****************** configure the example working mode *******************************/
// extern lv_obj_t *ui_Label1;
#define DEMO_UVC_XFER_BUFFER_SIZE (88 * 1024) // Double buffer
#define DEMO_KEY_RESOLUTION "resolution"
#define DEMO_SWITCH_BUTTON_IO 0

#define BIT0_FRAME_START (0x01 << 0)
static EventGroupHandle_t s_evt_handle;

typedef struct
{
    uint16_t width;
    uint16_t height;
} camera_frame_size_t;

typedef struct
{
    camera_frame_size_t camera_frame_size;
    uvc_frame_size_t *camera_frame_list;
    size_t camera_frame_list_num;
    size_t camera_currect_frame_index;
} camera_resolution_info_t;

static camera_resolution_info_t camera_resolution_info = {0};
static esp_painter_handle_t painter = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static uint8_t *rgb_frame_buf1 = NULL;
static uint8_t *rgb_frame_buf2 = NULL;
static uint8_t *cur_frame_buf = NULL;
static uint8_t *jpg_frame_buf1 = NULL;
static uint8_t *jpg_frame_buf2 = NULL;
static uint8_t *xfer_buffer_a = NULL;
static uint8_t *xfer_buffer_b = NULL;
static uint8_t *frame_buffer = NULL;
static PingPongBuffer_t *ppbuffer_handle = NULL;
static uint16_t current_width = 0;
static uint16_t current_height = 0;
static bool if_ppbuffer_init = false;
extern lv_obj_t *ui_Image1;
int x = 0;
bool is_startScanQr = false;
lv_img_dsc_t camera_img = {
    .header.always_zero = 0,
    .header.w = 480,                   // 图像宽度
    .header.h = 320,                   // 图像高度
    .data_size = 480 * 320 * 2,        // 图像数据的总大小，rgb565格式
    .header.cf = LV_IMG_CF_TRUE_COLOR, // 图像颜色格式，这里假设是真彩色
    .data = NULL                       // 图像数据的指针，在后续可以指向实际的图像数据
};

static int esp_jpeg_decoder_one_picture(uint8_t *input_buf, int len, uint8_t *output_buf)
{
    esp_err_t ret = ESP_OK;
    // Generate default configuration
    jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();

    // Empty handle to jpeg_decoder
    jpeg_dec_handle_t jpeg_dec = NULL;

    // Create jpeg_dec
    jpeg_dec = jpeg_dec_open(&config);

    // Create io_callback handle
    jpeg_dec_io_t *jpeg_io = calloc(1, sizeof(jpeg_dec_io_t));
    if (jpeg_io == NULL)
    {
        return ESP_FAIL;
    }

    // Create out_info handle
    jpeg_dec_header_info_t *out_info = calloc(1, sizeof(jpeg_dec_header_info_t));
    if (out_info == NULL)
    {
        return ESP_FAIL;
    }
    // Set input buffer and buffer len to io_callback
    jpeg_io->inbuf = input_buf;
    jpeg_io->inbuf_len = len;

    // Parse jpeg picture header and get picture for user and decoder
    ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
    if (ret < 0)
    {
        goto _exit;
    }

    jpeg_io->outbuf = output_buf;
    int inbuf_consumed = jpeg_io->inbuf_len - jpeg_io->inbuf_remain;
    jpeg_io->inbuf = input_buf + inbuf_consumed;
    jpeg_io->inbuf_len = jpeg_io->inbuf_remain;

    // Start decode jpeg raw data
    ret = jpeg_dec_process(jpeg_dec, jpeg_io);
    if (ret < 0)
    {
        goto _exit;
    }

_exit:
    // Decoder deinitialize
    jpeg_dec_close(jpeg_dec);
    free(out_info);
    free(jpeg_io);
    return ret;
}

/*二维码： 对jpeg解码成大端565*/
static int qr_esp_jpeg_decoder_one_picture(uint8_t *input_buf, int len, uint8_t *output_buf)
{
    esp_err_t ret = ESP_OK;
    // Generate default configuration
    jpeg_dec_config_t config = qr_DEFAULT_JPEG_DEC_CONFIG();

    // Empty handle to jpeg_decoder
    jpeg_dec_handle_t jpeg_dec = NULL;

    // Create jpeg_dec
    jpeg_dec = jpeg_dec_open(&config);

    // Create io_callback handle
    jpeg_dec_io_t *jpeg_io = calloc(1, sizeof(jpeg_dec_io_t));
    if (jpeg_io == NULL)
    {
        return ESP_FAIL;
    }

    // Create out_info handle
    jpeg_dec_header_info_t *out_info = calloc(1, sizeof(jpeg_dec_header_info_t));
    if (out_info == NULL)
    {
        return ESP_FAIL;
    }
    // Set input buffer and buffer len to io_callback
    jpeg_io->inbuf = input_buf;
    jpeg_io->inbuf_len = len;

    // Parse jpeg picture header and get picture for user and decoder
    ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
    if (ret < 0)
    {
        goto _exit;
    }

    jpeg_io->outbuf = output_buf;
    int inbuf_consumed = jpeg_io->inbuf_len - jpeg_io->inbuf_remain;
    jpeg_io->inbuf = input_buf + inbuf_consumed;
    jpeg_io->inbuf_len = jpeg_io->inbuf_remain;

    // Start decode jpeg raw data
    ret = jpeg_dec_process(jpeg_dec, jpeg_io);
    if (ret < 0)
    {
        goto _exit;
    }

_exit:
    // Decoder deinitialize
    jpeg_dec_close(jpeg_dec);
    free(out_info);
    free(jpeg_io);
    return ret;
}

/***************************/

static void adaptive_jpg_frame_buffer(size_t length)
{
    if (jpg_frame_buf1 != NULL)
    {
        free(jpg_frame_buf1);
    }

    if (jpg_frame_buf2 != NULL)
    {
        free(jpg_frame_buf2);
    }

    jpg_frame_buf1 = (uint8_t *)heap_caps_aligned_alloc(16, length, MALLOC_CAP_SPIRAM);
    assert(jpg_frame_buf1 != NULL);
    jpg_frame_buf2 = (uint8_t *)heap_caps_aligned_alloc(16, length, MALLOC_CAP_SPIRAM);
    assert(jpg_frame_buf2 != NULL);
    ESP_ERROR_CHECK(ppbuffer_create(ppbuffer_handle, jpg_frame_buf2, jpg_frame_buf1));
    if_ppbuffer_init = true;

    if (qr_frame_buf1 != NULL)
    {
        free(qr_frame_buf1);
    }

    if (qr_frame_buf2 != NULL)
    {
        free(qr_frame_buf2);
    }

    qr_frame_buf1 = (uint8_t *)heap_caps_aligned_alloc(16, length, MALLOC_CAP_SPIRAM);
    assert(qr_frame_buf1 != NULL);
    qr_frame_buf2 = (uint8_t *)heap_caps_aligned_alloc(16, length, MALLOC_CAP_SPIRAM);
    assert(qr_frame_buf2 != NULL);
    ESP_ERROR_CHECK(ppbuffer_create(qr_ppbuffer_handle, qr_frame_buf2, qr_frame_buf1));
    if_qr_ppbuffer_init = true;
}
int ad = 0;
static int decoded_num = 0;
static void camera_frame_cb(uvc_frame_t *frame, void *ptr)
{

    if (is_startScanQr && !decoded_num)
    {
        sd_write_jpg(frame->data, frame->data_bytes);
    }
    // ESP_LOGI(TAG, "camera_frame_cb frame: len =");
    if (current_width != frame->width || current_height != frame->height)
    {
        current_width = frame->width;
        current_height = frame->height;
        adaptive_jpg_frame_buffer(current_width * current_height * 2);
        if (rgb_frame_buf1 != NULL)
        {
            memset(rgb_frame_buf1, 0, BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
        }
        if (rgb_frame_buf2 != NULL)
        {
            memset(rgb_frame_buf2, 0, BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
        }
    }

    static void *jpeg_buffer = NULL;
    static void *qr_buffer = NULL;

    /*二维码*/
    ppbuffer_get_write_buf(qr_ppbuffer_handle, &qr_buffer);
    qr_esp_jpeg_decoder_one_picture((uint8_t *)frame->data, frame->data_bytes, qr_buffer);

    ppbuffer_set_write_done(qr_ppbuffer_handle);

    /********************************/

    /* A buffer equal to the screen size can be directly written into the LCD buffer to improve the frame rate */
    if (current_width == BSP_LCD_H_RES && current_height <= BSP_LCD_V_RES)
    {
        size_t length = (BSP_LCD_V_RES - current_height) * BSP_LCD_V_RES;
        esp_jpeg_decoder_one_picture((uint8_t *)frame->data, frame->data_bytes, cur_frame_buf + length);
    }
    else
    {
        ppbuffer_get_write_buf(ppbuffer_handle, &jpeg_buffer);
        assert(jpeg_buffer != NULL);
        assert(qr_buffer != NULL);
        esp_jpeg_decoder_one_picture((uint8_t *)frame->data, frame->data_bytes, jpeg_buffer);
    }
    ppbuffer_set_write_done(ppbuffer_handle);

    vTaskDelay(pdMS_TO_TICKS(1));
}

static void _display_task(void *arg)
{
    uint16_t *lcd_buffer = NULL;  // LCD 缓冲区指针
    uint16_t *QR_buffer = NULL;   // LCD 缓冲区指针
    int64_t count_start_time = 0; // 记录帧计数开始时间
    int frame_count = 0;          // 帧计数
    int fps = 0;                  // 帧率
    int x_start = 0;              // 在LCD上绘制图像的起始X坐标
    int y_start = 0;              // 在LCD上绘制图像的起始Y坐标
    int width = 0;                // 绘制的图像宽度
    int height = 0;               // 绘制的图像高度
    int x_jump = 0;               // X坐标跳跃值
    int y_jump = 0;               // Y坐标跳跃值

    // 等待帧缓冲区初始化完成
    while (!if_ppbuffer_init)
    {
        vTaskDelay(1);
    }
    while (1)
    {
        // 获取读取缓冲区的指针
        if (ppbuffer_get_read_buf(ppbuffer_handle, (void *)&lcd_buffer) == ESP_OK)
        {

            // 根据当前图像分辨率调整在LCD上的绘制位置和方式
            if (current_width == BSP_LCD_H_RES && current_height <= BSP_LCD_V_RES)
            {
                x_start = 0;
                y_start = (BSP_LCD_V_RES - current_height) / 2;
                // esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, current_width, current_height, cur_frame_buf);
            }
            else if (current_width < BSP_LCD_H_RES && current_height < BSP_LCD_V_RES)
            {
                // x_start = (BSP_LCD_H_RES - current_width) / 2;
                // y_start = (BSP_LCD_V_RES - current_height) / 2;
                // esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 1, 1, cur_frame_buf);
                // esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_start + current_width, y_start + current_height, lcd_buffer);
                x_start = 100;
                y_start = 300;
                //  esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 1, 1, cur_frame_buf);
                //  esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_start + current_width, y_start + current_height, lcd_buffer);
                // ESP_LOGI(TAG, "current_width = %d, current_height = %d BSP_LCD_H_RES=%d BSP_LCD_V_RES=%d ", current_width, current_height, BSP_LCD_H_RES, BSP_LCD_V_RES);
                camera_img.data = lcd_buffer;
                if (is_startScanQr)
                {
                    bsp_display_lock(0);
                    //  lv_img_set_src(ui_Img, &camera_img);
                    lv_img_set_src(ui_Image1, &camera_img);
                    bsp_display_unlock();
                    /*二维码解码*/
                    ppbuffer_get_read_buf(qr_ppbuffer_handle, (void *)&QR_buffer);
                    esp_image_scanner_t *esp_scn = esp_code_scanner_create();
                    esp_code_scanner_config_t config = {ESP_CODE_SCANNER_MODE_FAST, ESP_CODE_SCANNER_IMAGE_RGB565, qr_width, qr_height};
                    esp_code_scanner_set_config(esp_scn, config);
                    decoded_num = esp_code_scanner_scan_image(esp_scn, QR_buffer);
                    // time_t now = 0;
                    // time(&now);
                    // 将时区设置为东八区
                    // setenv("TZ", "CST-8", 1);
                    // tzset();
                    // struct tm timeinfo = {0};
                    // localtime_r(&now, &timeinfo);
                    // 格式化当前时间并将其添加到文件名中

                    if (decoded_num) // 如果识别到了一个二维码
                    {
                        esp_code_scanner_symbol_t result = esp_code_scanner_result(esp_scn);
                        ESP_LOGI("decode", "Decoded %s symbol \"%s\"\n", result.type_name, result.data);
                        bsp_display_lock(0);
                        lv_label_set_text(ui_LabMsgqr, result.data);
                        send_mqtt_message(client, "/topic/qos0", result.data);
                        bsp_display_unlock();
                    }
                    else
                    {
                        ESP_LOGI("decode", "error");
                        // remove(fname);
                    }
                    esp_code_scanner_destroy(esp_scn);
                }
                /*********************************************/
            }
            else
            {
                // 处理分辨率大于LCD的情况（当前未启用）
                if (current_width < BSP_LCD_H_RES)
                {
                    width = current_width;
                    x_start = (BSP_LCD_H_RES - current_width) / 2;
                    x_jump = 0;
                }
                else
                {
                    width = BSP_LCD_H_RES;
                    x_start = 0;
                    x_jump = (current_width - BSP_LCD_H_RES) / 2;
                }

                if (current_height < BSP_LCD_V_RES)
                {
                    height = current_height;
                    y_start = (BSP_LCD_V_RES - current_height) / 2;
                    y_jump = 0;
                }
                else
                {
                    height = BSP_LCD_V_RES;
                    y_start = 0;
                    y_jump = (current_height - BSP_LCD_V_RES) / 2;
                }

                // esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 1, 1, cur_frame_buf);
                for (int i = y_start; i < height; i++)
                {
                    // esp_lcd_panel_draw_bitmap(panel_handle, x_start, i, x_start + width, i + 1, &lcd_buffer[(y_jump + i) * current_width + x_jump]);
                }
            }
            // 设置读取缓冲区已处理完毕
            ppbuffer_set_read_done(ppbuffer_handle);
            /*qr缓冲区处理完成*/
            ppbuffer_set_read_done(qr_ppbuffer_handle);
            /***************/
            // 计算帧率
            if (count_start_time == 0)
            {
                count_start_time = esp_timer_get_time();
            }
            if (++frame_count == 20)
            {
                frame_count = 0;
                fps = 20 * 1000000 / (esp_timer_get_time() - count_start_time);
                count_start_time = esp_timer_get_time();
                // ESP_LOGI(TAG, "camera fps: %d %d*%d", fps, current_width, current_height);
                //  char charValue2 = static_cast<char>(fps);
                char fps_str[15]; // 假设fps的最大位数为10
                snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
            }
            // 在LCD上绘制帧率信息
            // esp_painter_draw_string_format(painter, x_start, y_start, NULL, COLOR_BRUSH_DEFAULT, "FPS: %d %d*%d", fps, current_width, current_height);
            // 切换当前帧缓冲区
            cur_frame_buf = (cur_frame_buf == rgb_frame_buf1) ? rgb_frame_buf2 : rgb_frame_buf1;
        }
        vTaskDelay(10);
    }
}

static esp_err_t _display_init(void)
{
    bsp_display_config_t disp_config = {0};
    bsp_i2c_init();

    bsp_display_start_with_config(&panel_handle);
    lv_fs_if_init();
    ESP_LOGI(TAG, "Display LVGL demo");
    bsp_display_lock(0);
    // lv_demo_widgets();      /* A widgets example */
    // lv_demo_music(); /* A modern, smartphone-like music player demo. */
    ui_init();
    // lv_demo_stress();       /* A stress test for LVGL. */
    // lv_demo_benchmark();    /* A demo to measure the performance of LVGL or to compare different settings. */
    bsp_display_unlock();

    // bsp_display_config_t disp_config = {0};
    //  bsp_display_new(&disp_config, &panel_handle, NULL);
    //  assert(panel_handle);
    //  esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, (void **)&rgb_frame_buf1, (void **)&rgb_frame_buf2);
    cur_frame_buf = rgb_frame_buf2;

    // esp_painter_config_t painter_config = {
    //     .brush.color = COLOR_RGB565_RED,
    //     .canvas = {
    //         .x = 0,
    //         .y = 0,
    //         .width = BSP_LCD_H_RES,
    //         .height = BSP_LCD_V_RES,
    //     },
    //     .default_font = &esp_painter_basic_font_24,
    //     .piexl_color_byte = 2,
    //     .lcd_panel = panel_handle,
    // };
    // ESP_ERROR_CHECK(esp_painter_new(&painter_config, &painter));
    xTaskCreate(_display_task, "_display", 4 * 1024, NULL, 5, NULL);
    return ESP_OK;
}

static void _get_value_from_nvs(char *key, void *value, size_t *size)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("memory", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        err = nvs_get_blob(my_handle, key, value, size);
        switch (err)
        {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "%s is not initialized yet!", key);
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}

static esp_err_t _set_value_to_nvs(char *key, void *value, size_t size)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("memory", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        err = nvs_set_blob(my_handle, key, value, size);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "NVS set failed %s", esp_err_to_name(err));
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "NVS commit failed");
        }

        nvs_close(my_handle);
    }

    return err;
}

static esp_err_t _usb_stream_init(void)
{
    uvc_config_t uvc_config = {
        .frame_interval = FRAME_INTERVAL_FPS_30,
        .xfer_buffer_size = DEMO_UVC_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_UVC_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = &camera_frame_cb,
        .frame_cb_arg = NULL,
        .frame_width = FRAME_RESOLUTION_ANY,
        .frame_height = FRAME_RESOLUTION_ANY,
        .flags = FLAG_UVC_SUSPEND_AFTER_START,
    };

    esp_err_t ret = uvc_streaming_config(&uvc_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uvc streaming config failed");
    }
    return ret;
}

static size_t _find_current_resolution(camera_frame_size_t *camera_frame_size)
{
    if (camera_resolution_info.camera_frame_list == NULL)
    {
        return -1;
    }

    size_t i = 0;
    while (i < camera_resolution_info.camera_frame_list_num)
    {
        if (camera_frame_size->width >= camera_resolution_info.camera_frame_list[i].width && camera_frame_size->height >= camera_resolution_info.camera_frame_list[i].height)
        {
            /* Find next resolution
               If current resolution is the min resolution, switch to the max resolution*/
            camera_frame_size->width = camera_resolution_info.camera_frame_list[i].width;
            camera_frame_size->height = camera_resolution_info.camera_frame_list[i].height;
            break;
        }
        else if (i == camera_resolution_info.camera_frame_list_num - 1)
        {
            camera_frame_size->width = camera_resolution_info.camera_frame_list[i].width;
            camera_frame_size->height = camera_resolution_info.camera_frame_list[i].height;
            break;
        }
        i++;
    }
    ESP_LOGI(TAG, "Current resolution is %dx%d", camera_frame_size->width, camera_frame_size->height);
    return i;
}

// 当切换按钮按下时的回调函数
static void switch_button_press_down_cb(void *arg, void *data)
{
    // 检查摄像头分辨率信息是否为空或者等待位掩码BIT0_FRAME_START是否在10毫秒内被置位
    if (camera_resolution_info.camera_frame_list == NULL || xEventGroupWaitBits(s_evt_handle, BIT0_FRAME_START, false, false, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        return; // 如果条件不满足，则直接返回
    }

    // 打印旧的分辨率信息
    ESP_LOGI(TAG, "旧分辨率为 %d*%d", camera_resolution_info.camera_frame_size.width, camera_resolution_info.camera_frame_size.height);

    // 增加当前分辨率索引，如果索引超过列表长度，则重置为0
    if (++camera_resolution_info.camera_currect_frame_index >= camera_resolution_info.camera_frame_list_num)
    {
        camera_resolution_info.camera_currect_frame_index = 0;
    }

    // 更新摄像头分辨率为当前索引对应的分辨率
    camera_resolution_info.camera_frame_size.width = camera_resolution_info.camera_frame_list[camera_resolution_info.camera_currect_frame_index].width;
    camera_resolution_info.camera_frame_size.height = camera_resolution_info.camera_frame_list[camera_resolution_info.camera_currect_frame_index].height;

    // 打印新的分辨率信息
    ESP_LOGI(TAG, "当前分辨率为 %d*%d", camera_resolution_info.camera_frame_size.width, camera_resolution_info.camera_frame_size.height);

    /* 将新的摄像头分辨率保存到非易失性存储器 (NVS) */
    usb_streaming_control(STREAM_UVC, CTRL_SUSPEND, NULL); // 暂停USB视频流
    ESP_ERROR_CHECK(uvc_frame_size_reset(camera_resolution_info.camera_frame_size.width,
                                         camera_resolution_info.camera_frame_size.height,
                                         FPS2INTERVAL(30)));                                                                         // 重置UVC帧大小和帧率
    ESP_ERROR_CHECK(_set_value_to_nvs(DEMO_KEY_RESOLUTION, &camera_resolution_info.camera_frame_size, sizeof(camera_frame_size_t))); // 将分辨率信息写入NVS
    esp_lcd_rgb_panel_restart(panel_handle);                                                                                         // 重新启动LCD RGB面板
    usb_streaming_control(STREAM_UVC, CTRL_RESUME, NULL);                                                                            // 恢复USB视频流
}

static esp_err_t _switch_button_init(void)
{
    button_config_t button_config = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = DEMO_SWITCH_BUTTON_IO,
            .active_level = 0,
        },
    };

    button_handle_t button_handle = iot_button_create(&button_config);
    assert(button_handle != NULL);
    esp_err_t ret = iot_button_register_cb(button_handle, BUTTON_PRESS_DOWN, switch_button_press_down_cb, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "button register callback fail");
    }
    return ret;
}

// USB视频流状态变化回调函数
static void _stream_state_changed_cb(usb_stream_state_t event, void *arg)
{
    ESP_LOGI(TAG, "_stream_state_changed_cb %d", event);
    switch (event)
    {
    // USB视频流已连接
    case STREAM_CONNECTED:
    {
        /* 从NVS获取摄像头分辨率 */
        size_t size = sizeof(camera_frame_size_t);
        _get_value_from_nvs(DEMO_KEY_RESOLUTION, &camera_resolution_info.camera_frame_size, &size);
        size_t frame_index = 0;

        // 获取UVC帧大小列表
        uvc_frame_size_list_get(NULL, &camera_resolution_info.camera_frame_list_num, NULL);

        if (camera_resolution_info.camera_frame_list_num)
        {
            ESP_LOGI(TAG, "UVC: 获取帧列表大小 = %u，当前 = %u", camera_resolution_info.camera_frame_list_num, frame_index);

            // 分配内存用于帧列表
            uvc_frame_size_t *_frame_list = (uvc_frame_size_t *)malloc(camera_resolution_info.camera_frame_list_num * sizeof(uvc_frame_size_t));

            // 重新分配摄像头分辨率信息的内存
            camera_resolution_info.camera_frame_list = (uvc_frame_size_t *)realloc(camera_resolution_info.camera_frame_list, camera_resolution_info.camera_frame_list_num * sizeof(uvc_frame_size_t));

            if (NULL == camera_resolution_info.camera_frame_list)
            {
                ESP_LOGE(TAG, "camera_resolution_info.camera_frame_list 内存分配失败");
            }

            // 获取UVC帧大小列表
            uvc_frame_size_list_get(_frame_list, NULL, NULL);

            for (size_t i = 0; i < camera_resolution_info.camera_frame_list_num; i++)
            {
                // 检查帧大小是否符合LCD分辨率
                if (_frame_list[i].width <= BSP_LCD_H_RES && _frame_list[i].height <= BSP_LCD_V_RES)
                {
                    camera_resolution_info.camera_frame_list[frame_index++] = _frame_list[i];
                    ESP_LOGI(TAG, "\t选择帧[%u] = %ux%u", i, _frame_list[i].width, _frame_list[i].height);
                }
                else
                {
                    ESP_LOGI(TAG, "\t丢弃帧[%u] = %ux%u", i, _frame_list[i].width, _frame_list[i].height);
                }
            }

            camera_resolution_info.camera_frame_list_num = frame_index;

            // 查找并设置当前分辨率的索引
            if (camera_resolution_info.camera_frame_size.width != 0 && camera_resolution_info.camera_frame_size.height != 0)
            {
                camera_resolution_info.camera_currect_frame_index = _find_current_resolution(&camera_resolution_info.camera_frame_size);
            }
            else
            {
                camera_resolution_info.camera_currect_frame_index = 0;
            }

            // 检查当前分辨率索引是否有效
            if (-1 == camera_resolution_info.camera_currect_frame_index)
            {
                ESP_LOGE(TAG, "查找当前分辨率失败");
                break;
            }

            // 重置UVC帧大小为当前分辨率
            ESP_ERROR_CHECK(uvc_frame_size_reset(camera_resolution_info.camera_frame_list[camera_resolution_info.camera_currect_frame_index].width,
                                                 camera_resolution_info.camera_frame_list[camera_resolution_info.camera_currect_frame_index].height, FPS2INTERVAL(30)));

            // 更新NVS中的摄像头分辨率信息
            camera_frame_size_t camera_frame_size = {
                .width = camera_resolution_info.camera_frame_list[camera_resolution_info.camera_currect_frame_index].width,
                .height = camera_resolution_info.camera_frame_list[camera_resolution_info.camera_currect_frame_index].height,
            };
            ESP_ERROR_CHECK(_set_value_to_nvs(DEMO_KEY_RESOLUTION, &camera_frame_size, sizeof(camera_frame_size_t)));

            // 释放帧列表的内存
            if (_frame_list != NULL)
            {
                free(_frame_list);
            }

            /* 等待USB摄像头连接 */
            usb_streaming_control(STREAM_UVC, CTRL_RESUME, NULL);
            xEventGroupSetBits(s_evt_handle, BIT0_FRAME_START);
        }
        else
        {
            ESP_LOGW(TAG, "UVC: 获取帧列表大小 = %u", camera_resolution_info.camera_frame_list_num);
        }
        ESP_LOGI(TAG, "设备已连接");
        break;
    }
    // USB视频流已断开连接
    case STREAM_DISCONNECTED:
        xEventGroupClearBits(s_evt_handle, BIT0_FRAME_START);
        ESP_LOGI(TAG, "设备已断开连接");
        break;
    // 未知事件
    default:
        ESP_LOGE(TAG, "未知事件");
        break;
    }
}
// 初始化wifi
void ConnectWifi()
{
    // NvsWriteDataToFlash("", "RJYI1818", "938250Rjyi");
    char WIFI_Name[50] = {0};
    char WIFI_PassWord[50] = {0}; /*读取保存的WIFI信息*/
    if (NvsReadDataFromFlash("WIFI Config", WIFI_Name, WIFI_PassWord) == 0x00)
    {
        printf("WIFI SSID     :%s\r\n", WIFI_Name);
        printf("WIFI PASSWORD :%s\r\n", WIFI_PassWord);
        printf("开始初始化WIFI Station 模式\r\n");
        wifi_init_sta(WIFI_Name, WIFI_PassWord); /*按照读取的信息初始化WIFI Station模式*/
        dns_server_start();                      // 开启DNS服务
        web_server_start();                      // 开启http服务
    }
    else
    {
        printf("未读取到WIFI配置信息\r\n");
        printf("开始初始化WIFI AP 模式\r\n");
        WIFI_AP_Init(); /*上电后配置WIFI为AP模式*/
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        dns_server_start(); // 开启DNS服务
        web_server_start(); // 开启http服务
    }
}
void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        /* NVS partition was truncated and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());
        /* Retry nvs_flash_init */
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ConnectWifi();
    /* Create event group */
    s_evt_handle = xEventGroupCreate();
    if (s_evt_handle == NULL)
    {
        ESP_LOGE(TAG, "line-%u event group create failed", __LINE__);
        assert(0);
    }

    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    xfer_buffer_a = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(xfer_buffer_a != NULL);
    xfer_buffer_b = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    frame_buffer = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(frame_buffer != NULL);

    /* malloc frame buffer for ppbuffer_handle*/
    ppbuffer_handle = (PingPongBuffer_t *)malloc(sizeof(PingPongBuffer_t));
    /* malloc frame buffer for ppbuffer_handle*/
    qr_ppbuffer_handle = (PingPongBuffer_t *)malloc(sizeof(PingPongBuffer_t));

    /* Initialize the screen */
    ESP_ERROR_CHECK(_display_init());

    // /* Initialize the button to switch resolution */
    ESP_ERROR_CHECK(_switch_button_init());

    // /* Initialize the screen according to the resolution stored in nvs */
    ESP_ERROR_CHECK(_usb_stream_init());

    // /* Register a callback to control uvc frame size */
    ESP_ERROR_CHECK(usb_streaming_state_register(&_stream_state_changed_cb, NULL));

    // /* Start stream with pre-configs, usb stream driver will create multi-tasks internal
    // to handle usb data from different pipes, and user's callback will be called after new frame ready. */
    ESP_ERROR_CHECK(usb_streaming_start());
    ESP_ERROR_CHECK(usb_streaming_connect_wait(portMAX_DELAY));
}
