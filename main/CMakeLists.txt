idf_component_register(SRCS "mascot_renderer.c" "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES spi_bus lcd board esp_timer)
