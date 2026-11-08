#include <stdint.h>
#include "esp_err.h"
#include "lcd_driver.h"
#include "assets/mascot/mascot_resource.h"
#include "assets/background/background_rgb666.h"

esp_err_t mascot_draw_frame(
    lcd_t *lcd,
    const uint8_t *background,
    uint16_t background_width,
    uint16_t background_height,
    uint16_t x,
    uint16_t y,
    uint8_t frame_index
);