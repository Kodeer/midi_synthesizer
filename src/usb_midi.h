#ifndef USB_MIDI_H
#define USB_MIDI_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief MIDI message callback function type
 * 
 * @param status MIDI status byte
 * @param data1 First data byte (e.g., note number)
 * @param data2 Second data byte (e.g., velocity)
 * @param user_data User-provided context pointer
 */
typedef void (*usb_midi_rx_callback_t)(uint8_t status, uint8_t data1, uint8_t data2, void* user_data);

/**
 * @brief Initialize USB MIDI subsystem
 * 
 * Initializes TinyUSB and sets up USB MIDI functionality.
 * This must be called before any other USB MIDI functions.
 * 
 * @return true if initialization successful, false otherwise
 */
bool usb_midi_init(void);

/**
 * @brief Set callback for received MIDI messages
 * 
 * @param callback Function to call when MIDI message is received
 * @param user_data User context pointer passed to callback
 */
void usb_midi_set_rx_callback(usb_midi_rx_callback_t callback, void* user_data);

/**
 * @brief Check if USB is mounted and ready
 * 
 * @return true if USB device is mounted, false otherwise
 */
bool usb_midi_is_mounted(void);

/**
 * @brief Process USB and MIDI tasks
 * 
 * This function must be called regularly (typically in main loop)
 * to handle USB events and process incoming MIDI messages.
 */
void usb_midi_task(void);

/**
 * @brief Send MIDI Note On/Off message
 * 
 * @param channel MIDI channel (0-15)
 * @param note Note number (0-127)
 * @param velocity Velocity (0-127)
 * @param note_on true for Note On, false for Note Off
 * @return Number of bytes sent, or 0 if failed
 */
int usb_midi_send_note(uint8_t channel, uint8_t note, uint8_t velocity, bool note_on);

/**
 * @brief Send MIDI Control Change message
 * 
 * @param channel MIDI channel (0-15)
 * @param controller Controller number (0-127)
 * @param value Controller value (0-127)
 * @return Number of bytes sent, or 0 if failed
 */
int usb_midi_send_cc(uint8_t channel, uint8_t controller, uint8_t value);

/**
 * @brief Send MIDI Program Change message
 * 
 * @param channel MIDI channel (0-15)
 * @param program Program number (0-127)
 * @return Number of bytes sent, or 0 if failed
 */
int usb_midi_send_program_change(uint8_t channel, uint8_t program);

/**
 * @brief Send MIDI Pitch Bend message
 * 
 * @param channel MIDI channel (0-15)
 * @param value Pitch bend value (0-16383, center is 8192)
 * @return Number of bytes sent, or 0 if failed
 */
int usb_midi_send_pitch_bend(uint8_t channel, uint16_t value);

/**
 * @brief Send raw MIDI message
 * 
 * @param data Pointer to MIDI message bytes
 * @param length Number of bytes to send (1-3 for most messages)
 * @return Number of bytes sent, or 0 if failed
 */
int usb_midi_send_raw(const uint8_t* data, uint8_t length);

#endif // USB_MIDI_H
