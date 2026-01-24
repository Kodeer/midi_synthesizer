#include "i2c_midi.h"
#ifdef USE_PCF857X_DRIVER
#include "drivers/pcf857x_driver.h"
#endif
#ifdef USE_CH423_DRIVER
#include "drivers/ch423_driver.h"
#endif
#include "hardware/gpio.h"
#include "../../src/debug_uart.h"
#include <string.h>

//--------------------------------------------------------------------+
// IO Expander Abstraction Layer
//--------------------------------------------------------------------+

/**
 * Write data to IO expander
 */
static bool io_write(i2c_midi_t *ctx, uint8_t data) {
    switch (ctx->config.io_type) {
#ifdef USE_PCF857X_DRIVER
        case IO_EXPANDER_PCF8574:
            return pcf857x_write(&ctx->driver.pcf857x, data);
#endif
#ifdef USE_CH423_DRIVER
        case IO_EXPANDER_CH423:
            // For CH423, extend 8-bit data to 16-bit (low byte only)
            return ch423_write(&ctx->driver.ch423, (uint16_t)data);
#endif
        default:
            debug_error("I2C_MIDI: Unknown IO expander type: %d", ctx->config.io_type);
            return false;
    }
}

/**
 * Set a specific pin on the IO expander
 */
static bool io_set_pin(i2c_midi_t *ctx, uint8_t pin, bool state) {
    switch (ctx->config.io_type) {
#ifdef USE_PCF857X_DRIVER
        case IO_EXPANDER_PCF8574:
            if (pin >= 8) return false;  // PCF857x driver handles 8 or 16 pins
            return pcf857x_set_pin(&ctx->driver.pcf857x, pin, state);
#endif
#ifdef USE_CH423_DRIVER
        case IO_EXPANDER_CH423:
            if (pin >= 16) return false;  // CH423 has 16 pins
            return ch423_set_pin(&ctx->driver.ch423, pin, state);
#endif
        default:
            debug_error("I2C_MIDI: Unknown IO expander type: %d", ctx->config.io_type);
            return false;
    }
}

/**
 * Get maximum number of pins for the configured IO expander
 */
static uint8_t io_get_max_pins(i2c_midi_t *ctx) {
    switch (ctx->config.io_type) {
#ifdef USE_PCF857X_DRIVER
        case IO_EXPANDER_PCF8574:
            return pcf857x_get_num_pins(&ctx->driver.pcf857x);
#endif
#ifdef USE_CH423_DRIVER
        case IO_EXPANDER_CH423:
            return 16;
#endif
        default:
            return 0;
    }
}

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

//--------------------------------------------------------------------+
// Initialization Functions
//--------------------------------------------------------------------+

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
    ctx->config.i2c_port = i2c_port;
#ifdef USE_PCF857X_DRIVER
    ctx->config.io_address = PCF857X_DEFAULT_ADDRESS;
    ctx->config.io_type = IO_EXPANDER_PCF8574;  // Default to PCF857x
#elif defined(USE_CH423_DRIVER)
    ctx->config.io_address = CH423_DEFAULT_ADDRESS;
    ctx->config.io_type = IO_EXPANDER_CH423;  // Default to CH423
#endif
    ctx->config.semitone_mode = I2C_MIDI_SEMITONE_PLAY; // Default: play semitones normally
    ctx->config.high_note = calculate_high_note(ctx->config.low_note, ctx->config.note_range, ctx->config.semitone_mode);
    ctx->pin_state = 0x00;

    const char* mode_str = (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_PLAY) ? "PLAY" :
                          (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_IGNORE) ? "IGNORE" : "SKIP";
    const char* io_str = 
#ifdef USE_PCF857X_DRIVER
        (ctx->config.io_type == IO_EXPANDER_PCF8574) ? "PCF857x" :
#endif
#ifdef USE_CH423_DRIVER
        (ctx->config.io_type == IO_EXPANDER_CH423) ? "CH423" :
#endif
        "Unknown";
    debug_info("I2C_MIDI: Config - Ch:%d, Notes:%d-%d, IO:%s@0x%02X, Semitone:%s", 
               ctx->config.midi_channel, ctx->config.low_note, ctx->config.high_note, 
               io_str, ctx->config.io_address, mode_str);

    // NOTE: I2C bus should already be initialized by the caller (e.g., midi_handler_init)
    // We do NOT re-initialize it here to avoid bus conflicts
    debug_info("I2C_MIDI: Using pre-initialized I2C bus (assumed %d Hz)", baudrate);

    // Initialize IO expander driver
    switch (ctx->config.io_type) {
#ifdef USE_PCF857X_DRIVER
        case IO_EXPANDER_PCF8574:
            if (!pcf857x_init(&ctx->driver.pcf857x, i2c_port, ctx->config.io_address, PCF8575_CHIP)) {
                debug_error("I2C_MIDI: PCF857x initialization failed");
            }
            break;
#endif
#ifdef USE_CH423_DRIVER
        case IO_EXPANDER_CH423:
            if (!ch423_init(&ctx->driver.ch423, i2c_port, ctx->config.io_address)) {
                debug_error("I2C_MIDI: CH423 initialization failed");
            }
            break;
#endif
        default:
            debug_error("I2C_MIDI: Unknown IO expander type");
            break;
    }
    
    // Always return true - IO expander might not be connected yet
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

    // NOTE: I2C bus should already be initialized by the caller (e.g., midi_handler_init)
    // We do NOT re-initialize it here to avoid bus conflicts
    debug_info("I2C_MIDI: Using pre-initialized I2C bus (assumed %d Hz)", baudrate);

    // Initialize IO expander driver
    switch (ctx->config.io_type) {
#ifdef USE_PCF857X_DRIVER
        case IO_EXPANDER_PCF8574:
            if (!pcf857x_init(&ctx->driver.pcf857x, config->i2c_port, ctx->config.io_address, PCF8575_CHIP)) {
                debug_error("I2C_MIDI: PCF857x initialization failed");
            }
            break;
#endif
#ifdef USE_CH423_DRIVER
        case IO_EXPANDER_CH423:
            if (!ch423_init(&ctx->driver.ch423, config->i2c_port, ctx->config.io_address)) {
                debug_error("I2C_MIDI: CH423 initialization failed");
            }
            break;
#endif
        default:
            break;
    }
    
    // Always return true - IO expander might not be connected yet
    return true;
}

//--------------------------------------------------------------------+
// MIDI Message Processing
//--------------------------------------------------------------------+

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
        // Debug output disabled for performance (causes stuttering)
        // debug_printf("I2C_MIDI: Ignored - wrong channel (msg ch:%d, expected:%d)\n", channel, ctx->config.midi_channel);
        return false;
    }

    // Handle semitone mode
    uint8_t original_note = note;
    if (is_semitone(note)) {
        if (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_IGNORE) {
            // Debug output disabled for performance (causes stuttering)
            // debug_printf("I2C_MIDI: Ignored - semitone note %d (mode: IGNORE)\n", note);
            return false;
        } else if (ctx->config.semitone_mode == I2C_MIDI_SEMITONE_SKIP) {
            note = map_note_for_mode(note, ctx->config.semitone_mode);
            // Debug output disabled for performance (causes stuttering)
            // debug_info("I2C_MIDI: Semitone %d mapped to %d (mode: SKIP)", original_note, note);
        }
    }

    // Check if note is in our range
    if (note < ctx->config.low_note || note > ctx->config.high_note) {
        // Debug output disabled for performance (causes stuttering)
        // debug_printf("I2C_MIDI: Ignored - note %d out of range (%d-%d)\n", note, ctx->config.low_note, ctx->config.high_note);
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
    
    uint8_t max_pins = io_get_max_pins(ctx);
    if (pin >= max_pins) {
        debug_error("I2C_MIDI: Pin calculation error: %d (max: %d)", pin, max_pins);
        return false; // Safety check
    }

    // Process based on message type
    bool note_on = false;
    
    if (message_type == MIDI_NOTE_ON && velocity > 0) {
        note_on = true;
        // Debug output disabled for performance (causes stuttering)
        // debug_info("I2C_MIDI: Note ON - Note:%d -> Pin:%d, Vel:%d", note, pin, velocity);
    } else if (message_type == MIDI_NOTE_OFF || (message_type == MIDI_NOTE_ON && velocity == 0)) {
        note_on = false;
        // Debug output disabled for performance (causes stuttering)
        // debug_info("I2C_MIDI: Note OFF - Note:%d -> Pin:%d", note, pin);
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

//--------------------------------------------------------------------+
// Pin Control Functions
//--------------------------------------------------------------------+

/**
 * Set a specific pin on the IO expander
 */
bool i2c_midi_set_pin(i2c_midi_t *ctx, uint8_t pin, bool state) {
    if (!ctx) {
        return false;
    }

    uint8_t max_pins = io_get_max_pins(ctx);
    if (pin >= max_pins) {
        debug_error("I2C_MIDI: Invalid pin %d (max: %d)", pin, max_pins);
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

    // Write to IO expander through abstraction layer
    return io_set_pin(ctx, pin, state);
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
    return io_write(ctx, 0x00);
}
