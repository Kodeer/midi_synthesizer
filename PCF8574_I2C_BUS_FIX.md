# PCF8574 I2C Bus Initialization Fix

## Problem Description

The PCF8574 I/O expander LEDs stopped working after implementing the menu system. The output pins would stay OFF even when MIDI notes were received.

## Root Cause

The I2C bus was being **initialized multiple times**:
1. First in `midi_handler_init()` at line 325
2. Again in `i2c_midi_init_with_config()` at line 232
3. Or again in `i2c_midi_init()` at line 183

This repeated re-initialization of the I2C bus was causing conflicts and preventing the PCF8574 from responding correctly.

## I2C Device Addresses

The following I2C devices are on the same I2C1 bus (GPIO 2=SDA, GPIO 3=SCL):
- **AT24C32 EEPROM**: 0x50
- **SSD1306 OLED**: 0x3C
- **PCF8574 I/O Expander**: 0x20 (default, configurable)

There are no address conflicts. The problem was the repeated bus initialization.

## Solution

Modified the `i2c_midi_init()` and `i2c_midi_init_with_config()` functions in [lib/i2c_midi/i2c_midi.c](lib/i2c_midi/i2c_midi.c) to **NOT** re-initialize the I2C bus.

### Changes Made

#### File: `lib/i2c_midi/i2c_midi.c`

**Before:**
```c
// Initialize I2C
i2c_init(i2c_port, baudrate);
gpio_set_function(sda_pin, GPIO_FUNC_I2C);
gpio_set_function(scl_pin, GPIO_FUNC_I2C);
gpio_pull_up(sda_pin);
gpio_pull_up(scl_pin);
```

**After:**
```c
// NOTE: I2C bus should already be initialized by the caller (e.g., midi_handler_init)
// We do NOT re-initialize it here to avoid bus conflicts
debug_info("I2C_MIDI: Using pre-initialized I2C bus (assumed %d Hz)", baudrate);
```

This change was applied to both:
- `i2c_midi_init()` function (line ~183)
- `i2c_midi_init_with_config()` function (line ~232)

Also improved error checking in `i2c_midi_init_with_config()` to check the return value from `pcf857x_init()`.

#### File: `src/midi_handler.c`

Updated comments to reflect that the I2C bus will NOT be re-initialized by the i2c_midi functions:
- Removed outdated comment: `"i2c_midi_init_with_config will re-init (harmless)"`
- Replaced with: `"I2C bus is already initialized above"`

## Initialization Sequence (Correct Order)

The proper initialization order in `midi_handler_init()` is now:

1. **Initialize I2C bus** (GPIO pins, pull-ups, frequency) ← ONCE ONLY
2. **Initialize EEPROM** via `config_init()`
3. **Initialize PCF8574** via `i2c_midi_init()` or `i2c_midi_init_with_config()`
4. **Initialize OLED** via display handler (if used)

All devices share the same pre-initialized I2C bus without any re-initialization.

## Testing

After this fix:
- The PCF8574 should respond correctly to MIDI notes
- LEDs on the PCF8574 output pins should light up when notes are played
- All I2C devices (EEPROM, OLED, PCF8574) should work together without conflicts

## Debug Output

With debug UART enabled, you should see:
```
MIDI Handler: I2C bus initialized at 100000 Hz
CONFIG: EEPROM initialized (address=0x50, capacity=4KB, start=0x0000)
I2C_MIDI: Using pre-initialized I2C bus (assumed 100000 Hz)
PCF8574: Initialized at address 0x20 (8 pins)
PCF8574: Device detected and responding
```

If the PCF8574 is not connected, you'll see:
```
PCF8574: Warning - device not responding (may not be connected)
```

This is non-fatal and the system will continue to work with other I2C devices.

## Related Files

- [lib/i2c_midi/i2c_midi.c](lib/i2c_midi/i2c_midi.c) - Main fix location
- [src/midi_handler.c](src/midi_handler.c) - I2C bus initialization
- [lib/i2c_midi/drivers/pcf857x_driver.c](lib/i2c_midi/drivers/pcf857x_driver.c) - PCF8574 driver
- [lib/i2c_midi/drivers/pcf857x_driver.h](lib/i2c_midi/drivers/pcf857x_driver.h) - PCF8574 definitions

## Build Status

✅ Build successful
✅ UF2 file generated at: `build/midi_synthesizer.uf2`

## Next Steps

1. Copy the new `build/midi_synthesizer.uf2` to your Raspberry Pi Pico
2. Reconnect the UART to see debug output
3. Test MIDI note input to verify PCF8574 LEDs are working
4. Verify all I2C devices are functioning correctly
