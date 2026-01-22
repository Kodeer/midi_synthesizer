#ifndef I2C_MIDI_H
#define I2C_MIDI_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// Conditional driver includes based on CMake options
#ifdef USE_PCF857X_DRIVER
#include "drivers/pcf857x_driver.h"
#endif

#ifdef USE_CH423_DRIVER
#include "drivers/ch423_driver.h"
#endif

// Default configuration values
#define I2C_MIDI_DEFAULT_NOTE_RANGE 8
#define I2C_MIDI_DEFAULT_LOW_NOTE 60  // Middle C
#define I2C_MIDI_DEFAULT_CHANNEL 10   // Percussion channel

/**
 * Supported IO expander types
 */
typedef enum {
#ifdef USE_PCF857X_DRIVER
    IO_EXPANDER_PCF8574 = 0,  // PCF857x driver (PCF8574/PCF8575)
#endif
#ifdef USE_CH423_DRIVER
    IO_EXPANDER_CH423 = 1     // CH423 16-bit I/O expander
#endif
} io_expander_type_t;

/**
 * Semitone handling modes
 */
typedef enum {
    I2C_MIDI_SEMITONE_PLAY = 0,    // Play semitones normally
    I2C_MIDI_SEMITONE_IGNORE = 1,  // Ignore/don't play semitones
    I2C_MIDI_SEMITONE_SKIP = 2     // Skip semitone and play next whole tone (C# -> D)
} i2c_midi_semitone_mode_t;

// MIDI message types
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90

/**
 * Configuration structure for I2C MIDI library
 */
typedef struct {
    uint8_t note_range;                     // Number of notes to handle (default 8)
    uint8_t low_note;                       // Lowest note to respond to (default 60 - Middle C)
    uint8_t high_note;                      // Highest note (calculated based on note_range and semitone_mode)
    uint8_t midi_channel;                   // MIDI channel to listen to (default 10)
    uint8_t io_address;                     // I2C address of IO expander
    i2c_inst_t *i2c_port;                   // I2C port to use (i2c0 or i2c1)
    io_expander_type_t io_type;             // Type of IO expander to use
    i2c_midi_semitone_mode_t semitone_mode; // How to handle semitone notes
} i2c_midi_config_t;

/**
 * I2C MIDI context structure
 */
typedef struct {
    i2c_midi_config_t config;
    union {
#ifdef USE_PCF857X_DRIVER
        pcf857x_t pcf857x;    // PCF857x driver context (PCF8574/PCF8575)
#endif
#ifdef USE_CH423_DRIVER
        ch423_t ch423;        // CH423 driver context
#endif
    } driver;
    uint8_t pin_state;         // Current state of IO pins (bit mask)
} i2c_midi_t;

/**
 * Initialize the I2C MIDI library with default configuration
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @param i2c_port I2C port to use (i2c0 or i2c1)
 * @param sda_pin GPIO pin for SDA
 * @param scl_pin GPIO pin for SCL
 * @param baudrate I2C baudrate (typically 100000 or 400000)
 * @return true if initialization successful, false otherwise
 */
bool i2c_midi_init(i2c_midi_t *ctx, i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin, uint baudrate);

/**
 * Initialize the I2C MIDI library with custom configuration
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @param config Pointer to configuration structure
 * @param sda_pin GPIO pin for SDA
 * @param scl_pin GPIO pin for SCL
 * @param baudrate I2C baudrate (typically 100000 or 400000)
 * @return true if initialization successful, false otherwise
 */
bool i2c_midi_init_with_config(i2c_midi_t *ctx, i2c_midi_config_t *config, uint sda_pin, uint scl_pin, uint baudrate);

/**
 * Process a MIDI message
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @param status MIDI status byte (includes message type and channel)
 * @param note MIDI note number
 * @param velocity MIDI velocity (for NOTE ON, 0 velocity is treated as NOTE OFF)
 * @return true if message was processed, false if ignored
 */
bool i2c_midi_process_message(i2c_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity);

/**
 * Set a specific pin on the PCF8574
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @param pin Pin number (0-7)
 * @param state true for HIGH, false for LOW
 * @return true if successful, false otherwise
 */
bool i2c_midi_set_pin(i2c_midi_t *ctx, uint8_t pin, bool state);

/**
 * Get the current pin state
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @return Current pin state as 8-bit mask
 */
uint8_t i2c_midi_get_pin_state(i2c_midi_t *ctx);

/**
 * Set semitone handling mode
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @param mode Semitone mode (PLAY, IGNORE, or SKIP)
 * @return true if successful, false otherwise
 */
bool i2c_midi_set_semitone_mode(i2c_midi_t *ctx, i2c_midi_semitone_mode_t mode);

/**
 * Reset all pins to LOW
 * 
 * @param ctx Pointer to i2c_midi context structure
 * @return true if successful, false otherwise
 */
bool i2c_midi_reset(i2c_midi_t *ctx);

#endif // I2C_MIDI_H
