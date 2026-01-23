#include "display_handler.h"
#include "../lib/oled_display/oled_display.h"
#include "debug_uart.h"

// Internal state
static void* display_i2c = NULL;
static bool display_initialized = false;

bool display_handler_init(void* i2c_inst)
{
    display_i2c = i2c_inst;
    
    // Initialize OLED Display
    if (oled_init(i2c_inst)) {
        debug_info("Display Handler: OLED Display initialized");
        display_initialized = true;
        
        // Clear display and show startup message
        oled_clear();
        display_handler_writeline(5, 20, "Zoft Synthesizer V1");
        oled_display();
        
        return true;
    } else {
        debug_error("Display Handler: Failed to initialize OLED Display");
        display_initialized = false;
        return false;
    }
}

void display_handler_update_note(uint8_t note, uint8_t velocity, uint8_t channel)
{
    if (!display_initialized) {
        return;
    }
    
    // Display the MIDI note information
    oled_display_single_note(note, velocity, channel);
}

void display_handler_clear(void)
{
    if (!display_initialized) {
        return;
    }
    
    oled_clear();
    oled_display();
}

void display_handler_writeline(uint8_t x, uint8_t y, const char* text)
{
    if (!display_initialized || text == NULL) {
        return;
    }
    
    oled_draw_string(x, y, text);
    oled_display();
}

void display_handler_writeline_inverted(uint8_t x, uint8_t y, const char* text)
{
    if (!display_initialized || text == NULL) {
        return;
    }
    
    oled_draw_string_inverted(x, y, text);
    oled_display();
}
