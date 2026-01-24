#include "midi_handler.h"
#include "i2c_midi.h"
#include "configuration_settings.h"
#include "debug_uart.h"
#include "display_handler.h"
#include "hardware/gpio.h"
#include "usb_midi.h"
#include <stdio.h>

//--------------------------------------------------------------------+
// MIDI Handler Module - Internal State
//--------------------------------------------------------------------+

// Configuration manager
static config_manager_t config_mgr;
static bool config_initialized = false;

// I2C MIDI context
static i2c_midi_t i2c_midi_ctx;

// LED feedback configuration
static uint8_t led_gpio_pin = 0xFF; // 0xFF = disabled
static bool led_enabled = true;

// Last MIDI activity timestamp (for timeout detection)
static uint64_t last_activity_time = 0;

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

// Configuration SysEx Commands (with EEPROM persistence)
#define SYSEX_CMD_CONFIG_MIDI_CHANNEL   0x20
#define SYSEX_CMD_CONFIG_NOTE_RANGE     0x21
#define SYSEX_CMD_CONFIG_LOW_NOTE       0x22
#define SYSEX_CMD_CONFIG_SEMITONE_MODE  0x23
#define SYSEX_CMD_CONFIG_IO_TYPE        0x30
#define SYSEX_CMD_CONFIG_IO_ADDRESS     0x31
#define SYSEX_CMD_CONFIG_DISPLAY_ENABLE 0x40
#define SYSEX_CMD_CONFIG_DISPLAY_BRIGHT 0x41
#define SYSEX_CMD_CONFIG_DISPLAY_TIMEOUT 0x42
#define SYSEX_CMD_CONFIG_RESET_DEFAULTS 0xF0
#define SYSEX_CMD_CONFIG_SAVE           0xF1
#define SYSEX_CMD_CONFIG_QUERY          0xF2

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
        
        // Configuration commands with EEPROM persistence
        case SYSEX_CMD_CONFIG_MIDI_CHANNEL:
            if (sysex_index >= 5 && config_initialized) {
                uint8_t channel = sysex_buffer[4];
                if (channel >= 1 && channel <= 16) {
                    if (config_update_midi_setting(&config_mgr, 0, channel)) {
                        // Also update runtime setting
                        midi_handler_set_channel(channel - 1);
                        debug_info("SysEx: MIDI channel saved to EEPROM: %d", channel);
                        snprintf(display_msg, sizeof(display_msg), "Saved CH:%d", channel);
                        display_handler_writeline(5, 40, display_msg);
                    }
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_NOTE_RANGE:
            if (sysex_index >= 5 && config_initialized) {
                uint8_t range = sysex_buffer[4];
                if (range >= 1 && range <= 16) {
                    if (config_update_midi_setting(&config_mgr, 1, range)) {
                        debug_info("SysEx: Note range saved to EEPROM: %d", range);
                        snprintf(display_msg, sizeof(display_msg), "Saved Range:%d", range);
                        display_handler_writeline(5, 40, display_msg);
                    }
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_LOW_NOTE:
            if (sysex_index >= 5 && config_initialized) {
                uint8_t low_note = sysex_buffer[4];
                if (low_note <= 127) {
                    if (config_update_midi_setting(&config_mgr, 2, low_note)) {
                        debug_info("SysEx: Low note saved to EEPROM: %d", low_note);
                        snprintf(display_msg, sizeof(display_msg), "Saved Low:%d", low_note);
                        display_handler_writeline(5, 40, display_msg);
                    }
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_SEMITONE_MODE:
            if (sysex_index >= 5 && config_initialized) {
                uint8_t mode = sysex_buffer[4];
                if (mode <= 2) {
                    if (config_update_midi_setting(&config_mgr, 3, mode)) {
                        // Also update runtime setting
                        i2c_midi_set_semitone_mode(&i2c_midi_ctx, (i2c_midi_semitone_mode_t)mode);
                        const char* mode_names[] = {"Play", "Ignore", "Skip"};
                        debug_info("SysEx: Semitone mode saved to EEPROM: %s", mode_names[mode]);
                        snprintf(display_msg, sizeof(display_msg), "Saved:%s", mode_names[mode]);
                        display_handler_writeline(5, 40, display_msg);
                    }
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_IO_TYPE:
            if (sysex_index >= 6 && config_initialized) {
                uint8_t io_type = sysex_buffer[4];
                uint8_t io_addr = sysex_buffer[5];
                if (io_type <= 1) {
                    if (config_update_io_settings(&config_mgr, io_type, io_addr)) {
                        debug_info("SysEx: IO settings saved to EEPROM: type=%d, addr=0x%02X", io_type, io_addr);
                        display_handler_writeline(5, 40, "IO Saved");
                    }
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_DISPLAY_ENABLE:
            if (sysex_index >= 5 && config_initialized) {
                uint8_t enabled = sysex_buffer[4];
                config_settings_t *settings = config_get_settings(&config_mgr);
                if (settings) {
                    config_update_display_settings(&config_mgr, enabled, 
                        settings->display_brightness, settings->display_timeout);
                    debug_info("SysEx: Display enable saved to EEPROM: %d", enabled);
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_RESET_DEFAULTS:
            if (config_initialized) {
                if (config_erase(&config_mgr)) {
                    debug_info("SysEx: Configuration reset to defaults");
                    display_handler_writeline(5, 40, "Reset to Defaults");
                    // Reload runtime settings from config
                    config_settings_t *settings = config_get_settings(&config_mgr);
                    if (settings) {
                        midi_handler_set_channel(settings->midi_channel - 1);
                        i2c_midi_set_semitone_mode(&i2c_midi_ctx, (i2c_midi_semitone_mode_t)settings->semitone_mode);
                    }
                }
            }
            break;
            
        case SYSEX_CMD_CONFIG_QUERY:
            if (config_initialized) {
                config_settings_t *settings = config_get_settings(&config_mgr);
                if (settings) {
                    debug_info("SysEx: Stored Config - Ch:%d, Range:%d, Low:%d, Semitone:%d, IO:0x%02X",
                              settings->midi_channel, settings->note_range, 
                              settings->low_note, settings->semitone_mode,
                              settings->io_expander_address);
                }
            }
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
    
    // Update activity timestamp for any MIDI message
    last_activity_time = time_us_64() / 1000;
    
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
    debug_info("MIDI Handler: Initializing...");
    
    // CRITICAL FIX: Initialize I2C bus FIRST before accessing any I2C devices
    // This prevents system hang when trying to read EEPROM on uninitialized bus
    i2c_inst_t *i2c_port = (i2c_inst_t*)i2c_inst;
    i2c_init(i2c_port, i2c_freq);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    debug_info("MIDI Handler: I2C bus initialized at %d Hz", i2c_freq);
    
    // Small delay to allow I2C bus to stabilize
    sleep_ms(10);
    
    // Now try to initialize configuration manager with EEPROM
    // Using AT24C32 (4KB) at address 0x50, storing config at address 0x0000
    if (config_init(&config_mgr, i2c_port, 0x50, 4, 0x0000)) {
        config_initialized = true;
        debug_info("MIDI Handler: Configuration loaded from EEPROM");
        
        // Get loaded settings
        config_settings_t *settings = config_get_settings(&config_mgr);
        if (settings) {
            debug_info("MIDI Handler: Using stored settings - Ch:%d, Notes:%d-%d, Mode:%d",
                      settings->midi_channel, settings->low_note,
                      settings->low_note + settings->note_range - 1,
                      settings->semitone_mode);
            
            // Initialize I2C MIDI with configuration from EEPROM
            // Note: I2C bus is already initialized above
            i2c_midi_config_t midi_config = {
                .note_range = settings->note_range,
                .low_note = settings->low_note,
                .high_note = settings->low_note + settings->note_range - 1,
                .midi_channel = settings->midi_channel,  // Keep as 1-16 (i2c_midi expects 1-indexed)
                .io_address = settings->io_expander_address,
                .i2c_port = i2c_port,
                .io_type = (io_expander_type_t)settings->io_expander_type,
                .semitone_mode = (i2c_midi_semitone_mode_t)settings->semitone_mode
            };
            
            if (!i2c_midi_init_with_config(&i2c_midi_ctx, &midi_config, sda_pin, scl_pin, i2c_freq)) {
                debug_error("MIDI Handler: Failed to initialize I2C MIDI with stored config");
                config_initialized = false;
                return false;
            }
        }
    } else {
        debug_warn("MIDI Handler: Failed to initialize configuration manager, using defaults");
        config_initialized = false;
        
        // Fallback to default initialization
        // Note: I2C bus is already initialized above
        if (!i2c_midi_init(&i2c_midi_ctx, i2c_inst, sda_pin, scl_pin, i2c_freq)) {
            debug_error("MIDI Handler: Failed to initialize I2C MIDI");
            return false;
        }
        
        // Set semitone mode from parameter if config not available
        i2c_midi_set_semitone_mode(&i2c_midi_ctx, semitone_mode);
    }
    
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

uint64_t midi_handler_get_last_note_time(void)
{
    return last_activity_time;
}

void midi_handler_init_activity_time(void)
{
    last_activity_time = time_us_64() / 1000;
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

uint8_t midi_handler_get_channel(void)
{
    // Return 1-16 (not 0-15)
    return i2c_midi_ctx.config.midi_channel + 1;
}

uint8_t midi_handler_get_semitone_mode(void)
{
    return (uint8_t)i2c_midi_ctx.config.semitone_mode;
}

void midi_handler_set_semitone_mode(uint8_t mode)
{
    if (mode > 2) {
        debug_error("MIDI Handler: Invalid semitone mode %d", mode);
        return;
    }
    
    i2c_midi_set_semitone_mode(&i2c_midi_ctx, (i2c_midi_semitone_mode_t)mode);
    
    const char* mode_names[] = {"PLAY", "IGNORE", "SKIP"};
    debug_info("MIDI Handler: Semitone mode set to %s", mode_names[mode]);
}

void midi_handler_all_notes_off(void)
{
    // Reset all I2C MIDI pins (turn all notes off)
    i2c_midi_reset(&i2c_midi_ctx);
    debug_info("MIDI Handler: All notes off");
}

bool midi_handler_save_config(void)
{
    if (!config_initialized) {
        debug_error("MIDI Handler: Configuration not initialized, cannot save");
        return false;
    }
    
    // Update configuration with current settings
    config_settings_t *settings = config_get_settings(&config_mgr);
    if (settings) {
        settings->midi_channel = i2c_midi_ctx.config.midi_channel + 1;  // Convert 0-15 to 1-16
        settings->low_note = i2c_midi_ctx.config.low_note;
        settings->note_range = i2c_midi_ctx.config.note_range;
        settings->semitone_mode = (uint8_t)i2c_midi_ctx.config.semitone_mode;
        settings->io_expander_address = i2c_midi_ctx.config.io_address;
        settings->io_expander_type = (uint8_t)i2c_midi_ctx.config.io_type;
        
        if (config_save(&config_mgr)) {
            debug_info("MIDI Handler: Configuration saved to EEPROM");
            return true;
        } else {
            debug_error("MIDI Handler: Failed to save configuration");
            return false;
        }
    }
    
    return false;
}

bool midi_handler_reset_to_defaults(void)
{
    if (!config_initialized) {
        debug_warn("MIDI Handler: Configuration not initialized");
    }
    
    // Load defaults
    config_load_defaults(&config_mgr);
    
    // Save to EEPROM
    if (config_save(&config_mgr)) {
        debug_info("MIDI Handler: Configuration reset to defaults and saved");
        return true;
    } else {
        debug_error("MIDI Handler: Failed to save default configuration");
        return false;
    }
}
