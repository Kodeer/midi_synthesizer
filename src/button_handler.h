#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

//--------------------------------------------------------------------+
// Button Handler Module
//--------------------------------------------------------------------+

// Button states
typedef enum {
    BUTTON_STATE_IDLE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_HELD,
    BUTTON_STATE_RELEASED
} button_state_t;

// Button events
typedef enum {
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_SHORT_PRESS,   // Quick press and release
    BUTTON_EVENT_LONG_PRESS,    // Held for 3 seconds
    BUTTON_EVENT_RELEASED       // Button released after being pressed
} button_event_t;

// Button callback function type
typedef void (*button_callback_t)(button_event_t event);

/**
 * Initialize the button handler
 * @param gpio_pin GPIO pin number for the button
 * @param active_low true if button is active low (pulls to ground when pressed)
 * @return true on success, false on failure
 */
bool button_init(uint8_t gpio_pin, bool active_low);

/**
 * Update button state (call this frequently from main loop)
 * @return Current button event
 */
button_event_t button_update(void);

/**
 * Set callback function for button events
 * @param callback Function to call when button event occurs
 */
void button_set_callback(button_callback_t callback);

/**
 * Check if button is currently pressed
 * @return true if pressed, false otherwise
 */
bool button_is_pressed(void);

/**
 * Get how long button has been held (in milliseconds)
 * @return Time in milliseconds
 */
uint32_t button_get_hold_time(void);

/**
 * Get timestamp of last button activity (press or release)
 * @return Timestamp in milliseconds
 */
uint64_t button_get_last_activity_time(void);

/**
 * Initialize activity timestamp to current time
 */
void button_init_activity_time(void);

#endif // BUTTON_HANDLER_H
