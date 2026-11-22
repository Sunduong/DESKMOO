/**
 * @file DESKMOO_UI_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "DESKMOO_UI.h"

#if defined(LV_USE_XML) && LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void check_font(lv_font_t ** font, const char * name);

/**********************
 *  STATIC VARIABLES
 **********************/

static uint32_t DESKMOO_UI_target = DESKMOO_UI_COMPILE_TARGET;

/*----------------
 * Translations
 *----------------*/

#ifndef LV_EDITOR_PREVIEW
    static const char * translation_languages[] = {"en", "de", NULL};
    static const char * translation_tags[] = {"dog", "cat", "house", NULL};
    static const char * translation_texts[] = {
        "This is a dog", "Das ist ein Hund", /* dog */
        "A curious little cat", "Eine neugierige kleine Katze", /* cat */
        "The house is cozy and warm", "Das Haus ist gemütlich und warm", /* house */
    };
#endif

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

/*----------------
 * Fonts
 *----------------*/



/*----------------
 * Images
 *----------------*/

/* Targets: any */
const void * nav_default = NULL;
extern const void * nav_default_data;
const void * home_focused = NULL;
extern const void * home_focused_data;
const void * home_selected = NULL;
extern const void * home_selected_data;
const void * home_focused_selected = NULL;
extern const void * home_focused_selected_data;
const void * stats_focused = NULL;
extern const void * stats_focused_data;
const void * stats_selected = NULL;
extern const void * stats_selected_data;
const void * stats_focused_selected = NULL;
extern const void * stats_focused_selected_data;
const void * settings_focused = NULL;
extern const void * settings_focused_data;
const void * settings_selected = NULL;
extern const void * settings_selected_data;
const void * settings_focused_selected = NULL;
extern const void * settings_focused_selected_data;

/*----------------
 * Global styles
 *----------------*/

/*----------------
 * Subjects
 *----------------*/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void DESKMOO_UI_init_gen(const char * asset_path)
{
    /* When running from the editor the theme set from the XML should overwrite this */
#if !defined(LV_EDITOR_PREVIEW)
#if LV_USE_THEME_SIMPLE
    lv_display_t * disp = lv_display_get_default();
    lv_theme_t * th = lv_theme_simple_init(disp);
    lv_display_set_theme(disp, th);
#else
    LV_LOG_WARN("Simple theme is selected in project.xml but LV_USE_THEME_SIMPLE is disabled");
#endif
#endif /*LV_EDITOR_PREVIEW*/


    /*----------------
     * Fonts
     *----------------*/




    /*----------------
     * Images
     *----------------*/

    /* Targets: any */
    #if DESKMOO_UI_CHECK_COMPILE_TARGET(DESKMOO_UI_TARGET_ALL)
    if (DESKMOO_UI_check_target(DESKMOO_UI_TARGET_ALL)) {
        /* nav_default */
        if (!nav_default) {
            nav_default = &nav_default_data;
        }
        /* home_focused */
        if (!home_focused) {
            home_focused = &home_focused_data;
        }
        /* home_selected */
        if (!home_selected) {
            home_selected = &home_selected_data;
        }
        /* home_focused_selected */
        if (!home_focused_selected) {
            home_focused_selected = &home_focused_selected_data;
        }
        /* stats_focused */
        if (!stats_focused) {
            stats_focused = &stats_focused_data;
        }
        /* stats_selected */
        if (!stats_selected) {
            stats_selected = &stats_selected_data;
        }
        /* stats_focused_selected */
        if (!stats_focused_selected) {
            stats_focused_selected = &stats_focused_selected_data;
        }
        /* settings_focused */
        if (!settings_focused) {
            settings_focused = &settings_focused_data;
        }
        /* settings_selected */
        if (!settings_selected) {
            settings_selected = &settings_selected_data;
        }
        /* settings_focused_selected */
        if (!settings_focused_selected) {
            settings_focused_selected = &settings_focused_selected_data;
        }
    }
    #endif

    /*----------------
     * Global styles
     *----------------*/

    /*----------------
     * Subjects
     *----------------*/
    /*----------------
     * Translations
     *----------------*/

    #ifndef LV_EDITOR_PREVIEW
        lv_translation_add_static(translation_languages, translation_tags, translation_texts);
        lv_translation_set_language(translation_languages[0]);
    #endif

#if defined(LV_USE_XML) && LV_USE_XML
    /* Register widgets */


    /* Register fonts */

    /* Register subjects */

    /* Register callbacks */

    /* Register easing callbacks */
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if defined(LV_USE_XML) && LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
    lv_xml_register_image(NULL, "nav_default", nav_default);
    lv_xml_register_image(NULL, "home_focused", home_focused);
    lv_xml_register_image(NULL, "home_selected", home_selected);
    lv_xml_register_image(NULL, "home_focused_selected", home_focused_selected);
    lv_xml_register_image(NULL, "stats_focused", stats_focused);
    lv_xml_register_image(NULL, "stats_selected", stats_selected);
    lv_xml_register_image(NULL, "stats_focused_selected", stats_focused_selected);
    lv_xml_register_image(NULL, "settings_focused", settings_focused);
    lv_xml_register_image(NULL, "settings_selected", settings_selected);
    lv_xml_register_image(NULL, "settings_focused_selected", settings_focused_selected);
#endif

#if !defined(LV_USE_XML) || LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manually from XML using lv_xml_create() */
#endif
}

void DESKMOO_UI_set_target(uint32_t target)
{
    DESKMOO_UI_target = target;
}

uint32_t DESKMOO_UI_get_target(void)
{
    return DESKMOO_UI_target;
}

bool DESKMOO_UI_check_target(uint32_t target)
{
    return (DESKMOO_UI_target & target) ? true : false;
}

/* Callbacks */

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void check_font(lv_font_t ** font, const char * name)
{
    if (!(*font)) {
        *font = (lv_font_t *)LV_FONT_DEFAULT;
        LV_LOG_WARN("font `%s` was not set. Using `LV_FONT_DEFAULT` instead", name);
    }
}