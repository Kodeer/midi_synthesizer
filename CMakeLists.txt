# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# CRITICAL: Tell SDK where to find our TinyUSB config BEFORE project()
set(PICO_CONFIG_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/src/tusb_config.h)
add_compile_definitions(CFG_TUSB_CONFIG_FILE="${CMAKE_CURRENT_LIST_DIR}/src/tusb_config.h")

# Configure TinyUSB before project - CRITICAL for MIDI support
set(FAMILY rp2040)
set(BOARD pico_sdk)
set(PICO_TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)

project(midi_synthesizer C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Configure I2C MIDI drivers (optional - defaults to both ON)
# Uncomment to disable drivers you don't need:
# set(USE_PCF857X_DRIVER OFF CACHE BOOL "Disable PCF857x driver" FORCE)
# set(USE_CH423_DRIVER OFF CACHE BOOL "Disable CH423 driver" FORCE)

# Add i2c_midi library subdirectory
add_subdirectory(lib/i2c_midi)

# Add oled_display library subdirectory
add_subdirectory(lib/oled_display)

# Add i2c_memory library subdirectory
add_subdirectory(lib/i2c_memory)

# Add buzzer library subdirectory
add_subdirectory(lib/buzzer)

# Add mallet_midi library subdirectory
add_subdirectory(lib/mallet_midi)

# Add i2c_pca9685_midi library subdirectory
add_subdirectory(lib/i2c_pca9685_midi)

# IMPORTANT: Set TinyUSB compile options BEFORE adding executable
set(PICO_TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)

# Add executable. Default name is the project name, version 0.1

add_executable(midi_synthesizer 
    src/midi_synthesizer.c
    src/usb_midi.c
    src/midi_handler.c
    src/debug_uart.c
    src/display_handler.c
    src/usb_descriptors.c
    src/configuration_settings.c
    src/button_handler.c
    src/menu_handler.c
)

# Add tusb_config.h directory
target_include_directories(midi_synthesizer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

pico_set_program_name(midi_synthesizer "midi_synthesizer")
pico_set_program_version(midi_synthesizer "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(midi_synthesizer 1)
pico_enable_stdio_usb(midi_synthesizer 0)

# Add the standard library to the build
target_link_libraries(midi_synthesizer
        pico_stdlib
        hardware_uart
        hardware_i2c
        hardware_pwm
        i2c_midi
        oled_display
        i2c_memory
        buzzer
        mallet_midi
        i2c_pca9685_midi)

# Link TinyUSB after stdlib so our config takes precedence
target_link_libraries(midi_synthesizer
        tinyusb_device
        tinyusb_board)

# Ensure TinyUSB finds our config - use compile definition
target_compile_definitions(midi_synthesizer PRIVATE
    CFG_TUSB_CONFIG_FILE="${CMAKE_CURRENT_LIST_DIR}/src/tusb_config.h"
)

# Add include directory
target_include_directories(midi_synthesizer PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Generate UF2 output
pico_add_extra_outputs(midi_synthesizer)

