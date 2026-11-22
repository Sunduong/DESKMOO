#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tune after hardware testing */
#define ENCODER_EDGES_PER_DETENT   2
#define ENCODER_REVERSE_DIRECTION  0
#define ENCODER_SW_DEBOUNCE_MS     30

esp_err_t encoder_driver_init(void);
int16_t encoder_driver_get_diff(void);
bool encoder_driver_is_pressed(void);

#ifdef __cplusplus
}
#endif

#endif // ENCODER_DRIVER_H
