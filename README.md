Organ Pedals / Tab Stops MIDI Encoder

A 48-input USB MIDI encoder designed for organ pedals or stop tabs using a Teensy 4.0 and three MCP23017 I²C GPIO expanders.

This project was developed as part of the Virtual Pipe Organ (VPO) build documented at RoyCreekRanch.com and JimKerkhoff.com. It provides a flexible and reliable way to convert simple switch closures into MIDI Control Change messages for use with Hauptwerk and other MIDI-enabled software.

⸻

Current Release

Stable Release: v1.0
Release page:
https://github.com/jbkerkhoff/Organ_Pedals_Tab_Stops_MIDI/releases/tag/v1.0

Status:
	•	Firmware complete and stable
	•	Tested with MIDI Monitor and GarageBand
	•	Integrated into VPO project hardware

⸻

Features
	•	48 digital inputs (3 × MCP23017 expanders)
	•	USB MIDI output via Teensy 4.0
	•	Continuous Controller (CC) output
	•	Value 127 on activation
	•	Value 0 on release
	•	Robust software debounce
	•	Per-input lockout protection
	•	SSD1306 OLED status display
	•	Active control count
	•	Last event indication
	•	Power-up MIDI safety (reset controllers, all notes off)

Designed primarily for:
	•	Organ tab stops
	•	Organ pedal sets
	•	Other multi-switch control applications

⸻

Hardware Overview

Core components:
	•	Teensy 4.0 (USB MIDI)
	•	3 × MCP23017-E/SP I²C GPIO expanders
	•	SSD1306 128×64 OLED (I²C)
	•	3.3V logic throughout
	•	JST-XH 2.54mm connectors for switch inputs

Each input expects a simple SPST switch closure to ground. Internal pull-ups are used to simplify wiring.

The production PCB was designed in KiCad and manufactured with support from PCBWay, including SMT assembly of surface-mount components.

⸻

Firmware

Firmware is written in C++ using the Arduino IDE and the Teensy USB MIDI stack.

Each switch input is mapped to a dedicated MIDI Continuous Controller number:

CC = BASE_CC + input index

Default mapping:
	•	BASE_CC = 20
	•	CC range = 20–67 (48 controls)

Mapping can easily be modified in the source code.

⸻

Documentation & Project Background

This encoder is part of a larger Virtual Pipe Organ project.

Project overview and build documentation:

Roy Creek Ranch (project narrative):
[https://roycreekranch.com](https://roycreekranch.com/2026/02/05/building-a-great-virtual-pipe-organ-midi-encoders-for-pedals-and-stops/

Technical design details:
[https://jimkerkhoff.com](https://jimkerkhoff.com/2026/02/04/designing-a-multi-input-midi-encoder-for-multiple-switch-closures/)

The JBK post provides a deeper technical look at:
	•	Design goals
	•	Prototyping process
	•	KiCad PCB layout
	•	Firmware development
	•	USB and MIDI implementation
	•	Manufacturing and assembly considerations

⸻

Applications Beyond MIDI

Although designed for a VPO system, this hardware and firmware architecture can be adapted for:
	•	USB HID control devices
	•	Custom automation controllers
	•	Station control systems (amateur radio)
	•	Serial or USB command generators

The core concept — detecting multiple grounded inputs via I²C expanders and converting them into structured USB output — is broadly reusable.

⸻

## License

This project is released under the MIT License.  
See the LICENSE file in this repository for details.

⸻

Acknowledgements

PCB fabrication and partial assembly support for the production boards was provided by PCBWay. Their assistance helped move this project from prototype to finished hardware.
