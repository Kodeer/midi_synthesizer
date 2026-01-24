# PWM Buzzer Library for Raspberry Pi Pico

A simple PWM-based buzzer library for generating tones and sound effects on the Raspberry Pi Pico.

## Features

- **PWM Tone Generation**: Generate any frequency from 20Hz to 20kHz
- **Duration Control**: Play tones for specific durations or continuously
- **Pre-defined Sound Effects**:
  - Boot melody (ascending musical notes)
  - Click sound (short beep)
  - Success sound (positive confirmation)
  - Error sound (alert pattern)
- **Non-blocking**: Timed tones use sleep, but can be easily adapted for async operation
- **Easy Integration**: Simple API with minimal setup

## Hardware Requirements

- Raspberry Pi Pico (or compatible RP2040/RP2350 board)
- Passive piezo buzzer (not active buzzer)
- 100Ω resistor (current limiting)

## Hardware Connection

```
Pico GPIO 15 --[100Ω]-- Piezo+
                        Piezo- -- GND
```

**Important**: Use a **passive piezo buzzer**. Active buzzers have built-in oscillators and won't work with PWM control.

## API Reference

### Initialization

```c
bool buzzer_init(uint8_t gpio_pin);
```
Initialize the buzzer on the specified GPIO pin. Must be called before any other buzzer functions.

**Parameters:**
- `gpio_pin`: GPIO pin number (e.g., 15)

**Returns:** `true` on success

### Basic Tone Control

```c
void buzzer_tone(uint16_t frequency, uint16_t duration_ms);
```
Play a tone at specified frequency and duration.

**Parameters:**
- `frequency`: Frequency in Hz (20-20000), or 0 to stop
- `duration_ms`: Duration in milliseconds, or 0 for continuous

**Examples:**
```c
buzzer_tone(440, 500);    // Play A4 for 500ms
buzzer_tone(1000, 0);     // Play 1kHz continuously
buzzer_tone(0, 0);        // Stop sound
```

```c
void buzzer_stop(void);
```
Stop any currently playing tone.

### Pre-defined Sound Effects

```c
void buzzer_click(void);
```
Play a short click sound (2kHz, 30ms). Ideal for button press feedback.

```c
void buzzer_success(void);
```
Play a success confirmation sound (two ascending tones).

```c
void buzzer_error(void);
```
Play an error alert sound (alternating low/high tones, 3 pulses).

```c
void buzzer_boot_melody(void);
```
Play a boot-up melody (C5, E5, G5, C6 ascending scale).

## Example Usage

```c
#include "buzzer.h"
#include "pico/stdlib.h"

int main() {
    // Initialize buzzer on GPIO 15
    buzzer_init(15);
    
    // Play boot melody
    buzzer_boot_melody();
    sleep_ms(500);
    
    // Play a click
    buzzer_click();
    sleep_ms(500);
    
    // Play custom tone
    buzzer_tone(880, 200);  // A5 for 200ms
    sleep_ms(300);
    
    // Play success sound
    buzzer_success();
    
    return 0;
}
```

## Musical Note Frequencies

Common note frequencies for `buzzer_tone()`:

| Note | Frequency (Hz) | Note | Frequency (Hz) |
|------|----------------|------|----------------|
| C4   | 262            | C5   | 523            |
| D4   | 294            | D5   | 587            |
| E4   | 330            | E5   | 659            |
| F4   | 349            | F5   | 698            |
| G4   | 392            | G5   | 784            |
| A4   | 440            | A5   | 880            |
| B4   | 494            | B5   | 988            |

## Technical Details

### PWM Configuration
- System clock: 125MHz
- PWM wrap value: Calculated dynamically based on frequency
- Duty cycle: 50% (square wave)
- Clock divider: Automatically adjusted for optimal resolution

### Frequency Range
- Recommended: 100Hz - 10kHz (best piezo response)
- Supported: 20Hz - 20kHz
- Outside this range may have reduced volume or accuracy

## Integration with MIDI Synthesizer

In the MIDI Synthesizer project, the buzzer is used for:
- **Boot-up**: Plays melody when system initializes successfully
- **Button clicks**: Provides tactile feedback for button presses
- **Configuration save**: Success tone on successful save
- **Errors**: Alert tone for error conditions

## Troubleshooting

### No Sound
- Verify GPIO connection
- Check that you're using a **passive** piezo buzzer (not active)
- Ensure 100Ω resistor is in series
- Try a different frequency (some buzzers have resonant frequencies)

### Weak Sound
- Check buzzer type (passive vs active)
- Try frequencies 2kHz - 4kHz (typical piezo resonance)
- Verify power supply is adequate
- Consider using a transistor amplifier for higher volume

### Distorted Sound
- Reduce frequency if too high for your buzzer
- Check for loose connections
- Ensure clean 3.3V power supply

## License

Part of the MIDI Synthesizer project. Provided as-is for educational and development purposes.
