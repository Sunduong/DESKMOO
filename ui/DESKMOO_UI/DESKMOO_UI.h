/**
 * @file DESKMOO_UI.h
 */

#ifndef LVGL_PRO_DESKMOO_UI_H
#define LVGL_PRO_DESKMOO_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/

/* Target(s) select at startup: one or more DESKMOO_UI_TARGET_* values OR-ed together. */
#ifndef DESKMOO_UI_COMPILE_TARGET
#define DESKMOO_UI_COMPILE_TARGET DESKMOO_UI_TARGET_ALL
#endif

/*********************
 *      INCLUDES
 *********************/

#include "DESKMOO_UI_gen.h"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the component library
 */
void DESKMOO_UI_init(const char * asset_path);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_PRO_DESKMOO_UI_H*/