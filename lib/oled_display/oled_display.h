#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

// SSD1306 OLED Display Configuration
#define OLED_I2C_ADDRESS    0x3C  // 0x78 >> 1 (I2C address in 7-bit format)
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)

// MIDI Note Structure for Display
typedef struct {
    uint8_t note;           // MIDI note number (0-127)
    uint8_t velocity;       // Note velocity (0-127)
    uint8_t channel;        // MIDI channel (0-15)
    bool active;            // Note on/off status
} midi_note_info_t;

/**
 * @brief Initialize the OLED display
 * 
 * @param i2c_inst I2C instance (i2c0 or i2c1)
 * @return true if initialization successful, false otherwise
 */
bool oled_init(void* i2c_inst);

/**
 * @brief Clear the entire display
 */
void oled_clear(void);

/**
 * @brief Draw a single pixel border around the display
 */
void oled_draw_border(void);

/**
 * @brief Update the display with buffered content
 */
void oled_display(void);

/**
 * @brief Set a pixel on the display buffer
 * 
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param color 1 for white, 0 for black
 */
void oled_set_pixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Draw a character at specified position
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 */
void oled_draw_char(uint8_t x, uint8_t y, char c);

/**
 * @brief Draw a string at specified position
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 */
void oled_draw_string(uint8_t x, uint8_t y, const char* str);

/**
 * @brief Draw an inverted character at specified position (white background, black text)
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 */
void oled_draw_char_inverted(uint8_t x, uint8_t y, char c);

/**
 * @brief Draw an inverted string at specified position (white background, black text)
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 */
void oled_draw_string_inverted(uint8_t x, uint8_t y, const char* str);

/**
 * @brief Display MIDI note information
 * 
 * @param notes Array of MIDI note info structures
 * @param num_notes Number of notes in the array
 */
void oled_display_midi_notes(const midi_note_info_t* notes, uint8_t num_notes);

/**
 * @brief Display a single active MIDI note with details
 * 
 * @param note_num MIDI note number (0-127)
 * @param velocity Note velocity (0-127)
 * @param channel MIDI channel (0-15)
 */
void oled_display_single_note(uint8_t note_num, uint8_t velocity, uint8_t channel);

/**
 * @brief Display MIDI channel activity bar
 * 
 * @param channel_activity Array of 16 channel activity levels (0-127)
 */
void oled_display_channel_activity(const uint8_t* channel_activity);

/**
 * @brief Convert MIDI note number to note name
 * 
 * @param note MIDI note number (0-127)
 * @param name_buffer Buffer to store note name (minimum 4 bytes)
 */
void oled_note_to_name(uint8_t note, char* name_buffer);

/**
 * @brief Initialize the screensaver
 */
void oled_screensaver_init(void);

/**
 * @brief Update and display the screensaver animation
 * Call this repeatedly to animate the screensaver
 */
void oled_screensaver_update(void);

#endif // OLED_DISPLAY_H
