#include <Wire.h>
#include <Adafruit_MCP23X17.h>


// Teensy: usbMIDI object is provided automatically when USB Type includes MIDI.
// No #include <USB-MIDI.h> required or allowed.

// ---------- Config ----------
static const uint8_t N_KEYS = 32;
static const uint8_t STABLE_COUNT = 4;    // ~4ms if SCAN_DELAY_MS=1
static const uint8_t SCAN_DELAY_MS = 1;   // scan loop delay (ms)
static const uint8_t MIDI_CHANNEL = 1;    // 1..16
static const uint8_t BASE_NOTE = 36;      // C2 = 36

// I2C addresses for the two MCP23017s
static const uint8_t MCP_ADDR[2] = {0x20, 0x21};

// Organ-style names starting at CC
const char* organNames[32] = {
  "CC","C#C","DD","D#D","EE","FF","F#F","GG","G#G","AA","A#A","BB",  // first octave
  "C","C#","D","D#","E","F","F#","G","G#","A","A#","B",              // second octave
  "c","c#","d","d#","e","f","f#","g"                                // into third octave
};

// ---------- Globals ----------
Adafruit_MCP23X17 mcp[2];

uint8_t stableState[N_KEYS];     // 1=released (pullup), 0=pressed
uint8_t debounceCount[N_KEYS];
uint8_t lastReported[N_KEYS];    // last state we emitted MIDI for

inline uint8_t noteOf(uint8_t i) { return BASE_NOTE + i; }

// Read key i (0..31): returns 0 pressed, 1 released
uint8_t readKey(uint8_t i) {
  uint8_t chip = (i < 16) ? 0 : 1;
  uint8_t pin  = i % 16;  // 0..15 across GPA0..7 (=0..7) and GPB0..7 (=8..15)
  return mcp[chip].digitalRead(pin);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) { }

  // Teensy 4.0 I2C: SDA=18, SCL=19, 3.3V logic
  Wire.begin();

  // Init the expanders
  for (int c = 0; c < 2; ++c) {
    if (!mcp[c].begin_I2C(MCP_ADDR[c])) {
      // If you want, blink LED or print to Serial here for error
      // but with USB Type=MIDI only, Serial may not be available.
    }
    for (int p = 0; p < 16; ++p) {
      mcp[c].pinMode(p, INPUT_PULLUP);   // switch to GND = pressed
    }
  }

  for (int i = 0; i < N_KEYS; ++i) {
    stableState[i]   = 1;  // assume released
    lastReported[i]  = 1;
    debounceCount[i] = 0;
  }
}

void sendNoteOn(uint8_t note, uint8_t pedal) {
  usbMIDI.sendNoteOn(note, 127, MIDI_CHANNEL);
  Serial.print("Pedal ");
  Serial.print(pedal);
  Serial.print(" ON  ->  ");
  Serial.println(organNames[pedal]);
}

void sendNoteOff(uint8_t note, uint8_t pedal) {
  usbMIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  Serial.print("Pedal ");
  Serial.print(pedal);
  Serial.print(" OFF ->  ");
  Serial.println(organNames[pedal]);
}
void loop() {
  for (uint8_t i = 0; i < N_KEYS; ++i) {
    uint8_t sample = readKey(i);  // 0 pressed, 1 released

    if (sample == stableState[i]) {
      debounceCount[i] = 0;       // no change
    } else {
      if (++debounceCount[i] >= STABLE_COUNT) {
        stableState[i] = sample;  // accept change
        debounceCount[i] = 0;

        if (stableState[i] != lastReported[i]) {
          uint8_t note = noteOf(i);
          if (stableState[i] == 0) sendNoteOn(note, i);   // pressed
          else                     sendNoteOff(note, i);  // released
          lastReported[i] = stableState[i];
        }
      }
    }
  }

  // Drain incoming usbMIDI (keeps stack happy even if we ignore it)
  while (usbMIDI.read()) { /* ignore */ }

  delay(SCAN_DELAY_MS);
}