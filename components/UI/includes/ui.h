// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.4
// LVGL version: 8.3.6
// Project name: 咖啡机3.6寸

#ifndef _咖啡机3_6寸_UI_H
#define _咖啡机3_6寸_UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

// #include "lv_i18n.h"
#include "ui_helpers.h"
#include "ui_events.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "time.h"
#include "WIFI.h"
#include "sd_card.h"
    void showkey_Animation(lv_obj_t *TargetObject, int delay);
    void hidkey_Animation(lv_obj_t *TargetObject, int delay);
    // SCREEN: ui_Logo
    void ui_Logo_screen_init(void);
    extern lv_obj_t *ui_Logo;
    void ui_event_gif(lv_event_t *e);
    extern lv_obj_t *ui_gif;
    extern lv_obj_t *ui_gifdata;

    // SCREEN: ui_CLOCK
    void ui_CLOCK_screen_init(void);
    void ui_event_CLOCK(lv_event_t *e);
    extern lv_obj_t *ui_CLOCK;
    extern lv_obj_t *ui_data;
    extern lv_obj_t *ui_Label10;
    extern lv_obj_t *ui_Label11;
    extern lv_obj_t *ui_Label12;
    // SCREEN: ui_Touch_to_start
    void ui_Touch_to_start_screen_init(void);
    void ui_event_PasswordKey2(lv_event_t *e);
    extern lv_obj_t *ui_Touch_to_start;
    extern lv_obj_t *ui_Panel4;
    void ui_event_ButStartstart(lv_event_t *e);
    extern lv_obj_t *ui_ButStartstart;
    extern lv_obj_t *ui_Label2;
    extern lv_obj_t *ui_Image1;
    extern lv_obj_t *ui_Label4;
    void ui_event_ButStartqr(lv_event_t *e);
    extern lv_obj_t *ui_ButStartqr;
    extern lv_obj_t *ui_Label3;
    extern lv_obj_t *ui_LabMsgqr;
    extern lv_obj_t *ui_labMsgWIfiMode;
    extern lv_obj_t *ui_labWifiMode;
    extern lv_obj_t *ui_labMsgWIfIP;
    extern lv_obj_t *ui_labWifiip;
    extern lv_obj_t *ui_labMsgWIfiID;
    extern lv_obj_t *ui_labWIfiid;
    extern lv_obj_t *ui_labMsgWIfiPD;
    extern lv_obj_t *ui_labWifipd;
    void ui_event_connection_wifi(lv_event_t *e);
    extern lv_obj_t *ui_connection_wifi;
    extern lv_obj_t *ui_Label15;
    extern lv_obj_t *ui_ConnWifi2;
    extern lv_obj_t *ui_Label16;
    extern lv_obj_t *ui_WifiList2;
    extern lv_obj_t *ui_PasswordKey2;
    void ui_event_Password2(lv_event_t *e);
    extern lv_obj_t *ui_Password2;
    void ui_event_BtnConnWIfi2(lv_event_t *e);
    extern lv_obj_t *ui_BtnConnWIfi2;
    extern lv_obj_t *ui_Label13;
    extern lv_obj_t *ui_WIFI_Name2;
    void ui_event_AreaWifiName2(lv_event_t *e);
    extern lv_obj_t *ui_AreaWifiName2;
    extern lv_obj_t *ui_Label14;
    void ui_event_Keyboard3(lv_event_t *e);
    extern lv_obj_t *ui_Keyboard3;
    void ui_event_butskip2(lv_event_t *e);
    extern lv_obj_t *ui_butskip2;
    extern lv_obj_t *ui_labskip2;
    // SCREEN: ui_Insert_pod
    void ui_Insert_pod_screen_init(void);
    void ui_event_Insert_pod(lv_event_t *e);
    extern lv_obj_t *ui_Insert_pod;
    extern lv_obj_t *ui_Image8;
    extern lv_obj_t *ui_LabMsgInsert;
    // SCREEN: ui_Reinforce
    void ui_Reinforce_screen_init(void);
    extern lv_obj_t *ui_Reinforce;
    // SCREEN: ui_chose_coffee
    void ui_chose_coffee_screen_init(void);
    extern lv_obj_t *ui_chose_coffee;
    extern lv_obj_t *ui_Image5;
    extern lv_obj_t *ui_Image6;
    void ui_event_Button3(lv_event_t *e);
    extern lv_obj_t *ui_Button3;
    extern lv_obj_t *ui_LabMsgIce;
    void ui_event_Button1(lv_event_t *e);
    extern lv_obj_t *ui_Button1;
    extern lv_obj_t *ui_LabMsghot;
    // SCREEN: ui_brewing
    void ui_brewing_screen_init(void);
    extern lv_obj_t *ui_brewing;
    extern lv_obj_t *ui_Panel2;
    extern lv_obj_t *ui_Panel1;
    extern lv_obj_t *ui_Panel3;
    void ui_event_Arc1(lv_event_t *e);
    extern lv_obj_t *ui_Arc1;
    extern lv_obj_t *ui_Image2;
    // SCREEN: ui_Done
    void ui_Done_screen_init(void);
    void ui_event_Done(lv_event_t *e);
    extern lv_obj_t *ui_Done;
    extern lv_obj_t *ui_Image3;
    extern lv_obj_t *ui_Label5;
    // SCREEN: ui_Error
    void ui_Error_screen_init(void);
    void ui_event_Error(lv_event_t *e);
    extern lv_obj_t *ui_Error;
    extern lv_obj_t *ui_ErrorMsgError;
    extern lv_obj_t *ui_MsgErrorInfo;
    extern lv_obj_t *ui_ImgErrormsg;
    extern lv_obj_t *ui____initial_actions0;

    LV_IMG_DECLARE(ui_img_7aaebc9043b138a2613feee3cb95b55_png); // assets\7aaebc9043b138a2613feee3cb95b55.png
    LV_IMG_DECLARE(ui_img_insert_png);                          // assets\Insert.png
    LV_IMG_DECLARE(ui_img_256895308);                           // assets\热咖啡-圆形杯-盘子上-从侧面看.png
    LV_IMG_DECLARE(xiaoji);                                     // assets\热咖啡-圆形杯-盘子上-从侧面看.png
    LV_IMG_DECLARE(xiaoji_ALPHA);
    LV_IMG_DECLARE(huo);
    LV_IMG_DECLARE(xiaoji_raw);
    LV_FONT_DECLARE(ui_font_Time);
    LV_IMG_DECLARE(ui_img_200_200insert_png); // assets\200_200Insert.png
    LV_IMG_DECLARE(ui_img_icecoffee_png);     // assets\IceCoffee.png
    LV_IMG_DECLARE(ui_img_hotcoffee_png);     // assets\HotCoffee.png
    LV_IMG_DECLARE(ui_img_makecoffee_png);    // assets\MakeCoffee.png
    LV_IMG_DECLARE(ui_img_200x172_done_png);  // assets\MakeCoffee.png
    LV_IMG_DECLARE(ui_img_error_png);         // assets\MakeCoffee.png
    LV_IMG_DECLARE(ui_img_ice_png);           // assets\MakeCoffee.png
        LV_IMG_DECLARE(ui_img_done_100x100_png);           // assets\MakeCoffee.png
    extern bool is_startScanQr;
    extern int ClockTime;
    void ui_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
