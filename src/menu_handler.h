#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

//--------------------------------------------------------------------+
// Menu Handler Module
//--------------------------------------------------------------------+

// Menu options
typedef enum {
    MENU_OPTION_RESET_DEFAULTS,
    MENU_OPTION_SAVE_CONFIG,
    MENU_OPTION_PLAYER_TYPE,
    MENU_OPTION_MIDI_CHANNEL,
    MENU_OPTION_NOTE_RANGE,
    MENU_OPTION_SEMITONE_MODE,
    MENU_OPTION_VIEW_SETTINGS,
    MENU_OPTION_ALL_NOTES_OFF,
    MENU_OPTION_EXIT,
    MENU_OPTION_COUNT  // Must be last
} menu_option_t;

/**
 * Initialize the menu system
 * @return true on success, false on failure
 */
bool menu_init(void);

/**
 * Check if menu is currently active
 * @return true if in menu mode, false otherwise
 */
bool menu_is_active(void);

/**
 * Enter menu mode
 */
void menu_enter(void);

/**
 * Exit menu mode
 */
void menu_exit(void);

/**
 * Move to next menu option
 */
void menu_next(void);

/**
 * Execute currently selected menu option
 */
void menu_execute(void);

/**
 * Get current menu selection
 * @return Current menu option
 */
menu_option_t menu_get_current(void);

/**
 * Update menu display
 */
void menu_update_display(void);

#endif // MENU_HANDLER_H
