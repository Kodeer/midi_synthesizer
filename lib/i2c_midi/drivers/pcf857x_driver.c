#include "pcf857x_driver.h"
#include "../../../src/debug_uart.h"

bool pcf857x_init(pcf857x_t *ctx, i2c_inst_t *i2c_port, uint8_t address, pcf857x_chip_type_t chip_type) {
    if (!ctx || !i2c_port) {
        debug_error("PCF857x: Init failed - invalid parameters");
        return false;
    }
    
    ctx->i2c_port = i2c_port;
    ctx->address = address;
    ctx->pin_state = 0x0000;
    ctx->chip_type = chip_type;
    ctx->num_pins = (chip_type == PCF8574_CHIP) ? 8 : 16;
    
    const char* chip_name = (chip_type == PCF8574_CHIP) ? "PCF8574" : "PCF8575";
    debug_info("%s: Initialized at address 0x%02X (%d pins)", chip_name, address, ctx->num_pins);
    
    // Try to write initial state (non-critical)
    if (!pcf857x_write(ctx, 0x0000)) {
        debug_error("%s: Warning - device not responding (may not be connected)", chip_name);
    } else {
        debug_info("%s: Device detected and responding", chip_name);
    }
    
    return true;
}

bool pcf857x_write(pcf857x_t *ctx, uint16_t data) {
    if (!ctx || !ctx->i2c_port) {
        return false;
    }
    
    const char* chip_name = (ctx->chip_type == PCF8574_CHIP) ? "PCF8574" : "PCF8575";
    uint8_t buffer[2];
    int bytes_to_write;
    
    if (ctx->chip_type == PCF8574_CHIP) {
        // PCF8574: 8-bit write
        buffer[0] = data & 0xFF;
        bytes_to_write = 1;
    } else {
        // PCF8575: 16-bit write (low byte, high byte)
        buffer[0] = data & 0xFF;         // P0-P7 (low byte)
        buffer[1] = (data >> 8) & 0xFF;  // P10-P17 (high byte)
        bytes_to_write = 2;
    }
    
    int result = i2c_write_blocking(ctx->i2c_port, ctx->address, buffer, bytes_to_write, false);
    if (result == bytes_to_write) {
        ctx->pin_state = data;
        debug_printf("%s: Write success: 0x%04X\n", chip_name, data);
        return true;
    } else {
        debug_error("%s: Write failed (result=%d, addr=0x%02X, data=0x%04X)", 
                   chip_name, result, ctx->address, data);
        return false;
    }
}

bool pcf857x_read(pcf857x_t *ctx, uint16_t *data) {
    if (!ctx || !ctx->i2c_port || !data) {
        return false;
    }
    
    const char* chip_name = (ctx->chip_type == PCF8574_CHIP) ? "PCF8574" : "PCF8575";
    uint8_t buffer[2];
    int bytes_to_read;
    
    if (ctx->chip_type == PCF8574_CHIP) {
        bytes_to_read = 1;
    } else {
        bytes_to_read = 2;
    }
    
    int result = i2c_read_blocking(ctx->i2c_port, ctx->address, buffer, bytes_to_read, false);
    if (result == bytes_to_read) {
        if (ctx->chip_type == PCF8574_CHIP) {
            *data = buffer[0];
        } else {
            *data = buffer[0] | ((uint16_t)buffer[1] << 8);
        }
        ctx->pin_state = *data;
        debug_printf("%s: Read success: 0x%04X\n", chip_name, *data);
        return true;
    } else {
        debug_error("%s: Read failed (result=%d, addr=0x%02X)", chip_name, result, ctx->address);
        return false;
    }
}

bool pcf857x_set_pin(pcf857x_t *ctx, uint8_t pin, bool state) {
    if (!ctx || pin >= ctx->num_pins) {
        return false;
    }
    
    uint16_t new_state = ctx->pin_state;
    
    if (state) {
        new_state |= (1 << pin);  // Set bit
    } else {
        new_state &= ~(1 << pin); // Clear bit
    }
    
    return pcf857x_write(ctx, new_state);
}

uint16_t pcf857x_get_pin_state(pcf857x_t *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->pin_state;
}

bool pcf857x_reset(pcf857x_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    const char* chip_name = (ctx->chip_type == PCF8574_CHIP) ? "PCF8574" : "PCF8575";
    debug_info("%s: Resetting all pins to LOW", chip_name);
    return pcf857x_write(ctx, 0x0000);
}

uint8_t pcf857x_get_num_pins(pcf857x_t *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->num_pins;
}
