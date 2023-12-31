// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.4
// LVGL version: 8.3.6
// Project name: 咖啡机3.6寸

#include "ui.h"

void ui_CLOCK_screen_init(void)
{
  ui_CLOCK = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_CLOCK, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_CLOCK, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_CLOCK, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_data = lv_label_create(ui_CLOCK);
    lv_obj_set_width(ui_data, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_data, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_data, 5);
    lv_obj_set_y(ui_data, 86);
    lv_obj_set_align(ui_data, LV_ALIGN_CENTER);
    lv_label_set_text(ui_data, "");
    lv_obj_set_style_text_font(ui_data, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label10 = lv_label_create(ui_CLOCK);
    lv_obj_set_width(ui_Label10, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label10, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label10, -111);
    lv_obj_set_y(ui_Label10, 0);
    lv_obj_set_align(ui_Label10, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label10, "");
    lv_obj_set_style_text_font(ui_Label10, &ui_font_Time, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label11 = lv_label_create(ui_CLOCK);
    lv_obj_set_width(ui_Label11, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label11, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label11, -11);
    lv_obj_set_y(ui_Label11, -23);
    lv_obj_set_align(ui_Label11, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label11, ":");
    lv_obj_set_style_text_font(ui_Label11, &ui_font_Time, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label12 = lv_label_create(ui_CLOCK);
    lv_obj_set_width(ui_Label12, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label12, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label12, 78);
    lv_obj_set_y(ui_Label12, 0);
    lv_obj_set_align(ui_Label12, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label12, "");
    lv_obj_set_style_text_font(ui_Label12, &ui_font_Time, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_CLOCK, ui_event_CLOCK, LV_EVENT_ALL, NULL);
}
