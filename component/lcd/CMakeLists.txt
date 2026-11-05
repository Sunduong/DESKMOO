set(app_src lcd_driver.c)
set(pri_req spi_bus esp_driver_spi esp_driver_gpio)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
