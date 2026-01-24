#include "lissajous_screensaver.h"
#include "oled_display.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

//--------------------------------------------------------------------+
// Lissajous Curve Screensaver Configuration
//--------------------------------------------------------------------+

#define LISSAJOUS_POINTS 500        // Number of points to draw per frame
#define LISSAJOUS_TIME_STEP 0.01f   // Time step between points
#define LISSAJOUS_TIME_INCREMENT 0.02f  // Time increment per frame

// Parameter change probability (1/N chance per frame)
#define PARAM_CHANGE_PROBABILITY 200

//--------------------------------------------------------------------+
// Lissajous Parameters
//--------------------------------------------------------------------+

typedef struct {
    float A;        // Amplitude X
    float B;        // Amplitude Y
    float a;        // Frequency X
    float b;        // Frequency Y
    float delta;    // Phase shift
    uint8_t hue;    // Color hue (for future color displays, unused on monochrome)
} lissajous_params_t;

static lissajous_params_t params;
static float current_time = 0.0f;
static uint32_t frame_count = 0;

//--------------------------------------------------------------------+
// Random Number Generator (simple LCG)
//--------------------------------------------------------------------+

static uint32_t lissajous_seed = 54321;

static uint32_t lissajous_rand(void) {
    lissajous_seed = (lissajous_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return lissajous_seed;
}

// Random float in range [min, max)
static float lissajous_rand_float(float min, float max) {
    float r = (float)(lissajous_rand() % 10000) / 10000.0f;
    return min + r * (max - min);
}

// Random integer in range [min, max]
static int lissajous_rand_int(int min, int max) {
    return min + (lissajous_rand() % (max - min + 1));
}

//--------------------------------------------------------------------+
// Parameter Generation
//--------------------------------------------------------------------+

static void generate_random_params(void) {
    // Amplitude - keep within reasonable bounds for 128x64 display
    // Center is at 64, 32, so max amplitude should be less than that
    params.A = (float)lissajous_rand_int(20, 50);
    params.B = (float)lissajous_rand_int(12, 26);
    
    // Frequency ratios - use simple ratios for interesting patterns
    // Common ratios: 1:1, 1:2, 2:3, 3:4, 3:5, 4:5, etc.
    int freq_options_a[] = {1, 1, 2, 3, 3, 4, 5, 2, 1};
    int freq_options_b[] = {1, 2, 3, 4, 5, 5, 4, 1, 3};
    int idx = lissajous_rand_int(0, 8);
    
    params.a = (float)freq_options_a[idx] * lissajous_rand_float(0.8f, 1.2f);
    params.b = (float)freq_options_b[idx] * lissajous_rand_float(0.8f, 1.2f);
    
    // Phase shift between 0 and 2*PI
    params.delta = lissajous_rand_float(0.0f, 2.0f * 3.14159265f);
    
    // Color hue (for future use with color displays)
    params.hue = lissajous_rand_int(0, 255);
}

//--------------------------------------------------------------------+
// Screensaver Functions
//--------------------------------------------------------------------+

void lissajous_screensaver_init(void) {
    // Seed random number generator with a varied value
    lissajous_seed = 54321;
    
    // Generate initial parameters
    generate_random_params();
    
    current_time = 0.0f;
    frame_count = 0;
}

void lissajous_screensaver_update(void) {
    // Occasionally change parameters for variety
    if ((lissajous_rand() % PARAM_CHANGE_PROBABILITY) == 0) {
        generate_random_params();
        current_time = 0.0f;  // Reset time for smooth transition
    }
    
    // Clear display
    oled_clear();
    
    // Calculate center of display
    const int center_x = OLED_WIDTH / 2;
    const int center_y = OLED_HEIGHT / 2;
    
    // Draw Lissajous curve
    int prev_x = -1;
    int prev_y = -1;
    
    for (int i = 0; i < LISSAJOUS_POINTS; i++) {
        float t = current_time + i * LISSAJOUS_TIME_STEP;
        
        // Lissajous parametric equations
        // x = A * sin(a * t + delta)
        // y = B * sin(b * t)
        float x = params.A * sinf(params.a * t + params.delta);
        float y = params.B * sinf(params.b * t);
        
        // Convert to screen coordinates
        int screen_x = center_x + (int)x;
        int screen_y = center_y + (int)y;
        
        // Clamp to display bounds
        if (screen_x < 0) screen_x = 0;
        if (screen_x >= OLED_WIDTH) screen_x = OLED_WIDTH - 1;
        if (screen_y < 0) screen_y = 0;
        if (screen_y >= OLED_HEIGHT) screen_y = OLED_HEIGHT - 1;
        
        // Draw pixel
        oled_set_pixel(screen_x, screen_y, 1);
        
        // Draw line from previous point for smoother curve
        if (prev_x >= 0) {
            // Simple line drawing using Bresenham's algorithm
            int dx = screen_x - prev_x;
            int dy = screen_y - prev_y;
            int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
            
            if (steps > 0 && steps < 20) {  // Only draw if distance is reasonable
                for (int j = 0; j <= steps; j++) {
                    int x_line = prev_x + (dx * j) / steps;
                    int y_line = prev_y + (dy * j) / steps;
                    
                    if (x_line >= 0 && x_line < OLED_WIDTH && 
                        y_line >= 0 && y_line < OLED_HEIGHT) {
                        oled_set_pixel(x_line, y_line, 1);
                    }
                }
            }
        }
        
        prev_x = screen_x;
        prev_y = screen_y;
    }
    
    // Update time
    current_time += LISSAJOUS_TIME_INCREMENT;
    
    // Keep time in reasonable range to avoid float overflow
    if (current_time > 1000.0f) {
        current_time = 0.0f;
    }
    
    frame_count++;
    
    // Update display
    oled_display();
}

void lissajous_get_params(float* a_freq, float* b_freq, float* phase) {
    if (a_freq) *a_freq = params.a;
    if (b_freq) *b_freq = params.b;
    if (phase) *phase = params.delta;
}
