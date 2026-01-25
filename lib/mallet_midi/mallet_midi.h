#ifndef MALLET_MIDI_H
#define MALLET_MIDI_H

#include <stdint.h>
#include <stdbool.h>

// Default configuration values
#define MALLET_MIDI_DEFAULT_NOTE_RANGE 8
#define MALLET_MIDI_DEFAULT_LOW_NOTE 60      // Middle C
#define MALLET_MIDI_DEFAULT_CHANNEL 10       // Percussion channel
#define MALLET_MIDI_DEFAULT_MIN_DEGREE 0     // Servo min position
#define MALLET_MIDI_DEFAULT_MAX_DEGREE 180   // Servo max position
#define MALLET_MIDI_DEFAULT_STRIKE_DURATION_MS 50  // How long to activate striker

// MIDI message types
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90

/**
 * Semitone handling modes
 */
typedef enum {
    MALLET_MIDI_SEMITONE_PLAY = 0,    // Play semitones normally
    MALLET_MIDI_SEMITONE_IGNORE = 1,  // Ignore/don't play semitones
    MALLET_MIDI_SEMITONE_SKIP = 2     // Skip semitone and play next whole tone (C# -> D)
} mallet_midi_semitone_mode_t;

/**
 * Configuration structure for Mallet MIDI library
 */
typedef struct {
    uint8_t note_range;                        // Number of notes to handle (default 8)
    uint8_t low_note;                          // Lowest note to respond to (default 60 - Middle C)
    uint8_t high_note;                         // Highest note (calculated based on note_range and semitone_mode)
    uint8_t midi_channel;                      // MIDI channel to listen to (default 10)
    uint8_t servo_pwm_slice;                   // PWM slice number for servo control
    uint8_t servo_gpio_pin;                    // GPIO pin for servo PWM signal
    uint8_t striker_gpio_pin;                  // GPIO pin to activate striker/mallet actuator
    uint16_t min_degree;                       // Minimum servo position in degrees (default 0)
    uint16_t max_degree;                       // Maximum servo position in degrees (default 180)
    uint16_t strike_duration_ms;               // Duration to activate striker in milliseconds
    mallet_midi_semitone_mode_t semitone_mode; // How to handle semitone notes
    float degree_per_step;                     // Calculated: degrees to move per note step
} mallet_midi_config_t;

/**
 * Mallet MIDI context structure
 */
typedef struct {
    mallet_midi_config_t config;
    uint8_t current_note;                      // Currently playing note (0 = none)
    uint16_t current_servo_position;           // Current servo position in degrees
    bool striker_active;                       // Is striker currently activated
    uint32_t striker_deactivate_time;          // Time when striker should be deactivated
} mallet_midi_t;

/**
 * Initialize the Mallet MIDI library with default configuration
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @param servo_gpio_pin GPIO pin for servo PWM signal
 * @param striker_gpio_pin GPIO pin to activate striker actuator
 * @return true if initialization successful, false otherwise
 */
bool mallet_midi_init(mallet_midi_t *ctx, uint8_t servo_gpio_pin, uint8_t striker_gpio_pin);

/**
 * Initialize the Mallet MIDI library with custom configuration
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @param config Pointer to configuration structure
 * @return true if initialization successful, false otherwise
 */
bool mallet_midi_init_with_config(mallet_midi_t *ctx, mallet_midi_config_t *config);

/**
 * Process a MIDI message
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @param status MIDI status byte (includes message type and channel)
 * @param note MIDI note number
 * @param velocity MIDI velocity (for NOTE ON, 0 velocity is treated as NOTE OFF)
 * @return true if message was processed, false if ignored
 */
bool mallet_midi_process_message(mallet_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity);

/**
 * Move servo to specific degree position
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @param degree Servo position in degrees (0-180 typically)
 * @return true if successful, false otherwise
 */
bool mallet_midi_move_servo(mallet_midi_t *ctx, uint16_t degree);

/**
 * Activate the striker/mallet actuator
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @return true if successful, false otherwise
 */
bool mallet_midi_activate_striker(mallet_midi_t *ctx);

/**
 * Deactivate the striker/mallet actuator
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @return true if successful, false otherwise
 */
bool mallet_midi_deactivate_striker(mallet_midi_t *ctx);

/**
 * Update function - call this regularly from main loop to handle timing
 * (e.g., deactivating striker after duration expires)
 * 
 * @param ctx Pointer to mallet_midi context structure
 */
void mallet_midi_update(mallet_midi_t *ctx);

/**
 * Set semitone handling mode
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @param mode Semitone mode (PLAY, IGNORE, or SKIP)
 * @return true if successful, false otherwise
 */
bool mallet_midi_set_semitone_mode(mallet_midi_t *ctx, mallet_midi_semitone_mode_t mode);

/**
 * Get current servo position in degrees
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @return Current servo position in degrees
 */
uint16_t mallet_midi_get_servo_position(mallet_midi_t *ctx);

/**
 * Calculate servo position for a given MIDI note
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @param note MIDI note number
 * @param out_degree Pointer to store calculated degree
 * @return true if note is in range, false otherwise
 */
bool mallet_midi_note_to_degree(mallet_midi_t *ctx, uint8_t note, uint16_t *out_degree);

/**
 * Reset to default position (lowest note position)
 * 
 * @param ctx Pointer to mallet_midi context structure
 * @return true if successful, false otherwise
 */
bool mallet_midi_reset(mallet_midi_t *ctx);

#endif // MALLET_MIDI_H
