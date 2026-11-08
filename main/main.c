#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h"
#include "spi_bus_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "mascot_renderer.h"

static const char *TAG = "APP_MAIN";

#define MASCOT_X_POSITION               191
#define MASCOT_Y_POSITION               47
#define MASCOT_FRAME_PERIOD_MS          180

lcd_t lcd;

static void mascot_animation_task(void *pvParameters)
{
    uint8_t frame = 0;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        int64_t start = esp_timer_get_time();
        esp_err_t err = mascot_draw_frame(&lcd, background_rgb666, lcd.width, lcd.height, MASCOT_X_POSITION, MASCOT_Y_POSITION, frame);
        int64_t elapsed = esp_timer_get_time() - start;

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to draw mascot frame %u: %s", frame, esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Frame %u rendered in %.2f ms", frame, elapsed / 1000.0);
        }

        frame++;
        if (frame >= MASCOT_FRAME_COUNT)
        {
            frame = 0;
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(MASCOT_FRAME_PERIOD_MS)); // 180ms per frame for ~5.6 FPS
    }
}

void app_main(void)
{
    spi_bus_init(SPI2_HOST); // Initialize the shared SPI bus for both LCD and SD card
    ESP_LOGI(TAG, "SPI Shared Bus initialized!");

    lcd_init(&lcd, SPI2_HOST);
    ESP_LOGI(TAG, "LCD initialized!");
    lcd_set_rotation(&lcd, 1);
    ESP_LOGI(TAG, "Drawing RGB666 background...");

    uint64_t start_timer = esp_timer_get_time();
    lcd_draw_bitmap(&lcd,
                    0, 0,
                    lcd.width - 1,
                    lcd.height - 1,
                    background_rgb666);
    uint64_t end_timer = esp_timer_get_time();
    uint64_t elapsed_time_us = end_timer - start_timer;
    ESP_LOGI(TAG, "RGB666 background transfer complete in %.2f ms", elapsed_time_us / 1000.0);
    
    xTaskCreate(mascot_animation_task, "Mascot Animation Task", 4096, NULL, 4, NULL);
}
