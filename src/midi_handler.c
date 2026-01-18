#include "midi_handler.h"
#include "i2c_midi.h"
#include "debug_uart.h"
#include "display_handler.h"
#include "hardware/gpio.h"
#include "usb_midi.h"
#include <stdio.h>

//--------------------------------------------------------------------+
// MIDI Handler Module - Internal State
//--------------------------------------------------------------------+

// I2C MIDI context
static i2c_midi_t i2c_midi_ctx;

// LED feedback configuration
static uint8_t led_gpio_pin = 0xFF; // 0xFF = disabled
static bool led_enabled = true;

// SysEx message handling
#define SYSEX_BUFFER_SIZE 32
#define SYSEX_MANUFACTURER_ID 0x7D  // Educational/development use
#define SYSEX_DEVICE_ID 0x00        // Device ID for this synthesizer

static uint8_t sysex_buffer[SYSEX_BUFFER_SIZE];
static uint8_t sysex_index = 0;
static bool sysex_receiving = false;

// SysEx Command IDs
#define SYSEX_CMD_SET_NOTE_RANGE    0x01
#define SYSEX_CMD_SET_CHANNEL       0x02
#define SYSEX_CMD_SET_SEMITONE_MODE 0x03
#define SYSEX_CMD_QUERY_CONFIG      0x10

//--------------------------------------------------------------------+
// SysEx Message Processing
//--------------------------------------------------------------------+
static void process_sysex_message(void)
{
    // Print raw SysEx message
    debug_info("SysEx: Received %d bytes:", sysex_index);
    for (uint8_t i = 0; i < sysex_index; i++) {
        debug_printf(" %02X", sysex_buffer[i]);
    }
    debug_printf("\n");
    
    // Minimum message: [0xF0] [MfrID] [DevID] [Cmd] [0xF7] = 5 bytes
    if (sysex_index < 5) {
        debug_error("SysEx: Message too short (%d bytes)", sysex_index);
        return;
    }
    
    // Verify manufacturer ID
    if (sysex_buffer[1] != SYSEX_MANUFACTURER_ID) {
        debug_info("SysEx: Ignored - wrong manufacturer ID (0x%02X)", sysex_buffer[1]);
        return;
    }
    
    // Verify device ID
    if (sysex_buffer[2] != SYSEX_DEVICE_ID) {
        debug_info("SysEx: Ignored - wrong device ID (0x%02X)", sysex_buffer[2]);
        return;
    }
    
    uint8_t command = sysex_buffer[3];
    char display_msg[32];
    
    switch (command) {
        case SYSEX_CMD_SET_NOTE_RANGE:
            if (sysex_index >= 6) {
                uint8_t low_note = sysex_buffer[4];
                uint8_t high_note = sysex_buffer[5];
                if (midi_handler_set_note_range(low_note, high_note)) {
                    debug_info("SysEx: Note range set to %d-%d", low_note, high_note);
                    snprintf(display_msg, sizeof(display_msg), "Range: %d-%d", low_note, high_note);
                    display_handler_writeline(5, 40, display_msg);
                }
            }
            break;
            
        case SYSEX_CMD_SET_CHANNEL:
            if (sysex_index >= 5) {
                uint8_t channel = sysex_buffer[4];
                if (midi_handler_set_channel(channel)) {
                    debug_info("SysEx: MIDI channel set to %d", channel == 0xFF ? -1 : channel + 1);
                    if (channel == 0xFF) {
                        display_handler_writeline(5, 40, "CH: All");
                    } else {
                        snprintf(display_msg, sizeof(display_msg), "CH: %d", channel + 1);
                        display_handler_writeline(5, 40, display_msg);
                    }
                }
            }
            break;
            
        case SYSEX_CMD_SET_SEMITONE_MODE:
            if (sysex_index >= 5) {
                uint8_t mode = sysex_buffer[4];
                if (mode <= I2C_MIDI_SEMITONE_SKIP) {
                    i2c_midi_set_semitone_mode(&i2c_midi_ctx, (i2c_midi_semitone_mode_t)mode);
                    const char* mode_names[] = {"Play", "Ignore", "Skip"};
                    debug_info("SysEx: Semitone mode set to %s", mode_names[mode]);
                    snprintf(display_msg, sizeof(display_msg), "Semitone: %s", mode_names[mode]);
                    display_handler_writeline(5, 40, display_msg);
                }
            }
            break;
            
        case SYSEX_CMD_QUERY_CONFIG:
            debug_info("SysEx: Config - Ch:%d, Range:%d-%d, Semitone:%d",
                      i2c_midi_ctx.config.midi_channel,
                      i2c_midi_ctx.config.low_note,
                      i2c_midi_ctx.config.high_note,
                      i2c_midi_ctx.config.semitone_mode);
            break;
            
        default:
            debug_error("SysEx: Unknown command 0x%02X", command);
            break;
    }
}

//--------------------------------------------------------------------+
// Internal MIDI Message Handler
//--------------------------------------------------------------------+
static void internal_midi_handler(uint8_t status, uint8_t data1, uint8_t data2, void* user_data)
{
    (void)user_data; // Unused parameter
    
    // Handle SysEx messages
    if (status == 0xF0) { // SysEx Start
        sysex_receiving = true;
        sysex_index = 0;
        sysex_buffer[sysex_index++] = status;
        return;
    }
    
    if (sysex_receiving) {
        if (sysex_index < SYSEX_BUFFER_SIZE) {
            sysex_buffer[sysex_index++] = status;
        }
        
        if (status == 0xF7) { // SysEx End
            sysex_receiving = false;
            process_sysex_message();
        }
        return;
    }
    
    // Process MIDI message with i2c_midi library
    // This will automatically filter by channel and note range
    i2c_midi_process_message(&i2c_midi_ctx, status, data1, data2);
    
    // Update display with note information
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    if (msg_type == 0x90 && data2 > 0) // Note On (velocity > 0)
    {
        display_handler_update_note(data1, data2, channel);
    }
    else if (msg_type == 0x80 || (msg_type == 0x90 && data2 == 0)) // Note Off
    {
        // Could clear display or show "Note Off" message if desired
    }
    
    // Handle LED feedback if enabled
    if (led_enabled && led_gpio_pin != 0xFF) {
        if (msg_type == 0x90 && data2 > 0) // Note On (velocity > 0)
        {
            gpio_put(led_gpio_pin, 1); // LED on
        }
        else if (msg_type == 0x80 || (msg_type == 0x90 && data2 == 0)) // Note Off
        {
            gpio_put(led_gpio_pin, 0); // LED off
        }
    }
    
    // Print MIDI message to debug UART
    debug_print_midi(status, data1, data2);
}

//--------------------------------------------------------------------+
// Public API Implementation
//--------------------------------------------------------------------+

bool midi_handler_init(void* i2c_inst, uint8_t sda_pin, uint8_t scl_pin, 
                       uint32_t i2c_freq, uint8_t led_pin, 
                       i2c_midi_semitone_mode_t semitone_mode)
{
    // Initialize I2C MIDI library
    if (!i2c_midi_init(&i2c_midi_ctx, i2c_inst, sda_pin, scl_pin, i2c_freq)) {
        debug_error("MIDI Handler: Failed to initialize I2C MIDI");
        return false;
    }
    
    // Set semitone mode
    i2c_midi_set_semitone_mode(&i2c_midi_ctx, semitone_mode);
    
    // Configure LED if provided
    led_gpio_pin = led_pin;
    if (led_gpio_pin != 0xFF) {
        gpio_init(led_gpio_pin);
        gpio_set_dir(led_gpio_pin, GPIO_OUT);
        gpio_put(led_gpio_pin, 0); // Start with LED off
        led_enabled = true;
        debug_info("MIDI Handler: LED feedback enabled on GPIO %d", led_gpio_pin);
    } else {
        led_enabled = false;
        debug_info("MIDI Handler: LED feedback disabled");
    }
    
    debug_info("MIDI Handler: I2C MIDI initialized (SDA=GP%d, SCL=GP%d, Freq=%dHz)", 
               sda_pin, scl_pin, i2c_freq);
    
    return true;
}

void* midi_handler_get_callback(void)
{
    return (void*)internal_midi_handler;
}

bool midi_handler_set_channel(uint8_t channel)
{
    if (channel > 15 && channel != 0xFF) {
        debug_error("MIDI Handler: Invalid channel %d", channel);
        return false;
    }
    
    // Update the i2c_midi context configuration
    i2c_midi_ctx.config.midi_channel = channel;
    
    if (channel == 0xFF) {
        debug_info("MIDI Handler: Listening to all channels");
    } else {
        debug_info("MIDI Handler: Listening to channel %d", channel + 1);
    }
    
    return true;
}

bool midi_handler_set_note_range(uint8_t min_note, uint8_t max_note)
{
    if (min_note > 127 || max_note > 127 || min_note > max_note) {
        debug_error("MIDI Handler: Invalid note range %d-%d", min_note, max_note);
        return false;
    }
    
    // Update the i2c_midi context configuration
    i2c_midi_ctx.config.low_note = min_note;
    i2c_midi_ctx.config.high_note = max_note;
    i2c_midi_ctx.config.note_range = max_note - min_note + 1;
    debug_info("MIDI Handler: Note range set to %d-%d", min_note, max_note);
    
    return true;
}

void midi_handler_set_led_enabled(bool enabled)
{
    led_enabled = enabled;
    
    // Turn off LED if disabling
    if (!enabled && led_gpio_pin != 0xFF) {
        gpio_put(led_gpio_pin, 0);
    }
    
    debug_info("MIDI Handler: LED feedback %s", enabled ? "enabled" : "disabled");
}

void midi_handler_process_message(uint8_t status, uint8_t data1, uint8_t data2)
{
    internal_midi_handler(status, data1, data2, NULL);
}
