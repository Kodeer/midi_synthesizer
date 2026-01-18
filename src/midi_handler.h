#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "../lib/i2c_midi/i2c_midi.h"

/**
 * @brief Initialize MIDI handler
 * 
 * Initializes the MIDI message handler with I2C MIDI support and LED feedback.
 * 
 * @param i2c_inst I2C instance for MIDI output (i2c0 or i2c1)
 * @param sda_pin GPIO pin for I2C SDA
 * @param scl_pin GPIO pin for I2C SCL
 * @param i2c_freq I2C frequency in Hz (e.g., 100000 for 100kHz)
 * @param led_pin GPIO pin for LED feedback (use 0xFF to disable LED)
 * @param semitone_mode Semitone handling mode (PLAY, IGNORE, or SKIP)
 * @return true if initialization successful, false otherwise
 */
bool midi_handler_init(void* i2c_inst, uint8_t sda_pin, uint8_t scl_pin, 
                       uint32_t i2c_freq, uint8_t led_pin, 
                       i2c_midi_semitone_mode_t semitone_mode);

/**
 * @brief Get the MIDI message callback function
 * 
 * Returns a pointer to the callback function that should be registered
 * with the USB MIDI subsystem.
 * 
 * @return Pointer to the MIDI message callback function
 */
void* midi_handler_get_callback(void);

/**
 * @brief Configure I2C MIDI channel filter
 * 
 * @param channel MIDI channel to listen to (0-15, or 0xFF for all channels)
 * @return true if successful, false otherwise
 */
bool midi_handler_set_channel(uint8_t channel);

/**
 * @brief Configure I2C MIDI note range
 * 
 * @param min_note Minimum MIDI note number (0-127)
 * @param max_note Maximum MIDI note number (0-127)
 * @return true if successful, false otherwise
 */
bool midi_handler_set_note_range(uint8_t min_note, uint8_t max_note);

/**
 * @brief Enable or disable LED feedback
 * 
 * @param enabled true to enable LED feedback, false to disable
 */
void midi_handler_set_led_enabled(bool enabled);

/**
 * @brief Process MIDI message manually
 * 
 * Can be used to inject MIDI messages from sources other than USB.
 * 
 * @param status MIDI status byte
 * @param data1 First data byte
 * @param data2 Second data byte
 */
void midi_handler_process_message(uint8_t status, uint8_t data1, uint8_t data2);

#endif // MIDI_HANDLER_H
