#include "navigation_controller.h"

#include "encoder_driver.h"
#include "esp_log.h"

static void read_encoder(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->enc_diff = encoder_driver_get_diff();
    data->key = LV_KEY_ENTER;
    data->state = encoder_driver_is_pressed() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void select_nav_button(lv_event_t *event)
{
    lv_obj_t *selected = lv_event_get_target_obj(event);
    lv_group_t *group = lv_obj_get_group(selected);
    uint32_t count = lv_group_get_obj_count(group);

    for (uint32_t i = 0; i < count; i++)
    {
        lv_obj_t *button = lv_group_get_obj_by_index(group, i);
        lv_obj_set_checked(button, button == selected);
    }
}

esp_err_t navigation_controller_init(lv_obj_t *screen)
{
    static const char *const names[] = {"btn_home", "btn_stats", "btn_settings"};
    enum { BUTTON_COUNT = sizeof(names) / sizeof(names[0]) };
    lv_obj_t *buttons[BUTTON_COUNT];
    for (size_t i = 0; i < BUTTON_COUNT; i++)
    {
        buttons[i] = lv_obj_find_by_name(screen, names[i]);
        if (buttons[i] == NULL)
        {
            ESP_LOGE("NAV", "Missing navigation button: %s", names[i]);
            return ESP_ERR_NOT_FOUND;
        }
    }

    lv_group_t *group = lv_group_create();
    if (group == NULL) return ESP_ERR_NO_MEM;
    lv_indev_t *encoder = lv_indev_create();
    if (encoder == NULL)
    {
        lv_group_delete(group);
        return ESP_ERR_NO_MEM;
    }

    lv_indev_set_type(encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_group(encoder, group);
    lv_indev_set_read_cb(encoder, read_encoder);
    lv_timer_set_period(lv_indev_get_read_timer(encoder), 10);

    for (size_t i = 0; i < BUTTON_COUNT; i++)
    {
        lv_group_add_obj(group, buttons[i]);
        lv_obj_add_event_cb(buttons[i], select_nav_button, LV_EVENT_SHORT_CLICKED, NULL);
    }
    /* LVGL focuses the first button added to the group. Home is also the initial selection. */
    lv_obj_set_checked(buttons[0], true);
    return ESP_OK;
}
