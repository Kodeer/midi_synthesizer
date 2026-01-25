#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "usb_midi.h"
#include "midi_handler.h"
#include "debug_uart.h"
#include "display_handler.h"
#include "button_handler.h"
#include "menu_handler.h"
#include "buzzer.h"

//--------------------------------------------------------------------+
// Hardware Configuration
//--------------------------------------------------------------------+

// Debug Configuration
#define DEBUG_ENABLED       true    // Set to false to disable all debug output

// Debug UART Configuration
#define DEBUG_UART          uart0
#define DEBUG_UART_TX_PIN   0
#define DEBUG_UART_RX_PIN   1
#define DEBUG_UART_BAUD     115200

// I2C MIDI Configuration
#define I2C_MIDI_INSTANCE   i2c1
#define I2C_MIDI_SDA_PIN    2
#define I2C_MIDI_SCL_PIN    3
#define I2C_MIDI_FREQ       400000

// MIDI Semitone Handling
#define SEMITONE_MODE       I2C_MIDI_SEMITONE_SKIP  // Options: I2C_MIDI_SEMITONE_PLAY, I2C_MIDI_SEMITONE_IGNORE, I2C_MIDI_SEMITONE_SKIP

// OLED Display Configuration
#define OLED_I2C_INSTANCE   I2C_MIDI_INSTANCE  // Share I2C bus with MIDI

// LED Feedback Configuration
#define LED_PIN             25

// Button Configuration
#define BUTTON_PIN          4
#define BUTTON_ACTIVE_LOW   true    // Button pulls to ground when pressed

// Buzzer Configuration
#define BUZZER_PIN          15      // PWM buzzer on GPIO 15

// Mallet MIDI Configuration (Servo-controlled xylophone striker)
#define MALLET_SERVO_PIN    16      // Servo PWM on GPIO 16
#define MALLET_STRIKER_PIN  17      // Striker GPIO on GPIO 17

//--------------------------------------------------------------------+
// Button Event Handler
//--------------------------------------------------------------------+
void handle_button_event(button_event_t event) {
    switch (event) {
        case BUTTON_EVENT_SHORT_PRESS:
            buzzer_click();  // Click sound on button press
            if (menu_is_active()) {
                // Cycle to next menu option
                menu_next();
            }
            break;
            
        case BUTTON_EVENT_LONG_PRESS:
            if (menu_is_active()) {
                // Execute current menu option
                menu_execute();
            } else {
                // Enter menu mode
                menu_enter();
            }
            break;
            
        case BUTTON_EVENT_RELEASED:
            // Button released - no action needed
            break;
            
        default:
            break;
    }
}

//--------------------------------------------------------------------+
// Main Entry Point
//--------------------------------------------------------------------+
int main()
{
    // Initialize debug UART
    debug_uart_init(DEBUG_UART, DEBUG_UART_TX_PIN, DEBUG_UART_RX_PIN, DEBUG_UART_BAUD);
    
    // Set debug enable/disable based on global setting
    debug_uart_set_enabled(DEBUG_ENABLED);
    
    debug_info("MIDI Synthesizer Starting...");
    
    // Initialize Buzzer
    if (!buzzer_init(BUZZER_PIN)) {
        debug_error("Failed to initialize Buzzer");
    } else {
        debug_info("Buzzer initialized on GPIO %d", BUZZER_PIN);
    }
    
    // Initialize MIDI handler with I2C MIDI and LED feedback
    if (!midi_handler_init(I2C_MIDI_INSTANCE, I2C_MIDI_SDA_PIN, I2C_MIDI_SCL_PIN, 
                          I2C_MIDI_FREQ, LED_PIN, SEMITONE_MODE)) {
        debug_error("Failed to initialize MIDI handler");
        buzzer_error();  // Play error sound
        // Blink LED rapidly to indicate error
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        while (true) {
            gpio_put(LED_PIN, 1);
            sleep_ms(100);
            gpio_put(LED_PIN, 0);
            sleep_ms(100);
        }
    }
    
    // Initialize Display Handler
    if (!display_handler_init(OLED_I2C_INSTANCE)) {
        debug_error("Failed to initialize Display Handler");
    }
    
    // Initialize Button Handler
    if (!button_init(BUTTON_PIN, BUTTON_ACTIVE_LOW)) {
        debug_error("Failed to initialize Button Handler");
    } else {
        button_set_callback(handle_button_event);
        debug_info("Button handler initialized on GPIO %d", BUTTON_PIN);
    }
    
    // Initialize Menu Handler
    if (!menu_init()) {
        debug_error("Failed to initialize Menu Handler");
    }

    // Configure MIDI handler (optional - uses defaults if not called)
    // midi_handler_set_channel(9);  // Channel 10 (0-indexed)
    // midi_handler_set_note_range(60, 67);  // Middle C to G
    
    // Initialize USB MIDI subsystem
    if (!usb_midi_init()) {
        debug_error("Failed to initialize USB MIDI");
        // Blink LED slowly to indicate error
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        while (true) {
            gpio_put(LED_PIN, 1);
            sleep_ms(500);
            gpio_put(LED_PIN, 0);
            sleep_ms(500);
        }
    }
    debug_info("USB MIDI initialized");
    
    // Register MIDI handler callback with USB MIDI
    usb_midi_set_rx_callback((usb_midi_rx_callback_t)midi_handler_get_callback(), NULL);
    
    // Play boot-up melody to indicate successful initialization
    buzzer_boot_melody();
    
    //debug_info("MIDI Synthesizer Ready!");
    debug_info("Waiting for USB connection...");
   
    // Main loop
    while (true) {
        // Update button state
        button_update();
        
        // Process USB and MIDI tasks
        usb_midi_task();
        
        // Update MIDI handler (for mallet striker timing)
        midi_handler_update();
        
        // Check if timer has triggered screensaver timeout
        display_handler_check_timeout();
        
        // Update screensaver if active
        if (display_handler_is_screensaver_active()) {
            display_handler_screensaver_update();
        }
        
        // Add a small delay to prevent tight loop
        sleep_us(100);
    }
}
