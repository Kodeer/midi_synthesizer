#include "i2c_midi.h"
#include "hardware/gpio.h"
#include "../../src/debug_uart.h"
#include <string.h>

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
static uint8_t calculate_high_note(uint8_t low_note, uint8_t note_range, i2c_midi_semitone_mode_t mode) {
    if (mode == I2C_MIDI_SEMITONE_PLAY) {
        // Normal mode: high_note = low_note + range - 1
        return low_note + note_range - 1;
    } else {
        // For IGNORE or SKIP modes, we need to count only whole tones
        uint8_t count = 0;
        uint8_t current_note = low_note;
        
        // Find the high note by counting whole tones
        while (count < note_range && current_note < 127) {
            if (!is_semitone(current_note)) {
                count++;
                if (count == note_range) {
                    return current_note;
                }
            }
            current_note++;
        }
        return current_note;
    }
}

/**
 * Map a received note to the appropriate note for processing based on semitone mode
 */
static uint8_t map_note_for_mode(uint8_t note, i2c_midi_semitone_mode_t mode) {
    if (mode == I2C_MIDI_SEMITONE_PLAY) {
        return note; // No mapping needed
    } else if (mode == I2C_MIDI_SEMITONE_SKIP) {
        // If it's a semitone, return the next whole tone
        if (is_semitone(note)) {
            uint8_t next_note = note + 1;
            // Skip to next whole tone
            while (next_note < 127 && is_semitone(next_note)) {
                next_note++;
            }
            return next_note;
        }
        return note;
    }
    // For IGNORE mode, return the note as-is (will be filtered out later)
    return note;
}

/**
 * Write data to PCF8574
 */
static bool pcf8574_write(i2c_midi_t *ctx, uint8_t data) {
    int result = i2c_write_blocking(ctx->config.i2c_port, ctx->config.pcf8574_address, &data, 1, false);
    if (result == 1) {
        debug_printf("I2C_MIDI: PCF8574 write success: 0x%02X\n", data);
        return true;
    } else {
        debug_error("I2C_MIDI: PCF8574 write failed (result=%d, addr=0x%02X, data=0x%02X)", result, ctx->config.pcf8574_address, data);
        return false;
    }
}

/**
 * Initialize the I2C MIDI library with default configuration
 */
bool i2c_midi_init(i2c_midi_t *ctx, i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin, uint baudrate) {
    if (!ctx || !i2c_port) {
        debug_error("I2C_MIDI: Init failed - invalid parameters");
        return false;
    }

    debug_info("I2C_MIDI: Initializing with defaults...");
    
    // Set up default configuration
    ctx->config.note_range = I2C_MIDI_DEFAULT_NOTE_RANGE;
    ctx->config.low_note = I2C_MIDI_DEFAULT_LOW_NOTE;
    ctx->config.midi_channel = I2C_MIDI_DEFAULT_CHANNEL;
    ctx->config.pcf8574_address = PCF8574_DEFAULT_ADDRESS;
    ctx->config.i2c_port = i2c_port;
    ctx->config.semitone_mode = I2C_MIDI_SEMITONE_PLAY; // Default: play semitones normally
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, ctx->config.note_range, ctx->config.semitone_mode);
    ctx->pin_state = 0x00;

    const char* mode_str = (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_PLAY) ? "PLAY" :
                          (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_IGNORE) ? "IGNORE" : "SKIP";
    debug_info("I2C_MIDI: Config - Ch:%d, Notes:%d-%d, PCF8574:0x%02X, Semitone:%s", 
               ctx->config.midi_channel, ctx->config.low_note, ctx->config.high_note, 
               ctx->config.pcf8574_address, mode_str);

    // Initialize I2C
    i2c_init(i2c_port, baudrate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    debug_info("I2C_MIDI: I2C initialized at %d Hz", baudrate);

    // Try to reset PCF8574 pins to LOW (non-critical, may fail if not connected)
    debug_info("I2C_MIDI: Testing PCF8574 connection...");
    if (!pcf8574_write(ctx, 0x00)) {
        debug_error("I2C_MIDI: Warning - PCF8574 not responding (may not be connected)");
    } else {
        debug_info("I2C_MIDI: PCF8574 detected and initialized");
    }
    
    // Always return true - PCF8574 might not be connected yet
    return true;
}

/**
 * Initialize the I2C MIDI library with custom configuration
 */
bool i2c_midi_init_with_config(i2c_midi_t *ctx, i2c_midi_config_t *config, uint sda_pin, uint scl_pin, uint baudrate) {
    if (!ctx || !config || !config->i2c_port) {
        return false;
    }

    // Copy configuration
    memcpy(&ctx->config, config, sizeof(i2c_midi_config_t));
    
    // Recalculate high note based on semitone mode
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, ctx->config.note_range, ctx->config.semitone_mode);
    ctx->pin_state = 0x00;

    // Initialize I2C
    i2c_init(config->i2c_port, baudrate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // Try to reset PCF8574 pins to LOW (non-critical, may fail if not connected)
    pcf8574_write(ctx, 0x00);
    
    // Always return true - PCF8574 might not be connected yet
    return true;
}

/**
 * Process a MIDI message
 */
bool i2c_midi_process_message(i2c_midi_t *ctx, uint8_t status, uint8_t note, uint8_t velocity) {
    if (!ctx) {
        return false;
    }

    // Extract message type and channel from status byte
    uint8_t message_type = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1; // MIDI channels are 1-16, status byte is 0-15

    // Check if message is for our channel
    if (channel != ctx->config.midi_channel) {
        debug_printf("I2C_MIDI: Ignored - wrong channel (msg ch:%d, expected:%d)\n", channel, ctx->config.midi_channel);
        return false;
    }

    // Handle semitone mode
    uint8_t original_note = note;
    if (is_semitone(note)) {
        if (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_IGNORE) {
            debug_printf("I2C_MIDI: Ignored - semitone note %d (mode: IGNORE)\n", note);
            return false;
        } else if (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_SKIP) {
            note = map_note_for_mode(note, ctx->config.semitone_mode);
            debug_info("I2C_MIDI: Semitone %d mapped to %d (mode: SKIP)", original_note, note);
        }
    }

    // Check if note is in our range
    if (note < ctx->config.low_note || note > ctx->config.high_note) {
        debug_printf("I2C_MIDI: Ignored - note %d out of range (%d-%d)\n", note, ctx->config.low_note, ctx->config.high_note);
        return false;
    }

    // Calculate which pin corresponds to this note
    uint8_t pin;
    if (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_PLAY) {
        // Normal mapping
        pin = note - ctx->config.low_note;
    } else {
        // Count only whole tones from low_note to current note
        pin = 0;
        for (uint8_t n = ctx->config.low_note; n < note; n++) {
            if (!is_semitone(n)) {
                pin++;
            }
        }
    }
    
    if (pin >= 8) {
        debug_error("I2C_MIDI: Pin calculation error: %d", pin);
        return false; // Safety check
    }

    // Process based on message type
    bool note_on = false;
    
    if (message_type == MIDI_NOTE_ON && velocity > 0) {
        note_on = true;
        debug_info("I2C_MIDI: Note ON - Note:%d -> Pin:%d, Vel:%d", note, pin, velocity);
    } else if (message_type == MIDI_NOTE_OFF || (message_type == MIDI_NOTE_ON && velocity == 0)) {
        note_on = false;
        debug_info("I2C_MIDI: Note OFF - Note:%d -> Pin:%d", note, pin);
    } else {
        debug_printf("I2C_MIDI: Ignored - not a note message (type:0x%02X)\n", message_type);
        return false; // Not a note message we care about
    }

    // Set the pin state
    bool result = i2c_midi_set_pin(ctx, pin, note_on);
    if (!result) {
        debug_error("I2C_MIDI: Failed to set pin %d to %d", pin, note_on);
    }
    return result;
}

/**
 * Set a specific pin on the PCF8574
 */
bool i2c_midi_set_pin(i2c_midi_t *ctx, uint8_t pin, bool state) {
    if (!ctx || pin >= 8) {
        debug_error("I2C_MIDI: Invalid pin %d", pin);
        return false;
    }

    uint8_t old_state = ctx->pin_state;
    
    // Update pin state
    if (state) {
        ctx->pin_state |= (1 << pin);  // Set bit
    } else {
        ctx->pin_state &= ~(1 << pin); // Clear bit
    }

    debug_printf("I2C_MIDI: Pin %d -> %s (state: 0x%02X -> 0x%02X)\n", 
                pin, state ? "HIGH" : "LOW", old_state, ctx->pin_state);

    // Write to PCF8574
    return pcf8574_write(ctx, ctx->pin_state);
}

/**
 * Get the current pin state
 */
uint8_t i2c_midi_get_pin_state(i2c_midi_t *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->pin_state;
}

/**
 * Set semitone handling mode
 */
bool i2c_midi_set_semitone_mode(i2c_midi_t *ctx, i2c_midi_semitone_mode_t mode) {
    if (!ctx) {
        return false;
    }
    
    if (mode > I2C_MIDI_SEMITONE_SKIP) {
        debug_error("I2C_MIDI: Invalid semitone mode: %d", mode);
        return false;
    }
    
    ctx->config.semitone_mode = mode;
    
    // Recalculate high_note based on new mode
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, ctx->config.note_range, mode);
    
    const char* mode_str = (mode == I2C_MIDI_SEMITONE_PLAY) ? "PLAY" :
                          (mode == I2C_MIDI_SEMITONE_IGNORE) ? "IGNORE" : "SKIP";
    debug_info("I2C_MIDI: Semitone mode set to %s, new range: %d-%d", 
               mode_str, ctx->config.low_note, ctx->config.high_note);
    
    return true;
}

/**
 * Reset all pins to LOW
 */
bool i2c_midi_reset(i2c_midi_t *ctx) {
    if (!ctx) {
        return false;
    }

    ctx->pin_state = 0x00;
    return pcf8574_write(ctx, 0x00);
}
