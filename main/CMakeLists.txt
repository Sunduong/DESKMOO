idf_component_register(SRCS "main.c" "mascot_renderer.c"
                       INCLUDE_DIRS "."
                       REQUIRES spi_bus lcd board esp_timer)
