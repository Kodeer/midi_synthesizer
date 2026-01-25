#ifndef CONFIGURATION_SETTINGS_H
#define CONFIGURATION_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "../lib/i2c_memory/drivers/at24cxx_driver.h"

// Configuration magic number to verify valid data in EEPROM
#define CONFIG_MAGIC_NUMBER 0x4D53594E  // "MSYN" - MIDI Synthesizer

// Configuration version for future compatibility
#define CONFIG_VERSION 1

/**
 * MIDI player types
 */
typedef enum {
    PLAYER_TYPE_I2C_MIDI = 0,    // I2C MIDI output (default)
    PLAYER_TYPE_MALLET_MIDI = 1  // Servo-controlled xylophone striker
} player_type_t;

/**
 * Global configuration structure
 * This structure holds all persistent settings for the MIDI synthesizer
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;                      // Magic number to verify valid config
    uint8_t version;                     // Configuration version
    
    // MIDI settings
    uint8_t midi_channel;                // MIDI channel to listen to (1-16)
    uint8_t note_range;                  // Number of notes to handle
    uint8_t low_note;                    // Lowest MIDI note to respond to
    uint8_t semitone_mode;               // Semitone handling mode (0=PLAY, 1=IGNORE, 2=SKIP)
    uint8_t player_type;                 // Player type (0=I2C_MIDI, 1=MALLET_MIDI)
    
    // I2C IO Expander settings
    uint8_t io_expander_type;            // IO expander type (0=PCF8574, 1=CH423)
    uint8_t io_expander_address;         // I2C address of IO expander
    
    // Display settings (for future use)
    uint8_t display_enabled;             // Display enable flag
    uint8_t display_brightness;          // Display brightness (0-255)
    uint8_t display_timeout;             // Display timeout in seconds (0=never)
    
    // Reserved for future expansion
    uint8_t reserved[16];
    
    // CRC for data integrity
    uint16_t crc;                        // CRC16 checksum
} config_settings_t;

/**
 * Configuration manager context
 */
typedef struct {
    at24cxx_t eeprom;                    // EEPROM driver context
    config_settings_t settings;          // Current settings in RAM
    bool initialized;                    // Initialization flag
    uint32_t eeprom_start_address;       // Start address in EEPROM
} config_manager_t;

/**
 * Initialize the configuration manager
 * 
 * @param ctx Pointer to configuration manager context
 * @param i2c_port I2C port for EEPROM communication
 * @param eeprom_address I2C address of EEPROM
 * @param eeprom_capacity_kb EEPROM capacity in kilobytes
 * @param start_address Starting address in EEPROM for config storage
 * @return true if successful, false otherwise
 */
bool config_init(config_manager_t *ctx, i2c_inst_t *i2c_port, uint8_t eeprom_address, 
                 uint16_t eeprom_capacity_kb, uint32_t start_address);

/**
 * Load configuration from EEPROM
 * If no valid config found, loads defaults and saves them to EEPROM
 * 
 * @param ctx Pointer to configuration manager context
 * @return true if successful, false otherwise
 */
bool config_load(config_manager_t *ctx);

/**
 * Save current configuration to EEPROM
 * 
 * @param ctx Pointer to configuration manager context
 * @return true if successful, false otherwise
 */
bool config_save(config_manager_t *ctx);

/**
 * Load default configuration settings
 * 
 * @param ctx Pointer to configuration manager context
 */
void config_load_defaults(config_manager_t *ctx);

/**
 * Get pointer to current settings
 * 
 * @param ctx Pointer to configuration manager context
 * @return Pointer to settings structure
 */
config_settings_t* config_get_settings(config_manager_t *ctx);

/**
 * Update a specific MIDI setting and save to EEPROM
 * 
 * @param ctx Pointer to configuration manager context
 * @param param Parameter ID
 * @param value New value
 * @return true if successful, false otherwise
 */
bool config_update_midi_setting(config_manager_t *ctx, uint8_t param, uint8_t value);

/**
 * Update IO expander settings and save to EEPROM
 * 
 * @param ctx Pointer to configuration manager context
 * @param type IO expander type
 * @param address I2C address
 * @return true if successful, false otherwise
 */
bool config_update_io_settings(config_manager_t *ctx, uint8_t type, uint8_t address);

/**
 * Update display settings and save to EEPROM
 * 
 * @param ctx Pointer to configuration manager context
 * @param enabled Display enable flag
 * @param brightness Display brightness
 * @param timeout Display timeout
 * @return true if successful, false otherwise
 */
bool config_update_display_settings(config_manager_t *ctx, uint8_t enabled, 
                                     uint8_t brightness, uint8_t timeout);

/**
 * Erase configuration from EEPROM (reset to defaults)
 * 
 * @param ctx Pointer to configuration manager context
 * @return true if successful, false otherwise
 */
bool config_erase(config_manager_t *ctx);

/**
 * Validate configuration data integrity
 * 
 * @param settings Pointer to settings structure
 * @return true if valid, false otherwise
 */
bool config_validate(const config_settings_t *settings);

#endif // CONFIGURATION_SETTINGS_H
