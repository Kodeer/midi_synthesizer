#include "menu_handler.h"
#include "display_handler.h"
#include "midi_handler.h"
#include "configuration_settings.h"
#include "debug_uart.h"
#include "oled_display.h"
#include "buzzer.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// Menu Handler Internal State
//--------------------------------------------------------------------+

static bool menu_active = false;
static menu_option_t current_option = MENU_OPTION_RESET_DEFAULTS;
static bool channel_selection_active = false;
static uint8_t selected_channel = 1;
static bool settings_view_active = false;
static uint8_t settings_view_offset = 0;

// Menu option names
static const char* menu_names[MENU_OPTION_COUNT] = {
    "Reset Defaults",
    "Save Config",
    "Player Type",
    "MIDI Channel",
    "Note Range",
    "Semitone Mode",
    "View Settings",
    "All Notes Off",
    "Exit Menu"
};

//--------------------------------------------------------------------+
// Settings View Functions
//--------------------------------------------------------------------+

static void display_settings_view(void) {
    const uint8_t max_lines = 6;  // Maximum lines that fit on display
    const uint8_t line_height = 10;
    const uint8_t start_y = 2;
    
    // Get current settings
    uint8_t channel = midi_handler_get_channel();
    uint8_t semitone = midi_handler_get_semitone_mode();
    uint8_t player_type = midi_handler_get_player_type();
    uint8_t note_range = midi_handler_get_note_range();
    uint8_t low_note = midi_handler_get_low_note();
    uint8_t high_note = midi_handler_get_high_note();
    uint8_t io_type = midi_handler_get_io_type();
    uint8_t io_addr = midi_handler_get_io_address();
    
    const char* semitone_names[] = {"PLAY", "IGNORE", "SKIP"};
    const char* player_names[] = {"I2C MIDI", "Mallet MIDI"};
    const char* io_type_names[] = {"PCF8574", "CH423"};
    
    // Build settings array - increased to accommodate more settings
    char settings[10][22];  // 10 settings, 21 chars + null terminator
    uint8_t total_settings = 0;
    
    snprintf(settings[total_settings++], 22, "Channel: %d", channel);
    snprintf(settings[total_settings++], 22, "Player: %s", player_names[player_type]);
    snprintf(settings[total_settings++], 22, "Semitone: %s", semitone_names[semitone]);
    snprintf(settings[total_settings++], 22, "Note Range: %d", note_range);
    snprintf(settings[total_settings++], 22, "Low Note: %d", low_note);
    snprintf(settings[total_settings++], 22, "High Note: %d", high_note);
    snprintf(settings[total_settings++], 22, "I2C Freq: 400kHz");
    snprintf(settings[total_settings++], 22, "IO Type: %s", io_type_names[io_type]);
    snprintf(settings[total_settings++], 22, "IO Addr: 0x%02X", io_addr);
    
    // Clear display
    display_handler_clear();
    
    // Display title
    display_handler_writeline(30, start_y, "SETTINGS");
    
    // Draw horizontal line
    for (uint8_t x = 1; x < 127; x++) {
        oled_set_pixel(x, start_y + 9, 1);
    }
    
    // Display settings starting from offset
    uint8_t y_pos = start_y + 13;
    for (uint8_t i = 0; i < max_lines && (settings_view_offset + i) < total_settings; i++) {
        display_handler_writeline(2, y_pos, settings[settings_view_offset + i]);
        y_pos += line_height;
    }
    
    // Show scroll indicator if more settings available
    if (settings_view_offset + max_lines < total_settings) {
        display_handler_writeline(110, 56, "v");
    }
    if (settings_view_offset > 0) {
        display_handler_writeline(110, 1, "^");
    }
    
    oled_display();
}

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
    
    // If in settings view mode, scroll through settings
    if (settings_view_active) {
        const uint8_t total_settings = 9;  // Total number of settings
        const uint8_t max_lines = 6;        // Max lines visible
        
        if (settings_view_offset + max_lines < total_settings) {
            settings_view_offset++;
            display_settings_view();
            debug_info("MENU: Settings view scroll: offset=%d", settings_view_offset);
        }
        return;
    }
    
    // If in channel selection mode, increment channel number
    if (channel_selection_active) {
        selected_channel++;
        if (selected_channel > 16) {
            selected_channel = 1;
        }
        
        // Update display with new channel
        char msg[32];
        display_handler_clear();
        snprintf(msg, sizeof(msg), "Channel: %d", selected_channel);
        display_handler_writeline(30, 28, msg);
        oled_display();
        
        debug_info("MENU: Channel selection: %d", selected_channel);
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
    
    // If in settings view mode, long press exits back to menu
    if (settings_view_active) {
        settings_view_active = false;
        settings_view_offset = 0;
        debug_info("MENU: Exited settings view");
        menu_update_display();
        return;
    }
    
    // If in channel selection mode, confirm and exit
    if (channel_selection_active) {
        // Convert 1-based display channel to 0-based for MIDI handler
        midi_handler_set_channel(selected_channel - 1);
        
        display_handler_clear();
        char msg[32];
        snprintf(msg, sizeof(msg), "Channel %d Set!", selected_channel);
        display_handler_writeline(5, 28, msg);
        buzzer_success();  // Play success sound
        debug_info("MENU: MIDI channel set to %d", selected_channel);
        
        sleep_ms(1500);
        
        // Exit channel selection mode and return to menu
        channel_selection_active = false;
        menu_update_display();
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
                buzzer_success();  // Play success sound
                debug_info("MENU: Configuration saved");
            } else {
                display_handler_clear();
                display_handler_writeline(5, 28, "Save Failed!");
                buzzer_error();  // Play error sound
                debug_error("MENU: Failed to save configuration");
            }
            sleep_ms(3000);
            menu_exit();
            break;
            
        case MENU_OPTION_MIDI_CHANNEL:
            // Enter channel selection mode
            channel_selection_active = true;
            selected_channel = midi_handler_get_channel();
            
            // Display channel selection screen
            display_handler_clear();
            snprintf(msg, sizeof(msg), "Channel: %d", selected_channel);
            display_handler_writeline(30, 28, msg);
            oled_display();
            
            debug_info("MENU: Entered channel selection mode (current: %d)", selected_channel);
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
            
        case MENU_OPTION_PLAYER_TYPE:
            // Cycle player types
            {
                uint8_t player_type = midi_handler_get_player_type();
                player_type = (player_type + 1) % 2;  // 0=I2C_MIDI, 1=MALLET_MIDI
                midi_handler_set_player_type(player_type);
                
                const char* player_names[] = {"I2C MIDI", "Mallet MIDI"};
                snprintf(msg, sizeof(msg), "Type: %s", player_names[player_type]);
                display_handler_clear();
                display_handler_writeline(5, 20, "Player Type");
                display_handler_writeline(5, 35, msg);
                debug_info("MENU: Player type set to %s", player_names[player_type]);
            }
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
            
        case MENU_OPTION_VIEW_SETTINGS:
            // Enter settings view mode
            settings_view_active = true;
            settings_view_offset = 0;
            display_settings_view();
            debug_info("MENU: Entered settings view mode");
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
