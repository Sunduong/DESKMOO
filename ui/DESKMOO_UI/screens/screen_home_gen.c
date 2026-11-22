/**
 * @file screen_home_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_home_gen.h"
#include "../DESKMOO_UI.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * screen_home_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if DESKMOO_UI_CHECK_COMPILE_TARGET(DESKMOO_UI_TARGET_ALL)
    if (DESKMOO_UI_check_target(DESKMOO_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_home_#");
        lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x173c19), 0);

        lv_obj_t * lv_label_0 = lv_label_create(lv_obj_0);
        lv_label_set_text(lv_label_0, "DESKMOO");
        lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
        lv_obj_set_y(lv_label_0, 30);
        lv_obj_set_style_text_color(lv_label_0, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * lv_image_0 = lv_image_create(lv_obj_0);
        lv_image_set_src(lv_image_0, nav_default);
        lv_obj_set_align(lv_image_0, LV_ALIGN_BOTTOM_MID);

        lv_obj_t * btn_home = lv_button_create(lv_obj_0);
        lv_obj_set_name(btn_home, "btn_home");
        lv_obj_set_align(btn_home, LV_ALIGN_BOTTOM_LEFT);
        lv_obj_set_x(btn_home, 0);
        lv_obj_set_y(btn_home, 0);
        lv_obj_set_width(btn_home, 159);
        lv_obj_set_height(btn_home, 34);
        lv_obj_set_style_bg_opa(btn_home, 0, 0);
        lv_obj_set_style_border_width(btn_home, 0, 0);
        lv_obj_set_style_shadow_width(btn_home, 0, 0);
        lv_obj_set_style_outline_width(btn_home, 0, 0);
        lv_obj_set_style_radius(btn_home, 0, 0);
        lv_obj_set_style_pad_all(btn_home, 0, 0);
        lv_obj_set_style_bg_image_src(btn_home, home_focused, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_image_src(btn_home, home_selected, LV_STATE_CHECKED);
        lv_obj_set_style_bg_image_src(btn_home, home_focused_selected, LV_STATE_CHECKED | LV_STATE_FOCUSED);

        lv_obj_t * btn_stats = lv_button_create(lv_obj_0);
        lv_obj_set_name(btn_stats, "btn_stats");
        lv_obj_set_align(btn_stats, LV_ALIGN_BOTTOM_LEFT);
        lv_obj_set_x(btn_stats, 159);
        lv_obj_set_y(btn_stats, 0);
        lv_obj_set_width(btn_stats, 164);
        lv_obj_set_height(btn_stats, 34);
        lv_obj_set_style_bg_opa(btn_stats, 0, 0);
        lv_obj_set_style_border_width(btn_stats, 0, 0);
        lv_obj_set_style_shadow_width(btn_stats, 0, 0);
        lv_obj_set_style_outline_width(btn_stats, 0, 0);
        lv_obj_set_style_radius(btn_stats, 0, 0);
        lv_obj_set_style_pad_all(btn_stats, 0, 0);
        lv_obj_set_style_bg_image_src(btn_stats, stats_focused, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_image_src(btn_stats, stats_selected, LV_STATE_CHECKED);
        lv_obj_set_style_bg_image_src(btn_stats, stats_focused_selected, LV_STATE_CHECKED | LV_STATE_FOCUSED);

        lv_obj_t * btn_settings = lv_button_create(lv_obj_0);
        lv_obj_set_name(btn_settings, "btn_settings");
        lv_obj_set_align(btn_settings, LV_ALIGN_BOTTOM_LEFT);
        lv_obj_set_x(btn_settings, 323);
        lv_obj_set_y(btn_settings, 0);
        lv_obj_set_width(btn_settings, 157);
        lv_obj_set_height(btn_settings, 34);
        lv_obj_set_style_bg_opa(btn_settings, 0, 0);
        lv_obj_set_style_border_width(btn_settings, 0, 0);
        lv_obj_set_style_shadow_width(btn_settings, 0, 0);
        lv_obj_set_style_outline_width(btn_settings, 0, 0);
        lv_obj_set_style_radius(btn_settings, 0, 0);
        lv_obj_set_style_pad_all(btn_settings, 0, 0);
        lv_obj_set_style_bg_image_src(btn_settings, settings_focused, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_image_src(btn_settings, settings_selected, LV_STATE_CHECKED);
        lv_obj_set_style_bg_image_src(btn_settings, settings_focused_selected, LV_STATE_CHECKED | LV_STATE_FOCUSED);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

