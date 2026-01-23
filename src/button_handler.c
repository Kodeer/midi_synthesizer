#include "button_handler.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "debug_uart.h"

//--------------------------------------------------------------------+
// Button Configuration
//--------------------------------------------------------------------+

#define BUTTON_DEBOUNCE_MS      50      // Debounce time in milliseconds
#define BUTTON_HOLD_TIME_MS     3000    // Hold time for long press (3 seconds)

//--------------------------------------------------------------------+
// Button Handler Internal State
//--------------------------------------------------------------------+

static uint8_t button_pin = 0xFF;
static bool button_active_low = true;
static button_state_t button_state = BUTTON_STATE_IDLE;
static button_callback_t button_callback = NULL;

static uint32_t button_press_time = 0;
static uint32_t button_release_time = 0;
static bool button_hold_triggered = false;
static uint64_t last_activity_time = 0;

//--------------------------------------------------------------------+
// Button Handler Implementation
//--------------------------------------------------------------------+

bool button_init(uint8_t gpio_pin, bool active_low) {
    if (gpio_pin > 28) {
        debug_error("BUTTON: Invalid GPIO pin %d", gpio_pin);
        return false;
    }
    
    button_pin = gpio_pin;
    button_active_low = active_low;
    
    // Initialize GPIO
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    
    // Enable pull-up if active low, pull-down if active high
    if (active_low) {
        gpio_pull_up(button_pin);
    } else {
        gpio_pull_down(button_pin);
    }
    
    button_state = BUTTON_STATE_IDLE;
    button_press_time = 0;
    button_release_time = 0;
    button_hold_triggered = false;
    
    debug_info("BUTTON: Initialized on GPIO %d (%s)", 
               gpio_pin, active_low ? "active low" : "active high");
    
    return true;
}

bool button_is_pressed(void) {
    if (button_pin == 0xFF) {
        return false;
    }
    
    bool pin_state = gpio_get(button_pin);
    return button_active_low ? !pin_state : pin_state;
}

uint32_t button_get_hold_time(void) {
    if (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_HELD) {
        return to_ms_since_boot(get_absolute_time()) - button_press_time;
    }
    return 0;
}

uint64_t button_get_last_activity_time(void) {
    return last_activity_time;
}

void button_init_activity_time(void) {
    last_activity_time = time_us_64() / 1000;
}

button_event_t button_update(void) {
    if (button_pin == 0xFF) {
        return BUTTON_EVENT_NONE;
    }
    
    bool is_pressed = button_is_pressed();
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    button_event_t event = BUTTON_EVENT_NONE;
    
    switch (button_state) {
        case BUTTON_STATE_IDLE:
            if (is_pressed) {
                // Button just pressed
                button_press_time = current_time;
                button_state = BUTTON_STATE_PRESSED;
                button_hold_triggered = false;
                last_activity_time = time_us_64() / 1000;
                
                // Stop screensaver if active
                extern bool display_handler_is_screensaver_active(void);
                extern void display_handler_screensaver_stop(void);
                if (display_handler_is_screensaver_active()) {
                    display_handler_screensaver_stop();
                }
                
                debug_info("BUTTON: Pressed");
            }
            break;
            
        case BUTTON_STATE_PRESSED:
            if (!is_pressed) {
                // Button released quickly - short press
                uint32_t press_duration = current_time - button_press_time;
                if (press_duration >= BUTTON_DEBOUNCE_MS) {
                    last_activity_time = time_us_64() / 1000;
                    event = BUTTON_EVENT_SHORT_PRESS;
                    debug_info("BUTTON: Short press (%d ms)", press_duration);
                }
                button_state = BUTTON_STATE_IDLE;
            } else if ((current_time - button_press_time) >= BUTTON_HOLD_TIME_MS) {
                // Button held for long time
                if (!button_hold_triggered) {
                    event = BUTTON_EVENT_LONG_PRESS;
                    button_hold_triggered = true;
                    button_state = BUTTON_STATE_HELD;
                    debug_info("BUTTON: Long press (held for %d ms)", 
                              current_time - button_press_time);
                }
            }
            break;
            
        case BUTTON_STATE_HELD:
            if (!is_pressed) {
                // Button released after hold
                event = BUTTON_EVENT_RELEASED;
                button_state = BUTTON_STATE_IDLE;
                debug_info("BUTTON: Released after hold");
            }
            break;
            
        case BUTTON_STATE_RELEASED:
            button_state = BUTTON_STATE_IDLE;
            break;
    }
    
    // Call callback if event occurred
    if (event != BUTTON_EVENT_NONE && button_callback != NULL) {
        button_callback(event);
    }
    
    return event;
}

void button_set_callback(button_callback_t callback) {
    button_callback = callback;
    debug_info("BUTTON: Callback registered");
}
