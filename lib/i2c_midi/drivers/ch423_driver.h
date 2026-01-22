#ifndef CH423_DRIVER_H
#define CH423_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// CH423 default I2C address
#define CH423_DEFAULT_ADDRESS 0x24

// CH423 Command bytes
#define CH423_CMD_WRITE_OC  0x01  // Write to open-collector outputs (OC0-OC7)
#define CH423_CMD_WRITE_PP  0x02  // Write to push-pull outputs (PP0-PP7)
#define CH423_CMD_READ_IO   0x03  // Read input status
#define CH423_CMD_SET_IO    0x04  // Set IO direction (0=output, 1=input)

/**
 * CH423 driver context structure
 */
typedef struct {
    i2c_inst_t *i2c_port;
    uint8_t address;
    uint16_t pin_state;       // Current state of all 16 pins
    uint16_t io_direction;    // Direction mask (0=output, 1=input)
} ch423_t;

/**
 * Initialize CH423 driver
 * 
 * @param ctx Pointer to CH423 context structure
 * @param i2c_port I2C port to use (i2c0 or i2c1)
 * @param address I2C address of CH423
 * @return true if initialization successful, false otherwise
 */
bool ch423_init(ch423_t *ctx, i2c_inst_t *i2c_port, uint8_t address);

/**
 * Write 16-bit value to CH423
 * Low byte (bits 0-7) goes to OC0-OC7 (open-collector outputs)
 * High byte (bits 8-15) goes to PP0-PP7 (push-pull outputs)
 * 
 * @param ctx Pointer to CH423 context structure
 * @param data 16-bit data to write
 * @return true if successful, false otherwise
 */
bool ch423_write(ch423_t *ctx, uint16_t data);

/**
 * Read 16-bit value from CH423
 * 
 * @param ctx Pointer to CH423 context structure
 * @param data Pointer to store read data (16-bit)
 * @return true if successful, false otherwise
 */
bool ch423_read(ch423_t *ctx, uint16_t *data);

/**
 * Set a specific pin on the CH423
 * 
 * @param ctx Pointer to CH423 context structure
 * @param pin Pin number (0-15)
 * @param state true for HIGH, false for LOW
 * @return true if successful, false otherwise
 */
bool ch423_set_pin(ch423_t *ctx, uint8_t pin, bool state);

/**
 * Get the current pin state
 * 
 * @param ctx Pointer to CH423 context structure
 * @return Current pin state as 16-bit mask
 */
uint16_t ch423_get_pin_state(ch423_t *ctx);

/**
 * Reset all pins to LOW
 * 
 * @param ctx Pointer to CH423 context structure
 * @return true if successful, false otherwise
 */
bool ch423_reset(ch423_t *ctx);

/**
 * Set IO direction for a pin (only if needed)
 * 
 * @param ctx Pointer to CH423 context structure
 * @param pin Pin number (0-15)
 * @param is_input true for input, false for output
 * @return true if successful, false otherwise
 */
bool ch423_set_io_direction(ch423_t *ctx, uint8_t pin, bool is_input);

#endif // CH423_DRIVER_H
