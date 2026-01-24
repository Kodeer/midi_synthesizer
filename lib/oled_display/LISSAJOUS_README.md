# Lissajous Curve Screensaver

A mathematical screensaver module that displays animated Lissajous curves on the SSD1306 OLED display.

## Overview

Lissajous curves are parametric curves formed by combining two perpendicular harmonic oscillations:

```
x(t) = A * sin(a * t + δ)
y(t) = B * sin(b * t)
```

Where:
- **A, B**: Amplitudes (control the size of the curve)
- **a, b**: Frequencies (control the shape and complexity)
- **δ**: Phase shift (rotates/transforms the pattern)
- **t**: Time parameter

## Features

- **Smooth Animation**: Draws 500 points per frame with line interpolation for smooth curves
- **Randomized Parameters**: Automatically generates varied patterns with:
  - Random amplitudes adapted for 128x64 display
  - Simple frequency ratios (1:1, 1:2, 2:3, 3:4, etc.) for aesthetically pleasing patterns
  - Random phase shifts for variety
- **Auto-Refresh**: Parameters change occasionally (1/200 chance per frame) to keep the display interesting
- **Optimized for OLED**: Designed specifically for the 128x64 pixel SSD1306 display

## API Usage

### Initialize the Screensaver

```c
#include "lissajous_screensaver.h"

lissajous_screensaver_init();
```

Call once at startup to initialize random parameters.

### Update Animation

```c
// In your main loop
while (1) {
    lissajous_screensaver_update();
    sleep_ms(16);  // ~60 FPS
}
```

Call repeatedly to animate the curves. Each call:
1. Optionally generates new random parameters
2. Clears the display buffer
3. Calculates and draws 500 curve points
4. Updates the display

### Get Current Parameters (Optional)

```c
float a, b, phase;
lissajous_get_params(&a, &b, &phase);
printf("Frequencies: a=%.2f, b=%.2f, phase=%.2f\n", a, b, phase);
```

## Integration Example

Replace the existing bouncing balls screensaver in your code:

```c
// In display_handler.c or main loop
#include "lissajous_screensaver.h"

// Initialize
lissajous_screensaver_init();

// In main loop or timer callback
void screensaver_task(void) {
    if (screensaver_active) {
        lissajous_screensaver_update();
    }
}
```

## Technical Details

### Display Mapping
- **Screen center**: (64, 32)
- **X amplitude range**: 20-50 pixels
- **Y amplitude range**: 12-26 pixels
- Keeps curves fully visible within 128x64 display bounds

### Frequency Ratios
Uses predefined ratio pairs for aesthetically interesting patterns:
- 1:1 - Circle or ellipse (with phase shift)
- 1:2 - Figure-eight
- 2:3 - Three-petal rose
- 3:4 - Complex knot
- 3:5 - Five-pointed star
- 4:5 - Intricate weaving pattern

Each ratio is slightly randomized (±20%) to add subtle variation.

### Performance
- **Points per frame**: 500
- **Line interpolation**: Bresenham's algorithm for smooth curves
- **Frame rate**: Typically 60 FPS with 16ms delay
- **Math operations**: Uses standard `sinf()` for float sine calculations

## Files

- `lissajous_screensaver.h` - Public API header
- `lissajous_screensaver.c` - Implementation
- Requires: `oled_display.h` for pixel/display functions

## Inspired By

This implementation is based on the Python pygame example provided, adapted for:
- Embedded systems (Raspberry Pi Pico)
- Monochrome OLED displays
- Low-memory environments
- Real-time performance

## Future Enhancements

Potential improvements:
- [ ] Add support for color OLED displays (using the `hue` parameter)
- [ ] User-configurable parameter ranges via menu
- [ ] Multiple curve overlay modes
- [ ] Fade trails for ghosting effect (on displays with grayscale support)
- [ ] Synchronization with MIDI tempo/beat

## License

Part of the MIDI Synthesizer project for Raspberry Pi Pico.
