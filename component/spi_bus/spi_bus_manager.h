#ifndef SPI_BUS_MANAGER_H
#define SPI_BUS_MANAGER_H

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "board_config.h"

/**
 * @brief Initialize the shared SPI bus.
 * Must be called ONCE before adding any devices (LCD, SD card).
 * Call this FIRST in app_main().
 *
 * @param host SPI host to use (e.g. SPI2_HOST)
 */
void spi_bus_init(spi_host_device_t host);
void spi_bus_all_cs_high(void);
void spi_bus_select_lcd(void);
void spi_bus_select_sd(void);

#endif // SPI_BUS_MANAGER_H
