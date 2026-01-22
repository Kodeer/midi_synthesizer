# I2C Memory Library

This library provides drivers for I2C-based memory devices (EEPROM, FRAM, etc.).

## Supported Devices

### AT24CXX EEPROM Driver
Driver for Microchip AT24CXX series I2C EEPROM chips.

**Supported Capacities:**
- AT24C01: 1KB (128 bytes)
- AT24C02: 2KB (256 bytes)
- AT24C04: 4KB (512 bytes)
- AT24C08: 8KB (1024 bytes)
- AT24C16: 16KB (2048 bytes)
- AT24C32: 32KB (4096 bytes)
- AT24C64: 64KB (8192 bytes)
- AT24C128: 128KB (16384 bytes)
- AT24C256: 256KB (32768 bytes)
- AT24C512: 512KB (65536 bytes)

## Features

- Automatic page size configuration based on capacity
- Automatic address mode selection (1-byte or 2-byte)
- Page write optimization (handles page boundaries automatically)
- Sequential read support
- Single byte read/write operations
- Full EEPROM erase function
- Debug output support

## Hardware Connections

### AT24CXX EEPROM:
```
Pico              AT24CXX
----              -------
3.3V     ------>  VCC
GND      ------>  GND
GPIO X   ------>  SDA
GPIO Y   ------>  SCL
                  
AT24CXX Pins:
A0, A1, A2  ---> Address configuration (GND or VCC)
WP          ---> Write Protect (connect to GND to enable writes)
```

## Usage

### Basic Initialization

```c
#include "drivers/at24cxx_driver.h"

at24cxx_t eeprom;

// Initialize 4KB EEPROM at address 0x50
at24cxx_init(&eeprom, i2c0, 0x50, 4);
```

### Write Single Byte

```c
// Write byte 0x42 to address 0x0100
at24cxx_write_byte(&eeprom, 0x0100, 0x42);
```

### Read Single Byte

```c
uint8_t data;
at24cxx_read_byte(&eeprom, 0x0100, &data);
printf("Read: 0x%02X\n", data);
```

### Write Multiple Bytes

```c
uint8_t buffer[64] = {0x01, 0x02, 0x03, /* ... */};

// Write 64 bytes starting at address 0x0000
// Automatically handles page boundaries
at24cxx_write(&eeprom, 0x0000, buffer, 64);
```

### Read Multiple Bytes

```c
uint8_t buffer[128];

// Read 128 bytes starting at address 0x0200
at24cxx_read(&eeprom, 0x0200, buffer, 128);
```

### Erase EEPROM

```c
// Write 0xFF to all locations
at24cxx_erase(&eeprom);
```

### Get Capacity

```c
uint32_t capacity = at24cxx_get_capacity(&eeprom);
printf("EEPROM capacity: %d bytes\n", capacity);
```

## AT24CXX I2C Addresses

The AT24CXX I2C address is determined by the A0, A1, A2 pins:

| A2 | A1 | A0 | Address |
|----|----|----|---------|
| 0  | 0  | 0  | 0x50    |
| 0  | 0  | 1  | 0x51    |
| 0  | 1  | 0  | 0x52    |
| 0  | 1  | 1  | 0x53    |
| 1  | 0  | 0  | 0x54    |
| 1  | 0  | 1  | 0x55    |
| 1  | 1  | 0  | 0x56    |
| 1  | 1  | 1  | 0x57    |

## Page Write Considerations

AT24CXX EEPROMs have page write limits:
- Writing across page boundaries requires multiple write cycles
- The driver automatically handles page boundaries
- Each write cycle takes approximately 5ms

**Page Sizes:**
- 1KB, 2KB: 8 bytes
- 4KB, 8KB, 16KB: 16 bytes
- 32KB: 32 bytes
- 64KB+: 64 bytes

## Example: Configuration Storage

```c
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "drivers/at24cxx_driver.h"

typedef struct {
    uint8_t midi_channel;
    uint8_t note_range;
    uint8_t low_note;
    uint8_t semitone_mode;
} config_t;

int main() {
    // Initialize I2C
    i2c_init(i2c0, 100000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);
    
    // Initialize 4KB EEPROM
    at24cxx_t eeprom;
    at24cxx_init(&eeprom, i2c0, 0x50, 4);
    
    // Save configuration
    config_t config = {
        .midi_channel = 10,
        .note_range = 8,
        .low_note = 60,
        .semitone_mode = 2
    };
    
    at24cxx_write(&eeprom, 0x0000, (uint8_t*)&config, sizeof(config_t));
    
    // Load configuration
    config_t loaded_config;
    at24cxx_read(&eeprom, 0x0000, (uint8_t*)&loaded_config, sizeof(config_t));
    
    return 0;
}
```

## Technical Notes

### Write Cycle Time
- Typical: 5ms
- Maximum: 10ms
- The driver uses 5ms delay after each write

### Endurance
- Write cycles: 1,000,000 typical
- Data retention: 100 years (25°C)
- Note: Avoid excessive writes to the same location

### Address Modes
- **1-byte address**: Used for ≤2KB EEPROMs
- **2-byte address**: Used for >2KB EEPROMs
- The driver automatically selects the correct mode

## Error Handling

All functions return `bool`:
- `true`: Operation successful
- `false`: Operation failed (check debug output for details)

Common failure reasons:
- I2C communication error
- Address out of range
- Invalid parameters
- EEPROM not connected

## License

This library is part of the midi_synthesizer project for Raspberry Pi Pico.
