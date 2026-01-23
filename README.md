# Zoft Synthesizer V1.0

A USB MIDI to I2C MIDI converter for Raspberry Pi Pico with OLED display support and System Exclusive (SysEx) configuration.

## Overview

The Zoft Synthesizer receives MIDI messages via USB and outputs them through an I2C interface to control external devices via GPIO expanders (PCF8574/PCF8575 or CH423). It features real-time note display on an SSD1306 OLED screen and comprehensive debug output via UART.

## Features

- **USB MIDI Device**: Full USB MIDI support using TinyUSB
- **I2C MIDI Output**: Controls GPIO expanders:
  - PCF8574 (8-bit, address 0x20)
  - PCF8575 (16-bit, address 0x20)
  - CH423 (16-bit, address 0x24)
- **OLED Display**: 128x64 SSD1306 display showing:
  - Startup message: "Zoft Synthesizer V1.0"
  - Real-time MIDI note information (note name, velocity, channel)
  - Interactive menu system with inverted text highlighting
- **Button Interface**: Single button on GPIO 4 for menu navigation:
  - Short press: Cycle through menu options
  - Hold 3 seconds: Enter menu or execute selected option
- **Menu System**: 7 configurable options:
  1. Reset Defaults
  2. Save Config
  3. MIDI Channel (1-16)
  4. Note Range
  5. Semitone Mode (Natural/Sharp/Flat)
  6. All Notes Off
  7. Exit Menu
- **Configuration Storage**: EEPROM persistence (AT24C32, address 0x50)
- **Debug Output**: Comprehensive UART logging at 115200 baud
- **LED Feedback**: Visual indication of MIDI note activity
- **Configurable via SysEx**:
  - Note range filtering
  - MIDI channel selection
  - Semitone handling modes
- **Semitone Handling**: Three modes for chromatic scale handling

## Hardware Requirements

### Raspberry Pi Pico
- RP2040 microcontroller
- USB connection for MIDI and power

### I2C Devices (on I2C1 bus)
- **GPIO Expander** (choose one):
  - **PCF8574**: 8-bit I/O expander, Address 0x20
  - **PCF8575**: 16-bit I/O expander, Address 0x20
  - **CH423**: 16-bit I/O expander with OC/PP outputs, Address 0x24
  - SDA: GP2
  - SCL: GP3
- **SSD1306 OLED Display**: 128x64 pixels, Address 0x3C
  - Shares I2C1 bus
- **AT24C32 EEPROM**: 32K EEPROM for configuration storage, Address 0x50
  - Shares I2C1 bus

### Button Interface
- GPIO: GP4
- Function: Menu navigation and configuration
- Active: Low (internal pull-up enabled)

### Debug UART
- TX: GP0
- RX: GP1
- Baud Rate: 115200

### LED Feedback
- GPIO: GP25 (onboard LED)

## Building the Project

### Prerequisites
- Raspberry Pi Pico SDK 2.2.0 or later
- CMake 3.13+
- Ninja build system
- ARM GCC compiler

### Build Steps

```bash
cd midi_synthesizer
mkdir build
cd build
cmake .. -G Ninja
ninja
```

The build produces `midi_synthesizer.uf2` in the `build/` directory.

### Flashing
1. Hold BOOTSEL button on Pico while connecting USB
2. Copy `midi_synthesizer.uf2` to the RPI-RP2 drive
3. Pico will reboot automatically

## Hardware Configuration

All hardware settings are defined in `src/midi_synthesizer.c`:

```c
// Debug Configuration
#define DEBUG_ENABLED       true    // Enable/disable all debug output

// Debug UART Configuration
#define DEBUG_UART          uart0
#define DEBUG_UART_TX_PIN   0
#define DEBUG_UART_RX_PIN   1
#define DEBUG_UART_BAUD     115200

// I2C MIDI Configuration
#define I2C_MIDI_INSTANCE   i2c1
#define I2C_MIDI_SDA_PIN    2
#define I2C_MIDI_SCL_PIN    3
#define I2C_MIDI_FREQ       100000

// MIDI Semitone Handling
#define SEMITONE_MODE       I2C_MIDI_SEMITONE_SKIP
// Options: I2C_MIDI_SEMITONE_PLAY, I2C_MIDI_SEMITONE_IGNORE, I2C_MIDI_SEMITONE_SKIP

// OLED Display Configuration
#define OLED_I2C_INSTANCE   I2C_MIDI_INSTANCE  // Shares I2C bus

// LED Feedback Configuration
#define LED_PIN             25
```

## Semitone Handling Modes

The synthesizer supports three modes for handling semitones (black keys):

- **PLAY** (0): Play all notes including semitones
- **IGNORE** (1): Ignore semitone notes completely
- **SKIP** (2): Map semitones to the next full tone

## Button and Menu System

### Button Operation

The synthesizer features a single button on GPIO 4 for menu navigation:

- **Short Press** (< 3 seconds): Cycle to next menu option
- **Long Press** (≥ 3 seconds): 
  - If not in menu: Enter menu mode
  - If in menu: Execute selected option

### Menu Options

When you hold the button for 3 seconds, the menu system activates showing:

```
======= MENU =======

 2. Save Config      
█1. Reset Defaults  █  <- Selected (inverted display)
 3. MIDI Channel     
```

The menu provides 7 configuration options:

1. **Reset Defaults**: Restore factory settings
   - Channel: 1
   - Note Range: 60-84 (C4-C6)
   - Semitone Mode: SKIP
   - Requires reboot to apply

2. **Save Config**: Save current settings to EEPROM
   - Persists across power cycles
   - Shows "Config Saved!" confirmation

3. **MIDI Channel**: Cycle through channels 1-16
   - Immediate effect
   - Updates display temporarily

4. **Note Range**: Configure via SysEx messages
   - Menu displays "Use SysEx" reminder

5. **Semitone Mode**: Toggle between PLAY/IGNORE/SKIP
   - PLAY: All notes including sharps/flats
   - IGNORE: Skip semitone notes
   - SKIP: Map semitones to next natural note

6. **All Notes Off**: Emergency stop
   - Resets all I2C MIDI pins
   - Exits menu automatically

7. **Exit Menu**: Return to normal operation
   - Displays "Zoft Synthesizer V1"

### Visual Feedback

- **Inverted Text**: Selected menu item shows black text on white background
- **Numbered Items**: Each option numbered 1-7
- **Padded Width**: Uniform highlighting bar across all options
- **Centered Header**: Menu title centered with equal padding

### Configuration Storage

Settings saved via the menu are stored in AT24C32 EEPROM:
- Survives power cycles
- Loaded automatically on startup
- Defaults used if EEPROM not present

## System Exclusive (SysEx) Configuration

The Zoft Synthesizer can be configured in real-time using MIDI System Exclusive messages.

### SysEx Message Format

```
F0 7D 00 [Command] [Data...] F7
```

- `F0` - SysEx Start byte
- `7D` - Manufacturer ID (Educational/Development use)
- `00` - Device ID (Zoft Synthesizer)
- `[Command]` - Command byte
- `[Data...]` - Command-specific data bytes
- `F7` - SysEx End byte

### Supported Commands

#### 1. Set Note Range (0x01)
Configure the minimum and maximum MIDI note numbers the synthesizer will respond to.

**Format:**
```
F0 7D 00 01 [low_note] [high_note] F7
```

**Parameters:**
- `low_note`: Minimum MIDI note (0-127)
- `high_note`: Maximum MIDI note (0-127)

**Example:**
```
F0 7D 00 01 3C 54 F7
```
Sets range to 60-84 (Middle C to C6)

**MIDI Note Reference:**
- 60 (0x3C) = C4 (Middle C)
- 72 (0x48) = C5
- 84 (0x54) = C6

#### 2. Set MIDI Channel (0x02)
Configure which MIDI channel the synthesizer listens to.

**Format:**
```
F0 7D 00 02 [channel] F7
```

**Parameters:**
- `channel`: MIDI channel (0-15 for channels 1-16, 0xFF for all channels)

**Examples:**
```
F0 7D 00 02 09 F7    # Listen to channel 10 (0x09 = channel 10)
F0 7D 00 02 FF F7    # Listen to all channels
```

#### 3. Set Semitone Mode (0x03)
Configure how the synthesizer handles semitones (chromatic notes).

**Format:**
```
F0 7D 00 03 [mode] F7
```

**Parameters:**
- `mode`: Semitone handling mode
  - `0x00` = PLAY - Play all notes including semitones
  - `0x01` = IGNORE - Ignore semitone notes completely
  - `0x02` = SKIP - Map semitones to next full tone

**Examples:**
```
F0 7D 00 03 00 F7    # Play all notes
F0 7D 00 03 01 F7    # Ignore semitones
F0 7D 00 03 02 F7    # Skip semitones
```

#### 4. Query Configuration (0x10)
Request current configuration. Response is sent via UART debug output.

**Format:**
```
F0 7D 00 10 F7
```

**Response (via UART):**
```
[INFO] SysEx: Config - Ch:9, Range:60-84, Semitone:2
```

## Sending SysEx Messages

### Software Options

**Windows:**
- **MIDI-OX**: Industry standard MIDI utility
- **Bome SendSX**: Simple SysEx sender

**macOS:**
- **SysEx Librarian**: Drag-and-drop SysEx management
- **MIDI Monitor**: View and send MIDI messages

**Cross-platform:**
- **SendMIDI** (CLI): `sendmidi dev "Pico" hex syx F0 7D 00 01 3C 54 F7`

### Example: MIDI-OX (Windows)
1. Select your USB MIDI device
2. Open Command Window → SysEx
3. Enter hex bytes: `F0 7D 00 01 3C 54 F7`
4. Click "Send SysEx"

### Example: Bome SendSX
1. Load or create new SysEx file
2. Enter hex: `F0 7D 00 01 3C 54 F7`
3. Click "Send"

## Debug Output

Connect a serial terminal (115200 baud, 8N1) to UART0 (GP0/GP1) to view debug messages:

```
[INFO] MIDI Synthesizer Starting...
[INFO] MIDI Handler: I2C MIDI initialized (SDA=GP2, SCL=GP3, Freq=100000Hz)
[INFO] Display Handler: OLED Display initialized
[INFO] USB MIDI initialized
[INFO] Waiting for USB connection...
[MIDI] Note On: Ch=1, Note=60 (C4), Vel=100
[INFO] SysEx: Received 7 bytes:
 F0 7D 00 01 3C 54 F7
[INFO] SysEx: Note range set to 60-84
```

## Project Structure

```
midi_synthesizer/
├── src/
│   ├── midi_synthesizer.c      # Main application
│   ├── usb_midi.c/h            # USB MIDI interface
│   ├── midi_handler.c/h        # MIDI message processing & SysEx
│   ├── display_handler.c/h     # OLED display management
│   ├── button_handler.c/h      # Button debouncing & event handling
│   ├── menu_handler.c/h        # Menu system with OLED integration
│   ├── configuration_settings.c/h  # EEPROM configuration management
│   ├── debug_uart.c/h          # Debug logging
│   ├── usb_descriptors.c       # USB device descriptors
│   └── tusb_config.h           # TinyUSB configuration
├── lib/
│   ├── i2c_midi/               # I2C MIDI library (PCF857x/CH423)
│   │   ├── i2c_midi.c/h
│   │   ├── drivers/
│   │   │   ├── pcf857x_driver.c/h  # PCF8574/PCF8575 driver
│   │   │   └── ch423_driver.c/h    # CH423 driver
│   │   └── CMakeLists.txt
│   ├── i2c_memory/             # EEPROM library (AT24Cxx)
│   │   ├── drivers/
│   │   │   └── at24cxx_driver.c/h  # AT24C32 EEPROM driver
│   │   └── CMakeLists.txt
│   └── oled_display/           # SSD1306 OLED driver
│       ├── oled_display.c/h
│       └── CMakeLists.txt
├── CMakeLists.txt              # Build configuration
├── pico_sdk_import.cmake       # Pico SDK import
└── README.md                   # This file
```

## Module Descriptions

### USB MIDI Module (`usb_midi.c/h`)
- TinyUSB integration
- USB-MIDI packet handling
- SysEx message parsing (proper CIN handling)
- Callback-based architecture

### MIDI Handler (`midi_handler.c/h`)
- Central MIDI message router
- SysEx command processing
- I2C MIDI integration
- LED feedback control
- Configuration management

### Display Handler (`display_handler.c/h`)
- OLED display initialization
- Text rendering (normal and inverted)
- Note information display
- Menu rendering with highlighting

### Button Handler (`button_handler.c/h`)
- GPIO button interface with debouncing
- Short/long press detection (3-second threshold)
- Event-based callback system
- 50ms debounce timing

### Menu Handler (`menu_handler.c/h`)
- Interactive menu system
- 7 configuration options
- Inverted text highlighting for selection
- Scrolling menu display (3 items visible)
- Integration with display and MIDI handlers

### Configuration Settings (`configuration_settings.c/h`)
- EEPROM persistence layer
- Load/save configuration
- Factory defaults restoration
- AT24C32 EEPROM integration

### I2C MIDI Library (`lib/i2c_midi/`)
- Multi-driver GPIO expander support
- PCF857x driver (supports both PCF8574 8-bit and PCF8575 16-bit)
- CH423 driver (16-bit with OC/PP outputs)
- IO abstraction layer
- Note-to-GPIO mapping
- Configurable note range
- Semitone handling logic

### OLED Display Library (`lib/oled_display/`)
- SSD1306 driver
- 5x7 font with full ASCII support (32-126)
- Pixel-level drawing
- String rendering (normal and inverted)
- Inverted text for menu highlighting

### I2C Memory Library (`lib/i2c_memory/`)
- AT24Cxx EEPROM driver
- Page-write support
- Configuration persistence
- Read/write abstraction

## Troubleshooting

### USB Device Not Recognized
- Check USB cable (must support data, not just power)
- Verify TinyUSB initialization in debug output
- Try different USB port

### No OLED Display
- Verify I2C connections (SDA=GP2, SCL=GP3)
- Check I2C address (0x3C for most SSD1306 displays)
- Monitor debug output for initialization errors

### EEPROM Not Responding
- Check I2C address (0x50 for AT24C32)
- The synthesizer operates with default settings if EEPROM is not present
- Verify I2C pull-up resistors (4.7kΩ recommended)

### Button Not Responding
- Check GPIO 4 connection
- Button should be active low (connects to ground when pressed)
- Internal pull-up is enabled in firmware
- Monitor debug output for button events

### GPIO Expander Not Responding
- The synthesizer continues to operate even if the GPIO expander is not connected
- Check I2C address:
  - PCF8574/PCF8575: 0x20 (default)
  - CH423: 0x24 (default)
- Verify correct `io_type` is configured:
  - PCF857x (supports both PCF8574 and PCF8575)
  - CH423
- Verify I2C pull-up resistors (4.7kΩ recommended)
- Monitor debug output for initialization errors

### SysEx Commands Not Working
- Ensure all 7 bytes are sent including F0 and F7
- Verify Device ID byte (0x00) is included
- Check raw SysEx output in UART debug
- Expected format: `F0 7D 00 [CMD] [DATA] F7`

### Debug Output Not Visible
- Check UART connections (TX=GP0, RX=GP1)
- Verify baud rate: 115200
- Ensure DEBUG_ENABLED is set to true

## License

This project is provided as-is for educational and development purposes.

## Version History

**V1.0** (Current)
- USB MIDI to I2C MIDI conversion
- SSD1306 OLED display support
- Interactive button-driven menu system
- Configuration persistence to EEPROM (AT24C32)
- Inverted text menu highlighting
- Single-button navigation (short press / 3-second hold)
- 7 menu options for real-time configuration
- SysEx configuration system
- Semitone handling (Play/Ignore/Skip modes)
- Comprehensive debug output
- Modular architecture

## Author

Developed for the Raspberry Pi Pico using the Pico SDK and TinyUSB.
