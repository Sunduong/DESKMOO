/**
 * @file DESKMOO_UI_gen.h
 */

#ifndef LVGL_PRO_DESKMOO_UI_GEN_H
#define LVGL_PRO_DESKMOO_UI_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#if defined(LV_USE_XML) && LV_USE_XML
    #include "lv_xml/lv_xml.h"
#endif



/* Prototypes for target functions, needed by responsive const definitions */

void DESKMOO_UI_set_target(uint32_t target);
uint32_t DESKMOO_UI_get_target(void);
bool DESKMOO_UI_check_target(uint32_t target);

/*********************
 *      DEFINES
 *********************/

#define DESKMOO_UI_TARGET_UNDEFINED  (0 << 1)
#define DESKMOO_UI_TARGET_TARGET1    (1 << 1)
#define DESKMOO_UI_TARGET_ALL        0x0FFFFFFF

/* By default compile for all targets, allowing to switch to any targets at runtime */
#ifndef DESKMOO_UI_COMPILE_TARGET
#define DESKMOO_UI_COMPILE_TARGET DESKMOO_UI_TARGET_ALL
#endif

#define DESKMOO_UI_CHECK_COMPILE_TARGET(target) (DESKMOO_UI_COMPILE_TARGET & (target) ? 1 : 0)

#ifndef LV_XML_EVAL_STRING_BUF_SIZE
    #define LV_XML_EVAL_STRING_BUF_SIZE 256
#endif

/**
 * Animation path of a style transition with a `bezier(…)` easing. The control
 * points come from the transition descriptor's `user_data`, as a transition
 * descriptor cannot store animation parameters itself.
 */
static inline int32_t DESKMOO_UI_style_transition_bezier_path_helper(const lv_anim_t * a)
{
    const lv_anim_bezier3_para_t * para = (const lv_anim_bezier3_para_t *)lv_anim_get_user_data(a);
    if(para == NULL) return lv_anim_path_linear(a);

    lv_anim_t a_tmp = *a;
    lv_anim_set_bezier3_param(&a_tmp, para->x1, para->y1, para->x2, para->y2);
    return lv_anim_path_custom_bezier3(&a_tmp);
}

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

/*----------------
 * Global styles
 *----------------*/

/*----------------
 * Fonts
 *----------------*/




/*----------------
 * Images
 *----------------*/

/* Targets: any */
extern const void * nav_default;
extern const void * home_focused;
extern const void * home_selected;
extern const void * home_focused_selected;
extern const void * stats_focused;
extern const void * stats_selected;
extern const void * stats_focused_selected;
extern const void * settings_focused;
extern const void * settings_selected;
extern const void * settings_focused_selected;

/*----------------
 * Subjects
 *----------------*/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

/*----------------
 * Easing Callbacks
 *----------------*/

/**
 * Initialize the component library
 */

void DESKMOO_UI_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widgets, components and screens of this library*/
#include "screens/screen_home_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_PRO_DESKMOO_UI_GEN_H*/