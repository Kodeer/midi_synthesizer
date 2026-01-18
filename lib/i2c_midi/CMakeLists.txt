# I2C MIDI Library for PCF8574 GPIO Extender

add_library(i2c_midi
    i2c_midi.c
)

target_include_directories(i2c_midi PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(i2c_midi
    pico_stdlib
    hardware_i2c
    hardware_gpio
)
