set(ui_dir "${CMAKE_CURRENT_LIST_DIR}/../ui/DESKMOO_UI")
include("${ui_dir}/file_list_gen.cmake")

idf_component_register(
    SRCS "main.c" "mascot_renderer.c" ${LV_EDITOR_PROJECT_SOURCES}
    INCLUDE_DIRS "." "${ui_dir}"
    REQUIRES spi_bus lcd board esp_timer encoder navigation
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
