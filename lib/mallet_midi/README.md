# Mallet MIDI Library

A library for controlling a xylophone striker mechanism using servo positioning and GPIO-controlled mallet actuators. Maps MIDI notes to servo positions for precise xylophone note striking.

## Features

- **MIDI Note to Servo Position Mapping**: Automatically calculates servo angles based on note range
- **Configurable Range**: Set minimum and maximum servo degrees for lowest and highest notes
- **GPIO Striker Control**: Activates GPIO pin to trigger mallet actuator after servo positioning
- **Semitone Handling**: Three modes - Play, Ignore, or Skip semitones
- **Adjustable Strike Duration**: Configure how long the striker remains activated
- **Auto-timing Management**: Automatically deactivates striker after configured duration

## Hardware Requirements

- Raspberry Pi Pico (or compatible RP2040/RP2350 board)
- Standard hobby servo (0-180 degrees typical)
  - **Power**: 5-6V (use VSYS pin 39 when USB-powered, or external supply)
  - **Not 3.3V**: Servos rated 4.8-6V will be slow and unreliable at 3.3V
  - **Example**: FS90 Micro Servo 9g (0.12s/60° at 4.8V, 0.10s/60° at 6.0V)
  - **Signal**: 3.3V PWM from Pico GPIO is compatible with 5V-powered servos
- Mallet actuator controlled via GPIO (solenoid, motor, etc.)
- Xylophone or similar percussion instrument

## How It Works

### Servo Position Calculation

The library maps MIDI notes to servo positions linearly:

```
Servo Degree = Min Degree + (Note Position × Degree Step)

where:
  Note Position = (MIDI Note - Low Note) accounting for semitone mode
  Degree Step = (Max Degree - Min Degree) / (Note Range - 1)
```

**Example**: 
- Note range: 8 notes (C4 to C5)
- Min degree: 0°
- Max degree: 180°
- Degree step: 180 / 7 ≈ 25.7° per note

| Note | Position | Servo Angle |
|------|----------|-------------|
| C4   | 0        | 0°          |
| D4   | 1        | 26°         |
| E4   | 2        | 51°         |
| F4   | 3        | 77°         |
| G4   | 4        | 103°        |
| A4   | 5        | 129°        |
| B4   | 6        | 154°        |
| C5   | 7        | 180°        |

### Strike Sequence

When a MIDI Note ON is received:
1. Calculate servo position for the note
2. Move servo to position
3. Wait 10ms for servo to settle
4. Activate striker GPIO pin
5. After configured duration (default 50ms), deactivate striker

## API Reference

### Initialization

```c
bool mallet_midi_init(mallet_midi_t *ctx, uint8_t servo_gpio_pin, uint8_t striker_gpio_pin);
```
Initialize with default configuration:
- Note range: 8 notes
- Low note: 60 (Middle C)
- MIDI channel: 10 (0-indexed, displays as Channel 10)
- Servo range: 0-180 degrees
- Strike duration: 50ms
- Semitone mode: PLAY

**Note**: For production use, prefer `mallet_midi_init_with_config()` to load settings from EEPROM/configuration system.

```c
bool mallet_midi_init_with_config(mallet_midi_t *ctx, mallet_midi_config_t *config);
```
Initialize with custom configuration.

### MIDI Processing

```c
bool mallet_midi_process_message(mallet_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity);
```
Process a MIDI message. Returns true if message was for our channel and in range.

### Servo Control

```c
bool mallet_midi_move_servo(mallet_midi_t *ctx, uint16_t degree);
```
Manually move servo to specific degree position (0-180).

```c
bool mallet_midi_note_to_degree(mallet_midi_t *ctx, uint8_t note, uint16_t *out_degree);
```
Calculate what servo position a note would map to.

```c
uint16_t mallet_midi_get_servo_position(mallet_midi_t *ctx);
```
Get current servo position.

### Striker Control

```c
bool mallet_midi_activate_striker(mallet_midi_t *ctx);
```
Activate the striker GPIO pin.

```c
bool mallet_midi_deactivate_striker(mallet_midi_t *ctx);
```
Deactivate the striker GPIO pin.

```c
void mallet_midi_update(mallet_midi_t *ctx);
```
**Must be called regularly from main loop** to handle automatic striker deactivation timing.

### Configuration

```c
bool mallet_midi_set_semitone_mode(mallet_midi_t *ctx, mallet_midi_semitone_mode_t mode);
```
Set semitone handling:
- `MALLET_MIDI_SEMITONE_PLAY` - Play all notes including semitones
- `MALLET_MIDI_SEMITONE_IGNORE` - Ignore semitone notes (don't strike)
- `MALLET_MIDI_SEMITONE_SKIP` - Map semitones to next whole tone

```c
bool mallet_midi_reset(mallet_midi_t *ctx);
```
Reset to initial position (lowest note) and deactivate striker.

## Example Usage

### Basic Setup

```c
#include "mallet_midi.h"

#define SERVO_PIN 15
#define STRIKER_PIN 16

mallet_midi_t mallet;

int main() {
    // Initialize
    mallet_midi_init(&mallet, SERVO_PIN, STRIKER_PIN);
    
    // Main loop
    while (1) {
        // Process MIDI messages from your MIDI input
        uint8_t status, note, velocity;
        if (midi_receive(&status, &note, &velocity)) {
            mallet_midi_process_message(&mallet, status, note, velocity);
        }
        
        // Update to handle striker timing
        mallet_midi_update(&mallet);
        
        sleep_ms(1);
    }
}
```

### Custom Configuration

```c
mallet_midi_config_t config = {
    .note_range = 13,              // Full octave including semitones
    .low_note = 60,                // C4
    .midi_channel = 9,             // Channel 10 (0-indexed)
    .servo_gpio_pin = 15,
    .striker_gpio_pin = 16,
    .min_degree = 10,              // Start at 10 degrees
    .max_degree = 170,             // End at 170 degrees
    .strike_duration_ms = 75,      // Longer strike
    .semitone_mode = MALLET_MIDI_SEMITONE_PLAY
};

mallet_midi_t mallet;
mallet_midi_init_with_config(&mallet, &config);
```

### Multiple Mallets

```c
// Control multiple strikers with one servo
mallet_midi_t bass_mallet;
mallet_midi_t treble_mallet;

// Bass xylophone on channel 10, notes 36-48
mallet_midi_config_t bass_config = {
    .note_range = 13,
    .low_note = 36,
    .midi_channel = 9,
    .servo_gpio_pin = 15,
    .striker_gpio_pin = 16,
    // ...
};

// Treble xylophone on channel 11, notes 60-72
mallet_midi_config_t treble_config = {
    .note_range = 13,
    .low_note = 60,
    .midi_channel = 10,
    .servo_gpio_pin = 17,
    .striker_gpio_pin = 18,
    // ...
};

mallet_midi_init_with_config(&bass_mallet, &bass_config);
mallet_midi_init_with_config(&treble_mallet, &treble_config);

// In main loop, process both
while (1) {
    if (midi_receive(&status, &note, &velocity)) {
        mallet_midi_process_message(&bass_mallet, status, note, velocity);
        mallet_midi_process_message(&treble_mallet, status, note, velocity);
    }
    mallet_midi_update(&bass_mallet);
    mallet_midi_update(&treble_mallet);
}
```

## Configuration Structure

```c
typedef struct {
    uint8_t note_range;              // Number of notes (e.g., 8, 13)
    uint8_t low_note;                // Lowest MIDI note number
    uint8_t high_note;               // Highest note (auto-calculated)
    uint8_t midi_channel;            // MIDI channel (0-15, channel 10 = 9)
    uint8_t servo_pwm_slice;         // PWM slice (auto-assigned)
    uint8_t servo_gpio_pin;          // GPIO for servo PWM
    uint8_t striker_gpio_pin;        // GPIO for striker control
    uint16_t min_degree;             // Servo min position (default 0)
    uint16_t max_degree;             // Servo max position (default 180)
    uint16_t strike_duration_ms;     // Strike duration (default 50ms)
    mallet_midi_semitone_mode_t semitone_mode;
    float degree_per_step;           // Auto-calculated degrees per note
} mallet_midi_config_t;
```

## Servo PWM Details

- **Frequency**: 50Hz (20ms period) - standard for hobby servos
- **Pulse Width**: 0.5ms (0°) to 2.5ms (180°)
- **Resolution**: Calculated based on system clock (typically 125MHz)
- **GPIO Function**: Automatically configured for PWM output

## Timing Considerations

1. **Servo Movement Time**: 10ms settle delay after position command before striking
   - Allows servo to reach position
   - Prevents mallet strike while servo is moving
2. **Strike Duration**: Default 50ms, configurable
   - Total strike cycle: 10ms settle + 50ms strike = 60ms minimum
3. **Update Frequency**: Call `mallet_midi_update()` at least every 10ms for accurate timing
4. **Performance**: System can handle rapid note sequences up to 135+ BPM (16th notes at proper servo voltage)
5. **Servo Speed**: Varies with voltage (FS90 example):
   - At 4.8V: 0.12s/60° (360ms for full 180° travel)
   - At 6.0V: 0.10s/60° (300ms for full 180° travel)
   - At 3.3V: ~50-70% slower (not recommended)

## Mechanical Setup Recommendations

1. **Servo Arm**: Attach pointer or guide rail to servo arm
2. **Striker Mechanism**: Position actuator at servo pointing position
3. **Calibration**: 
   - Play lowest note, adjust `min_degree` until servo points to first xylophone bar
   - Play highest note, adjust `max_degree` until servo points to last bar
   - Fine-tune with `note_range` to match your xylophone

## Troubleshooting

### Servo jitters or doesn't move smoothly
- Check power supply (servos can draw significant current)
- Ensure servo is powered from 5V (VSYS or external), not 3.3V
- Verify PWM signal quality
- Reduce servo movement speed by adding delays

### Servo is slow or unreliable
- **Most common**: Servo powered from 3.3V instead of 5-6V
- Symptoms: Movement takes 50-70% longer than rated speed
- Solution: Use VSYS (Pin 39) for 5V or external 5-6V supply
- Performance: At 4.8V, FS90 moves 360ms for 180°; at 6.0V: 300ms

### Striker doesn't activate
- Verify GPIO pin can drive your actuator (may need transistor/relay)
- Check `strike_duration_ms` is long enough
- Ensure `mallet_midi_update()` is called regularly

### Wrong notes being struck
- Verify `low_note` and `note_range` match your xylophone
- Check `min_degree` and `max_degree` calibration
- Use `mallet_midi_note_to_degree()` to debug note-to-degree mapping
- Verify semitone mode setting matches your instrument layout

### Notes not responding
- Check MIDI channel configuration (0-indexed: channel 1 = 0, channel 10 = 9)
- Verify note is within configured range (`low_note` to `high_note`)
- Use debug output to monitor received MIDI messages

## Integration with Main Project

Add to your main `CMakeLists.txt`:

```cmake
add_subdirectory(lib/mallet_midi)

target_link_libraries(your_target
    mallet_midi
    # ... other libraries
)
```

## License

This library is part of the MIDI Synthesizer project for Raspberry Pi Pico.
