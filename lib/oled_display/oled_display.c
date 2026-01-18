#include "oled_display.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>

// SSD1306 Commands
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_INVERTDISPLAY       0xA7
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_COMSCANINC          0xC0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB

// Display buffer
static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];
static i2c_inst_t* i2c_instance = NULL;

// Simple 5x7 font (ASCII 32-127)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x00, 0x02, 0x05, 0x02}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x10, 0x08, 0x08, 0x10, 0x08}, // '~'
};

// Note names
static const char* note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// Send command to SSD1306
static void oled_send_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(i2c_instance, OLED_I2C_ADDRESS, buf, 2, false);
}

// Send data to SSD1306
static void oled_send_data(const uint8_t* data, size_t len) {
    uint8_t buf[len + 1];
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    i2c_write_blocking(i2c_instance, OLED_I2C_ADDRESS, buf, len + 1, false);
}

bool oled_init(void* i2c_inst) {
    i2c_instance = (i2c_inst_t*)i2c_inst;
    
    // Initialization sequence
    oled_send_command(SSD1306_DISPLAYOFF);
    oled_send_command(SSD1306_SETDISPLAYCLOCKDIV);
    oled_send_command(0x80);
    oled_send_command(SSD1306_SETMULTIPLEX);
    oled_send_command(OLED_HEIGHT - 1);
    oled_send_command(SSD1306_SETDISPLAYOFFSET);
    oled_send_command(0x00);
    oled_send_command(0x40); // Set start line
    oled_send_command(SSD1306_CHARGEPUMP);
    oled_send_command(0x14);
    oled_send_command(SSD1306_MEMORYMODE);
    oled_send_command(0x00); // Horizontal addressing mode
    oled_send_command(SSD1306_SEGREMAP | 0x01);
    oled_send_command(SSD1306_COMSCANDEC);
    oled_send_command(SSD1306_SETCOMPINS);
    oled_send_command(0x12);
    oled_send_command(SSD1306_SETCONTRAST);
    oled_send_command(0xCF);
    oled_send_command(SSD1306_SETPRECHARGE);
    oled_send_command(0xF1);
    oled_send_command(SSD1306_SETVCOMDETECT);
    oled_send_command(0x40);
    oled_send_command(SSD1306_DISPLAYALLON_RESUME);
    oled_send_command(SSD1306_NORMALDISPLAY);
    oled_send_command(SSD1306_DISPLAYON);
    
    oled_clear();
    oled_display();
    
    return true;
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_display(void) {
    oled_send_command(SSD1306_COLUMNADDR);
    oled_send_command(0);
    oled_send_command(OLED_WIDTH - 1);
    oled_send_command(SSD1306_PAGEADDR);
    oled_send_command(0);
    oled_send_command(OLED_PAGES - 1);
    
    // Send buffer in chunks
    for (int i = 0; i < sizeof(oled_buffer); i += 16) {
        size_t chunk_size = (sizeof(oled_buffer) - i) < 16 ? (sizeof(oled_buffer) - i) : 16;
        oled_send_data(&oled_buffer[i], chunk_size);
    }
}

void oled_set_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    
    if (color) {
        oled_buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y & 7));
    } else {
        oled_buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y & 7));
    }
}

void oled_draw_char(uint8_t x, uint8_t y, char c) {
    if (c < 32 || c > 126) c = 32; // Printable ASCII only (32-126)
    
    const uint8_t* glyph = font5x7[c - 32];
    
    for (int i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                oled_set_pixel(x + i, y + j, 1);
            }
        }
    }
}

void oled_draw_string(uint8_t x, uint8_t y, const char* str) {
    uint8_t curr_x = x;
    while (*str) {
        if (curr_x + 6 > OLED_WIDTH) break;
        oled_draw_char(curr_x, y, *str);
        curr_x += 6;
        str++;
    }
}

void oled_note_to_name(uint8_t note, char* name_buffer) {
    if (note > 127) note = 127;
    
    uint8_t octave = (note / 12) - 1;
    uint8_t note_index = note % 12;
    
    sprintf(name_buffer, "%s%d", note_names[note_index], octave);
}

void oled_display_single_note(uint8_t note_num, uint8_t velocity, uint8_t channel) {
    char buffer[32];
    
    oled_clear();
    
    // Title
    oled_draw_string(30, 0, "MIDI NOTE");
    
    // Note name
    char note_name[8];
    oled_note_to_name(note_num, note_name);
    sprintf(buffer, "Note: %s (%d)", note_name, note_num);
    oled_draw_string(0, 16, buffer);
    
    // Velocity
    sprintf(buffer, "Vel: %d", velocity);
    oled_draw_string(0, 28, buffer);
    
    // Channel
    sprintf(buffer, "Ch: %d", channel + 1);
    oled_draw_string(0, 40, buffer);
    
    // Velocity bar
    uint8_t bar_width = (velocity * 100) / 127;
    for (int i = 0; i < bar_width; i++) {
        for (int j = 54; j < 62; j++) {
            oled_set_pixel(i + 14, j, 1);
        }
    }
    
    oled_display();
}

void oled_display_midi_notes(const midi_note_info_t* notes, uint8_t num_notes) {
    oled_clear();
    
    // Header
    oled_draw_string(20, 0, "ACTIVE NOTES");
    
    uint8_t y_pos = 12;
    uint8_t displayed = 0;
    
    for (uint8_t i = 0; i < num_notes && displayed < 6; i++) {
        if (notes[i].active) {
            char buffer[32];
            char note_name[8];
            
            oled_note_to_name(notes[i].note, note_name);
            sprintf(buffer, "%s V%d C%d", note_name, notes[i].velocity, notes[i].channel + 1);
            oled_draw_string(0, y_pos, buffer);
            
            y_pos += 9;
            displayed++;
        }
    }
    
    if (displayed == 0) {
        oled_draw_string(15, 30, "No active notes");
    }
    
    oled_display();
}

void oled_display_channel_activity(const uint8_t* channel_activity) {
    oled_clear();
    
    // Header
    oled_draw_string(10, 0, "CHANNEL ACTIVITY");
    
    // Display 16 channels as bars
    for (int ch = 0; ch < 16; ch++) {
        uint8_t x = (ch % 8) * 16;
        uint8_t y = 16 + (ch / 8) * 24;
        
        // Channel number
        char buffer[4];
        sprintf(buffer, "%d", ch + 1);
        oled_draw_string(x, y, buffer);
        
        // Activity bar (vertical)
        uint8_t bar_height = (channel_activity[ch] * 14) / 127;
        for (int i = 0; i < bar_height; i++) {
            for (int j = 0; j < 4; j++) {
                oled_set_pixel(x + j + 6, y + 14 - i, 1);
            }
        }
    }
    
    oled_display();
}
