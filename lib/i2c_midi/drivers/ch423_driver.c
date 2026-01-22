#include "ch423_driver.h"
#include "../../src/debug_uart.h"

bool ch423_init(ch423_t *ctx, i2c_inst_t *i2c_port, uint8_t address) {
    if (!ctx || !i2c_port) {
        debug_error("CH423: Init failed - invalid parameters");
        return false;
    }
    
    ctx->i2c_port = i2c_port;
    ctx->address = address;
    ctx->pin_state = 0x0000;
    ctx->io_direction = 0x0000;  // All pins as outputs by default
    
    debug_info("CH423: Initialized at address 0x%02X", address);
    
    // Try to write initial state (non-critical)
    if (!ch423_write(ctx, 0x0000)) {
        debug_error("CH423: Warning - device not responding (may not be connected)");
    } else {
        debug_info("CH423: Device detected and responding");
    }
    
    return true;
}

bool ch423_write(ch423_t *ctx, uint16_t data) {
    if (!ctx || !ctx->i2c_port) {
        return false;
    }
    
    // CH423 expects 3 bytes:
    // Byte 0: Command (CH423_CMD_WRITE_OC for OC pins, CH423_CMD_WRITE_PP for PP pins)
    // Byte 1: Low byte (OC0-OC7)
    // Byte 2: High byte (PP0-PP7)
    
    uint8_t buffer[3];
    
    // Write to open-collector outputs (OC0-OC7) - low byte
    buffer[0] = CH423_CMD_WRITE_OC;
    buffer[1] = (uint8_t)(data & 0xFF);  // Low byte
    
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 2, false);
    if (result != 2) {
        debug_error("CH423: Write OC failed (result=%d, addr=0x%02X)", result, ctx->address);
        return false;
    }
    
    // Write to push-pull outputs (PP0-PP7) - high byte
    buffer[0] = CH423_CMD_WRITE_PP;
    buffer[1] = (uint8_t)(data >> 8);  // High byte
    
    result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 2, false);
    if (result != 2) {
        debug_error("CH423: Write PP failed (result=%d, addr=0x%02X)", result, ctx->address);
        return false;
    }
    
    ctx->pin_state = data;
    debug_printf("CH423: Write success: 0x%04X\n", data);
    return true;
}

bool ch423_read(ch423_t *ctx, uint16_t *data) {
    if (!ctx || !ctx->i2c_port || !data) {
        return false;
    }
    
    uint8_t buffer[3];
    buffer[0] = CH423_CMD_READ_IO;
    
    // Write command
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 1, true);
    if (result != 1) {
        debug_error("CH423: Read command failed (result=%d)", result);
        return false;
    }
    
    // Read 2 bytes back
    result = i2c_read_blocking(ctx->i2c_port, ctx->address, buffer, 2, false);
    if (result != 2) {
        debug_error("CH423: Read data failed (result=%d)", result);
        return false;
    }
    
    *data = ((uint16_t)buffer[1] << 8) | buffer[0];
    ctx->pin_state = *data;
    debug_printf("CH423: Read success: 0x%04X\n", *data);
    return true;
}

bool ch423_set_pin(ch423_t *ctx, uint8_t pin, bool state) {
    if (!ctx || pin > 15) {
        debug_error("CH423: Invalid pin number %d (must be 0-15)", pin);
        return false;
    }
    
    uint16_t new_state = ctx->pin_state;
    
    if (state) {
        new_state |= (1 << pin);  // Set bit
    } else {
        new_state &= ~(1 << pin); // Clear bit
    }
    
    return ch423_write(ctx, new_state);
}

uint16_t ch423_get_pin_state(ch423_t *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->pin_state;
}

bool ch423_reset(ch423_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    debug_info("CH423: Resetting all pins to LOW");
    return ch423_write(ctx, 0x0000);
}

bool ch423_set_io_direction(ch423_t *ctx, uint8_t pin, bool is_input) {
    if (!ctx || pin > 15) {
        return false;
    }
    
    uint16_t new_direction = ctx->io_direction;
    
    if (is_input) {
        new_direction |= (1 << pin);  // Set bit for input
    } else {
        new_direction &= ~(1 << pin); // Clear bit for output
    }
    
    // Send direction configuration to CH423
    uint8_t buffer[3];
    buffer[0] = CH423_CMD_SET_IO;
    buffer[1] = (uint8_t)(new_direction & 0xFF);      // Low byte
    buffer[2] = (uint8_t)(new_direction >> 8);        // High byte
    
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, 3, false);
    if (result != 3) {
        debug_error("CH423: Set IO direction failed (result=%d)", result);
        return false;
    }
    
    ctx->io_direction = new_direction;
    debug_printf("CH423: Set IO direction: 0x%04X\n", new_direction);
    return true;
}
