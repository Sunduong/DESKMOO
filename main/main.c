#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h"
#include "spi_bus_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "mascot_renderer.h"
#include "lvgl.h"

static const char *TAG = "APP_MAIN";
lcd_t lcd;

#define MASCOT_X_POSITION                           191
#define MASCOT_Y_POSITION                           47
#define MASCOT_FRAME_PERIOD_MS                      180
#define NAV_BAR_WIDTH                               480
#define NAV_BAR_HEIGHT                              32
#define DRAW_LINES                                  16

static uint8_t draw_buffer[NAV_BAR_WIDTH * DRAW_LINES * 2]          __attribute__((aligned(4)));
DMA_ATTR static uint8_t lcd_row_rgb666[NAV_BAR_WIDTH * 3];

static uint32_t deskmoo_tick(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void deskmoo_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    /* Con trỏ LCD được gắn vào display lúc khởi tạo. */
    lcd_t *lcd = lv_display_get_user_data(display);

    const int32_t width = area->x2 - area->x1 + 1;
    const int32_t height = area->y2 - area->y1 + 1;
    const int32_t nav_y = lcd->height - NAV_BAR_HEIGHT;
    ESP_LOGI(TAG, "width %ld, height %ld, nav_y %ld", width, height, nav_y);
    /*
     * Dành cho RGB565 + PARTIAL, với stride mặc định của LVGL.
     *
     * Stride là số byte giữa hai hàng, có thể chứa padding (byte đệm để chia hết
     * cho một số cố định).
     *
     * Ví dụ:
     * LV_COLOR_FORMAT_RGB565
     *
     * | Hàng ảnh | Pixel trái | Pixel giữa | Pixel phải |
     * |----------|------------|------------|------------|
     * | Hàng 0   | A          | B          | C          |
     * | Hàng 1   | D          | E          | F          |
     *
     * Stride is 6 bytes.
     */
    const uint32_t stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_RGB565);
    ESP_LOGI(TAG, "stride %ld", stride);

    ESP_LOGI("LVGL_FLUSH", "START");
    ESP_ERROR_CHECK(lcd_streaming_begin_write(lcd, area->x1, nav_y + area->y1, area->x2, nav_y + area->y2));
    ESP_LOGI("LVGL_FLUSH", "SPI ACQUIRED");

    esp_err_t err = ESP_OK;
    for (int32_t y = 0; y < height; y++)
    {
        const uint8_t *src = px_map + y * stride;
        for (int32_t x = 0; x < width; x++)
        {
            /* RGB565 is native little-endian on ESP32-S3. */
            uint16_t pixel = (uint16_t)src[x * 2] | ((uint16_t)src[x * 2 + 1] << 8);
            uint8_t r5 = (pixel >> 11) & 0x1F;
            uint8_t g6 = (pixel >> 5) & 0x3F;
            uint8_t b5 = pixel & 0x1F;

            /* Convert from RGB565 on RAM to RGB666 on RAM. */
            uint8_t r6 = (r5 << 1) | (r5 >> 4);
            uint8_t b6 = (b5 << 1) | (b5 >> 4);

            /* LCD receives 6-bit color. */
            lcd_row_rgb666[x * 3] = r6 << 2;
            lcd_row_rgb666[x * 3 + 1] = g6 << 2;
            lcd_row_rgb666[x * 3 + 2] = b6 << 2;
        }

        err = lcd_streaming_write(lcd, lcd_row_rgb666, width * 3);
        if (err != ESP_OK)
        {
            break;
        }
    }

    lcd_streaming_end_write(lcd);
    ESP_LOGI("LVGL_FLUSH", "DONE");
    ESP_ERROR_CHECK(err);
    lv_display_flush_ready(display);
}

static void deskmoo_test_click_callback(lv_timer_t *timer)
{
    lv_obj_t *home_btn = lv_timer_get_user_data(timer);

    lv_obj_send_event(home_btn, LV_EVENT_SHORT_CLICKED, NULL);
}

static void deskmoo_home_button_event_callback(lv_event_t * event)
{
    lv_obj_t *label = lv_event_get_user_data(event);

    static uint8_t count = 0;
    count++;

    if (count > 10)
    {
        count = 0;
    }
    lv_label_set_text_fmt(label, "HOME %u", count);
}

lv_display_t *deskmoo_lvgl_display_init(lcd_t *lcd)
{
    lv_display_t *display = lv_display_create(NAV_BAR_WIDTH, NAV_BAR_HEIGHT);
    configASSERT(display != NULL);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof(draw_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Callback dùng dòng này để lấy lại con trỏ LCD. */
    lv_display_set_user_data(display, lcd);
    /* Đăng ký callback, chưa gọi callback tại đây. */
    lv_display_set_flush_cb(display, deskmoo_flush_cb);

    return display;
}

static void deskmoo_lvgl_task(void *pvParameters)
{
    lv_init();
    lv_tick_set_cb(deskmoo_tick);

    lv_display_t *display = deskmoo_lvgl_display_init(&lcd);
    if (display == NULL)
    {
        ESP_LOGE("LVGL", "Display port initialization failed");
        vTaskDelete(NULL);
        return;
    }

    lv_display_set_default(display);
    lv_obj_t *screen = lv_screen_active();

    /* Nền toàn footer. */
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    /* Button là con của screen footer. */
    lv_obj_t *home_btn = lv_button_create(screen);

    lv_obj_set_size(home_btn, 120, 28);
    lv_obj_center(home_btn);

    /* Hình thức của button. */
    lv_obj_set_style_bg_color(home_btn, lv_color_hex(0x205080), 0);
    lv_obj_set_style_bg_opa(home_btn, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(home_btn, 2, 0);
    lv_obj_set_style_border_color(home_btn, lv_color_hex(0x102840), 0);

    lv_obj_set_style_radius(home_btn, 6, 0);
    lv_obj_set_style_pad_all(home_btn, 0, 0);
    lv_obj_set_style_shadow_width(home_btn, 0, 0);

    /* Label là con của button. */
    lv_obj_t *home_label = lv_label_create(home_btn);

    lv_label_set_text(home_label, "HOME 0");
    lv_obj_set_style_text_color(home_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(home_label);

    lv_obj_add_event_cb(home_btn, deskmoo_home_button_event_callback, LV_EVENT_SHORT_CLICKED, home_label);

    /* Tao mot timer de test button event */
    lv_timer_t *test_timer = lv_timer_create(deskmoo_test_click_callback, 1000, home_btn);
    configASSERT(test_timer != NULL);

    while (true)
    {
        lv_timer_handler();
        vTaskDelay(1); // Một FreeRTOS tick, không phải mặc định 1 ms.
    }
}

static void mascot_animation_task(void *pvParameters)
{
    uint8_t frame = 0;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        ESP_LOGI("MASCOT", "FRAME %u START", frame);
        int64_t start = esp_timer_get_time();
        esp_err_t err = mascot_draw_frame(&lcd, background_rgb666, lcd.width, lcd.height, MASCOT_X_POSITION, MASCOT_Y_POSITION, frame);
        int64_t elapsed = esp_timer_get_time() - start;

        if (err != ESP_OK)
        {
            ESP_LOGE("MASCOT", "Failed to draw mascot frame %u: %s", frame, esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI("MASCOT", "Frame %u rendered in %.2f ms", frame, elapsed / 1000.0);
        }
        ESP_LOGI("MASCOT", "FRAME %u END, err=%s", frame, esp_err_to_name(err));
        frame++;
        if (frame >= MASCOT_FRAME_COUNT)
        {
            frame = 0;
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(MASCOT_FRAME_PERIOD_MS));
    }
}

void app_main(void)
{
    spi_bus_init(SPI2_HOST); // Initialize the shared SPI bus for both LCD and SD card.
    ESP_LOGI(TAG, "SPI Shared Bus initialized!");

    lcd_init(&lcd, SPI2_HOST);
    ESP_LOGI(TAG, "LCD initialized!");
    lcd_set_rotation(&lcd, 1);
    ESP_LOGI(TAG, "Drawing RGB666 background...");

    uint64_t start_timer = esp_timer_get_time();
    lcd_draw_bitmap(&lcd, 0, 0, lcd.width - 1, lcd.height - 1, background_rgb666);
    uint64_t end_timer = esp_timer_get_time();
    uint64_t elapsed_time_us = end_timer - start_timer;
    ESP_LOGI(TAG, "RGB666 background transfer complete in %.2f ms", elapsed_time_us / 1000.0);

    xTaskCreate(mascot_animation_task, "Mascot Animation Task", 4096, NULL, 5, NULL);
    xTaskCreate(deskmoo_lvgl_task, "Deskmoo LVGL Task", 8192, NULL, 4, NULL);
}

