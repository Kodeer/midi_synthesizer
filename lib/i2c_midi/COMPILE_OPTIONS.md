# I2C MIDI Library - Compile Options

## Driver Selection

The I2C MIDI library supports conditional compilation of drivers to reduce code size. You can choose which I2C GPIO expander drivers to include in your build.

## Available Drivers

1. **PCF857x Driver** (`USE_PCF857X_DRIVER`)
   - Supports PCF8574 (8-bit) and PCF8575 (16-bit) GPIO expanders
   - Default I2C address: 0x20

2. **CH423 Driver** (`USE_CH423_DRIVER`)
   - Supports CH423 16-bit GPIO expander
   - Separate open-collector and push-pull outputs
   - Default I2C address: 0x24

## CMake Options

By default, **both drivers are enabled**. You can disable drivers you don't need using CMake options.

### Option Syntax

```cmake
option(USE_PCF857X_DRIVER "Enable PCF857x (PCF8574/PCF8575) driver" ON)
option(USE_CH423_DRIVER "Enable CH423 driver" ON)
```

## How to Configure

### Method 1: Edit CMakeLists.txt

Edit the main project's `CMakeLists.txt` or `lib/i2c_midi/CMakeLists.txt` before the library is added:

```cmake
# Disable CH423 driver, keep only PCF857x
set(USE_PCF857X_DRIVER ON CACHE BOOL "Enable PCF857x driver")
set(USE_CH423_DRIVER OFF CACHE BOOL "Disable CH423 driver")

add_subdirectory(lib/i2c_midi)
```

### Method 2: CMake Command Line

When configuring your build, pass the options via command line:

```bash
cmake -DUSE_PCF857X_DRIVER=ON -DUSE_CH423_DRIVER=OFF ..
```

### Method 3: CMake GUI

If using CMake GUI:
1. Configure your project
2. Look for `USE_PCF857X_DRIVER` and `USE_CH423_DRIVER` options
3. Check/uncheck as needed
4. Reconfigure and generate

## Build Configurations

### Both Drivers (Default)
```cmake
set(USE_PCF857X_DRIVER ON)
set(USE_CH423_DRIVER ON)
```
**Size:** ~79 KB (full functionality)

### PCF857x Only
```cmake
set(USE_PCF857X_DRIVER ON)
set(USE_CH423_DRIVER OFF)
```
**Size:** ~78 KB (saves ~1 KB)
**Use case:** Using PCF8574 or PCF8575 only

### CH423 Only
```cmake
set(USE_PCF857X_DRIVER OFF)
set(USE_CH423_DRIVER ON)
```
**Size:** ~78 KB (saves ~1 KB)
**Use case:** Using CH423 only

## Compilation Rules

1. **At least one driver must be enabled**
   - Build will fail if both drivers are disabled
   - Error: "At least one I2C driver must be enabled"

2. **Enum values change based on enabled drivers**
   - With both: `IO_EXPANDER_PCF8574 = 0`, `IO_EXPANDER_CH423 = 1`
   - PCF857x only: `IO_EXPANDER_PCF8574 = 0`
   - CH423 only: `IO_EXPANDER_CH423 = 1`

3. **Default driver selection**
   - If PCF857x enabled → defaults to PCF8574
   - If only CH423 enabled → defaults to CH423

## Code Impact

### What Gets Excluded

When a driver is disabled:
- Driver source file (`.c`) is not compiled
- Driver header include is conditionally excluded
- Driver enum value is not defined
- Driver case statements in switch blocks are removed
- Union member for driver context may be excluded

### Example: PCF857x Only Build

```c
// Only these are compiled:
#ifdef USE_PCF857X_DRIVER
    #include "drivers/pcf857x_driver.h"
    
    typedef enum {
        IO_EXPANDER_PCF8574 = 0,
    } io_expander_type_t;
    
    union {
        pcf857x_t pcf857x;
    } driver;
#endif
```

## Testing Your Configuration

After changing driver options:

1. **Clean the build:**
   ```bash
   rm -rf build/*
   ```

2. **Reconfigure:**
   ```bash
   cmake -B build
   ```

3. **Check the output:**
   ```
   -- I2C MIDI: PCF857x driver enabled
   -- I2C MIDI: CH423 driver enabled
   ```

4. **Build:**
   ```bash
   cmake --build build
   ```

5. **Verify size:**
   ```bash
   ls -lh build/midi_synthesizer.uf2
   ```

## Recommendations

- **Production builds:** Enable only the driver you're using to minimize code size
- **Development builds:** Enable all drivers for testing flexibility
- **Multi-board projects:** Create different build configurations per hardware variant

## Troubleshooting

### Error: "At least one I2C driver must be enabled"
**Cause:** Both `USE_PCF857X_DRIVER` and `USE_CH423_DRIVER` are OFF  
**Fix:** Enable at least one driver

### Error: "IO_EXPANDER_PCF8574 undeclared"
**Cause:** PCF857x driver is disabled but code references it  
**Fix:** Either enable the driver or update your code to use the enabled driver

### Error: "pcf857x_t does not name a type"
**Cause:** PCF857x driver is disabled but union references it  
**Fix:** This shouldn't happen with conditional compilation; check your CMake cache

### Build size didn't change
**Cause:** CMake cache still has old settings  
**Fix:** Delete `build/CMakeCache.txt` and reconfigure

## Example Project Configuration

### Main CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)

# Select only PCF857x driver for production
set(USE_PCF857X_DRIVER ON CACHE BOOL "Enable PCF857x driver")
set(USE_CH423_DRIVER OFF CACHE BOOL "Disable CH423 driver")

# Initialize Pico SDK
include(pico_sdk_import.cmake)

project(midi_synthesizer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Add I2C MIDI library (with configured drivers)
add_subdirectory(lib/i2c_midi)

# Your main executable
add_executable(midi_synthesizer
    src/midi_synthesizer.c
    # ... other sources
)

target_link_libraries(midi_synthesizer
    i2c_midi
    pico_stdlib
    # ... other libraries
)

pico_add_extra_outputs(midi_synthesizer)
```
