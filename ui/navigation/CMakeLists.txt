idf_component_register(
    SRCS "navigation_controller.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl__lvgl
    PRIV_REQUIRES encoder
)
