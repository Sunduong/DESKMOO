#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "spi_bus_manager.h"

// ──────────────────────────── Display Dimensions ────────────────────────────
#define LCD_WIDTH               320
#define LCD_HEIGHT              480

// ──────────────────────────── RGB Color hex codes - Use for RGB666 on module ────────────────────────────
#define COLOR_BLACK             0x000000
#define COLOR_WHITE             0xFFFFFF
#define COLOR_RED               0xFF0000
#define COLOR_GREEN             0x00FF00
#define COLOR_BLUE              0x0000FF
#define COLOR_YELLOW            0xFFFF00
#define COLOR_CYAN              0x00FFFF
#define COLOR_MAGENTA           0xFF00FF
#define COLOR_ORANGE            0xFF7F00

// ──────────────────────────── LCD Device Structure ────────────────────────────
typedef struct {
    spi_device_handle_t spi; // SPI device handle
    int dc_pin;              // Data/Command control pin
    int rst_pin;             // Reset pin
    uint16_t width;          // Display width
    uint16_t height;         // Display height
    uint8_t rotation;        // Display rotation (0-3)
    uint8_t *framebuffer;    // DMA-capable framebuffer (allocated once)
} lcd_t;


void lcd_set_rotation(lcd_t *lcd, uint8_t rotation);
void lcd_fill_rect(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void lcd_fill_screen(lcd_t *lcd, uint32_t color);
void lcd_init(lcd_t *lcd, spi_host_device_t host);
void lcd_draw_pixel(lcd_t *lcd, uint16_t x, uint16_t y, uint32_t color);
void lcd_draw_hline(lcd_t *lcd, uint16_t x, uint16_t y, uint16_t width, uint32_t color);
void lcd_draw_vline(lcd_t *lcd, uint16_t x, uint16_t y, uint16_t height, uint32_t color);
void lcd_draw_rect(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void lcd_draw_bitmap(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint8_t *data);
#endif // LCD_DRIVER_H
