cmake_minimum_required(VERSION 3.13)

# I2C PCA9685 MIDI Library
add_library(i2c_pca9685_midi STATIC
    i2c_pca9685_midi.c
    drivers/pca9685_driver.c
)

target_include_directories(i2c_pca9685_midi PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(i2c_pca9685_midi
    pico_stdlib
    hardware_i2c
    hardware_gpio
)

# Optional: Enable debug output
# Uncomment to enable debug messages
# target_compile_definitions(i2c_pca9685_midi PRIVATE
#     PCA9685_DEBUG
#     PCA9685_MIDI_DEBUG
# )
