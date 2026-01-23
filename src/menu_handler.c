#include "menu_handler.h"
#include "display_handler.h"
#include "midi_handler.h"
#include "configuration_settings.h"
#include "debug_uart.h"
#include "oled_display.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// Menu Handler Internal State
//--------------------------------------------------------------------+

static bool menu_active = false;
static menu_option_t current_option = MENU_OPTION_RESET_DEFAULTS;

// Menu option names
static const char* menu_names[MENU_OPTION_COUNT] = {
    "Reset Defaults",
    "Save Config",
    "MIDI Channel",
    "Note Range",
    "Semitone Mode",
    "All Notes Off",
    "Exit Menu"
};

//--------------------------------------------------------------------+
// Menu Handler Implementation
//--------------------------------------------------------------------+

bool menu_init(void) {
    menu_active = false;
    current_option = MENU_OPTION_RESET_DEFAULTS;
    debug_info("MENU: Initialized");
    return true;
}

bool menu_is_active(void) {
    return menu_active;
}

void menu_enter(void) {
    if (menu_active) {
        return;  // Already in menu
    }
    
    // Stop screensaver if active
    if (display_handler_is_screensaver_active()) {
        display_handler_screensaver_stop();
    }
    
    menu_active = true;
    current_option = MENU_OPTION_RESET_DEFAULTS;
    
    debug_info("MENU: Entered menu mode");
    
    // Show menu
    menu_update_display();
}

void menu_exit(void) {
    if (!menu_active) {
        return;  // Not in menu
    }
    
    menu_active = false;
    
    debug_info("MENU: Exited menu mode");
    
    // Clear display and show home screen
    display_handler_show_home();
}

void menu_next(void) {
    if (!menu_active) {
        return;
    }
    
    // Move to next option
    current_option = (current_option + 1) % MENU_OPTION_COUNT;
    
    debug_info("MENU: Selected option %d: %s", current_option, menu_names[current_option]);
    
    // Update display
    menu_update_display();
}

void menu_execute(void) {
    if (!menu_active) {
        return;
    }
    
    debug_info("MENU: Executing option %d: %s", current_option, menu_names[current_option]);
    
    char msg[32];
    
    switch (current_option) {
        case MENU_OPTION_RESET_DEFAULTS:
            // Reset configuration to defaults
            if (midi_handler_reset_to_defaults()) {
                display_handler_clear();
                display_handler_writeline(5, 20, "Reset Complete!");
                display_handler_writeline(5, 35, "Reboot to apply");
                debug_info("MENU: Configuration reset to defaults");
            } else {
                display_handler_clear();
                display_handler_writeline(5, 28, "Reset Failed!");
                debug_error("MENU: Failed to reset configuration");
            }
            sleep_ms(3500);
            menu_exit();
            break;
            
        case MENU_OPTION_SAVE_CONFIG:
            // Save current configuration
            if (midi_handler_save_config()) {
                display_handler_clear();
                display_handler_writeline(5, 28, "Config Saved!");
                debug_info("MENU: Configuration saved");
            } else {
                display_handler_clear();
                display_handler_writeline(5, 28, "Save Failed!");
                debug_error("MENU: Failed to save configuration");
            }
            sleep_ms(3000);
            menu_exit();
            break;
            
        case MENU_OPTION_MIDI_CHANNEL:
            // Cycle MIDI channel
            {
                uint8_t channel = midi_handler_get_channel() + 1;
                if (channel > 16) channel = 1;
                midi_handler_set_channel(channel);
                
                snprintf(msg, sizeof(msg), "Channel: %d", channel);
                display_handler_clear();
                display_handler_writeline(5, 28, msg);
                debug_info("MENU: MIDI channel set to %d", channel);
            }
            sleep_ms(1500);
            menu_update_display();
            break;
            
        case MENU_OPTION_NOTE_RANGE:
            // This would open a sub-menu or cycle through presets
            display_handler_clear();
            display_handler_writeline(5, 20, "Note Range");
            display_handler_writeline(5, 35, "Use SysEx");
            debug_info("MENU: Note range - use SysEx commands");
            sleep_ms(1500);
            menu_update_display();
            break;
            
        case MENU_OPTION_SEMITONE_MODE:
            // Cycle semitone modes
            {
                uint8_t mode = midi_handler_get_semitone_mode();
                mode = (mode + 1) % 3;  // 0=PLAY, 1=IGNORE, 2=SKIP
                midi_handler_set_semitone_mode(mode);
                
                const char* mode_names[] = {"PLAY", "IGNORE", "SKIP"};
                snprintf(msg, sizeof(msg), "Mode: %s", mode_names[mode]);
                display_handler_clear();
                display_handler_writeline(5, 20, "Semitone Mode");
                display_handler_writeline(5, 35, msg);
                debug_info("MENU: Semitone mode set to %s", mode_names[mode]);
            }
            sleep_ms(1500);
            menu_update_display();
            break;
            
        case MENU_OPTION_ALL_NOTES_OFF:
            // Send all notes off
            midi_handler_all_notes_off();
            display_handler_clear();
            display_handler_writeline(5, 28, "All Notes Off!");
            debug_info("MENU: All notes off");
            sleep_ms(2000);
            menu_exit();
            break;
            
        case MENU_OPTION_EXIT:
            menu_exit();
            break;
            
        default:
            break;
    }
}

void menu_update_display(void) {
    if (!menu_active) {
        return;
    }
    
    // Clear the display first
    display_handler_clear();
    
    // Display menu title - centered
    // "MENU" is 4 chars * 6 pixels = 24 pixels, center at (128-24)/2 = 52
    display_handler_writeline(52, 1, "MENU");
    
    // Draw horizontal line below heading
    // Position: 1 (top border) + 1 (offset) + 8 (text height) + 1 (space) = 11 pixels from top
    for (uint8_t x = 1; x < 127; x++) {
        oled_set_pixel(x, 11, 1);
    }
    oled_display();
    
    // Display current menu option with selection indicator
    char line[32];
    
    // Longest menu name is "Reset Defaults" (15 chars)
    // With " #. " prefix (4 chars) = 19 chars total, pad to 20 for alignment
    const uint8_t max_line_len = 20;
    
    // Show 3 options: previous, current (inverted), next
    // This creates a scrolling menu effect
    for (int8_t i = -1; i <= 1; i++) {
        int8_t option_idx = (current_option + i + MENU_OPTION_COUNT) % MENU_OPTION_COUNT;
        uint8_t row = (i + 2) * 14 + 4;  // Rows at pixels 18, 32, 46
        
        // Format with number: " #. Menu Name"
        snprintf(line, sizeof(line), " %d. %s", option_idx + 1, menu_names[option_idx]);
        
        // Pad with spaces to max_line_len
        uint8_t len = strlen(line);
        while (len < max_line_len && len < sizeof(line) - 1) {
            line[len++] = ' ';
        }
        line[len] = '\0';
        
        if (i == 0) {
            // Current selection - show inverted (white background, black text)
            display_handler_writeline_inverted(0, row, line);
        } else {
            // Other options - normal text
            display_handler_writeline(0, row, line);
        }
    }
}
