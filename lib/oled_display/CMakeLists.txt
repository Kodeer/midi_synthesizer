add_library(oled_display INTERFACE)

target_sources(oled_display INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/oled_display.c
    ${CMAKE_CURRENT_LIST_DIR}/lissajous_screensaver.c
)

target_include_directories(oled_display INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(oled_display INTERFACE
    hardware_i2c
    pico_stdlib
)
