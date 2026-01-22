# Quick Reference - Configuration Settings

## Boot Behavior
```
Power On → Load EEPROM (0x50:0x0000) → Validate CRC → Apply
         ↓ (if invalid)
         Load Defaults → Save to EEPROM → Apply
```

## SysEx Format
```
F0 7D 00 <CMD> [DATA...] F7
```

## Quick Command Reference

### Persistent Commands (Save to EEPROM)
| CMD | Function | Data | Range |
|-----|----------|------|-------|
| 20 | MIDI Channel | ch | 1-16 |
| 21 | Note Range | range | 1-16 |
| 22 | Low Note | note | 0-127 |
| 23 | Semitone Mode | mode | 0-2 |
| 30 | IO Type+Addr | type addr | 0-1, 0x00-0x7F |
| 40 | Display Enable | en | 0-1 |
| F0 | Reset Defaults | - | - |
| F2 | Query Stored | - | - |

### Runtime Commands (No Persistence)
| CMD | Function | Data | Range |
|-----|----------|------|-------|
| 01 | Note Range | range | 1-16 |
| 02 | MIDI Channel | ch | 1-16 |
| 03 | Semitone Mode | mode | 0-2 |
| 10 | Query Runtime | - | - |

## Defaults
```
Channel:  10
Range:    8 notes
Low Note: 60 (C4)
Semitone: 0 (Play)
IO Type:  0 (PCF857x)
IO Addr:  0x20
Display:  Enabled
```

## Semitone Modes
```
0 = Play normally
1 = Ignore (skip semitones)
2 = Skip (play next whole tone)
```

## Storage Details
```
EEPROM:   AT24C32 @ 0x50
Location: 0x0000
Size:     32 bytes
CRC:      CRC16/XMODEM
Magic:    0x4D53594E
Version:  1
```

## Common Tasks

### Set Channel 1
```
F0 7D 00 20 01 F7
```

### Set 16-Note Range
```
F0 7D 00 21 10 F7
```

### Start at Low C (C3/48)
```
F0 7D 00 22 30 F7
```

### Ignore Semitones
```
F0 7D 00 23 01 F7
```

### Use CH423 at 0x24
```
F0 7D 00 30 01 24 F7
```

### Reset Everything
```
F0 7D 00 F0 F7
```

## Python Example
```python
import mido
p = mido.open_output('Synth')
p.send(mido.Message('sysex', data=[0x7D,0x00,0x20,0x0A]))  # Ch 10
p.send(mido.Message('sysex', data=[0x7D,0x00,0xF2]))      # Query
p.close()
```

## Debugging
- UART0 (115200 baud): Debug messages
- LED (GPIO25): MIDI activity
- Display: Configuration confirmations

## Files
- `src/configuration_settings.h/c` - Core module
- `src/midi_handler.c` - SysEx integration
- `SYSEX_COMMANDS.md` - Full documentation
- `CONFIGURATION_INTEGRATION.md` - Integration details
