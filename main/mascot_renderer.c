#include "mascot_renderer.h"
#include <stdbool.h>
#include "esp_heap_caps.h"
#include "board_config.h"

typedef struct
{
    const uint8_t *data;
    uint32_t size;
    uint32_t pos;

    uint8_t run_remaining; // remaining pixels in the current record
    uint8_t run_index;  // current palette index for the record
} rle_decoder_t;


/*
 * Get next palette index from:
 *
 * [count][index] ---> 1 record
 * [count][index] ---> 1 record
 * ...
 */
static bool rle_next(rle_decoder_t *decoder, uint8_t *index)
{
    if (decoder->run_remaining == 0)
    {
        if (decoder->pos + 2 > decoder->size)
        {
            return false;
        }

        uint8_t count = decoder->data[decoder->pos++];
        uint8_t value = decoder->data[decoder->pos++];

        if (count == 0) 
        {
            return false;
        }

        decoder->run_remaining = count; 
        decoder->run_index = value;
        // From now to run_remaining equal 0, we don't need to read the data from RLE stream.
    }

    *index = decoder->run_index;
    decoder->run_remaining--;
    return true;
}


esp_err_t mascot_draw_frame(lcd_t *lcd, const uint8_t *background, uint16_t background_width, uint16_t background_height, uint16_t dst_x, uint16_t dst_y, uint8_t frame_index)
{
    if (lcd == NULL || background == NULL) return ESP_ERR_INVALID_ARG;
    if (frame_index >= MASCOT_FRAME_COUNT) return ESP_ERR_INVALID_ARG;

    /*
     * Sprite must be completely inside background/LCD.
     */
    if ((dst_x + MASCOT_WIDTH) > background_width || (dst_y + MASCOT_HEIGHT) > background_height)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * DMA working buffer.
     *
     * 4092 bytes:
     * divisible by RGB666's 3 bytes/pixel.
     */
    uint8_t *dma_buffer = heap_caps_malloc(CHUNK_SIZE_PER_TRANSACTION, MALLOC_CAP_DMA);
    if (dma_buffer == NULL) return ESP_ERR_NO_MEM;

    rle_decoder_t decoder = 
    {
        .data = mascot_frames[frame_index],
        .size = mascot_frame_sizes[frame_index],
        .pos = 0,
        .run_remaining = 0,
        .run_index = 0,
    };

    esp_err_t err = lcd_streaming_begin_write(lcd, dst_x, dst_y, dst_x + MASCOT_WIDTH - 1, dst_y + MASCOT_HEIGHT - 1);
    if (err != ESP_OK)
    {
        free(dma_buffer);
        return err;
    }

    uint32_t dma_used = 0;
    const uint32_t total_pixels = MASCOT_WIDTH * MASCOT_HEIGHT;

    for (uint32_t pixel = 0; pixel < total_pixels; pixel++)
    {
        uint8_t palette_index;
        if (!rle_next(&decoder, &palette_index))
        {
            err = ESP_ERR_INVALID_SIZE;
            break;
        }

        /*
         * Convert linear sprite pixel position
         * back into x/y.
         */
        uint32_t sprite_y = pixel / MASCOT_WIDTH;
        uint32_t sprite_x = pixel % MASCOT_WIDTH;

        uint8_t r, g, b;

        if (palette_index == 0x00)
        {
            /*
             * Transparent pixel:
             * restore corresponding background pixel.
             */
            uint32_t background_offset = ((dst_y + sprite_y) * background_width + (dst_x + sprite_x)) * 3;

            r = background[background_offset + 0];
            g = background[background_offset + 1];
            b = background[background_offset + 2];
        }
        else
        {
            /*
             * Visible mascot pixel:
             * palette is already RGB666.
             */
            uint32_t palette_offset = palette_index * 3;

            r = mascot_palette[palette_offset + 0];
            g = mascot_palette[palette_offset + 1];
            b = mascot_palette[palette_offset + 2];
        }

        dma_buffer[dma_used++] = r;
        dma_buffer[dma_used++] = g;
        dma_buffer[dma_used++] = b;

        /*
         * Flush DMA buffer when full.
         */
        if (dma_used == CHUNK_SIZE_PER_TRANSACTION)
        {
            err = lcd_streaming_write(lcd, dma_buffer, dma_used);
            if (err != ESP_OK) break;
            dma_used = 0;
        }
    }

    /*
     * Send remaining pixels.
     */
    if (err == ESP_OK && dma_used > 0)
    {
        err = lcd_streaming_write(lcd, dma_buffer, dma_used);
    }

    lcd_streaming_end_write(lcd);
    free(dma_buffer);
    return err;
}
