#include "display_handler.h"
#include "../lib/oled_display/oled_display.h"
#include "../lib/oled_display/lissajous_screensaver.h"
#include "debug_uart.h"
#include "pico/time.h"
#include "button_handler.h"
#include "midi_handler.h"
#include "menu_handler.h"

// Internal state
static void* display_i2c = NULL;
static bool display_initialized = false;
static bool is_home_screen = true;
static bool screensaver_active = false;
static bool screensaver_pending = false;

// Timer configuration
#define SCREENSAVER_TIMEOUT_MS 30000  // 30 seconds
#define TIMER_CHECK_INTERVAL_MS 1000   // Check every 1 second

static alarm_id_t timeout_alarm = 0;

// Timer callback - runs in interrupt context
static int64_t timeout_check_callback(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;
    
    // Don't start screensaver if menu active or already active
    if (screensaver_active || menu_is_active()) {
        return TIMER_CHECK_INTERVAL_MS * 1000;  // Continue checking (return microseconds)
    }
    
    uint64_t current_time = time_us_64() / 1000;
    uint64_t last_midi = midi_handler_get_last_note_time();
    uint64_t last_button = button_get_last_activity_time();
    uint64_t last_activity = (last_midi > last_button) ? last_midi : last_button;
    
    // Handle case where no activity has occurred yet (both timestamps = 0)
    // In this case, use boot time as reference
    if (last_activity == 0) {
        // No activity yet - check time since boot
        if (current_time >= SCREENSAVER_TIMEOUT_MS) {
            screensaver_pending = true;
        }
    } else if ((current_time - last_activity) >= SCREENSAVER_TIMEOUT_MS) {
        // Set flag for main loop to start screensaver
        // (can't do display operations from interrupt context)
        screensaver_pending = true;
    }
    
    return TIMER_CHECK_INTERVAL_MS * 1000;  // Repeat every 1 second (return microseconds)
}

bool display_handler_init(void* i2c_inst)
{
    display_i2c = i2c_inst;
    
    // Initialize OLED Display
    if (oled_init(i2c_inst)) {
        debug_info("Display Handler: OLED Display initialized");
        display_initialized = true;
        
        // Clear display and show startup message
        oled_clear();
        oled_draw_border();
        display_handler_writeline(5, 20, "Zoft Synthesizer V1");
        oled_display();
        is_home_screen = true;
        
        // Initialize activity tracking - set to current time so screensaver
        // will start 30 seconds after boot if no activity
        extern void button_init_activity_time(void);
        extern void midi_handler_init_activity_time(void);
        button_init_activity_time();
        midi_handler_init_activity_time();
        
        // Start the timeout check timer
        timeout_alarm = add_alarm_in_ms(TIMER_CHECK_INTERVAL_MS, timeout_check_callback, NULL, true);
        debug_info("Display Handler: Timeout timer started (alarm_id=%d)", timeout_alarm);
        
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
    
    screensaver_active = false;
    // Display the MIDI note information
    oled_display_single_note(note, velocity, channel);
    is_home_screen = false;
}

void display_handler_clear(void)
{
    if (!display_initialized) {
        return;
    }
    
    oled_clear();
    oled_draw_border();
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

void display_handler_show_home(void)
{
    if (!display_initialized) {
        return;
    }
    
    screensaver_active = false;
    oled_clear();
    oled_draw_border();
    oled_draw_string(5, 20, "Zoft Synthesizer V1");
    oled_display();
    is_home_screen = true;
}

bool display_handler_is_home(void)
{
    return is_home_screen;
}

void display_handler_screensaver_start(void)
{
    if (!display_initialized) {
        return;
    }
    
    screensaver_active = true;
    is_home_screen = false;
    lissajous_screensaver_init();
}

void display_handler_screensaver_stop(void)
{
    if (!display_initialized || !screensaver_active) {
        return;
    }
    
    screensaver_active = false;
    screensaver_pending = false;
    display_handler_show_home();
    debug_info("Display: Screensaver stopped");
}

void display_handler_screensaver_update(void)
{
    if (!display_initialized || !screensaver_active) {
        return;
    }
    
    lissajous_screensaver_update();
}

bool display_handler_is_screensaver_active(void)
{
    return screensaver_active;
}

void display_handler_check_timeout(void)
{
    if (screensaver_pending && !screensaver_active) {
        screensaver_pending = false;
        screensaver_active = true;
        is_home_screen = false;
        lissajous_screensaver_init();
        debug_info("Display: Screensaver started by timer");
    }
}
