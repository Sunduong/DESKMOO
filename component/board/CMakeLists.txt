set(app_src board_config.c)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio)
