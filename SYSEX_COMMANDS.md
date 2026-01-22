# MIDI SysEx Commands

This document describes the System Exclusive (SysEx) MIDI commands supported by the MIDI Synthesizer.

## Message Format

All SysEx messages follow this format:
```
F0 7D <device_id> <command> [data...] F7
```

- `F0`: SysEx start byte
- `7D`: Manufacturer ID (Educational/Development use)
- `<device_id>`: Device ID (default: `00`)
- `<command>`: Command byte (see below)
- `[data...]`: Optional data bytes (command-specific)
- `F7`: SysEx end byte

## Legacy Runtime Commands (No EEPROM Persistence)

These commands update runtime settings but **do not save to EEPROM**.

### 0x01 - Set Note Range (Runtime)
Updates the number of notes to handle (1-16).

**Message:** `F0 7D 00 01 <range> F7`
- `<range>`: Note range (1-16)

**Example:** `F0 7D 00 01 08 F7` (Set range to 8 notes)

### 0x02 - Set MIDI Channel (Runtime)
Updates the MIDI channel to listen to.

**Message:** `F0 7D 00 02 <channel> F7`
- `<channel>`: MIDI channel (1-16)

**Example:** `F0 7D 00 02 0A F7` (Set to channel 10)

### 0x03 - Set Semitone Mode (Runtime)
Updates semitone handling behavior.

**Message:** `F0 7D 00 03 <mode> F7`
- `<mode>`: 
  - `00` = Play semitones normally
  - `01` = Ignore semitones
  - `02` = Skip semitones (play next whole tone)

**Example:** `F0 7D 00 03 02 F7` (Skip semitones)

### 0x10 - Query Configuration
Returns current runtime configuration (displays on debug UART).

**Message:** `F0 7D 00 10 F7`

## Configuration Commands (With EEPROM Persistence)

These commands **update EEPROM** immediately and persist across reboots.

### 0x20 - Set MIDI Channel (Persistent)
Saves MIDI channel to EEPROM.

**Message:** `F0 7D 00 20 <channel> F7`
- `<channel>`: MIDI channel (1-16)

**Example:** `F0 7D 00 20 0A F7` (Save channel 10 to EEPROM)

### 0x21 - Set Note Range (Persistent)
Saves note range to EEPROM.

**Message:** `F0 7D 00 21 <range> F7`
- `<range>`: Note range (1-16)

**Example:** `F0 7D 00 21 08 F7` (Save 8-note range to EEPROM)

### 0x22 - Set Low Note (Persistent)
Saves the lowest MIDI note to respond to.

**Message:** `F0 7D 00 22 <note> F7`
- `<note>`: MIDI note number (0-127)

**Example:** `F0 7D 00 22 3C F7` (Save low note C4/60 to EEPROM)

### 0x23 - Set Semitone Mode (Persistent)
Saves semitone handling mode to EEPROM.

**Message:** `F0 7D 00 23 <mode> F7`
- `<mode>`: 
  - `00` = Play semitones normally
  - `01` = Ignore semitones
  - `02` = Skip semitones

**Example:** `F0 7D 00 23 01 F7` (Save "Ignore" mode to EEPROM)

### 0x30 - Set IO Expander Type & Address (Persistent)
Saves IO expander configuration to EEPROM.

**Message:** `F0 7D 00 30 <type> <address> F7`
- `<type>`: 
  - `00` = PCF857x (PCF8574/PCF8575)
  - `01` = CH423
- `<address>`: I2C address (7-bit format, 0x00-0x7F)

**Example:** `F0 7D 00 30 00 20 F7` (PCF857x at 0x20)

### 0x40 - Set Display Enable (Persistent)
Enables/disables the display.

**Message:** `F0 7D 00 40 <enabled> F7`
- `<enabled>`: 
  - `00` = Disabled
  - `01` = Enabled

**Example:** `F0 7D 00 40 01 F7` (Enable display)

### 0xF0 - Reset to Factory Defaults
Erases EEPROM configuration and loads defaults. Settings are automatically saved after reset.

**Message:** `F0 7D 00 F0 F7`

**Default Settings:**
- MIDI Channel: 10 (percussion)
- Note Range: 8 notes
- Low Note: 60 (Middle C)
- Semitone Mode: Play
- IO Expander: PCF857x at 0x20
- Display: Enabled

### 0xF2 - Query Stored Configuration
Displays configuration stored in EEPROM (via debug UART).

**Message:** `F0 7D 00 F2 F7`

## Configuration Storage

All configuration commands (0x20-0x40, 0xF0, 0xF2) interact with persistent storage:

- **Storage Medium:** AT24CXX EEPROM (I2C address 0x50)
- **Location:** Address 0x0000
- **Size:** 32 bytes
- **Validation:** CRC16 checksum
- **Magic Number:** 0x4D53594E ('MSYN')
- **Version:** 1

### Boot Behavior

On startup:
1. Load configuration from EEPROM
2. Validate magic number and CRC
3. If valid: apply settings
4. If invalid: load defaults and save to EEPROM

## Examples

### Example 1: Configure MIDI Settings
```
F0 7D 00 20 01 F7  # Set MIDI channel to 1 (save to EEPROM)
F0 7D 00 21 10 F7  # Set note range to 16 (save to EEPROM)
F0 7D 00 22 30 F7  # Set low note to 48/C3 (save to EEPROM)
F0 7D 00 23 02 F7  # Skip semitones (save to EEPROM)
```

### Example 2: Change IO Expander
```
F0 7D 00 30 01 24 F7  # Use CH423 at I2C address 0x24
```

### Example 3: Reset Everything
```
F0 7D 00 F0 F7  # Reset to factory defaults
F0 7D 00 F2 F7  # Query to verify defaults loaded
```

## Error Handling

- Invalid values are ignored
- Configuration errors are logged to debug UART
- Failed EEPROM writes do not affect runtime settings
- CRC validation failures trigger default configuration load

## Testing with Python

```python
import mido

# Open MIDI output
port = mido.open_output('Your MIDI Port')

# Set MIDI channel to 10 (persistent)
msg = mido.Message('sysex', data=[0x7D, 0x00, 0x20, 0x0A])
port.send(msg)

# Set note range to 8 (persistent)
msg = mido.Message('sysex', data=[0x7D, 0x00, 0x21, 0x08])
port.send(msg)

# Query stored configuration
msg = mido.Message('sysex', data=[0x7D, 0x00, 0xF2])
port.send(msg)

port.close()
```

## Notes

- Configuration changes take effect immediately
- EEPROM writes occur on every command (wear leveling not implemented)
- Multiple rapid changes may cause EEPROM wear (AT24CXX rated for ~1M writes)
- Recommended: batch configuration changes and minimize writes
- Runtime commands (0x01-0x10) are faster but don't persist
