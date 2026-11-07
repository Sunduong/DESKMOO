#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h"
#include "spi_bus_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "assets/background_rgb666.h"

static const char *TAG = "APP_MAIN";

lcd_t lcd;

void app_main(void)
{
    spi_bus_init(SPI2_HOST); // Initialize the shared SPI bus for both LCD and SD card
    ESP_LOGI(TAG, "SPI Shared Bus initialized!");

    lcd_init(&lcd, SPI2_HOST);
    ESP_LOGI(TAG, "LCD initialized!");
    lcd_set_rotation(&lcd, 1);
    ESP_LOGI(TAG, "Drawing RGB666 background...");
    lcd_draw_bitmap(&lcd,
                    0, 0,
                    lcd.width - 1,
                    lcd.height - 1,
                    background_rgb666);
    ESP_LOGI(TAG, "RGB666 background transfer complete");
    //vTaskLCD has higher priority than other tasks
    // xTaskCreate(vTaskLCD, "LCD Task", 4096 * 2, NULL, 4, NULL);
}
