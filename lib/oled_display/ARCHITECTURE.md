# Lissajous Screensaver Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Main Application                          │
│  (midi_synthesizer.c / display_handler.c)                   │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ calls periodically (~60 FPS)
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│           lissajous_screensaver_update()                     │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  1. Check if parameters should change (1/200 chance)  │  │
│  │  2. Clear OLED buffer                                 │  │
│  │  3. Calculate 500 curve points                        │  │
│  │  4. Draw lines between points                         │  │
│  │  5. Update time variable                              │  │
│  │  6. Push buffer to display                            │  │
│  └───────────────────────────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ uses
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              OLED Display Driver (oled_display.c)           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ oled_clear() │  │oled_set_pixel│  │oled_display()│      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ I2C communication
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                 SSD1306 OLED Display                         │
│                     128×64 pixels                            │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow

```
Time t → Lissajous Equations → Screen Coordinates → Pixels → Display
         x = A·sin(a·t + δ)      (center_x + x,
         y = B·sin(b·t)           center_y + y)
```

## Parameter Generation Flow

```
┌─────────────────────────────────────────────────────────────┐
│              generate_random_params()                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Random Number Generator (LCG)                              │
│         │                                                    │
│         ├─→ Amplitude A (20-50 pixels)                      │
│         │                                                    │
│         ├─→ Amplitude B (12-26 pixels)                      │
│         │                                                    │
│         ├─→ Frequency ratio (1:1, 1:2, 2:3, etc.)          │
│         │   with ±20% variation                             │
│         │                                                    │
│         └─→ Phase δ (0 to 2π radians)                       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Rendering Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                    Frame Rendering                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  For i = 0 to 499 (500 points):                            │
│    │                                                         │
│    ├─ Calculate t = current_time + i × 0.01                │
│    │                                                         │
│    ├─ Compute:                                              │
│    │    x = A × sin(a×t + δ)                               │
│    │    y = B × sin(b×t)                                   │
│    │                                                         │
│    ├─ Convert to screen coordinates:                        │
│    │    screen_x = 64 + x                                  │
│    │    screen_y = 32 + y                                  │
│    │                                                         │
│    ├─ Clamp to display bounds [0..127], [0..63]           │
│    │                                                         │
│    ├─ Draw pixel at (screen_x, screen_y)                   │
│    │                                                         │
│    └─ Draw line from previous point (interpolation)        │
│         using Bresenham's algorithm                         │
│                                                              │
│  Increment current_time by 0.02                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Memory Layout

```
┌─────────────────────────────────────────────────────────────┐
│                    Static Memory (~40 bytes)                 │
├─────────────────────────────────────────────────────────────┤
│  lissajous_params_t params {                                │
│      float A;           // 4 bytes                          │
│      float B;           // 4 bytes                          │
│      float a;           // 4 bytes                          │
│      float b;           // 4 bytes                          │
│      float delta;       // 4 bytes                          │
│      uint8_t hue;       // 1 byte                           │
│  }                                                           │
│  float current_time;    // 4 bytes                          │
│  uint32_t frame_count;  // 4 bytes                          │
│  uint32_t lissajous_seed; // 4 bytes (RNG state)            │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    Stack Usage (~200 bytes)                  │
├─────────────────────────────────────────────────────────────┤
│  - Loop variables                                            │
│  - Temporary float calculations                              │
│  - Function call overhead                                    │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│            Display Buffer (1024 bytes, shared)               │
├─────────────────────────────────────────────────────────────┤
│  oled_buffer[128 × 64 / 8] - in oled_display.c             │
│  (managed by OLED driver, not by screensaver)               │
└─────────────────────────────────────────────────────────────┘
```

## State Machine

```
                    ┌──────────────┐
                    │  Initialize  │
                    │  Screensaver │
                    └──────┬───────┘
                           │
                           │ lissajous_screensaver_init()
                           │ - Generate random parameters
                           │ - Reset time to 0
                           │
                           ▼
                    ┌──────────────┐
           ┌────────│   Drawing    │◄────────┐
           │        │    Loop      │         │
           │        └──────┬───────┘         │
           │               │                 │
           │               │ Every frame     │
           │               ▼                 │
           │        ┌──────────────┐         │
           │        │ Random check │         │
           │        │  (1/200)     │         │
           │        └──┬───────┬───┘         │
           │           │       │             │
           │    No     │       │ Yes         │
           │   change  │       │             │
           └───────────┘       │             │
                               │             │
                        ┌──────▼───────┐     │
                        │  Generate    │     │
                        │  New Params  │     │
                        └──────────────┘     │
                               │             │
                               └─────────────┘
```

## Timing Diagram

```
Time (ms) →
0     16    32    48    64    80    96    112   128
│     │     │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ F0  │ F1  │ F2  │ F3  │ F4  │ F5  │ F6  │ F7  │  Frames
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │     │     │     │     │     │
│  Update  │  Update   │  Update   │  Update   │  Function calls
│     │     │     │     │     │     │     │     │
│     │     │     │     │     │     │     │     │
│ sleep(16) │ sleep(16) │ sleep(16) │ sleep(16) │  Delays
│     │     │     │     │     │     │     │     │

Each frame (16ms) consists of:
- Clear buffer (~0.1ms)
- Calculate 500 points (~5-8ms)
- Draw lines (~3-5ms)
- Update display (~2-3ms)
- Delay remainder to 16ms

Parameter change check happens every frame (0.5% probability)
```

## Frequency Ratio Patterns

```
1:1 Ratio               1:2 Ratio              2:3 Ratio
(Circle/Ellipse)        (Figure-8)             (Three-lobe)

     ╱─╲                   ╱│╲                  ╱╲  ╱╲
    │   │                 ╱ │ ╲                │  ╲╱  │
    │   │                │  │  │               │  │   │
     ╲─╱                 │  │  │                ╲─╱ ╲─╱
                         │  │  │
                          ╲─╱─╱

3:4 Ratio               3:5 Ratio              4:5 Ratio
(Complex knot)          (Five-pointed star)    (Dense weave)

   ╱─╲ ╱─╲                 ╱│╲                ╱─╲╱─╲
  │ ╳ ╳ ╳ │               ╱ │ ╲              │╳ ╳ ╳ │
  │ ╳ ╳ ╳ │              │╲─│─╱│             │╳ ╳ ╳ │
   ╲─╱ ╲─╱               │ ╲│╱ │              ╲─╱╲─╱
                          ╲─│─╱
```

## Integration Points

```
Your Application
    │
    ├─ Inactivity Detection
    │   └─> Enable screensaver
    │
    ├─ Main Loop / Timer
    │   └─> Call lissajous_screensaver_update()
    │
    ├─ User Input (Button/MIDI)
    │   └─> Disable screensaver, restore normal display
    │
    └─ Menu System (Optional)
        └─> Screensaver selection
            ├─ Bouncing Balls
            └─ Lissajous Curves ✓
```

## API Surface

```
┌─────────────────────────────────────────────────────────────┐
│               Public API (3 functions)                       │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  void lissajous_screensaver_init(void);                     │
│    Initialize screensaver with random parameters            │
│    Call once at startup or when activating                  │
│                                                              │
│  void lissajous_screensaver_update(void);                   │
│    Update and render one frame                              │
│    Call repeatedly at ~60 FPS                               │
│                                                              │
│  void lissajous_get_params(float* a, float* b, float* δ);   │
│    Get current parameters (for debugging)                   │
│    Optional - not required for normal operation             │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Dependencies

```
lissajous_screensaver.c
    │
    ├─ <math.h>
    │   └─ sinf() - Single precision sine function
    │
    ├─ <stdlib.h>
    │   └─ abs() - Absolute value
    │
    ├─ <string.h>
    │   └─ (future use)
    │
    └─ oled_display.h
        ├─ oled_clear()
        ├─ oled_set_pixel()
        └─ oled_display()
```

This architecture provides a clean separation between the Lissajous curve mathematics, the rendering logic, and the hardware display driver, making it easy to maintain and extend.
