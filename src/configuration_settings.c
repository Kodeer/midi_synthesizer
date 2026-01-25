#include "configuration_settings.h"
#include "debug_uart.h"
#include <string.h>

// Default configuration values
#define DEFAULT_MIDI_CHANNEL        10      // Percussion channel
#define DEFAULT_NOTE_RANGE          8       // 8 notes
#define DEFAULT_LOW_NOTE            60      // Middle C
#define DEFAULT_SEMITONE_MODE       0       // PLAY mode
#define DEFAULT_PLAYER_TYPE         0       // I2C_MIDI
#define DEFAULT_IO_EXPANDER_TYPE    0       // PCF8574
#define DEFAULT_IO_EXPANDER_ADDRESS 0x20    // PCF8574 default
#define DEFAULT_DISPLAY_ENABLED     1       // Enabled
#define DEFAULT_DISPLAY_BRIGHTNESS  128     // Medium brightness
#define DEFAULT_DISPLAY_TIMEOUT     30      // 30 seconds

/**
 * Calculate CRC16 for data integrity checking
 */
static uint16_t calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

/**
 * Initialize the configuration manager
 */
bool config_init(config_manager_t *ctx, i2c_inst_t *i2c_port, uint8_t eeprom_address,
                 uint16_t eeprom_capacity_kb, uint32_t start_address) {
    if (!ctx || !i2c_port) {
        debug_error("CONFIG: Init failed - invalid parameters");
        return false;
    }
    
    debug_info("CONFIG: Initializing configuration manager...");
    
    // Initialize EEPROM driver
    if (!at24cxx_init(&ctx->eeprom, i2c_port, eeprom_address, eeprom_capacity_kb)) {
        debug_error("CONFIG: EEPROM initialization failed");
        return false;
    }
    
    ctx->eeprom_start_address = start_address;
    ctx->initialized = false;
    
    debug_info("CONFIG: EEPROM initialized (address=0x%02X, capacity=%dKB, start=0x%04X)",
               eeprom_address, eeprom_capacity_kb, start_address);
    
    // Load configuration from EEPROM (or defaults if not found)
    if (!config_load(ctx)) {
        debug_error("CONFIG: Failed to load configuration");
        return false;
    }
    
    ctx->initialized = true;
    debug_info("CONFIG: Configuration manager initialized successfully");
    return true;
}

/**
 * Load default configuration settings
 */
void config_load_defaults(config_manager_t *ctx) {
    if (!ctx) {
        return;
    }
    
    debug_info("CONFIG: Loading default settings...");
    
    config_settings_t *s = &ctx->settings;
    
    // Set magic number and version
    s->magic = CONFIG_MAGIC_NUMBER;
    s->version = CONFIG_VERSION;
    
    // MIDI settings
    s->midi_channel = DEFAULT_MIDI_CHANNEL;
    s->note_range = DEFAULT_NOTE_RANGE;
    s->low_note = DEFAULT_LOW_NOTE;
    s->semitone_mode = DEFAULT_SEMITONE_MODE;
    s->player_type = DEFAULT_PLAYER_TYPE;
    
    // I2C IO Expander settings
    s->io_expander_type = DEFAULT_IO_EXPANDER_TYPE;
    s->io_expander_address = DEFAULT_IO_EXPANDER_ADDRESS;
    
    // Display settings
    s->display_enabled = DEFAULT_DISPLAY_ENABLED;
    s->display_brightness = DEFAULT_DISPLAY_BRIGHTNESS;
    s->display_timeout = DEFAULT_DISPLAY_TIMEOUT;
    
    // Clear reserved bytes
    memset(s->reserved, 0, sizeof(s->reserved));
    
    // Calculate CRC (exclude the CRC field itself)
    s->crc = calculate_crc16((uint8_t*)s, sizeof(config_settings_t) - sizeof(s->crc));
    
    debug_info("CONFIG: Defaults loaded - Ch:%d, Notes:%d-%d, IO:0x%02X",
               s->midi_channel, s->low_note, s->low_note + s->note_range - 1, s->io_expander_address);
}

/**
 * Validate configuration data integrity
 */
bool config_validate(const config_settings_t *settings) {
    if (!settings) {
        return false;
    }
    
    // Check magic number
    if (settings->magic != CONFIG_MAGIC_NUMBER) {
        debug_error("CONFIG: Invalid magic number (0x%08X)", settings->magic);
        return false;
    }
    
    // Check version
    if (settings->version != CONFIG_VERSION) {
        debug_warn("CONFIG: Version mismatch (found %d, expected %d)", 
                   settings->version, CONFIG_VERSION);
        // Could handle version migration here in the future
    }
    
    // Validate CRC
    uint16_t calculated_crc = calculate_crc16((uint8_t*)settings, 
                                              sizeof(config_settings_t) - sizeof(settings->crc));
    if (calculated_crc != settings->crc) {
        debug_error("CONFIG: CRC mismatch (calculated=0x%04X, stored=0x%04X)",
                   calculated_crc, settings->crc);
        return false;
    }
    
    // Validate ranges
    if (settings->midi_channel < 1 || settings->midi_channel > 16) {
        debug_error("CONFIG: Invalid MIDI channel (%d)", settings->midi_channel);
        return false;
    }
    
    if (settings->note_range < 1 || settings->note_range > 16) {
        debug_error("CONFIG: Invalid note range (%d)", settings->note_range);
        return false;
    }
    
    if (settings->low_note > 127) {
        debug_error("CONFIG: Invalid low note (%d)", settings->low_note);
        return false;
    }
    
    if (settings->semitone_mode > 2) {
        debug_error("CONFIG: Invalid semitone mode (%d)", settings->semitone_mode);
        return false;
    }
    
    debug_info("CONFIG: Validation passed");
    return true;
}

/**
 * Load configuration from EEPROM
 */
bool config_load(config_manager_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    debug_info("CONFIG: Loading configuration from EEPROM (address=0x%04X)...", 
               ctx->eeprom_start_address);
    
    // Read configuration from EEPROM
    config_settings_t temp_settings;
    if (!at24cxx_read(&ctx->eeprom, ctx->eeprom_start_address, 
                      (uint8_t*)&temp_settings, sizeof(config_settings_t))) {
        debug_error("CONFIG: Failed to read from EEPROM");
        goto load_defaults;
    }
    
    // Validate loaded configuration
    if (!config_validate(&temp_settings)) {
        debug_warn("CONFIG: Invalid configuration in EEPROM");
        goto load_defaults;
    }
    
    // Copy valid configuration
    memcpy(&ctx->settings, &temp_settings, sizeof(config_settings_t));
    debug_info("CONFIG: Configuration loaded successfully");
    debug_info("CONFIG: Ch:%d, Range:%d notes, Low:%d, Mode:%d, IO:0x%02X",
               ctx->settings.midi_channel, ctx->settings.note_range,
               ctx->settings.low_note, ctx->settings.semitone_mode, 
               ctx->settings.io_expander_address);
    return true;

load_defaults:
    debug_info("CONFIG: Loading defaults and saving to EEPROM...");
    config_load_defaults(ctx);
    
    // Save defaults to EEPROM
    if (!config_save(ctx)) {
        debug_error("CONFIG: Failed to save default configuration");
        return false;
    }
    
    return true;
}

/**
 * Save current configuration to EEPROM
 */
bool config_save(config_manager_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    debug_info("CONFIG: Saving configuration to EEPROM...");
    
    // Update CRC before saving
    ctx->settings.crc = calculate_crc16((uint8_t*)&ctx->settings,
                                        sizeof(config_settings_t) - sizeof(ctx->settings.crc));
    
    // Write configuration to EEPROM
    if (!at24cxx_write(&ctx->eeprom, ctx->eeprom_start_address,
                       (uint8_t*)&ctx->settings, sizeof(config_settings_t))) {
        debug_error("CONFIG: Failed to write to EEPROM");
        return false;
    }
    
    debug_info("CONFIG: Configuration saved successfully");
    return true;
}

/**
 * Get pointer to current settings
 */
config_settings_t* config_get_settings(config_manager_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    return &ctx->settings;
}

/**
 * Update a specific MIDI setting and save to EEPROM
 */
bool config_update_midi_setting(config_manager_t *ctx, uint8_t param, uint8_t value) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    debug_info("CONFIG: Updating MIDI setting - param=%d, value=%d", param, value);
    
    switch (param) {
        case 0: // MIDI Channel
            if (value < 1 || value > 16) {
                debug_error("CONFIG: Invalid MIDI channel (%d)", value);
                return false;
            }
            ctx->settings.midi_channel = value;
            break;
            
        case 1: // Note Range
            if (value < 1 || value > 16) {
                debug_error("CONFIG: Invalid note range (%d)", value);
                return false;
            }
            ctx->settings.note_range = value;
            break;
            
        case 2: // Low Note
            if (value > 127) {
                debug_error("CONFIG: Invalid low note (%d)", value);
                return false;
            }
            ctx->settings.low_note = value;
            break;
            
        case 3: // Semitone Mode
            if (value > 2) {
                debug_error("CONFIG: Invalid semitone mode (%d)", value);
                return false;
            }
            ctx->settings.semitone_mode = value;
            break;
            
        default:
            debug_error("CONFIG: Unknown MIDI parameter (%d)", param);
            return false;
    }
    
    // Save to EEPROM
    return config_save(ctx);
}

/**
 * Update IO expander settings and save to EEPROM
 */
bool config_update_io_settings(config_manager_t *ctx, uint8_t type, uint8_t address) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    debug_info("CONFIG: Updating IO settings - type=%d, address=0x%02X", type, address);
    
    if (type > 1) {
        debug_error("CONFIG: Invalid IO expander type (%d)", type);
        return false;
    }
    
    ctx->settings.io_expander_type = type;
    ctx->settings.io_expander_address = address;
    
    // Save to EEPROM
    return config_save(ctx);
}

/**
 * Update display settings and save to EEPROM
 */
bool config_update_display_settings(config_manager_t *ctx, uint8_t enabled,
                                     uint8_t brightness, uint8_t timeout) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    debug_info("CONFIG: Updating display settings - enabled=%d, brightness=%d, timeout=%d",
               enabled, brightness, timeout);
    
    ctx->settings.display_enabled = enabled;
    ctx->settings.display_brightness = brightness;
    ctx->settings.display_timeout = timeout;
    
    // Save to EEPROM
    return config_save(ctx);
}

/**
 * Erase configuration from EEPROM (reset to defaults)
 */
bool config_erase(config_manager_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    
    debug_info("CONFIG: Erasing configuration and resetting to defaults...");
    
    // Load defaults
    config_load_defaults(ctx);
    
    // Erase EEPROM area (write zeros)
    uint8_t zero_buffer[sizeof(config_settings_t)];
    memset(zero_buffer, 0, sizeof(zero_buffer));
    
    if (!at24cxx_write(&ctx->eeprom, ctx->eeprom_start_address,
                       zero_buffer, sizeof(zero_buffer))) {
        debug_error("CONFIG: Failed to erase EEPROM");
        return false;
    }
    
    // Save defaults
    if (!config_save(ctx)) {
        debug_error("CONFIG: Failed to save defaults after erase");
        return false;
    }
    
    debug_info("CONFIG: Configuration erased and defaults restored");
    return true;
}
