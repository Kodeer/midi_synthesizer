#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize display handler
 * 
 * Initializes the OLED display and shows startup message.
 * 
 * @param i2c_inst I2C instance for display (i2c0 or i2c1)
 * @return true if initialization successful, false otherwise
 */
bool display_handler_init(void* i2c_inst);

/**
 * @brief Update display with MIDI note information
 * 
 * @param note MIDI note number (0-127)
 * @param velocity Note velocity (0-127)
 * @param channel MIDI channel (0-15)
 */
void display_handler_update_note(uint8_t note, uint8_t velocity, uint8_t channel);

/**
 * @brief Clear the display
 */
void display_handler_clear(void);

/**
 * @brief Write a text line to the display at specified position
 * 
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param text Text string to display
 */
void display_handler_writeline(uint8_t x, uint8_t y, const char* text);

/**
 * @brief Write an inverted text line to the display (white background, black text)
 * 
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param text Text string to display
 */
void display_handler_writeline_inverted(uint8_t x, uint8_t y, const char* text);

/**
 * @brief Show the home screen
 */
void display_handler_show_home(void);

/**
 * @brief Check if display is showing the home screen
 * 
 * @return true if in home screen state, false otherwise
 */
bool display_handler_is_home(void);

/**
 * @brief Start the screensaver
 */
void display_handler_screensaver_start(void);

/**
 * @brief Stop the screensaver and return to home screen
 */
void display_handler_screensaver_stop(void);

/**
 * @brief Update screensaver animation (call repeatedly)
 */
void display_handler_screensaver_update(void);

/**
 * @brief Check if screensaver is active
 * 
 * @return true if screensaver is running, false otherwise
 */
bool display_handler_is_screensaver_active(void);

/**
 * @brief Check for pending screensaver timeout and start if needed
 * Call this from main loop
 */
void display_handler_check_timeout(void);

#endif // DISPLAY_HANDLER_H
