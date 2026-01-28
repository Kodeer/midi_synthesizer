#include "i2c_pca9685_midi.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

// Debug output (can be disabled)
#ifdef PCA9685_MIDI_DEBUG
#define PCA9685_MIDI_PRINTF(...) printf(__VA_ARGS__)
#else
#define PCA9685_MIDI_PRINTF(...)
#endif

/**
 * Check if a note is a semitone (sharp/flat)
 */
static bool is_semitone(uint8_t note) {
    uint8_t note_in_octave = note % 12;
    // Semitones are: C# (1), D# (3), F# (6), G# (8), A# (10)
    return (note_in_octave == 1 || note_in_octave == 3 || note_in_octave == 6 || 
            note_in_octave == 8 || note_in_octave == 10);
}

/**
 * Calculate high note based on note range and semitone mode
 */
static uint8_t calculate_high_note(uint8_t low_note, uint8_t note_range, pca9685_midi_semitone_mode_t mode) {
    if (mode == PCA9685_MIDI_SEMITONE_PLAY) {
        // Include all notes including semitones
        return low_note + note_range - 1;
    } else {
        // Only count whole tones
        uint8_t current_note = low_note;
        uint8_t count = 0;
        
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

bool pca9685_midi_init(pca9685_midi_t *ctx, i2c_inst_t *i2c_port, uint8_t sda_pin, uint8_t scl_pin, uint32_t i2c_speed) {
    if (!ctx || !i2c_port) {
        PCA9685_MIDI_PRINTF("PCA9685_MIDI: Invalid parameters\n");
        return false;
    }
    
    // Initialize with default configuration
    pca9685_midi_config_t default_config = {
        .note_range = PCA9685_MIDI_DEFAULT_NOTE_RANGE,
        .low_note = PCA9685_MIDI_DEFAULT_LOW_NOTE,
        .high_note = 0,  // Will be calculated
        .midi_channel = PCA9685_MIDI_DEFAULT_CHANNEL,
        .i2c_address = PCA9685_DEFAULT_ADDRESS,
        .i2c_port = i2c_port,
        .semitone_mode = PCA9685_MIDI_SEMITONE_PLAY,
        .strike_mode = PCA9685_STRIKE_MODE_SIMPLE,
        .rest_angle = PCA9685_MIDI_DEFAULT_REST_ANGLE,
        .strike_angle = PCA9685_MIDI_DEFAULT_STRIKE_ANGLE,
        .strike_duration_ms = PCA9685_MIDI_DEFAULT_STRIKE_DURATION_MS,
        .min_degree = PCA9685_MIDI_DEFAULT_MIN_DEGREE,
        .max_degree = PCA9685_MIDI_DEFAULT_MAX_DEGREE
    };
    
    return pca9685_midi_init_with_config(ctx, &default_config, sda_pin, scl_pin, i2c_speed);
}

bool pca9685_midi_init_with_config(pca9685_midi_t *ctx, pca9685_midi_config_t *config,
                                   uint8_t sda_pin, uint8_t scl_pin, uint32_t i2c_speed) {
    if (!ctx || !config) {
        PCA9685_MIDI_PRINTF("PCA9685_MIDI: Invalid parameters\n");
        return false;
    }
    
    // Validate configuration
    if (config->note_range > 16) {
        PCA9685_MIDI_PRINTF("PCA9685_MIDI: Note range limited to 16 (got %d)\n", config->note_range);
        config->note_range = 16;
    }
    
    // Copy configuration
    memcpy(&ctx->config, config, sizeof(pca9685_midi_config_t));
    
    // Calculate high note
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, ctx->config.note_range, 
                                                 ctx->config.semitone_mode);
    
    // Initialize I2C
    i2c_init(config->i2c_port, i2c_speed);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    // Initialize PCA9685 driver
    if (!pca9685_init(&ctx->pca9685, config->i2c_port, config->i2c_address, PCA9685_DEFAULT_FREQUENCY)) {
        PCA9685_MIDI_PRINTF("PCA9685_MIDI: Failed to initialize PCA9685 driver\n");
        return false;
    }
    
    // Initialize servo states
    for (int i = 0; i < 16; i++) {
        ctx->servo_states[i].current_note = 0;
        ctx->servo_states[i].current_angle = ctx->config.rest_angle;
        ctx->servo_states[i].striking = false;
        ctx->servo_states[i].return_time = 0;
    }
    
    // Set all servos to rest position
    pca9685_set_all_servos(&ctx->pca9685, ctx->config.rest_angle);
    
    ctx->initialized = true;
    
    PCA9685_MIDI_PRINTF("PCA9685_MIDI: Initialized\n");
    PCA9685_MIDI_PRINTF("  Channel: %d\n", ctx->config.midi_channel + 1);
    PCA9685_MIDI_PRINTF("  Note range: %d-%d (%d notes)\n", 
                        ctx->config.low_note, ctx->config.high_note, ctx->config.note_range);
    PCA9685_MIDI_PRINTF("  Strike mode: %s\n", 
                        ctx->config.strike_mode == PCA9685_STRIKE_MODE_SIMPLE ? "SIMPLE" : "POSITION");
    
    return true;
}

bool pca9685_midi_note_to_servo(pca9685_midi_t *ctx, uint8_t note, uint8_t *servo_index) {
    if (!ctx || !ctx->initialized || !servo_index) {
        return false;
    }
    
    // Check if note is in range
    if (note < ctx->config.low_note || note > ctx->config.high_note) {
        return false;
    }
    
    // Handle semitone modes
    if (ctx->config.semitone_mode == PCA9685_MIDI_SEMITONE_IGNORE && is_semitone(note)) {
        return false;  // Ignore semitones
    }
    
    if (ctx->config.semitone_mode == PCA9685_MIDI_SEMITONE_SKIP && is_semitone(note)) {
        note++;  // Skip to next whole tone
        if (note > ctx->config.high_note) {
            return false;
        }
    }
    
    // Calculate servo index
    if (ctx->config.semitone_mode == PCA9685_MIDI_SEMITONE_PLAY) {
        // Direct mapping
        *servo_index = note - ctx->config.low_note;
    } else {
        // Count only whole tones
        uint8_t index = 0;
        for (uint8_t n = ctx->config.low_note; n < note; n++) {
            if (!is_semitone(n)) {
                index++;
            }
        }
        *servo_index = index;
    }
    
    // Ensure we don't exceed available servos
    if (*servo_index >= ctx->config.note_range) {
        return false;
    }
    
    return true;
}

bool pca9685_midi_strike_servo(pca9685_midi_t *ctx, uint8_t servo_index) {
    if (!ctx || !ctx->initialized || servo_index >= 16) {
        return false;
    }
    
    uint16_t strike_angle;
    
    if (ctx->config.strike_mode == PCA9685_STRIKE_MODE_SIMPLE) {
        // Simple mode: all servos strike at same angle
        strike_angle = ctx->config.strike_angle;
    } else {
        // Position mode: calculate unique angle for this servo
        // Map servo index to angle range
        if (ctx->config.note_range > 1) {
            strike_angle = ctx->config.min_degree + 
                          ((servo_index * (ctx->config.max_degree - ctx->config.min_degree)) / 
                           (ctx->config.note_range - 1));
        } else {
            strike_angle = (ctx->config.min_degree + ctx->config.max_degree) / 2;
        }
    }
    
    // Move servo to strike position
    if (!pca9685_set_servo_angle(&ctx->pca9685, servo_index, strike_angle)) {
        return false;
    }
    
    // Update state
    ctx->servo_states[servo_index].current_angle = strike_angle;
    ctx->servo_states[servo_index].striking = true;
    ctx->servo_states[servo_index].return_time = time_us_32() + (ctx->config.strike_duration_ms * 1000);
    
    return true;
}

bool pca9685_midi_process_message(pca9685_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    // Extract message type and channel
    uint8_t message_type = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    // Check if this message is for our channel
    if (channel != ctx->config.midi_channel) {
        return false;
    }
    
    // Handle NOTE ON and NOTE OFF
    if (message_type == MIDI_NOTE_ON && velocity > 0) {
        // NOTE ON
        uint8_t servo_index;
        if (!pca9685_midi_note_to_servo(ctx, note, &servo_index)) {
            return false;  // Note not in our range or ignored
        }
        
        PCA9685_MIDI_PRINTF("PCA9685_MIDI: Note ON: %d -> Servo %d\n", note, servo_index);
        
        // Strike the servo
        return pca9685_midi_strike_servo(ctx, servo_index);
        
    } else if (message_type == MIDI_NOTE_OFF || (message_type == MIDI_NOTE_ON && velocity == 0)) {
        // NOTE OFF - immediately return to rest
        uint8_t servo_index;
        if (!pca9685_midi_note_to_servo(ctx, note, &servo_index)) {
            return false;
        }
        
        PCA9685_MIDI_PRINTF("PCA9685_MIDI: Note OFF: %d -> Servo %d\n", note, servo_index);
        
        // Return to rest position
        pca9685_set_servo_angle(&ctx->pca9685, servo_index, ctx->config.rest_angle);
        ctx->servo_states[servo_index].current_angle = ctx->config.rest_angle;
        ctx->servo_states[servo_index].striking = false;
        ctx->servo_states[servo_index].return_time = 0;
        
        return true;
    }
    
    return false;
}

void pca9685_midi_update(pca9685_midi_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    
    uint32_t current_time = time_us_32();
    
    // Check each servo for automatic return to rest
    for (int i = 0; i < 16; i++) {
        if (ctx->servo_states[i].striking && ctx->servo_states[i].return_time > 0) {
            if (current_time >= ctx->servo_states[i].return_time) {
                // Time to return to rest
                pca9685_set_servo_angle(&ctx->pca9685, i, ctx->config.rest_angle);
                ctx->servo_states[i].current_angle = ctx->config.rest_angle;
                ctx->servo_states[i].striking = false;
                ctx->servo_states[i].return_time = 0;
            }
        }
    }
}

bool pca9685_midi_set_semitone_mode(pca9685_midi_t *ctx, pca9685_midi_semitone_mode_t mode) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    ctx->config.semitone_mode = mode;
    
    // Recalculate high note
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, ctx->config.note_range, mode);
    
    PCA9685_MIDI_PRINTF("PCA9685_MIDI: Semitone mode set to %d, high note now %d\n", mode, ctx->config.high_note);
    
    return true;
}

bool pca9685_midi_set_strike_mode(pca9685_midi_t *ctx, pca9685_strike_mode_t mode) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    ctx->config.strike_mode = mode;
    
    PCA9685_MIDI_PRINTF("PCA9685_MIDI: Strike mode set to %s\n", 
                        mode == PCA9685_STRIKE_MODE_SIMPLE ? "SIMPLE" : "POSITION");
    
    return true;
}

bool pca9685_midi_all_notes_off(pca9685_midi_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    PCA9685_MIDI_PRINTF("PCA9685_MIDI: All notes off\n");
    
    // Return all servos to rest position
    pca9685_set_all_servos(&ctx->pca9685, ctx->config.rest_angle);
    
    // Clear all states
    for (int i = 0; i < 16; i++) {
        ctx->servo_states[i].current_angle = ctx->config.rest_angle;
        ctx->servo_states[i].striking = false;
        ctx->servo_states[i].return_time = 0;
    }
    
    return true;
}

bool pca9685_midi_reset(pca9685_midi_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    PCA9685_MIDI_PRINTF("PCA9685_MIDI: Reset\n");
    
    // Reset PCA9685
    if (!pca9685_reset(&ctx->pca9685)) {
        return false;
    }
    
    // Return all servos to rest
    return pca9685_midi_all_notes_off(ctx);
}
