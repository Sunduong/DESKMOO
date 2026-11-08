#include "lcd_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stddef.h>
#include "esp_err.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "LCD_DRIVER";

_Static_assert(CHUNK_SIZE_PER_TRANSACTION % 3 == 0,
               "RGB666 DMA chunks must contain complete pixels");
_Static_assert(CHUNK_SIZE_PER_TRANSACTION % 4 == 0,
               "SPI DMA chunks must be 4-byte aligned");

static void lcd_spi_pre_transfer(spi_transaction_t *t)
{
    int dc = (int) t->user;
    gpio_set_level(SPI_LCD_DC_PIN, dc);
}

static void lcd_send_cmd(lcd_t *lcd, uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8, // Command is 1 byte
        .tx_buffer = &cmd,
        .user = (void*) 0, // DC low for command
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(lcd->spi, &t));
}

static void lcd_send_data(lcd_t *lcd, const uint8_t *data, int len)
{
    if (len == 0) return; // No need to send anything
    if (data == NULL) return; // No data to send

    spi_transaction_t t =   {
        .length = len * 8, // Data length in bits
        .tx_buffer = data,
        .user = (void*) 1, // DC high for data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(lcd->spi, &t));
}

static void lcd_set_window(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // CASET: Set column range (X coordinates)
    lcd_send_cmd(lcd, 0x2A); // CASET
    uint8_t col[4] = {
        (x1 >> 8) & 0xFF,   //Start column - High byte
        x1 & 0xFF,          //Start column - Low byte
        (x2 >> 8) & 0xFF,   //End column - High byte
        x2 & 0xFF,          //End column - Low byte
    };
    lcd_send_data(lcd, col, 4);

    // PASET: Set row range (Y coordinates)
    lcd_send_cmd(lcd, 0x2B);
    uint8_t row[4] = {
        (y1 >> 8) & 0xFF,   //Start row - High byte
        y1 & 0xFF,          //Start row - Low byte
        (y2 >> 8) & 0xFF,   //End row - High byte
        y2 & 0xFF,          //End row - Low byte
    };
    lcd_send_data(lcd, row, 4);

    // RAMWR: Prepare to receive pixel data
    lcd_send_cmd(lcd, 0x2C);
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Initialization
// ═══════════════════════════════════════════════════════
/*
┌─────────────────────────────────────────────────────────────────┐
│ INITIALIZATION FLOW │
├─────────────────────────────────────────────────────────────────┤
│ │
│ 1. HARDWARE RESET (RST pin) │
│ RST LOW → wait 10ms → RST HIGH → wait 100ms │
│ │ │
│ ▼ │
│ 2. SOFTWARE RESET (0x01) │
│ Send SWRESET → wait 120ms │
│ │ │
│ ▼ │
│ 3. SLEEP OUT (0x11) │
│ Send SLPOUT → wait 120ms ⚠️ DON'T SKIP THIS DELAY! │
│ │ │
│ ▼ │
│ 4. SET COLOR FORMAT (0x3A) │
│ Send COLMOD + data 0x55 (RGB565) │
│ │ │
│ ▼ │
│ 5. SET ROTATION (0x36) │
│ Send MADCTL + rotation value (0x00 for portrait) │
│ │ │
│ ▼ │
│ 6. NORMAL DISPLAY MODE (0x13) │
│ Send NORON │
│ │ │
│ ▼ │
│ 7. DISPLAY ON (0x29) │
│ Send DISPON → wait 100ms │
│ │ │
│ ▼ │
│ ✅ DISPLAY IS NOW ACTIVE! │
│ │
└─────────────────────────────────────────────────────────────────┘
*/
void lcd_init(lcd_t *lcd, spi_host_device_t host)
{
    lcd->dc_pin = SPI_LCD_DC_PIN;
    lcd->rst_pin = LCD_RESET_PIN;
    lcd->width = LCD_WIDTH;
    lcd->height = LCD_HEIGHT;
    lcd->rotation = 0;

    ESP_LOGI(TAG, "Initializing LCD...");

    // Configure GPIOs
    gpio_set_direction(lcd->dc_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd->rst_pin, GPIO_MODE_OUTPUT);

    // Add LCD device on bus
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000, // 20 MHz
        .mode = 0, // SPI mode 0
        .spics_io_num = SPI_LCD_CS_PIN, // SPI_LCD_CS_PIN, control manually
        .queue_size = 7,
        .pre_cb = lcd_spi_pre_transfer, // DC pin callback
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &lcd->spi));
    ESP_LOGI(TAG, "LCD device added to SPI bus...");

    // Hardware reset
    gpio_set_level(lcd->rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10)); // Hold reset low for 10ms
    gpio_set_level(lcd->rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for LCD to reset
    ESP_LOGI(TAG, "LCD hardware reset complete...");

    // Software reset
    lcd_send_cmd(lcd, 0x01); // SWRESET
    vTaskDelay(pdMS_TO_TICKS(120)); // Wait for reset to complete
    ESP_LOGI(TAG, "LCD software reset complete...");

    // Sleep out
    lcd_send_cmd(lcd, 0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(120)); // Wait for sleep out to complete
    ESP_LOGI(TAG, "LCD sleep out complete...");

    // Set color mode to RGB666
    lcd_send_cmd(lcd, 0x3A); // COLMOD 
    uint8_t pixel_format = 0x66; // 18-bit RGB666
    lcd_send_data(lcd, &pixel_format, 1);
    ESP_LOGI(TAG, "LCD color mode set to RGB666...");

    // Set rotation
    lcd_send_cmd(lcd, 0x36); // MADCTL
    uint8_t madctl = 0x48; // Portrait, no flips
    lcd_send_data(lcd, &madctl, 1);
    ESP_LOGI(TAG, "LCD rotation set to portrait...");

    // Normal display mode
    lcd_send_cmd(lcd, 0x13); // NORON
    ESP_LOGI(TAG, "LCD normal display mode set...");

    // Display on
    lcd_send_cmd(lcd, 0x29); // DISPON
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for display to turn on
    ESP_LOGI(TAG, "LCD display turned on...");

    ESP_LOGI(TAG, "LCD initialization complete!");
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Rotation
// ═══════════════════════════════════════════════════════
void lcd_set_rotation(lcd_t *lcd, uint8_t rotation)
{
    /*  Lock SPI bus — prevent SD card from interrupting
    For now, use portMAX_DELAY to keep simple coding. 
    Upgrade later for timeout + error handling */
    ESP_ERROR_CHECK(spi_device_acquire_bus(lcd->spi, portMAX_DELAY));
    lcd->rotation = rotation % 4;

    uint8_t madctl = 0;
    switch (lcd->rotation) {
        case 0: // Portrait
            madctl = 0x00;
            lcd->width = LCD_WIDTH;
            lcd->height = LCD_HEIGHT;
            break;
        case 1: // Landscape (Rotate 90° CW-clockwise)
            madctl = 0xE8;
            lcd->width = LCD_HEIGHT;
            lcd->height = LCD_WIDTH;
            break;
        case 2: // Portrait (Rotate 180° CW-clockwise)
            madctl = 0xC0;
            lcd->width = LCD_WIDTH;
            lcd->height = LCD_HEIGHT;
            break;
        case 3: // Landscape (Rotate 270° CW-clockwise)
            madctl = 0xA0;
            lcd->width = LCD_HEIGHT;
            lcd->height = LCD_WIDTH;
            break;
        default:
            madctl = 0x00;
            lcd->width = LCD_WIDTH;
            lcd->height = LCD_HEIGHT;
    }
    lcd_send_cmd(lcd, 0x36); // MADCTL
    lcd_send_data(lcd, &madctl, 1);
    spi_device_release_bus(lcd->spi);
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Drawing Functions
// ═══════════════════════════════════════════════════════
void lcd_fill_rect(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    /*  Lock SPI bus — prevent SD card from interrupting
    For now, use portMAX_DELAY to keep simple coding. 
    Upgrade later for timeout + error handling */
    ESP_ERROR_CHECK(spi_device_acquire_bus(lcd->spi, portMAX_DELAY));

    // if (lcd == NULL) return;
    // if (x1 > x2 || y1 > y2) return;
    // if (x1 >= lcd->width || y1 >= lcd->height) return;
    
    // if (x2 >= lcd->width) x2 = lcd->width - 1;
    // if (y2 >= lcd->height) y2 = lcd->height - 1;

    // Set the drawing window
    lcd_set_window(lcd, x1, y1, x2, y2);
    
    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
    // uint32_t total_pixels = (uint32_t)width * height;
    
    // Allocate a line buffer (DMA-capable memory)
    // Size: one row of pixels × 3 bytes per pixel
    uint8_t *line_buf = heap_caps_malloc(width * 3, MALLOC_CAP_DMA);
    if (line_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate line buffer!");
        spi_device_release_bus(lcd->spi); 
        return;
    }

    // Fill line buffer with the color
    // ⚠️ RGB666 is big-endian on SPI, but ESP32 is little-endian!
    // We need to byte-swap the color value
    uint8_t r = (uint8_t) (((color >> 16) & 0xFF) & 0xFC); //RED
    uint8_t g = (uint8_t) (((color >> 8) & 0xFF) & 0xFC);  //GREEN
    uint8_t b = (uint8_t) (((color >> 0) & 0xFF) & 0xFC);  //BLUE

     // Fill the line buffer with the RGB666 color data
     for (int i = 0; i < width; i++) {
        line_buf[i * 3 + 0] = r;  // Red
        line_buf[i * 3 + 1] = g;  // Green
        line_buf[i * 3 + 2] = b;  // Blue
    }
    
    // Send line by line
    for (int y = 0; y < height; y++) {
        lcd_send_data(lcd, (uint8_t*)line_buf, width * 3);
    }
    free(line_buf);
    spi_device_release_bus(lcd->spi);
}

void lcd_fill_screen(lcd_t *lcd, uint32_t color)
{
    lcd_fill_rect(lcd, 0, 0, lcd->width - 1, lcd->height - 1, color);
}

void lcd_draw_pixel(lcd_t *lcd, uint16_t x, uint16_t y, uint32_t color)
{
    /*  Lock SPI bus — prevent SD card from interrupting
    For now, use portMAX_DELAY to keep simple coding. 
    Upgrade later for timeout + error handling */
    ESP_ERROR_CHECK(spi_device_acquire_bus(lcd->spi, portMAX_DELAY));

    lcd_set_window(lcd, x, y, x, y);
    //Send 3 bytes for 1 pixel
    uint8_t data[3];
    data[0] = ((color >> 16) & 0xFF) & 0xFC; //RED
    data[1] = ((color >> 8) & 0xFF) & 0xFC; //GREEN
    data[2] = ((color >> 0) & 0xFF) & 0xFC; //BLUE
    lcd_send_data(lcd, data, 3);
    spi_device_release_bus(lcd->spi);
}

void lcd_draw_hline(lcd_t *lcd, uint16_t x, uint16_t y, uint16_t width, uint32_t color)
{
    lcd_fill_rect(lcd, x, y, x + width - 1, y, color);
}

void lcd_draw_vline(lcd_t *lcd, uint16_t x, uint16_t y, uint16_t height, uint32_t color)
{
    lcd_fill_rect(lcd, x, y, x, y + height - 1, color);
}

void lcd_draw_rect(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_draw_hline(lcd, x1, y1, x2 - x1 + 1, color); //Top line
    lcd_draw_vline(lcd, x2, y1, y2 - y1 + 1, color); //Right line
    lcd_draw_hline(lcd, x1, y2, x2 - x1 + 1, color); //Bottom line
    lcd_draw_vline(lcd, x1, y1, y2 - y1 + 1, color); //Left line
}

static esp_err_t lcd_send_data_dma_chunks(lcd_t *lcd, const uint8_t *data,
                                          uint32_t total_data_byte)
{
    if (lcd == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (total_data_byte == 0) {
        return ESP_OK;
    }

    uint8_t *dma_buffer = heap_caps_malloc(CHUNK_SIZE_PER_TRANSACTION,
                                           MALLOC_CAP_DMA);
    if (dma_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate bitmap DMA buffer");
        return ESP_ERR_NO_MEM;
    }

    uint32_t offset = 0;
    esp_err_t err = ESP_OK;

    while (offset < total_data_byte) {
        uint32_t remaining = total_data_byte - offset;
        uint32_t chunk = remaining > CHUNK_SIZE_PER_TRANSACTION
                             ? CHUNK_SIZE_PER_TRANSACTION
                             : remaining;

        memcpy(dma_buffer, data + offset, chunk);

        spi_transaction_t transaction = {
            .length = chunk * 8,
            .tx_buffer = dma_buffer,
            .user = (void *)1,
        };

        err = spi_device_polling_transmit(lcd->spi, &transaction);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Bitmap transfer failed at byte %lu: %s",
                     (unsigned long)offset, esp_err_to_name(err));
            break;
        }

        offset += chunk;
    }

    free(dma_buffer);
    return err;
}

/**
 * @brief  draw bitmap full tile from constant data
 *
 * Draw full tile if tile is completely inside the lcd range
 * 
 * @param lcd       LCD handle
 * @param x1        start x
 * @param y1        start y
 * @param x2        finish x
 * @param y2        finish y
 * @param data      data buffer to draw
 */
void lcd_draw_bitmap(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint8_t *data)
{
    if (lcd == NULL || data == NULL) return;

    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
    uint32_t total_data_byte = height * width * 3;

    ESP_ERROR_CHECK(spi_device_acquire_bus(lcd->spi, portMAX_DELAY));
    lcd_set_window(lcd, x1, y1, x2, y2);
    esp_err_t err = lcd_send_data_dma_chunks(lcd, data, total_data_byte);
    spi_device_release_bus(lcd->spi);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Bitmap drawing failed: %s", esp_err_to_name(err));
    }
}

// ═══════════════════════════════════════════════════════
// PUBLIC: DESKMOO support Functions
// ═══════════════════════════════════════════════════════

/*  Streaming API
    Resource decoder --> Compositor --> Streaming LCD interface
*/
esp_err_t lcd_streaming_begin_write(lcd_t *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    if (lcd == NULL) return ESP_ERR_INVALID_ARG;
    if (x1 > x2 || y1 > y2) return ESP_ERR_INVALID_ARG;
    if (x1 >= lcd->width || y1 >= lcd->height) return ESP_ERR_INVALID_ARG;

    esp_err_t err = spi_device_acquire_bus(lcd->spi, portMAX_DELAY);
    if (err != ESP_OK) return err;

    lcd_set_window(lcd, x1, y1, x2, y2);
    return ESP_OK;
}

esp_err_t lcd_streaming_write(lcd_t *lcd, const uint8_t *data, size_t data_len)
{
    if (lcd == NULL || data == NULL || data_len == 0) return ESP_ERR_INVALID_ARG;
    spi_transaction_t transaction = {
        .length = data_len * 8,
        .tx_buffer = data,
        .user = (void *)1,
    };
    return spi_device_polling_transmit(lcd->spi, &transaction);
}

void lcd_streaming_end_write(lcd_t *lcd)
{
    if (lcd != NULL)
    {
        spi_device_release_bus(lcd->spi);
    }
}

