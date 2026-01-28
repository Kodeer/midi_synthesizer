#include "pca9685_driver.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Debug output (can be disabled)
#ifdef PCA9685_DEBUG
#define PCA9685_PRINTF(...) printf(__VA_ARGS__)
#else
#define PCA9685_PRINTF(...)
#endif

/**
 * Write a single byte to a PCA9685 register
 */
static bool pca9685_write_register(pca9685_t *ctx, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 2, false);
    return result == 2;
}

/**
 * Read a single byte from a PCA9685 register
 */
static bool pca9685_read_register(pca9685_t *ctx, uint8_t reg, uint8_t *value) {
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, &reg, 1, true);
    if (result != 1) return false;
    
    result = i2c_read_blocking(ctx->i2c_port, ctx->address, value, 1, false);
    return result == 1;
}

/**
 * Software reset of PCA9685 via I2C general call
 */
static void pca9685_software_reset(i2c_inst_t *i2c_port) {
    uint8_t reset_cmd = 0x06;  // SWRST - Software Reset
    i2c_write_blocking(i2c_port, 0x00, &reset_cmd, 1, false);  // General call address
    sleep_ms(10);  // Wait for reset to complete
}

bool pca9685_init(pca9685_t *ctx, i2c_inst_t *i2c_port, uint8_t address, uint8_t frequency) {
    if (!ctx || !i2c_port) {
        PCA9685_PRINTF("PCA9685: Invalid parameters\n");
        return false;
    }
    
    ctx->i2c_port = i2c_port;
    ctx->address = address;
    ctx->frequency = frequency;
    ctx->initialized = false;
    
    // Perform software reset
    pca9685_software_reset(i2c_port);
    
    // Set MODE1 register: Auto-increment enabled, normal mode (not sleep)
    if (!pca9685_write_register(ctx, PCA9685_MODE1, PCA9685_MODE1_AI)) {
        PCA9685_PRINTF("PCA9685: Failed to write MODE1\n");
        return false;
    }
    
    // Set MODE2 register: Totem pole output (not open drain)
    if (!pca9685_write_register(ctx, PCA9685_MODE2, PCA9685_MODE2_OUTDRV)) {
        PCA9685_PRINTF("PCA9685: Failed to write MODE2\n");
        return false;
    }
    
    // Set PWM frequency
    if (!pca9685_set_pwm_frequency(ctx, frequency)) {
        PCA9685_PRINTF("PCA9685: Failed to set frequency\n");
        return false;
    }
    
    // Initialize all channels to 0 (servos at neutral/off)
    if (!pca9685_set_all_servos(ctx, 90)) {  // Start at 90 degrees (center)
        PCA9685_PRINTF("PCA9685: Failed to initialize channels\n");
        return false;
    }
    
    ctx->initialized = true;
    PCA9685_PRINTF("PCA9685: Initialized at address 0x%02X, frequency %dHz\n", address, frequency);
    
    return true;
}

bool pca9685_set_pwm_frequency(pca9685_t *ctx, uint8_t frequency) {
    if (!ctx || frequency < 24 || frequency > 1526) {
        PCA9685_PRINTF("PCA9685: Invalid frequency %d (must be 24-1526 Hz)\n", frequency);
        return false;
    }
    
    // Calculate prescale value
    // prescale = round(25MHz / (4096 * frequency)) - 1
    uint8_t prescale = (uint8_t)((PCA9685_INTERNAL_CLOCK / (4096.0f * frequency)) - 1 + 0.5f);
    
    PCA9685_PRINTF("PCA9685: Setting frequency to %dHz (prescale=%d)\n", frequency, prescale);
    
    // Read current MODE1 register
    uint8_t old_mode;
    if (!pca9685_read_register(ctx, PCA9685_MODE1, &old_mode)) {
        return false;
    }
    
    // Enter sleep mode to change prescale
    uint8_t sleep_mode = (old_mode & ~PCA9685_MODE1_RESTART) | PCA9685_MODE1_SLEEP;
    if (!pca9685_write_register(ctx, PCA9685_MODE1, sleep_mode)) {
        return false;
    }
    
    // Write prescale value
    if (!pca9685_write_register(ctx, PCA9685_PRESCALE, prescale)) {
        return false;
    }
    
    // Restore old mode
    if (!pca9685_write_register(ctx, PCA9685_MODE1, old_mode)) {
        return false;
    }
    
    sleep_ms(5);  // Wait for oscillator to stabilize
    
    // Restart with auto-increment
    if (!pca9685_write_register(ctx, PCA9685_MODE1, old_mode | PCA9685_MODE1_RESTART | PCA9685_MODE1_AI)) {
        return false;
    }
    
    ctx->frequency = frequency;
    return true;
}

bool pca9685_set_pwm(pca9685_t *ctx, uint8_t channel, uint16_t on_time, uint16_t off_time) {
    if (!ctx || !ctx->initialized || channel > 15) {
        return false;
    }
    
    // Clamp values to 12-bit
    on_time &= 0x0FFF;
    off_time &= 0x0FFF;
    
    // Calculate register address for this channel
    uint8_t reg_base = PCA9685_LED0_ON_L + (channel * 4);
    
    // Write all 4 bytes (ON_L, ON_H, OFF_L, OFF_H) using auto-increment
    uint8_t buffer[5];
    buffer[0] = reg_base;
    buffer[1] = on_time & 0xFF;         // ON_L
    buffer[2] = (on_time >> 8) & 0x0F;  // ON_H
    buffer[3] = off_time & 0xFF;        // OFF_L
    buffer[4] = (off_time >> 8) & 0x0F; // OFF_H
    
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 5, false);
    return result == 5;
}

bool pca9685_set_servo_pulse(pca9685_t *ctx, uint8_t channel, uint16_t pulse_us) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    // Clamp pulse width to safe servo range
    if (pulse_us < PCA9685_SERVO_MIN_PULSE_US) pulse_us = PCA9685_SERVO_MIN_PULSE_US;
    if (pulse_us > PCA9685_SERVO_MAX_PULSE_US) pulse_us = PCA9685_SERVO_MAX_PULSE_US;
    
    // Calculate OFF time based on pulse width
    // Each count = 1000000us / (frequency * 4096)
    // For 50Hz: each count = 1000000 / (50 * 4096) = 4.88us
    float pulse_length = 1000000.0f / ctx->frequency;  // Length of one cycle in us
    float count_per_us = 4096.0f / pulse_length;       // Counts per microsecond
    uint16_t off_time = (uint16_t)(pulse_us * count_per_us + 0.5f);
    
    return pca9685_set_pwm(ctx, channel, 0, off_time);
}

bool pca9685_set_servo_angle(pca9685_t *ctx, uint8_t channel, uint16_t degrees) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    // Clamp to 0-180 degrees
    if (degrees > 180) degrees = 180;
    
    // Map degrees to pulse width
    // 0° = 500us, 180° = 2500us
    // pulse_us = 500 + (degrees * 2000 / 180)
    uint16_t pulse_us = PCA9685_SERVO_MIN_PULSE_US + 
                        ((degrees * (PCA9685_SERVO_MAX_PULSE_US - PCA9685_SERVO_MIN_PULSE_US)) / 180);
    
    return pca9685_set_servo_pulse(ctx, channel, pulse_us);
}

bool pca9685_set_all_servos(pca9685_t *ctx, uint16_t degrees) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    // Clamp to 0-180 degrees
    if (degrees > 180) degrees = 180;
    
    // Calculate pulse width
    uint16_t pulse_us = PCA9685_SERVO_MIN_PULSE_US + 
                        ((degrees * (PCA9685_SERVO_MAX_PULSE_US - PCA9685_SERVO_MIN_PULSE_US)) / 180);
    
    // Calculate OFF time
    float pulse_length = 1000000.0f / ctx->frequency;
    float count_per_us = 4096.0f / pulse_length;
    uint16_t off_time = (uint16_t)(pulse_us * count_per_us + 0.5f);
    
    // Write to ALL_LED registers
    uint8_t buffer[5];
    buffer[0] = PCA9685_ALL_LED_ON_L;
    buffer[1] = 0;                          // ALL_LED_ON_L
    buffer[2] = 0;                          // ALL_LED_ON_H
    buffer[3] = off_time & 0xFF;            // ALL_LED_OFF_L
    buffer[4] = (off_time >> 8) & 0x0F;     // ALL_LED_OFF_H
    
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 5, false);
    return result == 5;
}

bool pca9685_reset(pca9685_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    pca9685_software_reset(ctx->i2c_port);
    ctx->initialized = false;
    
    // Re-initialize
    return pca9685_init(ctx, ctx->i2c_port, ctx->address, ctx->frequency);
}

bool pca9685_sleep(pca9685_t *ctx, bool sleep) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    uint8_t mode;
    if (!pca9685_read_register(ctx, PCA9685_MODE1, &mode)) {
        return false;
    }
    
    if (sleep) {
        mode |= PCA9685_MODE1_SLEEP;
    } else {
        mode &= ~PCA9685_MODE1_SLEEP;
    }
    
    return pca9685_write_register(ctx, PCA9685_MODE1, mode);
}
