# Configuration Settings Module

This module provides persistent storage and management of all global configuration settings for the MIDI synthesizer using an AT24CXX EEPROM.

## Features

- **Persistent Storage**: All settings stored in non-volatile EEPROM memory
- **Auto-initialization**: On boot, loads from EEPROM or creates defaults
- **Data Integrity**: CRC16 checksum validation
- **MIDI SysEx Ready**: Easy integration with MIDI SysEx parameter updates
- **Atomic Updates**: Each setting change is immediately saved
- **Version Management**: Configuration versioning for future compatibility

## Configuration Structure

### MIDI Settings
- **MIDI Channel** (1-16): MIDI channel to listen to
- **Note Range** (1-16): Number of notes to handle
- **Low Note** (0-127): Lowest MIDI note to respond to
- **Semitone Mode** (0-2): How to handle semitone notes
  - 0 = PLAY: Play semitones normally
  - 1 = IGNORE: Ignore semitone notes
  - 2 = SKIP: Skip to next whole tone

### I2C IO Expander Settings
- **IO Expander Type**: 0=PCF8574, 1=CH423
- **IO Expander Address**: I2C address of the expander (e.g., 0x20)

### Display Settings
- **Display Enabled**: 0=disabled, 1=enabled
- **Display Brightness**: 0-255 brightness level
- **Display Timeout**: Timeout in seconds (0=never)

## Usage

### Initialization

```c
#include "configuration_settings.h"

config_manager_t config_mgr;

// Initialize with EEPROM parameters
// AT24C32 = 4KB capacity, address 0x50, start at address 0x0000
bool success = config_init(&config_mgr, i2c0, 0x50, 4, 0x0000);
if (!success) {
    // Handle initialization failure
}
```

**Boot Logic:**
1. Attempts to read configuration from EEPROM
2. Validates magic number and CRC
3. If invalid or not found, loads defaults
4. Saves defaults to EEPROM for next boot

### Reading Settings

```c
// Get pointer to current settings
config_settings_t *settings = config_get_settings(&config_mgr);

// Use settings
uint8_t channel = settings->midi_channel;
uint8_t low_note = settings->low_note;
uint8_t note_range = settings->note_range;
```

### Updating Settings via MIDI SysEx

When a MIDI SysEx message is received with configuration changes:

```c
// Example: SysEx message to set MIDI channel to 5
// Format: F0 7D [param] [value] F7
// Where param 0 = MIDI channel

void handle_sysex_config(uint8_t param, uint8_t value) {
    switch (param) {
        case 0: // MIDI Channel
        case 1: // Note Range
        case 2: // Low Note
        case 3: // Semitone Mode
            config_update_midi_setting(&config_mgr, param, value);
            break;
            
        // Add more cases for other parameter types
    }
}
```

### Manual Setting Updates

```c
// Update MIDI settings
config_update_midi_setting(&config_mgr, 0, 10);  // Set channel to 10
config_update_midi_setting(&config_mgr, 1, 8);   // Set note range to 8
config_update_midi_setting(&config_mgr, 2, 60);  // Set low note to Middle C
config_update_midi_setting(&config_mgr, 3, 1);   // Set semitone mode to IGNORE

// Update IO expander settings
config_update_io_settings(&config_mgr, 0, 0x20); // PCF8574 at 0x20

// Update display settings
config_update_display_settings(&config_mgr, 1, 128, 30); // Enabled, medium brightness, 30s timeout
```

Each update function:
1. Validates the new value
2. Updates the RAM copy
3. Calculates new CRC
4. Writes to EEPROM
5. Returns success/failure

### Reset to Defaults

```c
// Erase EEPROM and restore factory defaults
config_erase(&config_mgr);
```

## EEPROM Memory Layout

```
Address Range    | Size    | Content
-----------------|---------|------------------------------------------
0x0000-0x0003   | 4 bytes | Magic Number (0x4D53594E = "MSYN")
0x0004          | 1 byte  | Version
0x0005          | 1 byte  | MIDI Channel
0x0006          | 1 byte  | Note Range
0x0007          | 1 byte  | Low Note
0x0008          | 1 byte  | Semitone Mode
0x0009          | 1 byte  | IO Expander Type
0x000A          | 1 byte  | IO Expander Address
0x000B          | 1 byte  | Display Enabled
0x000C          | 1 byte  | Display Brightness
0x000D          | 1 byte  | Display Timeout
0x000E-0x001D   | 16 bytes| Reserved for future use
0x001E-0x001F   | 2 bytes | CRC16 Checksum

Total: 32 bytes
```

## Default Values

| Parameter | Default | Description |
|-----------|---------|-------------|
| MIDI Channel | 10 | Percussion channel |
| Note Range | 8 | Handle 8 notes |
| Low Note | 60 | Middle C (C4) |
| Semitone Mode | 0 | PLAY mode |
| IO Expander Type | 0 | PCF8574 |
| IO Expander Address | 0x20 | Default I2C address |
| Display Enabled | 1 | Enabled |
| Display Brightness | 128 | Medium brightness |
| Display Timeout | 30 | 30 seconds |

## MIDI SysEx Integration

### Recommended SysEx Format

```
F0 7D [device_id] [param_id] [value] F7
```

- `F0`: SysEx start
- `7D`: Manufacturer ID (Educational/Development)
- `device_id`: Device identifier (optional)
- `param_id`: Parameter to change
- `value`: New value
- `F7`: SysEx end

### Parameter IDs

```c
// MIDI Settings (0x00-0x0F)
#define SYSEX_PARAM_MIDI_CHANNEL    0x00
#define SYSEX_PARAM_NOTE_RANGE      0x01
#define SYSEX_PARAM_LOW_NOTE        0x02
#define SYSEX_PARAM_SEMITONE_MODE   0x03

// IO Settings (0x10-0x1F)
#define SYSEX_PARAM_IO_TYPE         0x10
#define SYSEX_PARAM_IO_ADDRESS      0x11

// Display Settings (0x20-0x2F)
#define SYSEX_PARAM_DISPLAY_ENABLE  0x20
#define SYSEX_PARAM_DISPLAY_BRIGHT  0x21
#define SYSEX_PARAM_DISPLAY_TIMEOUT 0x22

// System Commands (0xF0-0xFF)
#define SYSEX_CMD_RESET_DEFAULTS    0xF0
#define SYSEX_CMD_SAVE_CONFIG       0xF1
#define SYSEX_CMD_REQUEST_CONFIG    0xF2
```

### Example SysEx Handler

```c
void process_sysex_message(const uint8_t *data, uint16_t length) {
    // Basic validation
    if (length < 5 || data[0] != 0xF0 || data[length-1] != 0xF7) {
        return; // Invalid SysEx
    }
    
    if (data[1] != 0x7D) {
        return; // Not our manufacturer ID
    }
    
    uint8_t param_id = data[2];
    uint8_t value = data[3];
    
    // MIDI settings
    if (param_id <= 0x03) {
        config_update_midi_setting(&config_mgr, param_id, value);
    }
    // IO settings
    else if (param_id == 0x10) {
        uint8_t address = data[4]; // Get address from next byte
        config_update_io_settings(&config_mgr, value, address);
    }
    // Display settings
    else if (param_id >= 0x20 && param_id <= 0x22) {
        // Handle display settings
    }
    // System commands
    else if (param_id == 0xF0) {
        config_erase(&config_mgr); // Reset to defaults
    }
}
```

## Error Handling

All functions return `bool` indicating success/failure:

```c
if (!config_update_midi_setting(&config_mgr, 0, 16)) {
    debug_error("Failed to update MIDI channel");
    // Handle error - setting not saved
}
```

## Data Integrity

- **CRC16**: All data protected by CRC16 checksum
- **Magic Number**: Verifies valid configuration data
- **Version Field**: Allows future compatibility
- **Range Validation**: All values validated before saving

## Memory Requirements

- **RAM**: ~32 bytes for settings structure
- **EEPROM**: 32 bytes per configuration
- **Code**: ~2-3 KB compiled size

## Hardware Requirements

- **EEPROM**: AT24CXX series (1KB-512KB)
- **I2C**: Shared with other I2C devices
- **Address**: Configurable (default 0x50)

## Example: Complete Integration

```c
#include "configuration_settings.h"
#include "i2c_midi.h"

config_manager_t config_mgr;
i2c_midi_t midi_ctx;

int main() {
    // Initialize I2C
    i2c_init(i2c0, 100000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    
    // Initialize configuration manager
    // AT24C32 (4KB) at address 0x50, start at 0x0000
    config_init(&config_mgr, i2c0, 0x50, 4, 0x0000);
    
    // Get current settings
    config_settings_t *settings = config_get_settings(&config_mgr);
    
    // Initialize MIDI with loaded settings
    i2c_midi_config_t midi_config = {
        .note_range = settings->note_range,
        .low_note = settings->low_note,
        .midi_channel = settings->midi_channel,
        .io_address = settings->io_expander_address,
        .i2c_port = i2c0,
        .io_type = settings->io_expander_type,
        .semitone_mode = settings->semitone_mode
    };
    
    i2c_midi_init_with_config(&midi_ctx, &midi_config, 4, 5, 100000);
    
    // Main loop
    while (1) {
        // Handle MIDI messages
        // When SysEx received, update config
        // Settings automatically saved to EEPROM
    }
}
```

## Troubleshooting

### Configuration Not Persisting
- Check EEPROM I2C address is correct
- Verify I2C bus is functioning
- Check debug output for write errors
- Ensure adequate write delay (5ms per write)

### Invalid Configuration on Boot
- EEPROM may be corrupted
- Use `config_erase()` to reset
- Check CRC calculation
- Verify magic number

### Settings Reverting to Defaults
- EEPROM write may be failing
- Check return values from update functions
- Verify EEPROM capacity is sufficient
- Check I2C bus stability

## Debug Output

Enable debug output to see configuration operations:

```
CONFIG: Initializing configuration manager...
CONFIG: EEPROM initialized (address=0x50, capacity=4KB, start=0x0000)
CONFIG: Loading configuration from EEPROM (address=0x0000)...
CONFIG: Validation passed
CONFIG: Configuration loaded successfully
CONFIG: Ch:10, Notes:60-67, Mode:0, IO:0x20
```

## Future Enhancements

- Multiple configuration profiles
- Configuration backup/restore
- Remote configuration via MIDI
- Configuration export/import
- Wear leveling for EEPROM longevity
