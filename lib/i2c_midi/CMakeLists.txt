# I2C MIDI Library with IO Expander Support

# Options to select which drivers to compile
option(USE_PCF857X_DRIVER "Enable PCF857x (PCF8574/PCF8575) driver" ON)
option(USE_CH423_DRIVER "Enable CH423 driver" ON)

# Ensure at least one driver is enabled
if(NOT USE_PCF857X_DRIVER AND NOT USE_CH423_DRIVER)
    message(FATAL_ERROR "At least one I2C driver must be enabled (USE_PCF857X_DRIVER or USE_CH423_DRIVER)")
endif()

# Build list of driver sources
set(DRIVER_SOURCES)
if(USE_PCF857X_DRIVER)
    list(APPEND DRIVER_SOURCES drivers/pcf857x_driver.c)
    message(STATUS "I2C MIDI: PCF857x driver enabled")
endif()

if(USE_CH423_DRIVER)
    list(APPEND DRIVER_SOURCES drivers/ch423_driver.c)
    message(STATUS "I2C MIDI: CH423 driver enabled")
endif()

add_library(i2c_midi
    i2c_midi.c
    ${DRIVER_SOURCES}
)

target_include_directories(i2c_midi PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add compile definitions for enabled drivers
if(USE_PCF857X_DRIVER)
    target_compile_definitions(i2c_midi PUBLIC USE_PCF857X_DRIVER=1)
endif()

if(USE_CH423_DRIVER)
    target_compile_definitions(i2c_midi PUBLIC USE_CH423_DRIVER=1)
endif()

target_link_libraries(i2c_midi
    pico_stdlib
    hardware_i2c
    hardware_gpio
)
