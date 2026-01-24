# OLED Display Library for MIDI Synthesizer

A library for controlling a 128x64 SSD1306-based OLED display over I2C to show MIDI note information.

## Features

- **SSD1306 Driver**: Full support for 128x64 OLED displays
- **I2C Communication**: Uses I2C address 0x3C (0x78 >> 1)
- **Visual Enhancements**:
  - Single-pixel border around display edges
  - Centered headings with horizontal divider lines
  - Text rendering with 1-pixel offset to avoid border overlap
- **Screensaver**:
  - Lissajous curve animation
  - Mathematical parametric curves (x = A·sin(a·t + δ), y = B·sin(b·t))
  - Auto-varying patterns with aesthetic frequency ratios
  - 30-second inactivity timeout
- **MIDI Note Display**: Show active MIDI notes with velocity and channel information
- **Note Name Conversion**: Converts MIDI note numbers to musical note names (C4, D#5, etc.)
- **Channel Activity Display**: Visual representation of activity across all 16 MIDI channels
- **Custom Font**: Built-in 5x7 pixel font for text rendering

## Hardware Requirements

- Raspberry Pi Pico (or compatible RP2040/RP2350 board)
- 128x64 OLED display with SSD1306 controller
- I2C connection (default address: 0x3C / 0x78)

## API Reference

### Initialization

```c
bool oled_init(void* i2c_inst);
```
Initialize the OLED display. Pass `i2c0` or `i2c1` as the I2C instance.

### Display Functions

```c
void oled_clear(void);
```
Clear the entire display buffer.

```c
void oled_draw_border(void);
```
Draw a single-pixel border around the display edges (128x64). Automatically called after `oled_clear()` in display handler functions.

```c
void oled_display(void);
```
Update the physical display with the current buffer content.

### Drawing Functions

```c
void oled_set_pixel(uint8_t x, uint8_t y, uint8_t color);
```
Set a single pixel (x: 0-127, y: 0-63, color: 0=black, 1=white).

```c
void oled_draw_char(uint8_t x, uint8_t y, char c);
void oled_draw_string(uint8_t x, uint8_t y, const char* str);
```
Draw text at specified coordinates.

### MIDI Display Functions

```c
void oled_display_single_note(uint8_t note_num, uint8_t velocity, uint8_t channel);
```
Display a single MIDI note with detailed information including note name, velocity, and channel.

```c
void oled_display_midi_notes(const midi_note_info_t* notes, uint8_t num_notes);
```
Display multiple active MIDI notes from an array.

```c
void oled_display_channel_activity(const uint8_t* channel_activity);
```
Show activity bars for all 16 MIDI channels.

```c
void oled_note_to_name(uint8_t note, char* name_buffer);
```
Convert MIDI note number (0-127) to note name (e.g., "C4", "A#5").

### Screensaver Functions

```c
void lissajous_screensaver_init(void);
```
Initialize the Lissajous curve screensaver with random parameters (amplitudes, frequencies, and phase shift).

```c
void lissajous_screensaver_update(void);
```
Update the screensaver animation frame. Call this periodically (e.g., ~60 FPS) to animate the Lissajous curves. Parameters automatically change for variety.

```c
void lissajous_get_params(float* a_freq, float* b_freq, float* phase);
```
Get current screensaver parameters for debugging (optional).

## Implementation Details

### Display Border
All text is rendered with a 1-pixel y-offset to prevent overlap with the top border. The border drawing function uses `oled_set_pixel()` to draw:
- Top edge: y=0, x=0-127
- Bottom edge: y=63, x=0-127  
- Left edge: x=0, y=0-63
- Right edge: x=127, y=0-63

### Screensaver Mathematics
The Lissajous curve screensaver draws parametric curves:
- **Equations**: x(t) = A·sin(a·t + δ), y(t) = B·sin(b·t)
- **Amplitudes**: A = 20-50 pixels (X), B = 12-26 pixels (Y)
- **Frequencies**: Aesthetic ratios (1:1, 1:2, 2:3, 3:4, 3:5, 4:5) with ±20% variation
- **Phase Shift**: Random δ from 0 to 2π for pattern variety
- **Points per Frame**: 500 with line interpolation for smooth curves
- **Auto-Refresh**: Parameters change randomly (~0.5% chance per frame)

### Text Rendering
All character drawing functions (`oled_draw_char`, `oled_draw_char_inverted`) automatically add 1 pixel to the y-coordinate to ensure proper spacing from the top border.

## Example Usage

```c
#include "oled_display.h"
#include "lissajous_screensaver.h"
#include "hardware/i2c.h"

int main() {
    // Initialize I2C
    i2c_init(i2c0, 400000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);
    
    // Initialize OLED
    oled_init(i2c0);
    
    // Display a MIDI note
    oled_display_single_note(60, 100, 0);  // Middle C, velocity 100, channel 1
    
    // Or display multiple notes
    midi_note_info_t notes[3] = {
        {60, 100, 0, true},   // C4
        {64, 90, 0, true},    // E4
        {67, 85, 0, true}     // G4
    };
    oled_display_midi_notes(notes, 3);
    
    // Screensaver usage
    lissajous_screensaver_init();
    while (screensaver_active) {
        lissajous_screensaver_update();
        sleep_ms(16);  // ~60 FPS
    }
    
    return 0;
}
```

## Configuration

The I2C address can be modified in `oled_display.h`:
```c
#define OLED_I2C_ADDRESS    0x3C  // Default 7-bit address
```

## License

This library is part of the MIDI Synthesizer project.
