/**
 * @file lissajous_example.c
 * @brief Example demonstrating the Lissajous curve screensaver
 * 
 * This example shows how to integrate the Lissajous screensaver
 * into your main application loop.
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "oled_display.h"
#include "lissajous_screensaver.h"
#include <stdio.h>

// I2C Configuration
#define I2C_PORT i2c1
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define I2C_FREQ 400000

/**
 * Example 1: Simple continuous screensaver
 */
void example_simple_screensaver(void) {
    // Initialize hardware
    stdio_init_all();
    
    // Setup I2C for OLED
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED display
    if (!oled_init(I2C_PORT)) {
        printf("Failed to initialize OLED\n");
        return;
    }
    
    // Initialize screensaver
    lissajous_screensaver_init();
    
    printf("Lissajous screensaver started. Press Ctrl+C to exit.\n");
    
    // Main animation loop
    while (1) {
        lissajous_screensaver_update();
        sleep_ms(16);  // ~60 FPS
    }
}

/**
 * Example 2: Screensaver with timeout
 * Activates screensaver after period of inactivity
 */
void example_screensaver_with_timeout(void) {
    // Initialize hardware
    stdio_init_all();
    
    // Setup I2C for OLED
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED display
    if (!oled_init(I2C_PORT)) {
        printf("Failed to initialize OLED\n");
        return;
    }
    
    // Initialize screensaver
    lissajous_screensaver_init();
    
    bool screensaver_active = false;
    uint32_t last_activity = to_ms_since_boot(get_absolute_time());
    const uint32_t TIMEOUT_MS = 30000;  // 30 seconds
    
    printf("Screensaver with timeout example. Inactivity timeout: %d seconds\n", 
           TIMEOUT_MS / 1000);
    
    while (1) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Check for activity (button press, MIDI message, etc.)
        // This is a placeholder - replace with actual activity detection
        bool activity_detected = false;  // Replace with your logic
        
        if (activity_detected) {
            last_activity = current_time;
            if (screensaver_active) {
                screensaver_active = false;
                oled_clear();
                oled_draw_string(20, 28, "Activity!");
                oled_display();
            }
        }
        
        // Check timeout
        if ((current_time - last_activity) > TIMEOUT_MS) {
            if (!screensaver_active) {
                screensaver_active = true;
                lissajous_screensaver_init();  // Reinitialize for fresh start
                printf("Screensaver activated\n");
            }
        }
        
        // Update display
        if (screensaver_active) {
            lissajous_screensaver_update();
        }
        
        sleep_ms(16);  // ~60 FPS
    }
}

/**
 * Example 3: Display parameter info
 * Shows current Lissajous parameters on serial output
 */
void example_with_debug_info(void) {
    // Initialize hardware
    stdio_init_all();
    
    // Setup I2C for OLED
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED display
    if (!oled_init(I2C_PORT)) {
        printf("Failed to initialize OLED\n");
        return;
    }
    
    // Initialize screensaver
    lissajous_screensaver_init();
    
    uint32_t frame_count = 0;
    uint32_t last_param_print = 0;
    
    printf("Lissajous screensaver with debug info\n");
    
    while (1) {
        lissajous_screensaver_update();
        frame_count++;
        
        // Print parameters every 60 frames (approx 1 second)
        if (frame_count - last_param_print >= 60) {
            float a, b, phase;
            lissajous_get_params(&a, &b, &phase);
            printf("Frame %lu: freq_a=%.2f, freq_b=%.2f, phase=%.2f rad\n",
                   frame_count, a, b, phase);
            last_param_print = frame_count;
        }
        
        sleep_ms(16);  // ~60 FPS
    }
}

/**
 * Example 4: Switch between screensavers
 * Alternates between Lissajous and bouncing balls
 */
void example_alternating_screensavers(void) {
    // Initialize hardware
    stdio_init_all();
    
    // Setup I2C for OLED
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED display
    if (!oled_init(I2C_PORT)) {
        printf("Failed to initialize OLED\n");
        return;
    }
    
    // Initialize both screensavers
    lissajous_screensaver_init();
    oled_screensaver_init();  // Bouncing balls
    
    bool use_lissajous = true;
    uint32_t switch_time = to_ms_since_boot(get_absolute_time());
    const uint32_t SWITCH_INTERVAL_MS = 15000;  // Switch every 15 seconds
    
    printf("Alternating screensavers example\n");
    
    while (1) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Switch screensaver type
        if ((current_time - switch_time) > SWITCH_INTERVAL_MS) {
            use_lissajous = !use_lissajous;
            switch_time = current_time;
            
            // Reinitialize for smooth transition
            if (use_lissajous) {
                lissajous_screensaver_init();
                printf("Switched to Lissajous curves\n");
            } else {
                oled_screensaver_init();
                printf("Switched to bouncing balls\n");
            }
        }
        
        // Update active screensaver
        if (use_lissajous) {
            lissajous_screensaver_update();
        } else {
            oled_screensaver_update();
        }
        
        sleep_ms(16);  // ~60 FPS
    }
}

/**
 * Main entry point
 * Uncomment the example you want to run
 */
int main(void) {
    // Choose one example:
    
    example_simple_screensaver();
    // example_screensaver_with_timeout();
    // example_with_debug_info();
    // example_alternating_screensavers();
    
    return 0;
}
