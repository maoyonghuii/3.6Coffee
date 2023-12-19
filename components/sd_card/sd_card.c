/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include "sd_card.h"
#include <dirent.h>
const char *gifFiles[100];
static const char *TAG = "FILE";
void list_files(const char *path)
{
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(path)) != NULL)
    {
        printf("当前目录：%s\n", path);
        int i = 0;
        while ((ent = readdir(dir)) != NULL)
        {
            ESP_LOGI(TAG, "ent->d_type :%d", ent->d_type);
            // 判断条目是否为目录
            if (ent->d_type == DT_DIR)
            {
                printf("文件夹：%s\n", ent->d_name);
                gifFiles[i] = strdup(ent->d_name); // 为文件名分配内存
                i++;
                // 打印文件夹名
            }
            else
            {
                // 打印文件夹名
                printf("文件名：%s\n", ent->d_name);
            }
        }
        closedir(dir);
    }
    else
    {
        perror("Unable to open directory");
    }
}

void sdCard_Init(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 1;
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;

    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);
    list_files("/sdcard/gif");
}

void sd_write_jpg(const char *_jpg_buf, int _jpg_buf_len)
{
    // 获取系统时间戳
    time_t now = 0;
    time(&now);
    setenv("TZ", "CST-8", 1);
    tzset();
    struct tm timeinfo = {0};
    // 结合设置的时区，转换为tm结构体
    localtime_r(&now, &timeinfo);
    char fname[64];
    strftime(fname, sizeof(fname), "/sdcard/pictures/%Y%m%d%H%M%S.jpg", &timeinfo);
    ESP_LOGI(TAG, "fname:%s", fname);
    // 以二进制写入模式打开文件
    FILE *f = fopen(fname, "w");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    // 写入数据
    fwrite(_jpg_buf, _jpg_buf_len, 1, f);
    fclose(f);
    ESP_LOGI(TAG, "File written");
}