set(app_src encoder_driver.c)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio board esp_timer)
