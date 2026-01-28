# organ_pedals_midi
A Teensy-based MIDI encoder for 32 organ pedal switches

Uses MCP23X17 expanders to detect switch closures on any of 32 organ pedals and generates organ pitches from CC (Great C) to "g" above middle "c". MIDI data is returned to the host computer via the Teensy programming port, and requires no other external power. 
