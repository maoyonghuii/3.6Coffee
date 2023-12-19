#pragma once

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
// #include "WIFI.h"
#include "time.h"
#define CONFIG_EXAMPLE_PIN_CMD 38
#define CONFIG_EXAMPLE_PIN_CLK 39
#define CONFIG_EXAMPLE_PIN_D0 40
#define SDMMC_SLOT_FLAG_INTERNAL_PULLUP  BIT(0)
extern const char *gifFiles[100];

#define MOUNT_POINT "/sdcard"


#ifdef __cplusplus
extern "C" {
#endif

void sdCard_Init(void);
void sd_write_jpg(const char *_jpg_buf, int _jpg_buf_len);





#ifdef __cplusplus
}
#endif