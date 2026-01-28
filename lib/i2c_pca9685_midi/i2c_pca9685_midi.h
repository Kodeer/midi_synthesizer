#ifndef I2C_PCA9685_MIDI_H
#define I2C_PCA9685_MIDI_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "drivers/pca9685_driver.h"

// Default configuration values
#define PCA9685_MIDI_DEFAULT_NOTE_RANGE 16   // 16 servos
#define PCA9685_MIDI_DEFAULT_LOW_NOTE 60     // Middle C
#define PCA9685_MIDI_DEFAULT_CHANNEL 10      // Percussion channel
#define PCA9685_MIDI_DEFAULT_MIN_DEGREE 0    // Servo rest position
#define PCA9685_MIDI_DEFAULT_MAX_DEGREE 180  // Servo strike position
#define PCA9685_MIDI_DEFAULT_STRIKE_ANGLE 120  // Angle to strike at
#define PCA9685_MIDI_DEFAULT_REST_ANGLE 30     // Angle to rest at
#define PCA9685_MIDI_DEFAULT_STRIKE_DURATION_MS 50  // How long to hold strike position

// MIDI message types
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90

/**
 * Semitone handling modes
 */
typedef enum {
    PCA9685_MIDI_SEMITONE_PLAY = 0,    // Play semitones normally
    PCA9685_MIDI_SEMITONE_IGNORE = 1,  // Ignore/don't play semitones
    PCA9685_MIDI_SEMITONE_SKIP = 2     // Skip semitone and play next whole tone (C# -> D)
} pca9685_midi_semitone_mode_t;

/**
 * Strike mode - how servos move to strike notes
 */
typedef enum {
    PCA9685_STRIKE_MODE_SIMPLE = 0,    // All servos: rest→strike→rest
    PCA9685_STRIKE_MODE_POSITION = 1   // Each servo has unique position per note
} pca9685_strike_mode_t;

/**
 * Configuration structure for PCA9685 MIDI library
 */
typedef struct {
    uint8_t note_range;                           // Number of notes to handle (max 16)
    uint8_t low_note;                             // Lowest note to respond to
    uint8_t high_note;                            // Highest note (calculated)
    uint8_t midi_channel;                         // MIDI channel to listen to (0-15)
    uint8_t i2c_address;                          // I2C address of PCA9685
    i2c_inst_t *i2c_port;                         // I2C port (i2c0 or i2c1)
    pca9685_midi_semitone_mode_t semitone_mode;   // How to handle semitone notes
    pca9685_strike_mode_t strike_mode;            // How servos strike notes
    
    // Strike mode angles (for SIMPLE mode)
    uint16_t rest_angle;                          // Servo rest position (default 30°)
    uint16_t strike_angle;                        // Servo strike position (default 120°)
    uint16_t strike_duration_ms;                  // Hold strike position duration
    
    // Position mode settings (for POSITION mode)
    uint16_t min_degree;                          // Minimum servo position
    uint16_t max_degree;                          // Maximum servo position
} pca9685_midi_config_t;

/**
 * Servo state tracking
 */
typedef struct {
    uint8_t current_note;                         // Currently assigned note (0 = none)
    uint16_t current_angle;                       // Current servo angle
    bool striking;                                // Is currently in strike position
    uint32_t return_time;                         // Time to return to rest position
} servo_state_t;

/**
 * PCA9685 MIDI context structure
 */
typedef struct {
    pca9685_midi_config_t config;
    pca9685_t pca9685;                            // PCA9685 driver context
    servo_state_t servo_states[16];               // State for each servo
    bool initialized;
} pca9685_midi_t;

/**
 * Initialize the PCA9685 MIDI library with default configuration
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param i2c_port I2C port to use (i2c0 or i2c1)
 * @param sda_pin GPIO pin for I2C SDA
 * @param scl_pin GPIO pin for I2C SCL
 * @param i2c_speed I2C speed in Hz (typically 100000 or 400000)
 * @return true if initialization successful, false otherwise
 */
bool pca9685_midi_init(pca9685_midi_t *ctx, i2c_inst_t *i2c_port, uint8_t sda_pin, uint8_t scl_pin, uint32_t i2c_speed);

/**
 * Initialize the PCA9685 MIDI library with custom configuration
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param config Pointer to configuration structure
 * @param sda_pin GPIO pin for I2C SDA
 * @param scl_pin GPIO pin for I2C SCL
 * @param i2c_speed I2C speed in Hz
 * @return true if initialization successful, false otherwise
 */
bool pca9685_midi_init_with_config(pca9685_midi_t *ctx, pca9685_midi_config_t *config, 
                                   uint8_t sda_pin, uint8_t scl_pin, uint32_t i2c_speed);

/**
 * Process a MIDI message
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param status MIDI status byte (includes message type and channel)
 * @param note MIDI note number (0-127)
 * @param velocity MIDI velocity (0-127, 0 = note off)
 * @return true if message was processed, false if ignored
 */
bool pca9685_midi_process_message(pca9685_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity);

/**
 * Update servo states (must be called regularly from main loop)
 * Handles automatic return to rest position after strike duration
 * 
 * @param ctx Pointer to pca9685_midi context structure
 */
void pca9685_midi_update(pca9685_midi_t *ctx);

/**
 * Set semitone handling mode
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param mode Semitone mode (PLAY, IGNORE, or SKIP)
 * @return true if successful, false otherwise
 */
bool pca9685_midi_set_semitone_mode(pca9685_midi_t *ctx, pca9685_midi_semitone_mode_t mode);

/**
 * Set strike mode
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param mode Strike mode (SIMPLE or POSITION)
 * @return true if successful, false otherwise
 */
bool pca9685_midi_set_strike_mode(pca9685_midi_t *ctx, pca9685_strike_mode_t mode);

/**
 * Manually strike a note on a specific servo
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param servo_index Servo index (0-15)
 * @return true if successful, false otherwise
 */
bool pca9685_midi_strike_servo(pca9685_midi_t *ctx, uint8_t servo_index);

/**
 * Return all servos to rest position
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @return true if successful, false otherwise
 */
bool pca9685_midi_all_notes_off(pca9685_midi_t *ctx);

/**
 * Reset the PCA9685 MIDI system
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @return true if successful, false otherwise
 */
bool pca9685_midi_reset(pca9685_midi_t *ctx);

/**
 * Get the servo index for a given MIDI note
 * 
 * @param ctx Pointer to pca9685_midi context structure
 * @param note MIDI note number
 * @param servo_index Output: servo index (0-15)
 * @return true if note is in range, false otherwise
 */
bool pca9685_midi_note_to_servo(pca9685_midi_t *ctx, uint8_t note, uint8_t *servo_index);

#endif // I2C_PCA9685_MIDI_H
