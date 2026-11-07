#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>
#include "driver/gpio.h"

// ──────────────────────────── Shared SPI Bus Pin Definitions ────────────────────────────
// These pins are SHARED by LCD and SD card on the same SPI2 bus
#define SPI_MOSI_PIN        11     //Channel 2 of Logic Analyzer
#define SPI_MISO_PIN        12     //Channel 5 of Logic Analyzer
#define SPI_SCK_PIN         10      //Channel 0 of Logic Analyzer

// ──────────────────────────── Device CS Pin Definitions ────────────────────────────
#define SPI_LCD_CS_PIN      9       //Channel 1 of Logic Analyzer
#define SPI_SD_CS_PIN       13      //Channel 4 of Logic Analyzer
#define SPI_TOUCH_CS_PIN    14      

// ──────────────────────────── LCD-specific Pins ────────────────────────────
#define SPI_LCD_DC_PIN      8    //Channel 3 of Logic Analyzer
#define LCD_RESET_PIN       7

// ──────────────────────────── SPI setting ────────────────────────────
// RGB666 needs 3 bytes per pixel, and SPI DMA prefers 4-byte alignment.
// 4092 is divisible by both 3 and 4, so a transaction never splits a pixel.
#define CHUNK_SIZE_PER_TRANSACTION  4092
#define SPI_BUS_MAX_TRANSFER_SIZE   (64 * 1024)
#define LCD_QUEUE_SIZE              7

// ──────────────────────────── BUTTON setting ────────────────────────────
#define BUTTON_UP_PIN           4
#define BUTTON_DOWN_PIN         5
#define BUTTON_SELECT_PIN       6
#define BUTTON_BACK_PIN         15

typedef enum {
    BUTTON_UP = 0,
    BUTTON_DOWN,
    BUTTON_SELECT,
    BUTTON_BACK,
    BUTTON_MAX,
} button_id_t;

extern const gpio_num_t button_pins[];

#endif //BOARD_CONFIG_H
