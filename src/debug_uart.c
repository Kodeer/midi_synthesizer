#include "debug_uart.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// Debug UART Module - Internal State
//--------------------------------------------------------------------+

static uart_inst_t* debug_uart_instance = NULL;
static bool debug_enabled = false;

// Internal buffer for formatting
#define DEBUG_BUFFER_SIZE 256
static char debug_buffer[DEBUG_BUFFER_SIZE];

//--------------------------------------------------------------------+
// MIDI Message Type Names
//--------------------------------------------------------------------+

static const char* get_midi_message_type_name(uint8_t status)
{
    uint8_t msg_type = status & 0xF0;
    
    switch (msg_type) {
        case 0x80: return "Note Off";
        case 0x90: return "Note On";
        case 0xA0: return "Polyphonic Aftertouch";
        case 0xB0: return "Control Change";
        case 0xC0: return "Program Change";
        case 0xD0: return "Channel Aftertouch";
        case 0xE0: return "Pitch Bend";
        case 0xF0: return "System";
        default: return "Unknown";
    }
}

//--------------------------------------------------------------------+
// Public API Implementation
//--------------------------------------------------------------------+

bool debug_uart_init(void* uart_inst, uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate)
{
    if (!uart_inst) {
        return false;
    }
    
    debug_uart_instance = (uart_inst_t*)uart_inst;
    
    // Initialize UART
    uart_init(debug_uart_instance, baud_rate);
    
    // Set GPIO functions for UART
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    
    // Enable debug by default
    debug_enabled = true;
    
    return true;
}

void debug_uart_set_enabled(bool enabled)
{
    debug_enabled = enabled;
}

bool debug_uart_is_enabled(void)
{
    return debug_enabled;
}

void debug_print(const char* str)
{
    if (!debug_uart_instance || !debug_enabled || !str) {
        return;
    }
    
    uart_puts(debug_uart_instance, str);
}

void debug_printf(const char* format, ...)
{
    if (!debug_uart_instance || !debug_enabled || !format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    vsnprintf(debug_buffer, DEBUG_BUFFER_SIZE, format, args);
    va_end(args);
    
    uart_puts(debug_uart_instance, debug_buffer);
}

void debug_print_midi(uint8_t status, uint8_t data1, uint8_t data2)
{
    if (!debug_uart_instance || !debug_enabled) {
        return;
    }
    
    uint8_t channel = (status & 0x0F) + 1; // MIDI channels are 1-16
    const char* msg_type = get_midi_message_type_name(status);
    
    snprintf(debug_buffer, DEBUG_BUFFER_SIZE,
             "MIDI: %s | Ch:%d | Status:0x%02X | Data1:%d | Data2:%d\n",
             msg_type, channel, status, data1, data2);
    
    uart_puts(debug_uart_instance, debug_buffer);
}

void debug_print_hex(const uint8_t* data, size_t length, const char* label)
{
    if (!debug_uart_instance || !debug_enabled || !data || length == 0) {
        return;
    }
    
    // Print label if provided
    if (label) {
        uart_puts(debug_uart_instance, label);
        uart_puts(debug_uart_instance, ": ");
    }
    
    // Print hex bytes
    for (size_t i = 0; i < length; i++) {
        snprintf(debug_buffer, DEBUG_BUFFER_SIZE, "%02X ", data[i]);
        uart_puts(debug_uart_instance, debug_buffer);
    }
    
    uart_puts(debug_uart_instance, "\n");
}

void debug_error(const char* format, ...)
{
    if (!debug_uart_instance || !format) {
        return;
    }
    
    // Errors always print, regardless of debug_enabled
    uart_puts(debug_uart_instance, "[ERROR] ");
    
    va_list args;
    va_start(args, format);
    vsnprintf(debug_buffer, DEBUG_BUFFER_SIZE, format, args);
    va_end(args);
    
    uart_puts(debug_uart_instance, debug_buffer);
    
    // Ensure newline
    if (debug_buffer[strlen(debug_buffer) - 1] != '\n') {
        uart_puts(debug_uart_instance, "\n");
    }
}

void debug_warn(const char* format, ...)
{
    if (!debug_uart_instance || !debug_enabled || !format) {
        return;
    }
    
    uart_puts(debug_uart_instance, "[WARN] ");
    
    va_list args;
    va_start(args, format);
    vsnprintf(debug_buffer, DEBUG_BUFFER_SIZE, format, args);
    va_end(args);
    
    uart_puts(debug_uart_instance, debug_buffer);
    
    // Ensure newline
    if (debug_buffer[strlen(debug_buffer) - 1] != '\n') {
        uart_puts(debug_uart_instance, "\n");
    }
}

void debug_info(const char* format, ...)
{
    if (!debug_uart_instance || !debug_enabled || !format) {
        return;
    }
    
    uart_puts(debug_uart_instance, "[INFO] ");
    
    va_list args;
    va_start(args, format);
    vsnprintf(debug_buffer, DEBUG_BUFFER_SIZE, format, args);
    va_end(args);
    
    uart_puts(debug_uart_instance, debug_buffer);
    
    // Ensure newline
    if (debug_buffer[strlen(debug_buffer) - 1] != '\n') {
        uart_puts(debug_uart_instance, "\n");
    }
}
