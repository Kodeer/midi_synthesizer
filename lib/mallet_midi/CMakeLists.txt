add_library(mallet_midi STATIC
    ${CMAKE_CURRENT_LIST_DIR}/mallet_midi.c
)

target_include_directories(mallet_midi PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(mallet_midi PUBLIC
    hardware_gpio
    hardware_pwm
    pico_stdlib
    pico_time
)

# Enable debug output
target_compile_definitions(mallet_midi PUBLIC
    DEBUG_UART_ENABLED=1
)
