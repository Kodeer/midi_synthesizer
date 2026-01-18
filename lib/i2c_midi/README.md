# I2C MIDI Library for PCF8574

This library enables interfacing MIDI note messages with the PCF8574 I2C GPIO extender chip. It translates MIDI NOTE ON/OFF messages into digital pin states on the PCF8574, making it ideal for controlling percussion instruments, solenoids, or other actuators via MIDI.

## Features

- Automatic MIDI channel filtering
- Configurable note range mapping
- **Semitone handling modes**: play, ignore, or skip to next whole tone
- Direct PCF8574 control via I2C
- Support for NOTE ON/OFF with velocity detection
- 8 available output pins (P0-P7)
- Debug output support for troubleshooting

## Semitone Handling Modes

The library supports three modes for handling semitone (sharp/flat) notes:

1. **PLAY Mode** (default): All notes including semitones are played normally
   - Example: C, C#, D, D#, E, F, F#, G, G#, A, A#, B → all mapped to pins

2. **IGNORE Mode**: Semitone notes are ignored and not played
   - Example: C, D, E, F, G, A, B are played; C#, D#, F#, G#, A# are ignored

3. **SKIP Mode**: Semitones are replaced with the next whole tone
   - Example: C# → D, D# → E, F# → G, G# → A, A# → B
   - Useful for instruments that only support diatonic scales

## Hardware Connections

### Default Configuration:
- **I2C Port**: I2C0
- **SDA Pin**: GPIO 4
- **SCL Pin**: GPIO 5
- **PCF8574 Address**: 0x20

### PCF8574 Wiring:
```
Pico              PCF8574
----              -------
3.3V     ------>  VCC
GND      ------>  GND
GPIO 4   ------>  SDA
GPIO 5   ------>  SCL
                  
PCF8574 Pins:     Your Device
P0-P7    ------>  Actuators/LEDs/etc
```

## Usage

### Basic Initialization (Default Settings)

```c
#include "i2c_midi.h"

i2c_midi_t i2c_midi_ctx;

// Initialize with defaults:
// - Channel 10 (percussion)
// - Notes 60-67 (Middle C to G)
// - I2C address 0x20
i2c_midi_init(&i2c_midi_ctx, i2c0, 4, 5, 100000);
```

### Custom Configuration

```c
i2c_midi_t i2c_midi_ctx;
i2c_midi_config_t config = {
    .note_range = 8,                         // Handle 8 notes
    .low_note = 36,                          // Start at C1 (bass drum in GM)
    .high_note = 43,                         // Calculated automatically
    .midi_channel = 10,                      // Percussion channel
    .pcf8574_address = 0x20,                 // PCF8574 I2C address
    .i2c_port = i2c0,                        // Use I2C0
    .semitone_mode = I2C_MIDI_SEMITONE_PLAY  // Play semitones (default)
};

i2c_midi_init_with_config(&i2c_midi_ctx, &config, 4, 5, 100000);
```

### Setting Semitone Mode

```c
// Play semitones normally (default)
i2c_midi_set_semitone_mode(&i2c_midi_ctx, I2C_MIDI_SEMITONE_PLAY);

// Ignore semitone notes (only play whole tones)
i2c_midi_set_semitone_mode(&i2c_midi_ctx, I2C_MIDI_SEMITONE_IGNORE);

// Skip semitones and play next whole tone (C# becomes D)
i2c_midi_set_semitone_mode(&i2c_midi_ctx, I2C_MIDI_SEMITONE_SKIP);
```

**Note:** When using IGNORE or SKIP mode, the `high_note` is automatically recalculated to span only whole tones within the specified `note_range`.

### Processing MIDI Messages

```c
// In your MIDI receive callback:
uint8_t status = midi_packet[1];   // Status byte
uint8_t note = midi_packet[2];     // Note number
uint8_t velocity = midi_packet[3]; // Velocity

// Process the message (automatically filters channel and note range)
i2c_midi_process_message(&i2c_midi_ctx, status, note, velocity);
```

### Manual Pin Control

```c
// Set pin 0 HIGH
i2c_midi_set_pin(&i2c_midi_ctx, 0, true);

// Set pin 0 LOW
i2c_midi_set_pin(&i2c_midi_ctx, 0, false);

// Reset all pins to LOW
i2c_midi_reset(&i2c_midi_ctx);

// Get current pin state
uint8_t state = i2c_midi_get_pin_state(&i2c_midi_ctx);
```

## Default Configuration

| Parameter | Default Value | Description |
|-----------|---------------|-------------|
| `note_range` | 8 | Number of notes to handle |
| `low_note` | 60 | Middle C (C4) |
| `high_note` | 67 | Calculated as low_note + note_range - 1 |
| `midi_channel` | 10 | MIDI channel (percussion) |
| `pcf8574_address` | 0x20 | I2C address of PCF8574 |

## Note to Pin Mapping

The library maps MIDI notes to PCF8574 pins based on the configured note range:

```
MIDI Note          PCF8574 Pin
---------          -----------
low_note + 0  -->  P0
low_note + 1  -->  P1
low_note + 2  -->  P2
low_note + 3  -->  P3
low_note + 4  -->  P4
low_note + 5  -->  P5
low_note + 6  -->  P6
low_note + 7  -->  P7
```

### Example: Default Configuration (Notes 60-67)
```
MIDI Note 60 (C4)  -->  Pin P0
MIDI Note 61 (C#4) -->  Pin P1
MIDI Note 62 (D4)  -->  Pin P2
MIDI Note 63 (D#4) -->  Pin P3
MIDI Note 64 (E4)  -->  Pin P4
MIDI Note 65 (F4)  -->  Pin P5
MIDI Note 66 (F#4) -->  Pin P6
MIDI Note 67 (G4)  -->  Pin P7
```

### Example: Percussion Configuration (Notes 36-43)
```
MIDI Note 36 (C1 - Bass Drum)      -->  Pin P0
MIDI Note 37 (C#1 - Side Stick)    -->  Pin P1
MIDI Note 38 (D1 - Snare)          -->  Pin P2
MIDI Note 39 (D#1 - Hand Clap)     -->  Pin P3
MIDI Note 40 (E1 - Snare)          -->  Pin P4
MIDI Note 41 (F1 - Low Tom)        -->  Pin P5
MIDI Note 42 (F#1 - Closed HH)     -->  Pin P6
MIDI Note 43 (G1 - Low Tom)        -->  Pin P7
```

## Message Filtering

The library automatically filters MIDI messages:
- Only processes messages on the configured MIDI channel
- Ignores notes outside the configured range (low_note to high_note)
- Returns `false` for ignored messages, `true` for processed messages

## MIDI Message Handling

- **NOTE ON** (velocity > 0): Sets corresponding pin HIGH
- **NOTE OFF**: Sets corresponding pin LOW
- **NOTE ON** (velocity = 0): Treated as NOTE OFF, sets pin LOW

## PCF8574 I2C Addresses

The PCF8574 typically has addresses in the range 0x20-0x27 depending on A0-A2 pins:

| A2 | A1 | A0 | Address |
|----|----|----|---------|
| 0  | 0  | 0  | 0x20    |
| 0  | 0  | 1  | 0x21    |
| 0  | 1  | 0  | 0x22    |
| 0  | 1  | 1  | 0x23    |
| 1  | 0  | 0  | 0x24    |
| 1  | 0  | 1  | 0x25    |
| 1  | 1  | 0  | 0x26    |
| 1  | 1  | 1  | 0x27    |

## Example: Complete Implementation

```c
#include "pico/stdlib.h"
#include "tusb.h"
#include "i2c_midi.h"

static i2c_midi_t i2c_midi_ctx;

void midi_task(void) {
    uint8_t packet[4];
    while (tud_midi_available()) {
        tud_midi_packet_read(packet);
        
        uint8_t status = packet[1];
        uint8_t note = packet[2];
        uint8_t velocity = packet[3];
        
        // Process with i2c_midi library
        i2c_midi_process_message(&i2c_midi_ctx, status, note, velocity);
    }
}

int main() {
    // Initialize I2C MIDI
    i2c_midi_init(&i2c_midi_ctx, i2c0, 4, 5, 100000);
    
    // Initialize TinyUSB
    tusb_init();
    
    while (true) {
        tud_task();
        midi_task();
    }
}
```

## Error Handling

The initialization functions return `false` if initialization fails. Check the return value:

```c
if (!i2c_midi_init(&i2c_midi_ctx, i2c0, 4, 5, 100000)) {
    // Handle initialization error
    // (e.g., I2C communication failure)
}
```

## License

This library is part of the midi_synthesizer project for Raspberry Pi Pico.
