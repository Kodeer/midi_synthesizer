#ifndef PCA9685_DRIVER_H
#define PCA9685_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// PCA9685 I2C default address
#define PCA9685_DEFAULT_ADDRESS 0x40

// PCA9685 Register addresses
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_SUBADR1 0x02
#define PCA9685_SUBADR2 0x03
#define PCA9685_SUBADR3 0x04
#define PCA9685_PRESCALE 0xFE
#define PCA9685_LED0_ON_L 0x06
#define PCA9685_LED0_ON_H 0x07
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_LED0_OFF_H 0x09
#define PCA9685_ALL_LED_ON_L 0xFA
#define PCA9685_ALL_LED_ON_H 0xFB
#define PCA9685_ALL_LED_OFF_L 0xFC
#define PCA9685_ALL_LED_OFF_H 0xFD

// MODE1 register bits
#define PCA9685_MODE1_RESTART 0x80
#define PCA9685_MODE1_SLEEP 0x10
#define PCA9685_MODE1_ALLCALL 0x01
#define PCA9685_MODE1_AI 0x20  // Auto-increment

// MODE2 register bits
#define PCA9685_MODE2_OUTDRV 0x04  // Totem pole (vs open drain)
#define PCA9685_MODE2_INVRT 0x10   // Invert output

// PWM frequency settings
#define PCA9685_INTERNAL_CLOCK 25000000  // 25MHz internal oscillator
#define PCA9685_DEFAULT_FREQUENCY 50     // 50Hz for servos

// Servo pulse width constants (in microseconds)
#define PCA9685_SERVO_MIN_PULSE_US 500   // 0.5ms = 0 degrees
#define PCA9685_SERVO_MAX_PULSE_US 2500  // 2.5ms = 180 degrees

/**
 * PCA9685 context structure
 */
typedef struct {
    i2c_inst_t *i2c_port;    // I2C port (i2c0 or i2c1)
    uint8_t address;         // I2C address of PCA9685
    uint8_t frequency;       // PWM frequency in Hz
    bool initialized;        // Initialization flag
} pca9685_t;

/**
 * Initialize the PCA9685 driver
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param i2c_port I2C port to use (i2c0 or i2c1)
 * @param address I2C address of PCA9685 (default 0x40)
 * @param frequency PWM frequency in Hz (default 50Hz for servos)
 * @return true if initialization successful, false otherwise
 */
bool pca9685_init(pca9685_t *ctx, i2c_inst_t *i2c_port, uint8_t address, uint8_t frequency);

/**
 * Set PWM frequency
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param frequency Frequency in Hz (24-1526 Hz, typically 50Hz for servos)
 * @return true if successful, false otherwise
 */
bool pca9685_set_pwm_frequency(pca9685_t *ctx, uint8_t frequency);

/**
 * Set PWM duty cycle for a channel
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param channel Channel number (0-15)
 * @param on_time ON time value (0-4095, 12-bit)
 * @param off_time OFF time value (0-4095, 12-bit)
 * @return true if successful, false otherwise
 */
bool pca9685_set_pwm(pca9685_t *ctx, uint8_t channel, uint16_t on_time, uint16_t off_time);

/**
 * Set servo position in degrees
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param channel Channel number (0-15)
 * @param degrees Servo position in degrees (0-180)
 * @return true if successful, false otherwise
 */
bool pca9685_set_servo_angle(pca9685_t *ctx, uint8_t channel, uint16_t degrees);

/**
 * Set servo position using pulse width in microseconds
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param channel Channel number (0-15)
 * @param pulse_us Pulse width in microseconds (500-2500)
 * @return true if successful, false otherwise
 */
bool pca9685_set_servo_pulse(pca9685_t *ctx, uint8_t channel, uint16_t pulse_us);

/**
 * Set all servos to the same position
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param degrees Servo position in degrees (0-180)
 * @return true if successful, false otherwise
 */
bool pca9685_set_all_servos(pca9685_t *ctx, uint16_t degrees);

/**
 * Reset the PCA9685 chip
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @return true if successful, false otherwise
 */
bool pca9685_reset(pca9685_t *ctx);

/**
 * Put PCA9685 into sleep mode (low power)
 * 
 * @param ctx Pointer to PCA9685 context structure
 * @param sleep true to enter sleep mode, false to wake
 * @return true if successful, false otherwise
 */
bool pca9685_sleep(pca9685_t *ctx, bool sleep);

#endif // PCA9685_DRIVER_H
