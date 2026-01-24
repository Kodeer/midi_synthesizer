# Lissajous Screensaver Integration Guide

## Summary

A new Lissajous curve screensaver module has been created for your MIDI synthesizer project. The screensaver draws beautiful mathematical curves based on parametric equations, similar to the Python pygame example you provided, but optimized for the embedded Raspberry Pi Pico and 128x64 OLED display.

## Files Created

### Core Module Files
1. **[lib/oled_display/lissajous_screensaver.h](lib/oled_display/lissajous_screensaver.h)** - Public API header
2. **[lib/oled_display/lissajous_screensaver.c](lib/oled_display/lissajous_screensaver.c)** - Implementation with Lissajous curve rendering

### Documentation Files
3. **[lib/oled_display/LISSAJOUS_README.md](lib/oled_display/LISSAJOUS_README.md)** - Comprehensive documentation
4. **[lib/oled_display/lissajous_example.c](lib/oled_display/lissajous_example.c)** - Usage examples (4 different scenarios)

### Updated Files
5. **[lib/oled_display/CMakeLists.txt](lib/oled_display/CMakeLists.txt)** - Added new source file to build

## Quick Integration

### Basic Usage

```c
#include "lissajous_screensaver.h"

// Initialize once at startup
lissajous_screensaver_init();

// In your main loop or timer callback (call at ~60 FPS)
void update_display(void) {
    if (screensaver_active) {
        lissajous_screensaver_update();
        sleep_ms(16);  // ~60 FPS
    }
}
```

### Integration with Existing Code

You can integrate this into your existing display handler. Here's an example:

```c
// In display_handler.c or menu_handler.c

#include "lissajous_screensaver.h"

static bool screensaver_active = false;
static uint32_t last_activity_time = 0;
const uint32_t SCREENSAVER_TIMEOUT_MS = 30000;  // 30 seconds

void check_and_update_screensaver(void) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Activate screensaver after timeout
    if (!screensaver_active && 
        (current_time - last_activity_time) > SCREENSAVER_TIMEOUT_MS) {
        screensaver_active = true;
        lissajous_screensaver_init();
    }
    
    // Update screensaver
    if (screensaver_active) {
        lissajous_screensaver_update();
    }
}

void on_user_activity(void) {
    last_activity_time = to_ms_since_boot(get_absolute_time());
    if (screensaver_active) {
        screensaver_active = false;
        // Return to normal display
    }
}
```

## Key Features

### 1. Animated Lissajous Curves
The screensaver uses parametric equations:
- `x(t) = A * sin(a * t + δ)`
- `y(t) = B * sin(b * t)`

### 2. Auto-Varying Parameters
- Amplitudes: 20-50 (X), 12-26 (Y) pixels - optimized for 128x64 display
- Frequencies: Uses aesthetic ratios (1:1, 1:2, 2:3, 3:4, etc.)
- Phase shifts: Random from 0 to 2π
- Parameters change randomly (1/200 chance per frame) for variety

### 3. Smooth Rendering
- 500 points drawn per frame
- Line interpolation between points using Bresenham's algorithm
- ~60 FPS update rate

### 4. Memory Efficient
- Minimal state (just parameters and time)
- No large buffers or history
- Suitable for embedded systems

## Comparison to Python Example

| Feature | Python Example | Embedded Implementation |
|---------|---------------|------------------------|
| Display | 800x600 pygame window | 128x64 OLED (SSD1306) |
| Points | 1000 per frame | 500 per frame (optimized) |
| Colors | RGB random colors | Monochrome (white on black) |
| Math | Python `math.sin()` | C `sinf()` (float precision) |
| Random | `random` module | Custom LCG generator |
| Event handling | Pygame events | Integration with button/MIDI handlers |
| Parameter change | 0.5% per frame | ~0.5% per frame (1/200) |

## Examples Provided

The [lissajous_example.c](lib/oled_display/lissajous_example.c) file includes 4 complete examples:

1. **Simple Screensaver** - Continuous animation
2. **Timeout-Based** - Activates after inactivity period
3. **Debug Mode** - Shows parameters on serial output
4. **Alternating Modes** - Switches between Lissajous and bouncing balls

## Build Status

✅ **Successfully compiled** - The module has been added to the build system and compiles without errors.

## Next Steps

### To Use This Screensaver:

1. **Add to your display update logic** - Call `lissajous_screensaver_update()` periodically
2. **Implement timeout** - Activate after user inactivity
3. **Handle activity** - Deactivate on button press or MIDI message
4. **Optional: Add menu option** - Let users choose between screensavers

### Alternative: Replace Existing Screensaver

To replace the bouncing balls screensaver in [oled_display.c](lib/oled_display/oled_display.c):

```c
// Option 1: Replace the implementation
void oled_screensaver_init(void) {
    lissajous_screensaver_init();
}

void oled_screensaver_update(void) {
    lissajous_screensaver_update();
}

// Option 2: Add menu selection
typedef enum {
    SCREENSAVER_BOUNCING_BALLS,
    SCREENSAVER_LISSAJOUS
} screensaver_type_t;
```

## Performance Notes

- **Frame rate**: ~60 FPS at 16ms per frame
- **CPU usage**: Minimal - mostly sine calculations
- **Memory**: ~40 bytes static + stack for calculations
- **Math library**: Requires `-lm` link flag (already in pico-sdk)

## Customization Options

You can easily modify parameters in [lissajous_screensaver.c](lib/oled_display/lissajous_screensaver.c):

```c
#define LISSAJOUS_POINTS 500           // More points = smoother curves
#define LISSAJOUS_TIME_STEP 0.01f      // Smaller = more detail
#define LISSAJOUS_TIME_INCREMENT 0.02f // Faster = quicker animation
#define PARAM_CHANGE_PROBABILITY 200   // Lower = more frequent changes
```

## Testing

To test the screensaver independently, you can compile [lissajous_example.c](lib/oled_display/lissajous_example.c) as a standalone program by modifying the main CMakeLists.txt to include it as a separate executable.

## Troubleshooting

### Screensaver not visible
- Check I2C initialization
- Verify OLED is powered and connected
- Ensure `oled_init()` succeeded

### Curves too large/small
- Adjust amplitude values in `generate_random_params()`:
  ```c
  params.A = (float)lissajous_rand_int(10, 60);  // Adjust these ranges
  params.B = (float)lissajous_rand_int(6, 30);
  ```

### Animation too fast/slow
- Adjust `LISSAJOUS_TIME_INCREMENT` (default: 0.02f)
- Or change frame delay: `sleep_ms(16)` to `sleep_ms(32)` for slower

### Not changing patterns
- Reduce `PARAM_CHANGE_PROBABILITY` (default: 200)
- Or call `lissajous_screensaver_init()` to force new parameters

## Technical Documentation

For more detailed technical information, see [LISSAJOUS_README.md](lib/oled_display/LISSAJOUS_README.md).

## License

This module is part of the MIDI Synthesizer project and follows the same license.
