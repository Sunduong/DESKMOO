#include "lcd_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

static const char *TAG = "LCD_DRIVER";

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
            madctl = 0x60;
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

static spi_transaction_t trans_pool[LCD_QUEUE_SIZE];

static void lcd_send_data_queue(lcd_t *lcd, const uint8_t *data, uint32_t total_data_byte)
{
    if (total_data_byte == 0) return; // No need to send anything
    if (data == NULL) return; // No data to send

    uint32_t offset = 0;
    int queued = 0; // Số transaction đang chờ trong queue
    int trans_idx = 0; // Index trong trans_pool

    while (offset < total_data_byte)
    {
        // Calculate chunk size
        int chunk = (total_data_byte - offset > CHUNK_SIZE_PER_TRANSACTION) ? CHUNK_SIZE_PER_TRANSACTION : (total_data_byte - offset);

        // Setup Transaction and clear any previous transaction state
        spi_transaction_t *t = &trans_pool[trans_idx % LCD_QUEUE_SIZE]; // Circular buffer
        memset(t, 0, sizeof(*t));
        t->length = chunk * 8;
        t->tx_buffer = data + offset;
        t->user = (void*) 1; // DC high for data

        spi_device_queue_trans(lcd->spi, t, portMAX_DELAY);
        queued++;
        trans_idx++;
        offset += chunk;

        // Nếu queue gần đầy → chờ ít nhất 1 transaction xong
        if (queued >= LCD_QUEUE_SIZE - 1)
        {
            spi_transaction_t *ret;
            spi_device_get_trans_result(lcd->spi, &ret, portMAX_DELAY);
            queued--;
        }
    }
 
    // Chờ tất cả transactions còn lại hoàn tất
    while (queued > 0)
    {
        spi_transaction_t *ret;
        spi_device_get_trans_result(lcd->spi, &ret, portMAX_DELAY);
        queued--;
    }
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
    // lcd_send_data(lcd, data, height * width * 3);
    // If caller provides a PSRAM-backed buffer and the platform supports
    // DMA from external RAM, callers may instead call `lcd_send_data_psram_dma`.
    lcd_send_data_psram_dma(lcd, data, total_data_byte);
    spi_device_release_bus(lcd->spi);
}

/**
 * @brief Send data from PSRAM directly to LCD using DMA-capable transactions.
 *
 * This function attempts to transmit directly from the provided `data`
 * pointer in chunks no larger than `SPI_BUS_MAX_TRANSFER_SIZE` using
 * polling transmits. If the underlying platform and allocation support
 * DMA from external RAM, this avoids copying into an intermediate buffer.
 */
void lcd_send_data_psram_dma(lcd_t *lcd, const uint8_t *data, uint32_t total_data_byte)
{
    if (total_data_byte == 0) return;
    if (data == NULL) return;
    // Determine safe maximum chunk size based on compile-time and SOC limits
    uint32_t soc_max = SOC_SPI_MAXIMUM_BUFFER_SIZE;
    uint32_t max_chunk = (SPI_BUS_MAX_TRANSFER_SIZE < soc_max) ? SPI_BUS_MAX_TRANSFER_SIZE : soc_max;
    if (max_chunk == 0) max_chunk = 4096; // sensible default
    ESP_LOGI(TAG, "lcd_send_data_psram_dma: total=%u, max_chunk=%u", (unsigned)total_data_byte, (unsigned)max_chunk);

    uint32_t offset = 0;
    while (offset < total_data_byte)
    {
        uint32_t remaining = total_data_byte - offset;
        uint32_t chunk = (remaining > max_chunk) ? max_chunk : remaining;

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = chunk * 8U;
        t.tx_buffer = data + offset;
        t.user = (void*)1; // DC high for data
        esp_err_t err = spi_device_polling_transmit(lcd->spi, &t);
        if (err == ESP_OK) {
            offset += chunk;
        }
    }
}