#include "usb_midi.h"
#include "tusb.h"
#include <string.h>

//--------------------------------------------------------------------+
// USB MIDI Module - Internal State
//--------------------------------------------------------------------+

// Track USB mount state
static bool usb_mounted = false;

// MIDI receive callback and user data
static usb_midi_rx_callback_t rx_callback = NULL;
static void* rx_callback_user_data = NULL;

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    usb_mounted = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    usb_mounted = false;
}

// Invoked when usb bus is suspended
// remote_wakeup_en: if host allows us to perform remote wakeup
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    // Could add power saving logic here
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    // Could add resume logic here
}

//--------------------------------------------------------------------+
// USB MIDI Public API Implementation
//--------------------------------------------------------------------+

bool usb_midi_init(void)
{
    // Initialize TinyUSB
    if (!tusb_init()) {
        return false;
    }
    
    usb_mounted = false;
    rx_callback = NULL;
    rx_callback_user_data = NULL;
    
    return true;
}

void usb_midi_set_rx_callback(usb_midi_rx_callback_t callback, void* user_data)
{
    rx_callback = callback;
    rx_callback_user_data = user_data;
}

bool usb_midi_is_mounted(void)
{
    return usb_mounted;
}

void usb_midi_task(void)
{
    // Handle USB tasks - this must be called regularly
    tud_task();
    
    // Only process MIDI when USB is mounted
    if (!usb_mounted || !rx_callback) {
        return;
    }
    
    // Process incoming MIDI messages
    uint8_t packet[4];
    while (tud_midi_available())
    {
        // Read 4 bytes (USB-MIDI Event Packet)
        if (tud_midi_packet_read(packet))
        {
            // Extract MIDI message bytes
            // packet[0] = Cable Number + Code Index Number (CIN)
            // packet[1] = Status/data byte 1
            // packet[2] = Data byte 2
            // packet[3] = Data byte 3
            
            uint8_t cin = packet[0] & 0x0F;
            
            // Handle SysEx messages (CIN 0x04 = SysEx start/continue, 0x05-0x07 = SysEx end variants)
            if (cin >= 0x04 && cin <= 0x07) {
                // Determine number of valid bytes based on CIN
                uint8_t num_bytes;
                if (cin == 0x04) {
                    num_bytes = 3; // SysEx start or continue (3 bytes)
                } else if (cin == 0x05) {
                    num_bytes = 1; // SysEx end with 1 byte
                } else if (cin == 0x06) {
                    num_bytes = 2; // SysEx end with 2 bytes
                } else { // cin == 0x07
                    num_bytes = 3; // SysEx end with 3 bytes
                }
                
                // Pass valid bytes (including 0x00 which is a valid data byte)
                for (int i = 1; i <= num_bytes; i++) {
                    rx_callback(packet[i], 0, 0, rx_callback_user_data);
                }
            } else {
                // Standard MIDI message
                uint8_t status = packet[1];
                uint8_t data1 = packet[2];
                uint8_t data2 = packet[3];
                
                // Call user callback
                rx_callback(status, data1, data2, rx_callback_user_data);
            }
        }
    }
}

int usb_midi_send_note(uint8_t channel, uint8_t note, uint8_t velocity, bool note_on)
{
    if (!usb_mounted) {
        return 0;
    }
    
    uint8_t msg[3];
    msg[0] = (note_on ? 0x90 : 0x80) | (channel & 0x0F); // Note On/Off + channel
    msg[1] = note & 0x7F;     // Note number (0-127)
    msg[2] = velocity & 0x7F; // Velocity (0-127)
    
    return tud_midi_stream_write(0, msg, 3);
}

int usb_midi_send_cc(uint8_t channel, uint8_t controller, uint8_t value)
{
    if (!usb_mounted) {
        return 0;
    }
    
    uint8_t msg[3];
    msg[0] = 0xB0 | (channel & 0x0F); // Control Change + channel
    msg[1] = controller & 0x7F;       // Controller number (0-127)
    msg[2] = value & 0x7F;            // Controller value (0-127)
    
    return tud_midi_stream_write(0, msg, 3);
}

int usb_midi_send_program_change(uint8_t channel, uint8_t program)
{
    if (!usb_mounted) {
        return 0;
    }
    
    uint8_t msg[2];
    msg[0] = 0xC0 | (channel & 0x0F); // Program Change + channel
    msg[1] = program & 0x7F;          // Program number (0-127)
    
    return tud_midi_stream_write(0, msg, 2);
}

int usb_midi_send_pitch_bend(uint8_t channel, uint16_t value)
{
    if (!usb_mounted) {
        return 0;
    }
    
    // Pitch bend is a 14-bit value split into two 7-bit bytes
    uint8_t msg[3];
    msg[0] = 0xE0 | (channel & 0x0F); // Pitch Bend + channel
    msg[1] = value & 0x7F;            // LSB (least significant 7 bits)
    msg[2] = (value >> 7) & 0x7F;     // MSB (most significant 7 bits)
    
    return tud_midi_stream_write(0, msg, 3);
}

int usb_midi_send_raw(const uint8_t* data, uint8_t length)
{
    if (!usb_mounted || !data || length == 0 || length > 3) {
        return 0;
    }
    
    return tud_midi_stream_write(0, data, length);
}
