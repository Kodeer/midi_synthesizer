# I2C PCA9685 MIDI Library

A library for controlling up to 16 servo-driven mallets using the PCA9685 16-channel 12-bit PWM servo driver. Maps MIDI NOTE ON/OFF messages to servo movements for striking percussion instruments like xylophones, marimbas, or drum pads.

## Features

- **16 Servo Channels**: Control up to 16 independent servo-driven mallets
- **MIDI Note Mapping**: Automatic mapping of MIDI notes to servo channels
- **Two Strike Modes**:
  - **Simple Mode**: All servos swing between rest and strike positions
  - **Position Mode**: Each servo has a unique position per note (like a single scanning servo)
- **Configurable Strike Parameters**: Adjust rest angle, strike angle, and strike duration
- **Semitone Handling**: Three modes - Play, Ignore, or Skip semitones
- **Automatic Return**: Servos automatically return to rest after configurable duration
- **I2C Communication**: Uses standard I2C interface (shared with other peripherals)
- **PCA9685 Driver**: Full-featured driver for the PCA9685 PWM controller

## Hardware Requirements

- Raspberry Pi Pico (or compatible RP2040/RP2350 board)
- PCA9685 16-Channel 12-Bit PWM Servo Driver Module
- Up to 16 RC servos (e.g., SG90, MG90S, or similar)
- External 5-6V power supply for servos (servos can draw significant current)
- Common ground between Pico and servo power supply

## PCA9685 Module Specifications

- **Chip**: NXP PCA9685
- **Channels**: 16 independent PWM outputs
- **Resolution**: 12-bit (4096 steps)
- **PWM Frequency**: 24Hz to 1526Hz (typically 50Hz for servos)
- **I2C Interface**: 400kHz Fast mode supported
- **Default Address**: 0x40 (configurable via solder jumpers)
- **Supply Voltage**: 2.3V to 5.5V logic, separate 5-6V servo power input

## Hardware Connections

### PCA9685 Module Wiring

```
Raspberry Pi Pico          PCA9685 Module
--------------------       ---------------
3.3V            ------>    VCC (logic power)
GND             ------>    GND
GPIO 2 (SDA)    ------>    SDA
GPIO 3 (SCL)    ------>    SCL

External 5-6V Supply       PCA9685 Module
--------------------       ---------------
5-6V            ------>    V+ (servo power)
GND             ------>    GND (common ground)

PCA9685 Module             Servos (x16)
--------------             ------------
PWM 0-15        ------>    Signal (yellow/white wire)
V+              ------>    VCC (red wire)
GND             ------>    GND (brown/black wire)
```

### I2C Address Selection

The PCA9685 module typically has solder jumpers (A0-A5) for address configuration:
- Default: 0x40 (all jumpers open)
- Address = 0x40 + (A5<<5 | A4<<4 | A3<<3 | A2<<2 | A1<<1 | A0)
- Up to 62 modules can share one I2C bus

### Power Considerations

- **Logic Power (VCC)**: 3.3V from Pico (< 10mA)
- **Servo Power (V+)**: 5-6V external supply
  - Each servo can draw 100-500mA when moving
  - 16 servos can draw up to 8A combined
  - **Never power servos from Pico's 3.3V or VSYS**
- **Common Ground**: Essential - connect Pico GND to servo power supply GND

## How It Works

### Strike Modes

#### Simple Mode (Default)
Each servo has two positions:
- **Rest Position** (default 30°): Mallet lifted away from instrument
- **Strike Position** (default 120°): Mallet swings down to strike note

When a MIDI NOTE ON is received:
1. Servo moves from rest to strike position
2. Holds strike position for configured duration (default 50ms)
3. Returns to rest position

This is ideal for instruments where each servo controls one mallet above one note.

#### Position Mode
Each servo moves to a unique position based on note index:
- Servo 0 (lowest note): min_degree (e.g., 0°)
- Servo 15 (highest note): max_degree (e.g., 180°)
- Intermediate servos: linearly interpolated

This is useful when multiple notes share one servo arm (position selection).

### MIDI Note to Servo Mapping

```
Example: 8 notes starting at Middle C (60)

MIDI Note    Note Name    Servo Channel
---------    ---------    -------------
60           C4           0
61           C#4          1 (or ignored based on semitone mode)
62           D4           2
63           D#4          3 (or ignored)
64           E4           4
65           F4           5
66           F#4          6 (or ignored)
67           G4           7
```

With `SEMITONE_IGNORE` mode, only natural notes (C, D, E, F, G, etc.) are mapped, allowing 16 servos to cover a wider range.

## API Reference

### Initialization

```c
bool pca9685_midi_init(pca9685_midi_t *ctx, i2c_inst_t *i2c_port, 
                       uint8_t sda_pin, uint8_t scl_pin, uint32_t i2c_speed);
```
Initialize with default configuration:
- 16 channels
- Channel 10 (MIDI percussion)
- Simple strike mode
- Rest angle: 30°
- Strike angle: 120°
- Strike duration: 50ms

```c
bool pca9685_midi_init_with_config(pca9685_midi_t *ctx, pca9685_midi_config_t *config,
                                   uint8_t sda_pin, uint8_t scl_pin, uint32_t i2c_speed);
```
Initialize with custom configuration.

### MIDI Processing

```c
bool pca9685_midi_process_message(pca9685_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity);
```
Process a MIDI message. Returns true if message was handled.

**Important**: Must call `pca9685_midi_update()` regularly in main loop for automatic servo return.

```c
void pca9685_midi_update(pca9685_midi_t *ctx);
```
Update servo states - handles automatic return to rest position. Call this in your main loop (at least every 10ms).

### Configuration

```c
bool pca9685_midi_set_semitone_mode(pca9685_midi_t *ctx, pca9685_midi_semitone_mode_t mode);
```
Set semitone handling:
- `PCA9685_MIDI_SEMITONE_PLAY` - Play all notes including semitones
- `PCA9685_MIDI_SEMITONE_IGNORE` - Ignore semitone notes
- `PCA9685_MIDI_SEMITONE_SKIP` - Map semitones to next whole tone

```c
bool pca9685_midi_set_strike_mode(pca9685_midi_t *ctx, pca9685_strike_mode_t mode);
```
Set strike mode:
- `PCA9685_STRIKE_MODE_SIMPLE` - Rest/strike positions
- `PCA9685_STRIKE_MODE_POSITION` - Unique position per note

### Manual Control

```c
bool pca9685_midi_strike_servo(pca9685_midi_t *ctx, uint8_t servo_index);
```
Manually trigger a servo strike (0-15).

```c
bool pca9685_midi_all_notes_off(pca9685_midi_t *ctx);
```
Return all servos to rest position immediately.

```c
bool pca9685_midi_reset(pca9685_midi_t *ctx);
```
Reset the PCA9685 and return all servos to rest.

## Example Usage

### Basic Setup

```c
#include "i2c_pca9685_midi.h"

#define I2C_PORT i2c1
#define SDA_PIN 2
#define SCL_PIN 3
#define I2C_SPEED 400000  // 400kHz

pca9685_midi_t pca_midi;

int main() {
    stdio_init_all();
    
    // Initialize with defaults
    if (!pca9685_midi_init(&pca_midi, I2C_PORT, SDA_PIN, SCL_PIN, I2C_SPEED)) {
        printf("Failed to initialize PCA9685 MIDI\n");
        return 1;
    }
    
    printf("PCA9685 MIDI initialized\n");
    
    while (1) {
        // Process MIDI messages from your MIDI input
        uint8_t status, note, velocity;
        if (midi_receive(&status, &note, &velocity)) {
            pca9685_midi_process_message(&pca_midi, status, note, velocity);
        }
        
        // IMPORTANT: Update servo states for automatic return
        pca9685_midi_update(&pca_midi);
        
        sleep_ms(1);
    }
}
```

### Custom Configuration

```c
pca9685_midi_config_t config = {
    .note_range = 13,                              // Full octave with semitones
    .low_note = 60,                                // C4
    .midi_channel = 9,                             // Channel 10 (0-indexed)
    .i2c_address = 0x40,                           // Default PCA9685 address
    .i2c_port = i2c1,
    .semitone_mode = PCA9685_MIDI_SEMITONE_PLAY,   // Play all notes
    .strike_mode = PCA9685_STRIKE_MODE_SIMPLE,     // Rest/strike mode
    .rest_angle = 20,                              // Mallet up at 20°
    .strike_angle = 130,                           // Mallet strikes at 130°
    .strike_duration_ms = 75,                      // Hold strike for 75ms
    .min_degree = 0,                               // For position mode
    .max_degree = 180                              // For position mode
};

pca9685_midi_t pca_midi;
pca9685_midi_init_with_config(&pca_midi, &config, SDA_PIN, SCL_PIN, I2C_SPEED);
```

### Integration with Existing MIDI System

```c
// In your MIDI handler
void midi_message_received(uint8_t status, uint8_t note, uint8_t velocity) {
    // Route to PCA9685 MIDI player
    if (pca9685_midi_process_message(&pca_midi, status, note, velocity)) {
        printf("Note %d handled by PCA9685\n", note);
    }
}

// In your main loop
while (1) {
    // ... other processing ...
    
    // Update PCA9685 servos
    pca9685_midi_update(&pca_midi);
    
    sleep_ms(1);
}
```

## Configuration Structure

```c
typedef struct {
    uint8_t note_range;                        // Number of notes (max 16)
    uint8_t low_note;                          // Lowest MIDI note
    uint8_t high_note;                         // Highest note (auto-calculated)
    uint8_t midi_channel;                      // MIDI channel (0-15)
    uint8_t i2c_address;                       // PCA9685 I2C address
    i2c_inst_t *i2c_port;                      // I2C port
    pca9685_midi_semitone_mode_t semitone_mode;
    pca9685_strike_mode_t strike_mode;
    
    // Simple mode settings
    uint16_t rest_angle;                       // Rest position degrees
    uint16_t strike_angle;                     // Strike position degrees
    uint16_t strike_duration_ms;               // Strike hold time
    
    // Position mode settings
    uint16_t min_degree;                       // Minimum position
    uint16_t max_degree;                       // Maximum position
} pca9685_midi_config_t;
```

## PCA9685 Low-Level Driver

The library includes a full-featured PCA9685 driver that can also be used independently:

```c
#include "drivers/pca9685_driver.h"

pca9685_t pca;
pca9685_init(&pca, i2c1, 0x40, 50);  // 50Hz for servos

// Set servo to 90 degrees
pca9685_set_servo_angle(&pca, 0, 90);

// Set custom pulse width (in microseconds)
pca9685_set_servo_pulse(&pca, 0, 1500);  // 1.5ms = 90°

// Low-level PWM control (0-4095)
pca9685_set_pwm(&pca, 0, 0, 307);  // ~1.5ms pulse at 50Hz
```

## Timing Considerations

1. **Strike Duration**: Default 50ms
   - Too short: Mallet may not hit note firmly
   - Too long: Limits maximum playing speed
   - Typical range: 30-100ms

2. **Servo Movement Time**: Depends on servo speed
   - SG90: ~0.12s/60° at 4.8V
   - MG90S: ~0.10s/60° at 6V
   - For 90° swing: ~180-150ms

3. **Maximum Playing Speed**: 
   - Limited by: strike_duration + servo_movement_time
   - Example: 50ms + 150ms = 200ms = 5 notes/second per servo
   - For rapid sequences, reduce strike_duration

4. **Update Frequency**: Call `pca9685_midi_update()` at least every 10ms

## Mechanical Setup Recommendations

### Mallet Mounting

1. **Servo Horn**: Attach mallet directly to servo horn
2. **Rest Position**: Mallet lifted clear of instrument (20-30°)
3. **Strike Position**: Mallet hits note firmly (100-140°)
4. **Strike Angle**: Adjust so mallet hits perpendicular to note

### Calibration Procedure

1. Mount servo above first note
2. Set rest_angle so mallet clears note
3. Manually test strike_angle for good contact
4. Fine-tune strike_duration for clean hits
5. Repeat for each servo/note

### Mounting Options

- **Individual Servos**: One servo per note (most common)
- **Scanning Servo**: One servo positions multiple mallets (position mode)
- **Hybrid**: Groups of servos for different note ranges

## Troubleshooting

### Servos jitter or don't move smoothly
- Check servo power supply (needs 5-6V, adequate current)
- Verify I2C connections and pull-ups
- Reduce I2C speed if long wires are used
- Check for electromagnetic interference from servo motors

### Wrong notes being struck
- Verify MIDI channel configuration (0-indexed vs 1-indexed)
- Check note range and low_note settings
- Use debug output to monitor note-to-servo mapping
- Verify semitone mode matches your instrument

### Servos don't return to rest
- Ensure `pca9685_midi_update()` is called regularly in main loop
- Check strike_duration_ms is reasonable (< 200ms)
- Verify servos aren't mechanically binding

### Weak or missed strikes
- Increase strike_angle for more force
- Increase strike_duration_ms for longer contact
- Check servo power voltage (should be 5-6V, not 3.3V)
- Verify servo horn is securely attached

### I2C communication errors
- Check SDA/SCL connections and pull-up resistors
- Verify I2C address (use i2c scanner to detect)
- Reduce I2C speed (try 100kHz instead of 400kHz)
- Check for address conflicts with other I2C devices

## Multiple PCA9685 Modules

Up to 62 PCA9685 modules can share one I2C bus (addresses 0x40-0x7F):

```c
// Module 1 at 0x40
pca9685_midi_t pca_midi_1;
pca9685_midi_config_t config_1 = {
    .i2c_address = 0x40,
    .low_note = 36,  // C2
    // ...
};
pca9685_midi_init_with_config(&pca_midi_1, &config_1, SDA_PIN, SCL_PIN, I2C_SPEED);

// Module 2 at 0x41
pca9685_midi_t pca_midi_2;
pca9685_midi_config_t config_2 = {
    .i2c_address = 0x41,
    .low_note = 52,  // E3
    // ...
};
pca9685_midi_init_with_config(&pca_midi_2, &config_2, SDA_PIN, SCL_PIN, I2C_SPEED);

// In main loop
while (1) {
    if (midi_receive(&status, &note, &velocity)) {
        pca9685_midi_process_message(&pca_midi_1, status, note, velocity);
        pca9685_midi_process_message(&pca_midi_2, status, note, velocity);
    }
    
    pca9685_midi_update(&pca_midi_1);
    pca9685_midi_update(&pca_midi_2);
}
```

## Integration with Main Project

Add to your main `CMakeLists.txt`:

```cmake
add_subdirectory(lib/i2c_pca9685_midi)

target_link_libraries(your_target
    i2c_pca9685_midi
    # ... other libraries
)
```

## Performance

- **Latency**: < 5ms from MIDI message to servo movement start
- **Simultaneous Notes**: All 16 servos can strike simultaneously
- **Maximum Note Rate**: ~5 notes/second per servo (depends on servo speed and strike duration)
- **I2C Bandwidth**: ~1ms to update all 16 servos at 400kHz

## References

- [PCA9685 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)
- [Servo Control Tutorial](https://learn.adafruit.com/16-channel-pwm-servo-driver)
- Original inspiration: Piano player robots, xylophone robots

## License

This library is part of the MIDI Synthesizer project for Raspberry Pi Pico.
