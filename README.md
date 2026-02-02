# Organ Stop / Pedal Encoder (48-Input, MIDI CC)

This project implements a 48-input organ stop / pedal encoder using a Teensy microcontroller and three MCP23017 I²C GPIO expanders. Each physical switch generates a MIDI Continuous Controller (CC) message suitable for use with Hauptwerk and other virtual pipe organ (VPO) software.

Features
	•	48 digital inputs (3 × MCP23017 expanders)
	•	Robust per-input debounce and lockout
	•	MIDI Control Change output (one CC per switch)
	•	CC value 127 = ON, 0 = OFF
	•	OLED status display (SSD1306, 128×64, I²C)
	•	Power-up MIDI “panic” (all notes / controllers off)
	•	Designed for latching organ tabs or pedal switches

Hardware
	•	Teensy (USB MIDI enabled)
	•	3 × MCP23017 I²C GPIO expanders
Addresses: 0x20, 0x21, 0x22
	•	0.96” OLED display (SSD1306, 128×64, I²C)
	•	Passive pull-ups provided by MCP23017 (switches pull to GND)

MIDI Behavior
	•	CC numbers: 20–67 (48 total, configurable)
	•	One CC per physical switch
	•	Messages are sent only on state change
	•	Compatible with Hauptwerk MIDI “Learn” functionality

OLED Display
	•	Displays project name on boot
	•	Shows number of active stops
	•	Shows most recent stop ON/OFF event
	•	Updates are rate-limited to avoid I²C overload

Power-Up Safety

On boot, the firmware sends:
	•	Sustain OFF
	•	Reset All Controllers
	•	All Notes OFF
	•	All CCs used by the encoder set to 0

This guarantees a clean starting state in the virtual organ.

Status

✅ Stable
✅ Fully tested with GarageBand and MIDI Monitor
✅ Ready for integration with Hauptwerk

## Acknowledgements

This project was supported in part by PCBWay, who provided encouragement and resources during development.

All design decisions, implementation, and testing were performed independently.