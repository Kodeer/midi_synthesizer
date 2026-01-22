#include "at24cxx_driver.h"
#include "../../../src/debug_uart.h"
#include "pico/stdlib.h"
#include <string.h>

// Write cycle time (typical 5ms for AT24CXX)
#define AT24CXX_WRITE_DELAY_MS 5

/**
 * Determine page size and address mode based on capacity
 */
static void configure_eeprom_params(at24cxx_t *ctx, uint16_t capacity_kb) {
    ctx->capacity_bytes = capacity_kb * 1024;
    
    // Determine if 2-byte addressing is needed (>2KB)
    ctx->two_byte_address = (capacity_kb > 2);
    
    // Determine page size based on capacity
    if (capacity_kb <= 2) {
        ctx->page_size = AT24CXX_PAGE_SIZE_8;
    } else if (capacity_kb <= 16) {
        ctx->page_size = AT24CXX_PAGE_SIZE_16;
    } else if (capacity_kb == 32) {
        ctx->page_size = AT24CXX_PAGE_SIZE_32;
    } else {
        ctx->page_size = AT24CXX_PAGE_SIZE_64;
    }
    
    debug_info("AT24CXX: Configured for %dKB (page_size=%d, addr_bytes=%d)", 
               capacity_kb, ctx->page_size, ctx->two_byte_address ? 2 : 1);
}

bool at24cxx_init(at24cxx_t *ctx, i2c_inst_t *i2c_port, uint8_t address, uint16_t capacity_kb) {
    if (!ctx || !i2c_port) {
        debug_error("AT24CXX: Init failed - invalid parameters");
        return false;
    }
    
    // Validate capacity
    if (capacity_kb != 1 && capacity_kb != 2 && capacity_kb != 4 && 
        capacity_kb != 8 && capacity_kb != 16 && capacity_kb != 32 && 
        capacity_kb != 64 && capacity_kb != 128 && capacity_kb != 256 && 
        capacity_kb != 512) {
        debug_error("AT24CXX: Invalid capacity %dKB", capacity_kb);
        return false;
    }
    
    ctx->i2c_port = i2c_port;
    ctx->address = address;
    
    configure_eeprom_params(ctx, capacity_kb);
    
    debug_info("AT24CXX: Initialized at address 0x%02X, capacity %dKB", address, capacity_kb);
    
    // Try to read first byte to verify device is present
    uint8_t test_byte;
    if (!at24cxx_read_byte(ctx, 0, &test_byte)) {
        debug_error("AT24CXX: Warning - device not responding (may not be connected)");
    } else {
        debug_info("AT24CXX: Device detected and responding");
    }
    
    return true;
}

bool at24cxx_write_byte(at24cxx_t *ctx, uint32_t mem_address, uint8_t data) {
    if (!ctx || !ctx->i2c_port) {
        return false;
    }
    
    if (mem_address >= ctx->capacity_bytes) {
        debug_error("AT24CXX: Write address 0x%04X out of range", mem_address);
        return false;
    }
    
    uint8_t buffer[3];
    int buffer_len;
    
    if (ctx->two_byte_address) {
        // 2-byte address mode
        buffer[0] = (mem_address >> 8) & 0xFF;  // High byte
        buffer[1] = mem_address & 0xFF;          // Low byte
        buffer[2] = data;
        buffer_len = 3;
    } else {
        // 1-byte address mode
        buffer[0] = mem_address & 0xFF;
        buffer[1] = data;
        buffer_len = 2;
    }
    
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, buffer_len, false);
    
    if (result != buffer_len) {
        debug_error("AT24CXX: Write failed at address 0x%04X (result=%d)", mem_address, result);
        return false;
    }
    
    // Wait for write cycle to complete
    sleep_ms(AT24CXX_WRITE_DELAY_MS);
    
    return true;
}

bool at24cxx_read_byte(at24cxx_t *ctx, uint32_t mem_address, uint8_t *data) {
    if (!ctx || !ctx->i2c_port || !data) {
        return false;
    }
    
    if (mem_address >= ctx->capacity_bytes) {
        debug_error("AT24CXX: Read address 0x%04X out of range", mem_address);
        return false;
    }
    
    uint8_t addr_buffer[2];
    int addr_len;
    
    if (ctx->two_byte_address) {
        // 2-byte address mode
        addr_buffer[0] = (mem_address >> 8) & 0xFF;
        addr_buffer[1] = mem_address & 0xFF;
        addr_len = 2;
    } else {
        // 1-byte address mode
        addr_buffer[0] = mem_address & 0xFF;
        addr_len = 1;
    }
    
    // Write address
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, addr_buffer, addr_len, true);
    if (result != addr_len) {
        debug_error("AT24CXX: Failed to set read address 0x%04X", mem_address);
        return false;
    }
    
    // Read data
    result = i2c_read_blocking(ctx->i2c_port, ctx->address, data, 1, false);
    if (result != 1) {
        debug_error("AT24CXX: Read failed at address 0x%04X", mem_address);
        return false;
    }
    
    return true;
}

bool at24cxx_write(at24cxx_t *ctx, uint32_t mem_address, const uint8_t *data, uint32_t length) {
    if (!ctx || !ctx->i2c_port || !data || length == 0) {
        return false;
    }
    
    if (mem_address + length > ctx->capacity_bytes) {
        debug_error("AT24CXX: Write would exceed capacity");
        return false;
    }
    
    uint32_t bytes_written = 0;
    
    while (bytes_written < length) {
        // Calculate how many bytes we can write in this page
        uint32_t current_address = mem_address + bytes_written;
        uint32_t page_offset = current_address % ctx->page_size;
        uint32_t bytes_to_page_end = ctx->page_size - page_offset;
        uint32_t bytes_remaining = length - bytes_written;
        uint32_t chunk_size = (bytes_to_page_end < bytes_remaining) ? bytes_to_page_end : bytes_remaining;
        
        // Prepare write buffer with address + data
        uint8_t buffer[66]; // Max: 2 byte address + 64 byte page
        int buffer_idx = 0;
        
        if (ctx->two_byte_address) {
            buffer[buffer_idx++] = (current_address >> 8) & 0xFF;
            buffer[buffer_idx++] = current_address & 0xFF;
        } else {
            buffer[buffer_idx++] = current_address & 0xFF;
        }
        
        // Copy data to buffer
        memcpy(&buffer[buffer_idx], &data[bytes_written], chunk_size);
        buffer_idx += chunk_size;
        
        // Write page
        int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, buffer_idx, false);
        
        if (result != buffer_idx) {
            debug_error("AT24CXX: Page write failed at address 0x%04X", current_address);
            return false;
        }
        
        // Wait for write cycle
        sleep_ms(AT24CXX_WRITE_DELAY_MS);
        
        bytes_written += chunk_size;
    }
    
    debug_printf("AT24CXX: Wrote %d bytes starting at 0x%04X\n", length, mem_address);
    return true;
}

bool at24cxx_read(at24cxx_t *ctx, uint32_t mem_address, uint8_t *data, uint32_t length) {
    if (!ctx || !ctx->i2c_port || !data || length == 0) {
        return false;
    }
    
    if (mem_address + length > ctx->capacity_bytes) {
        debug_error("AT24CXX: Read would exceed capacity");
        return false;
    }
    
    uint8_t addr_buffer[2];
    int addr_len;
    
    if (ctx->two_byte_address) {
        addr_buffer[0] = (mem_address >> 8) & 0xFF;
        addr_buffer[1] = mem_address & 0xFF;
        addr_len = 2;
    } else {
        addr_buffer[0] = mem_address & 0xFF;
        addr_len = 1;
    }
    
    // Write starting address
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, addr_buffer, addr_len, true);
    if (result != addr_len) {
        debug_error("AT24CXX: Failed to set read address 0x%04X", mem_address);
        return false;
    }
    
    // Sequential read
    result = i2c_read_blocking(ctx->i2c_port, ctx->address, data, length, false);
    if (result != (int)length) {
        debug_error("AT24CXX: Sequential read failed (expected %d, got %d)", length, result);
        return false;
    }
    
    debug_printf("AT24CXX: Read %d bytes from 0x%04X\n", length, mem_address);
    return true;
}

bool at24cxx_erase(at24cxx_t *ctx) {
    if (!ctx || !ctx->i2c_port) {
        return false;
    }
    
    debug_info("AT24CXX: Erasing EEPROM (%d bytes)...", ctx->capacity_bytes);
    
    uint8_t erase_buffer[64];
    memset(erase_buffer, 0xFF, sizeof(erase_buffer));
    
    uint32_t address = 0;
    while (address < ctx->capacity_bytes) {
        uint32_t chunk_size = ctx->page_size;
        if (address + chunk_size > ctx->capacity_bytes) {
            chunk_size = ctx->capacity_bytes - address;
        }
        
        if (!at24cxx_write(ctx, address, erase_buffer, chunk_size)) {
            debug_error("AT24CXX: Erase failed at address 0x%04X", address);
            return false;
        }
        
        address += chunk_size;
    }
    
    debug_info("AT24CXX: Erase complete");
    return true;
}

uint32_t at24cxx_get_capacity(at24cxx_t *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->capacity_bytes;
}
