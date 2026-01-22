#ifndef PCF857X_DRIVER_H
#define PCF857X_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// PCF857x default I2C address
#define PCF857X_DEFAULT_ADDRESS 0x20
#define PCF8575_DEFAULT_ADDRESS 0x20

// PCF857x chip types
typedef enum {
    PCF8574_CHIP = 0,  // 8-bit I/O expander
    PCF8575_CHIP = 1   // 16-bit I/O expander
} pcf857x_chip_type_t;

/**
 * PCF857x driver context structure (supports PCF8574 and PCF8575)
 */
typedef struct {
    i2c_inst_t *i2c_port;
    uint8_t address;
    uint16_t pin_state;           // 16-bit to support both chips
    pcf857x_chip_type_t chip_type;
    uint8_t num_pins;             // 8 for PCF8574, 16 for PCF8575
} pcf857x_t;

/**
 * Initialize PCF857x driver
 * 
 * @param ctx Pointer to PCF857x context structure
 * @param i2c_port I2C port to use (i2c0 or i2c1)
 * @param address I2C address of PCF857x
 * @param chip_type PCF8574_CHIP (8-bit) or PCF8575_CHIP (16-bit)
 * @return true if initialization successful, false otherwise
 */
bool pcf857x_init(pcf857x_t *ctx, i2c_inst_t *i2c_port, uint8_t address, pcf857x_chip_type_t chip_type);

/**
 * Write data to PCF857x
 * 
 * @param ctx Pointer to PCF857x context structure
 * @param data Data to write (8-bit for PCF8574, 16-bit for PCF8575)
 * @return true if successful, false otherwise
 */
bool pcf857x_write(pcf857x_t *ctx, uint16_t data);

/**
 * Read data from PCF857x
 * 
 * @param ctx Pointer to PCF857x context structure
 * @param data Pointer to store read data (16-bit)
 * @return true if successful, false otherwise
 */
bool pcf857x_read(pcf857x_t *ctx, uint16_t *data);

/**
 * Set a specific pin on the PCF857x
 * 
 * @param ctx Pointer to PCF857x context structure
 * @param pin Pin number (0-7 for PCF8574, 0-15 for PCF8575)
 * @param state true for HIGH, false for LOW
 * @return true if successful, false otherwise
 */
bool pcf857x_set_pin(pcf857x_t *ctx, uint8_t pin, bool state);

/**
 * Get the current pin state
 * 
 * @param ctx Pointer to PCF857x context structure
 * @return Current pin state (8-bit for PCF8574, 16-bit for PCF8575)
 */
uint16_t pcf857x_get_pin_state(pcf857x_t *ctx);

/**
 * Reset all pins to LOW
 * 
 * @param ctx Pointer to PCF857x context structure
 * @return true if successful, false otherwise
 */
bool pcf857x_reset(pcf857x_t *ctx);

/**
 * Get the number of pins available
 * 
 * @param ctx Pointer to PCF857x context structure
 * @return Number of pins (8 or 16)
 */
uint8_t pcf857x_get_num_pins(pcf857x_t *ctx);

#endif // PCF857X_DRIVER_H
