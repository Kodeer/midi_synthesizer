# I2C GPIO Expander Drivers

This directory contains low-level drivers for I2C GPIO expander chips.

## PCF857x Driver (pcf857x_driver.h/c)

Unified driver supporting both PCF8574 (8-bit) and PCF8575 (16-bit) GPIO expanders.

### Features
- Single API for both PCF8574 (8-bit) and PCF8575 (16-bit)
- Automatic byte width handling (1 byte for PCF8574, 2 bytes for PCF8575)
- Pin state tracking and readback
- Individual pin control with get/set operations
- Runtime chip type selection

### Supported Chips

#### PCF8574 (8-bit)
- 8 bidirectional I/O pins (P0-P7)
- Single byte I2C read/write
- Default address: 0x20
- Compatible with: PCF8574, PCF8574A

#### PCF8575 (16-bit)
- 16 bidirectional I/O pins (P00-P07, P10-P17)
- Two byte I2C read/write (low byte, high byte)
- Default address: 0x20
- Compatible with: PCF8575, PCF8575C

### Usage Example

```c
#include "drivers/pcf857x_driver.h"

pcf857x_t pcf8574_ctx;
pcf857x_t pcf8575_ctx;

// Initialize PCF8574 (8-bit)
pcf857x_init(&pcf8574_ctx, i2c0, 0x20, PCF8574_CHIP);

// Initialize PCF8575 (16-bit)
pcf857x_init(&pcf8575_ctx, i2c0, 0x21, PCF8575_CHIP);

// Write data (works for both)
pcf857x_write(&pcf8574_ctx, 0x55);     // 8-bit: 0b01010101
pcf857x_write(&pcf8575_ctx, 0xAAAA);   // 16-bit: 0b1010101010101010

// Set individual pins
pcf857x_set_pin(&pcf8574_ctx, 0, true);  // Set pin 0 HIGH
pcf857x_set_pin(&pcf8575_ctx, 15, true); // Set pin 15 HIGH (PCF8575 only)

// Get number of pins
uint8_t num_pins = pcf857x_get_num_pins(&pcf8574_ctx); // Returns 8 or 16
```

### Pin Mapping

#### PCF8574 (8-bit)
| Pin Number | Chip Pin | Bit Position |
|------------|----------|--------------|
| 0          | P0       | Bit 0        |
| 1          | P1       | Bit 1        |
| 2          | P2       | Bit 2        |
| 3          | P3       | Bit 3        |
| 4          | P4       | Bit 4        |
| 5          | P5       | Bit 5        |
| 6          | P6       | Bit 6        |
| 7          | P7       | Bit 7        |

#### PCF8575 (16-bit)
| Pin Number | Chip Pin | Bit Position |
|------------|----------|--------------|
| 0          | P00      | Bit 0        |
| 1          | P01      | Bit 1        |
| 2          | P02      | Bit 2        |
| 3          | P03      | Bit 3        |
| 4          | P04      | Bit 4        |
| 5          | P05      | Bit 5        |
| 6          | P06      | Bit 6        |
| 7          | P07      | Bit 7        |
| 8          | P10      | Bit 8        |
| 9          | P11      | Bit 9        |
| 10         | P12      | Bit 10       |
| 11         | P13      | Bit 11       |
| 12         | P14      | Bit 12       |
| 13         | P15      | Bit 13       |
| 14         | P16      | Bit 14       |
| 15         | P17      | Bit 15       |

### I2C Protocol

**PCF8574 Write (1 byte):**
```
START | ADDR+W | DATA | STOP
```

**PCF8574 Read (1 byte):**
```
START | ADDR+R | DATA | STOP
```

**PCF8575 Write (2 bytes):**
```
START | ADDR+W | DATA_LOW | DATA_HIGH | STOP
```

**PCF8575 Read (2 bytes):**
```
START | ADDR+R | DATA_LOW | DATA_HIGH | STOP
```

## CH423 Driver (ch423_driver.h/c)

16-bit GPIO expander with separate open-collector (OC) and push-pull (PP) outputs.

### Features
- 16 GPIO pins (8 OC + 8 PP outputs)
- Open-collector outputs (OC0-OC7) for higher current loads
- Push-pull outputs (PP0-PP7) for standard digital outputs
- Command-based I2C protocol
- Individual and bulk pin control

### Usage Example

```c
#include "drivers/ch423_driver.h"

ch423_t ch423_ctx;

// Initialize
ch423_init(&ch423_ctx, i2c0, CH423_DEFAULT_ADDRESS);

// Write to OC outputs (pins 0-7)
ch423_write_oc(&ch423_ctx, 0xFF);  // All OC pins HIGH

// Write to PP outputs (pins 8-15)
ch423_write_pp(&ch423_ctx, 0x00);  // All PP pins LOW

// Set individual pin
ch423_set_pin(&ch423_ctx, 5, true);   // Set OC5 HIGH
ch423_set_pin(&ch423_ctx, 10, false); // Set PP2 LOW
```

### Pin Mapping

| Pin Number | Output Type | Chip Pin |
|------------|-------------|----------|
| 0-7        | OC          | OC0-OC7  |
| 8-15       | PP          | PP0-PP7  |

### I2C Commands

| Command       | Byte | Description                    |
|---------------|------|--------------------------------|
| WRITE_OC      | 0x01 | Write to open-collector pins   |
| WRITE_PP      | 0x02 | Write to push-pull pins        |
| READ_IO       | 0x03 | Read all input states          |
| SET_IO        | 0x04 | Configure I/O direction        |

## Adding New Drivers

To add support for a new GPIO expander:

1. Create `new_driver.h` and `new_driver.c` in this directory
2. Define the driver context structure
3. Implement the driver API:
   - `init()` - Initialize the device
   - `write()` - Write to all pins
   - `read()` - Read from all pins
   - `set_pin()` - Set individual pin
   - `get_pin_state()` - Get pin state
4. Add driver to `i2c_midi.h` enum and union
5. Update switch statements in `i2c_midi.c`
6. Update CMakeLists.txt if needed
7. Document in this README

## References

- [PCF8574 Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [PCF8575 Datasheet](https://www.ti.com/lit/ds/symlink/pcf8575.pdf)
- [CH423 Datasheet](http://www.wch-ic.com/products/CH423.html)
