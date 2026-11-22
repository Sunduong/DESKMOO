
#include "encoder_driver.h"

#include <limits.h>
#include <stdint.h>

#include "board_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "ENCODER_DRIVER";

/* Button polling period */
#define BUTTON_SAMPLE_PERIOD_US     5000U
#define BUTTON_SAMPLE_PERIOD_MS     5U

#define BUTTON_REQUIRED_STABLE_SAMPLES       ((ENCODER_SW_DEBOUNCE_MS + BUTTON_SAMPLE_PERIOD_MS - 1) / BUTTON_SAMPLE_PERIOD_MS)

#if (ENCODER_EDGES_PER_DETENT != 2) && (ENCODER_EDGES_PER_DETENT != 4)
#error "ENCODER_EDGES_PER_DETENT must be 2 or 4"
#endif

#if (ENCODER_SW_DEBOUNCE_MS < 5)
#error "ENCODER_SW_DEBOUNCE_MS must be >= 5"
#endif

/* =========================================================
 * ROTARY ENCODER - ROTATION STATE
 * ========================================================= */
typedef struct
{
    uint8_t previous_encoder_ab;                                        // Previous combined CLK/DT state (2 bits)
    int8_t transition_count;                                            // Accumulated valid transitions toward a detent
    int16_t unread_rotation_steps;                                      // Completed rotation steps waiting to be read
} encoder_rotation_state_t;

/* =========================================================
 * ROTARY ENCODER - PUSH BUTTON STATE
 * ========================================================= */
typedef struct
{
    bool candidate_pressed;             // Raw button state currently being verified
    uint8_t stable_sample_count;        // Number of consecutive identical samples
    bool debounced_pressed;             // Accepted button state after debounce
} encoder_button_state_t;

/* =========================================================
 * PRIVATE DRIVER STATE
 * ========================================================= */
static encoder_rotation_state_t s_rotation = {0};
static encoder_button_state_t s_button = {0};
static bool s_encoder_initialized = false;
static esp_timer_handle_t s_sw_timer = NULL;
static portMUX_TYPE s_encoder_state_lock = portMUX_INITIALIZER_UNLOCKED;            // Protect shared state across ISR and LVGL tasks

/*
 * Quadrature transition table.
 *
 * Index = (previous_AB << 2) | current_AB
 *
 * A = CLK, B = DT
 *
 * Valid transition: +1 or -1 quarter-step.
 * Invalid or unchanged transition: 0.
 *
 * Example positive sequence:
 * 11 -> 01 -> 00 -> 10 -> 11
 */
static const int8_t s_quadrature_transition_table[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0
};

/* Read encoder CLK/DT as a 2-bit state */
static inline uint8_t encoder_read_ab(void)
{
    uint8_t a = (uint8_t)gpio_get_level(ENCODER_CLK_PIN);
    uint8_t b = (uint8_t)gpio_get_level(ENCODER_DT_PIN);

    return (uint8_t)((a << 1) | b);
}

/*
 * GPIO interrupt callback.
 *
 * Triggered on BOTH edges of CLK and DT.
 *
 * ISR work must stay short:
 * - Read GPIO
 * - Decode transition
 * - Update accumulated steps
 *
 * No logging, malloc or LVGL calls here.
 */
static void encoder_gpio_isr(void *arg)
{
    (void)arg;

    uint8_t current_ab = encoder_read_ab();

    portENTER_CRITICAL_ISR(&s_encoder_state_lock);

    uint8_t previous_ab = s_rotation.previous_encoder_ab;

    uint8_t transition_index = (uint8_t)((previous_ab << 2) | current_ab);

    int8_t delta = s_quadrature_transition_table[transition_index];

#if ENCODER_REVERSE_DIRECTION
    delta = -delta;
#endif

    /*
     * Both bits changed simultaneously: invalid quadrature transition.
     *
     * Reset partial movement and resynchronize.
     */
    if ((previous_ab ^ current_ab) == 0x03) 
    {
        s_rotation.transition_count = 0;
    }
    else if (delta != 0) 
    {
        s_rotation.transition_count += delta;

        if (s_rotation.transition_count >= ENCODER_EDGES_PER_DETENT)
        {
            if (s_rotation.unread_rotation_steps < INT16_MAX) 
            {
                s_rotation.unread_rotation_steps++;
            }
            s_rotation.transition_count = 0;
        }
        else if (s_rotation.transition_count <= -ENCODER_EDGES_PER_DETENT) 
        {
            if (s_rotation.unread_rotation_steps > INT16_MIN)
            {
                s_rotation.unread_rotation_steps--;
            }
            s_rotation.transition_count = 0;
        }
    }
    s_rotation.previous_encoder_ab = current_ab;
    portEXIT_CRITICAL_ISR(&s_encoder_state_lock );
}

/*
 * Periodic switch debounce.
 *
 * Runs in esp_timer task context.
 * Sampling period: 5 ms.
 *
 * A state is accepted only after consecutive identical readings spanning the configured
 * debounce sample count.
 */
static void encoder_sw_timer_cb(void *arg)
{
    (void)arg;

    /* KY-040 SW is normally active LOW */
    bool raw_pressed = (gpio_get_level(ENCODER_SW_PIN) == 0) ? 1U : 0U;

    if (raw_pressed != s_button.candidate_pressed) 
    {
        s_button.candidate_pressed = raw_pressed;
        s_button.stable_sample_count = 1;
    }
    else if (s_button.stable_sample_count < BUTTON_REQUIRED_STABLE_SAMPLES)     // GPIO state remains unchanged. Continue counting stable samples.
    {
        s_button.stable_sample_count++;
    }


    // Accept the new button state after enough consecutive identical samples.
    if (s_button.stable_sample_count >= BUTTON_REQUIRED_STABLE_SAMPLES) 
    {
        portENTER_CRITICAL(&s_encoder_state_lock );
        s_button.debounced_pressed = s_button.candidate_pressed;
        portEXIT_CRITICAL(&s_encoder_state_lock );
    }
}

/**
 * @brief Initialize KY-040 rotary encoder.
 *
 * CLK/DT: GPIO interrupts, quadrature decoding.
 * SW: Periodic debounce using esp_timer.
 *
 * Call once during system initialization.
 */
esp_err_t encoder_driver_init(void)
{
    if (s_encoder_initialized) 
    {
        return ESP_OK;
    }

    /*
     * CLK and DT:
     * input + pull-up + interrupts on both edges.
     */
    gpio_config_t ab_config = 
    {
        .pin_bit_mask = (1ULL << ENCODER_CLK_PIN) | (1ULL << ENCODER_DT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };

    esp_err_t err = gpio_config(&ab_config);
    if (err != ESP_OK) 
    {
        return err;
    }

    /*
     * SW:
     * input + pull-up, sampled using esp_timer.
     */
    gpio_config_t sw_config = 
    {
        .pin_bit_mask = (1ULL << ENCODER_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&sw_config);
    if (err != ESP_OK) 
    {
        return err;
    }

    /* Initialize decoder and button states */
    s_rotation.previous_encoder_ab = encoder_read_ab();
    s_rotation.transition_count = 0;
    s_rotation.unread_rotation_steps = 0;

    s_button.candidate_pressed = (gpio_get_level(ENCODER_SW_PIN) == 0) ? 1U : 0U;

    s_button.debounced_pressed = s_button.candidate_pressed;
    s_button.stable_sample_count = BUTTON_REQUIRED_STABLE_SAMPLES;

    /*
     * Install GPIO ISR service.
     *
     * It may already be installed by another module.
     * No ESP_INTR_FLAG_IRAM is requested.
     */
    err = gpio_install_isr_service(0);

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) 
    {
        return err;
    }

    err = gpio_isr_handler_add(ENCODER_CLK_PIN, encoder_gpio_isr, NULL);
    if (err != ESP_OK) 
    {
        return err;
    }

    err = gpio_isr_handler_add(ENCODER_DT_PIN, encoder_gpio_isr, NULL);
    if (err != ESP_OK) 
    {
        gpio_isr_handler_remove(ENCODER_CLK_PIN);
        return err;
    }

    /*
     * Create periodic timer for SW debounce.
     */
    esp_timer_create_args_t timer_args = 
    {
        .callback = encoder_sw_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ky040_sw",
    };

    err = esp_timer_create(&timer_args, &s_sw_timer);
    if (err != ESP_OK) 
    {
        goto cleanup_isr;
    }

    err = esp_timer_start_periodic(s_sw_timer, BUTTON_SAMPLE_PERIOD_US);
    if (err != ESP_OK) 
    {
        esp_timer_delete(s_sw_timer);
        s_sw_timer = NULL;
        goto cleanup_isr;
    }

    s_encoder_initialized = true;

    ESP_LOGI(TAG, "KY-040 initialized");
    ESP_LOGI(TAG, "CLK=%d DT=%d SW=%d", ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);
    ESP_LOGI(TAG, "Edges/detent=%d, SW debounce=%d ms", ENCODER_EDGES_PER_DETENT, ENCODER_SW_DEBOUNCE_MS);

    return ESP_OK;

cleanup_isr:
    gpio_isr_handler_remove(ENCODER_CLK_PIN);
    gpio_isr_handler_remove(ENCODER_DT_PIN);
    return err;
}

/**
 * @brief Read accumulated encoder movement.
 *
 * Positive: clockwise (subject to wiring/direction config).
 * Negative: counter-clockwise.
 * Zero: no movement.
 *
 * The accumulated value is cleared after reading.
 */
int16_t encoder_driver_get_diff(void)
{
    if (!s_encoder_initialized) 
    {
        return 0;
    }

    portENTER_CRITICAL(&s_encoder_state_lock );

    int16_t diff = s_rotation.unread_rotation_steps;
    s_rotation.unread_rotation_steps = 0;

    portEXIT_CRITICAL(&s_encoder_state_lock );

    return diff;
}

/**
 * @brief Get debounced encoder switch state.
 *
 * @return true when SW is pressed.
 */
bool encoder_driver_is_pressed(void)
{
    if (!s_encoder_initialized) 
    {
        return false;
    }
    portENTER_CRITICAL(&s_encoder_state_lock );
    bool pressed = s_button.debounced_pressed;
    portEXIT_CRITICAL(&s_encoder_state_lock );
    return pressed;
}
