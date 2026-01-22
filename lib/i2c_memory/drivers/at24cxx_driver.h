#ifndef AT24CXX_DRIVER_H
#define AT24CXX_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// AT24CXX default I2C address (0x50-0x57 depending on A0-A2 pins)
#define AT24CXX_DEFAULT_ADDRESS 0x50

// AT24CXX page sizes (bytes) for different capacities
#define AT24CXX_PAGE_SIZE_8     8    // 1KB, 2KB
#define AT24CXX_PAGE_SIZE_16    16   // 4KB, 8KB, 16KB
#define AT24CXX_PAGE_SIZE_32    32   // 32KB
#define AT24CXX_PAGE_SIZE_64    64   // 64KB, 128KB, 256KB, 512KB

/**
 * AT24CXX EEPROM driver context structure
 */
typedef struct {
    i2c_inst_t *i2c_port;
    uint8_t address;
    uint32_t capacity_bytes;    // Total capacity in bytes
    uint8_t page_size;          // Page size for page writes
    bool two_byte_address;      // true for >2KB EEPROMs
} at24cxx_t;

/**
 * Initialize AT24CXX EEPROM driver
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @param i2c_port I2C port to use (i2c0 or i2c1)
 * @param address I2C address of AT24CXX (typically 0x50-0x57)
 * @param capacity_kb Capacity in kilobytes (1, 2, 4, 8, 16, 32, 64, 128, 256, 512)
 * @return true if initialization successful, false otherwise
 */
bool at24cxx_init(at24cxx_t *ctx, i2c_inst_t *i2c_port, uint8_t address, uint16_t capacity_kb);

/**
 * Write a single byte to EEPROM
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @param mem_address Memory address to write to (0 to capacity-1)
 * @param data Data byte to write
 * @return true if successful, false otherwise
 */
bool at24cxx_write_byte(at24cxx_t *ctx, uint32_t mem_address, uint8_t data);

/**
 * Read a single byte from EEPROM
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @param mem_address Memory address to read from (0 to capacity-1)
 * @param data Pointer to store read data
 * @return true if successful, false otherwise
 */
bool at24cxx_read_byte(at24cxx_t *ctx, uint32_t mem_address, uint8_t *data);

/**
 * Write multiple bytes to EEPROM (handles page boundaries automatically)
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @param mem_address Starting memory address
 * @param data Pointer to data buffer to write
 * @param length Number of bytes to write
 * @return true if successful, false otherwise
 */
bool at24cxx_write(at24cxx_t *ctx, uint32_t mem_address, const uint8_t *data, uint32_t length);

/**
 * Read multiple bytes from EEPROM
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @param mem_address Starting memory address
 * @param data Pointer to buffer to store read data
 * @param length Number of bytes to read
 * @return true if successful, false otherwise
 */
bool at24cxx_read(at24cxx_t *ctx, uint32_t mem_address, uint8_t *data, uint32_t length);

/**
 * Erase EEPROM by writing 0xFF to all locations
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @return true if successful, false otherwise
 */
bool at24cxx_erase(at24cxx_t *ctx);

/**
 * Get the total capacity in bytes
 * 
 * @param ctx Pointer to AT24CXX context structure
 * @return Capacity in bytes
 */
uint32_t at24cxx_get_capacity(at24cxx_t *ctx);

#endif // AT24CXX_DRIVER_H
