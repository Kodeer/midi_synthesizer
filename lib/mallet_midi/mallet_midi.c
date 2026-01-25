#include "mallet_midi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/time.h"
#include <string.h>

// Include debug UART if available
#ifdef DEBUG_UART_ENABLED
#include "../../src/debug_uart.h"
#else
#define debug_info(...)
#define debug_error(...)
#define debug_warning(...)
#endif

//--------------------------------------------------------------------+
// Servo Control Constants
//--------------------------------------------------------------------+

// Standard servo PWM frequency is 50Hz (20ms period)
#define SERVO_PWM_FREQ 50
#define SERVO_PWM_PERIOD_US 20000  // 20ms in microseconds

// Standard servo pulse widths (in microseconds)
#define SERVO_MIN_PULSE_US 500     // 0.5ms = 0 degrees
#define SERVO_MAX_PULSE_US 2500    // 2.5ms = 180 degrees

//--------------------------------------------------------------------+
// Semitone Handling Functions
//--------------------------------------------------------------------+

/**
 * Check if a MIDI note is a semitone (sharp/flat note)
 * Semitones are: C# D# F# G# A# (positions 1, 3, 6, 8, 10 in the octave)
 */
static bool is_semitone(uint8_t note) {
    uint8_t note_in_octave = note % 12;
    return (note_in_octave == 1 || note_in_octave == 3 || 
            note_in_octave == 6 || note_in_octave == 8 || note_in_octave == 10);
}

/**
 * Calculate the high note based on low note, range, and semitone mode
 */
static uint8_t calculate_high_note(uint8_t low_note, uint8_t note_range, mallet_midi_semitone_mode_t mode) {
    if (mode == MALLET_MIDI_SEMITONE_PLAY) {
        // Normal mode: high_note = low_note + range - 1
        return low_note + note_range - 1;
    } else {
        // For IGNORE or SKIP modes, we need to count only whole tones
        uint8_t count = 0;
        uint8_t current_note = low_note;
        
        while (count < note_range) {
            if (!is_semitone(current_note)) {
                count++;
            }
            if (count < note_range) {
                current_note++;
            }
        }
        return current_note;
    }
}

/**
 * Map a MIDI note to its position in the configured range
 * Returns the position (0 to note_range-1) or -1 if note is out of range
 */
static int8_t note_to_position(mallet_midi_t *ctx, uint8_t note) {
    // Check if note is within range
    if (note < ctx->config.low_note || note > ctx->config.high_note) {
        return -1;
    }
    
    // Handle different semitone modes
    if (ctx->config.semitone_mode == MALLET_MIDI_SEMITONE_IGNORE && is_semitone(note)) {
        return -1;  // Ignore semitones
    }
    
    if (ctx->config.semitone_mode == MALLET_MIDI_SEMITONE_SKIP && is_semitone(note)) {
        // Find next whole tone
        note++;
        if (note > ctx->config.high_note) {
            return -1;
        }
    }
    
    // Count position considering semitone mode
    int8_t position = 0;
    for (uint8_t n = ctx->config.low_note; n < note; n++) {
        if (ctx->config.semitone_mode == MALLET_MIDI_SEMITONE_PLAY || !is_semitone(n)) {
            position++;
        }
    }
    
    return position;
}

//--------------------------------------------------------------------+
// Servo Control Functions
//--------------------------------------------------------------------+

/**
 * Convert degree to PWM pulse width in microseconds
 */
static uint16_t degree_to_pulse_us(uint16_t degree) {
    if (degree > 180) degree = 180;
    
    // Map 0-180 degrees to SERVO_MIN_PULSE_US to SERVO_MAX_PULSE_US
    return SERVO_MIN_PULSE_US + ((degree * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / 180);
}

/**
 * Set servo position using PWM
 */
static bool set_servo_pwm(mallet_midi_t *ctx, uint16_t degree) {
    uint16_t pulse_us = degree_to_pulse_us(degree);
    
    // Get PWM slice
    uint slice_num = ctx->config.servo_pwm_slice;
    
    // Calculate PWM wrap value for 50Hz (20ms period)
    // System clock is typically 125MHz
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t divider = 64;  // Use divider to get manageable wrap value
    uint32_t wrap = (clock_freq / divider / SERVO_PWM_FREQ) - 1;
    
    // Calculate duty cycle for desired pulse width
    uint32_t level = (pulse_us * (wrap + 1)) / SERVO_PWM_PERIOD_US;
    
    pwm_set_clkdiv(slice_num, (float)divider);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, level);
    pwm_set_enabled(slice_num, true);
    
    return true;
}

//--------------------------------------------------------------------+
// Public API Implementation
//--------------------------------------------------------------------+

bool mallet_midi_init(mallet_midi_t *ctx, uint8_t servo_gpio_pin, uint8_t striker_gpio_pin) {
    if (ctx == NULL) {
        debug_error("MALLET_MIDI: NULL context pointer");
        return false;
    }
    
    // Initialize with default config
    memset(ctx, 0, sizeof(mallet_midi_t));
    ctx->config.note_range = MALLET_MIDI_DEFAULT_NOTE_RANGE;
    ctx->config.low_note = MALLET_MIDI_DEFAULT_LOW_NOTE;
    ctx->config.midi_channel = MALLET_MIDI_DEFAULT_CHANNEL;
    ctx->config.min_degree = MALLET_MIDI_DEFAULT_MIN_DEGREE;
    ctx->config.max_degree = MALLET_MIDI_DEFAULT_MAX_DEGREE;
    ctx->config.strike_duration_ms = MALLET_MIDI_DEFAULT_STRIKE_DURATION_MS;
    ctx->config.semitone_mode = MALLET_MIDI_SEMITONE_PLAY;
    ctx->config.servo_gpio_pin = servo_gpio_pin;
    ctx->config.striker_gpio_pin = striker_gpio_pin;
    
    // Calculate high note
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, 
                                                 ctx->config.note_range, 
                                                 ctx->config.semitone_mode);
    
    // Calculate degree per step
    ctx->config.degree_per_step = (float)(ctx->config.max_degree - ctx->config.min_degree) / 
                                  (float)(ctx->config.note_range - 1);
    
    // Setup servo PWM
    gpio_set_function(servo_gpio_pin, GPIO_FUNC_PWM);
    ctx->config.servo_pwm_slice = pwm_gpio_to_slice_num(servo_gpio_pin);
    
    // Setup striker GPIO
    gpio_init(striker_gpio_pin);
    gpio_set_dir(striker_gpio_pin, GPIO_OUT);
    gpio_put(striker_gpio_pin, false);
    
    // Initialize position to lowest note
    ctx->current_servo_position = ctx->config.min_degree;
    set_servo_pwm(ctx, ctx->current_servo_position);
    
    debug_info("MALLET_MIDI: Initialized - Notes %d-%d (Ch %d), Servo %d-%d deg, Step: %.2f deg/note",
               ctx->config.low_note, ctx->config.high_note, ctx->config.midi_channel + 1,
               ctx->config.min_degree, ctx->config.max_degree, ctx->config.degree_per_step);
    
    return true;
}

bool mallet_midi_init_with_config(mallet_midi_t *ctx, mallet_midi_config_t *config) {
    if (ctx == NULL || config == NULL) {
        debug_error("MALLET_MIDI: NULL pointer");
        return false;
    }
    
    // Copy configuration
    memcpy(&ctx->config, config, sizeof(mallet_midi_config_t));
    
    // Recalculate high note and degree per step
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, 
                                                 ctx->config.note_range, 
                                                 ctx->config.semitone_mode);
    
    ctx->config.degree_per_step = (float)(ctx->config.max_degree - ctx->config.min_degree) / 
                                  (float)(ctx->config.note_range - 1);
    
    // Setup servo PWM
    gpio_set_function(ctx->config.servo_gpio_pin, GPIO_FUNC_PWM);
    ctx->config.servo_pwm_slice = pwm_gpio_to_slice_num(ctx->config.servo_gpio_pin);
    
    // Setup striker GPIO
    gpio_init(ctx->config.striker_gpio_pin);
    gpio_set_dir(ctx->config.striker_gpio_pin, GPIO_OUT);
    gpio_put(ctx->config.striker_gpio_pin, false);
    
    // Initialize position
    ctx->current_servo_position = ctx->config.min_degree;
    ctx->striker_active = false;
    set_servo_pwm(ctx, ctx->current_servo_position);
    
    debug_info("MALLET_MIDI: Configured - Notes %d-%d (Ch %d), Servo %d-%d deg, Step: %.2f deg/note",
               ctx->config.low_note, ctx->config.high_note, ctx->config.midi_channel + 1,
               ctx->config.min_degree, ctx->config.max_degree, ctx->config.degree_per_step);
    
    return true;
}

bool mallet_midi_process_message(mallet_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity) {
    if (ctx == NULL) {
        return false;
    }
    
    // Extract message type and channel
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    // Check if this is our channel
    if (channel != ctx->config.midi_channel) {
        return false;
    }
    
    // Handle NOTE ON and NOTE OFF
    if (msg_type == MIDI_NOTE_ON && velocity > 0) {
        // Note ON
        uint16_t degree;
        if (mallet_midi_note_to_degree(ctx, note, &degree)) {
            // Move servo to position
            mallet_midi_move_servo(ctx, degree);
            
            // Small delay to allow servo to reach position
            sleep_ms(10);
            
            // Activate striker
            mallet_midi_activate_striker(ctx);
            
            ctx->current_note = note;
            
            return true;
        }
    } else if (msg_type == MIDI_NOTE_OFF || (msg_type == MIDI_NOTE_ON && velocity == 0)) {
        // Note OFF - just record it
        if (ctx->current_note == note) {
            ctx->current_note = 0;
        }
        return true;
    }
    
    return false;
}

bool mallet_midi_move_servo(mallet_midi_t *ctx, uint16_t degree) {
    if (ctx == NULL) {
        return false;
    }
    
    // Clamp to configured range
    if (degree < ctx->config.min_degree) degree = ctx->config.min_degree;
    if (degree > ctx->config.max_degree) degree = ctx->config.max_degree;
    
    ctx->current_servo_position = degree;
    return set_servo_pwm(ctx, degree);
}

bool mallet_midi_activate_striker(mallet_midi_t *ctx) {
    if (ctx == NULL) {
        return false;
    }
    
    gpio_put(ctx->config.striker_gpio_pin, true);
    ctx->striker_active = true;
    ctx->striker_deactivate_time = to_ms_since_boot(get_absolute_time()) + ctx->config.strike_duration_ms;
    
    return true;
}

bool mallet_midi_deactivate_striker(mallet_midi_t *ctx) {
    if (ctx == NULL) {
        return false;
    }
    
    gpio_put(ctx->config.striker_gpio_pin, false);
    ctx->striker_active = false;
    
    return true;
}

void mallet_midi_update(mallet_midi_t *ctx) {
    if (ctx == NULL || !ctx->striker_active) {
        return;
    }
    
    // Check if it's time to deactivate striker
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time >= ctx->striker_deactivate_time) {
        mallet_midi_deactivate_striker(ctx);
    }
}

bool mallet_midi_set_semitone_mode(mallet_midi_t *ctx, mallet_midi_semitone_mode_t mode) {
    if (ctx == NULL) {
        return false;
    }
    
    ctx->config.semitone_mode = mode;
    
    // Recalculate high note and degree per step
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, 
                                                 ctx->config.note_range, 
                                                 ctx->config.semitone_mode);
    
    ctx->config.degree_per_step = (float)(ctx->config.max_degree - ctx->config.min_degree) / 
                                  (float)(ctx->config.note_range - 1);
    
    debug_info("MALLET_MIDI: Semitone mode set to %d, high note now %d, step: %.2f deg",
               mode, ctx->config.high_note, ctx->config.degree_per_step);
    
    return true;
}

uint16_t mallet_midi_get_servo_position(mallet_midi_t *ctx) {
    if (ctx == NULL) {
        return 0;
    }
    return ctx->current_servo_position;
}

bool mallet_midi_note_to_degree(mallet_midi_t *ctx, uint8_t note, uint16_t *out_degree) {
    if (ctx == NULL || out_degree == NULL) {
        return false;
    }
    
    // Get position in range
    int8_t position = note_to_position(ctx, note);
    if (position < 0) {
        return false;
    }
    
    // Calculate degree based on position
    *out_degree = ctx->config.min_degree + (uint16_t)(position * ctx->config.degree_per_step);
    
    // Clamp to max degree
    if (*out_degree > ctx->config.max_degree) {
        *out_degree = ctx->config.max_degree;
    }
    
    return true;
}

bool mallet_midi_reset(mallet_midi_t *ctx) {
    if (ctx == NULL) {
        return false;
    }
    
    // Deactivate striker
    mallet_midi_deactivate_striker(ctx);
    
    // Move to lowest note position
    mallet_midi_move_servo(ctx, ctx->config.min_degree);
    
    ctx->current_note = 0;
    
    debug_info("MALLET_MIDI: Reset to initial position");
    return true;
}
