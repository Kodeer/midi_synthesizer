#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

/**
 * @brief Initialize UART for debug output
 * 
 * Configures UART with specified pins and baud rate for debug output.
 * 
 * @param uart_inst UART instance (uart0 or uart1)
 * @param tx_pin GPIO pin for UART TX
 * @param rx_pin GPIO pin for UART RX
 * @param baud_rate Baud rate (e.g., 115200)
 * @return true if initialization successful, false otherwise
 */
bool debug_uart_init(void* uart_inst, uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate);

/**
 * @brief Enable or disable debug output
 * 
 * @param enabled true to enable debug output, false to disable
 */
void debug_uart_set_enabled(bool enabled);

/**
 * @brief Check if debug output is enabled
 * 
 * @return true if debug output is enabled, false otherwise
 */
bool debug_uart_is_enabled(void);

/**
 * @brief Print a string to debug UART
 * 
 * @param str String to print
 */
void debug_print(const char* str);

/**
 * @brief Print a formatted string to debug UART
 * 
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void debug_printf(const char* format, ...);

/**
 * @brief Print MIDI message details to debug UART
 * 
 * Formats and prints MIDI message information if debug is enabled.
 * 
 * @param status MIDI status byte
 * @param data1 First data byte
 * @param data2 Second data byte
 */
void debug_print_midi(uint8_t status, uint8_t data1, uint8_t data2);

/**
 * @brief Print a hex dump to debug UART
 * 
 * @param data Pointer to data buffer
 * @param length Number of bytes to print
 * @param label Optional label to print before hex dump
 */
void debug_print_hex(const uint8_t* data, size_t length, const char* label);

/**
 * @brief Print an error message to debug UART
 * 
 * Always prints regardless of debug_enabled status.
 * 
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void debug_error(const char* format, ...);

/**
 * @brief Print an info message to debug UART
 * 
 * Only prints if debug is enabled.
 * 
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void debug_info(const char* format, ...);

#endif // DEBUG_UART_H
