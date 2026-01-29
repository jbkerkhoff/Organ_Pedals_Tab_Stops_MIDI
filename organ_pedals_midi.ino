#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Teensy: usbMIDI object is provided automatically when USB Type includes MIDI.
// No #include <USB-MIDI.h> required or allowed.

// ============================================================
// ----------------------- Configuration ----------------------
// ============================================================

// Total number of physical inputs (3 MCP23017 × 16 pins)
static const uint8_t N_KEYS = 48;

// Number of MCP23017 expanders on the I2C bus
static const uint8_t N_MCP  = 3;

// Debounce parameters
// A state change must be seen STABLE_COUNT times consecutively
// With SCAN_DELAY_MS=1, STABLE_COUNT=4 ≈ 4 ms debounce
static const uint8_t STABLE_COUNT  = 4;
static const uint8_t SCAN_DELAY_MS = 1;

// MIDI parameters
static const uint8_t MIDI_CHANNEL = 1;    // 1..16
static const uint8_t BASE_NOTE    = 36;   // C2 = 36 (used for temporary Note testing)

// I2C addresses of the MCP23017 expanders
static const uint8_t MCP_ADDR[N_MCP] = {0x20, 0x21, 0x22};

// ============================================================
// ------------------------ Globals ----------------------------
// ============================================================

// One Adafruit MCP object per expander
Adafruit_MCP23X17 mcp[N_MCP];

// Create a display object
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // -1 = no reset pin

// OLED state
uint8_t  activeKeys = 0;
int8_t   lastKey    = -1;
bool     lastOn     = false;
bool     oledDirty  = true;
uint32_t lastOLEDUpdate = 0;

// Per-key state tracking
uint8_t  stableState[N_KEYS];     // Last accepted stable state: 1=released, 0=pressed
uint8_t  debounceCount[N_KEYS];   // Counts consecutive differing samples
uint8_t  lastReported[N_KEYS];    // Last state we sent MIDI for

// Cached GPIO values for each MCP (read once per scan)
uint16_t gpioCache[N_MCP];

// Per-key lockout timer (prevents double-triggering after an edge)
uint32_t lockoutUntil[N_KEYS];

// Convert key index → MIDI note number
inline uint8_t noteOf(uint8_t i) { return BASE_NOTE + i; }

// ============================================================
// -------------------- Power-up Sequences --------------------
// ============================================================

void powerUpSelfTest() {
  Serial.println();
  Serial.println("=== Power-up self-test ===");

  // Confirm each MCP is present (begin_I2C already did the real check),
  // but this gives you a clean "all good" line every boot.
  for (uint8_t c = 0; c < N_MCP; ++c) {
    Serial.print("MCP 0x");
    Serial.print(MCP_ADDR[c], HEX);
    Serial.println(" OK");
  }

  // Take one snapshot of GPIO so you can spot "everything reads high" quickly
  readAllGPIO();
  for (uint8_t c = 0; c < N_MCP; ++c) {
    Serial.print("GPIO 0x");
    Serial.print(MCP_ADDR[c], HEX);
    Serial.print(" = 0x");
    Serial.println(gpioCache[c], HEX);
  }

  Serial.println("=========================");
  Serial.println();
}

void midiPanicOnBoot() {
  // “All Notes Off” + related safety resets across all channels
  for (uint8_t ch = 1; ch <= 16; ch++) {
    usbMIDI.sendControlChange(64, 0, ch);   // sustain off
    usbMIDI.sendControlChange(121, 0, ch);  // reset controllers
    usbMIDI.sendControlChange(123, 0, ch);  // all notes off
  }
  usbMIDI.send_now();
  Serial.println("MIDI panic sent (All Notes Off, etc).");
}

// ============================================================
// -------------------- MCP Input Handling --------------------
// ============================================================

// Read all MCP GPIO ports in one shot per scan
// This dramatically reduces I2C traffic and improves reliability
void readAllGPIO() {
  for (uint8_t c = 0; c < N_MCP; ++c) {
    // bit 0..7  = GPA0..GPA7
    // bit 8..15 = GPB0..GPB7
    gpioCache[c] = mcp[c].readGPIOAB();
  }
}

// Read a single logical key from the cached GPIO values
// Returns: 0 = pressed (pulled to GND), 1 = released (pull-up)
uint8_t readKey(uint8_t i) {
  uint8_t chip = i / 16;   // Which MCP (0..2)
  uint8_t bit  = i % 16;   // Which pin within MCP (0..15)
  return (gpioCache[chip] >> bit) & 0x01;
}

// ============================================================
// -------------------- MIDI Output ----------------------------
// ============================================================

void sendNoteOn(uint8_t note, uint8_t key) {
  usbMIDI.sendNoteOn(note, 127, MIDI_CHANNEL);
  Serial.print("Key ");
  Serial.print(key);
  Serial.println(" ON");
}

void sendNoteOff(uint8_t note, uint8_t key) {
  usbMIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  Serial.print("Key ");
  Serial.print(key);
  Serial.println(" OFF");
}

void debugBadBits() {
  static uint32_t lastPrint = 0;
  uint32_t now = millis();
  if (now - lastPrint < 200) return;   // print 5x/sec
  lastPrint = now;

  uint16_t g = gpioCache[1];   // MCP at 0x21

  auto bit01 = [&](uint8_t b) -> uint8_t {
    return (g >> b) & 0x01;
  };

  Serial.print("0x21 GPIOAB=0x");
  Serial.print(g, HEX);
  Serial.print("  A5=");
  Serial.print(bit01(5));
  Serial.print(" A7=");
  Serial.print(bit01(7));
  Serial.print(" B0=");
  Serial.println(bit01(8));
}

void updateOLED() {
  uint32_t now = millis();

  // Limit redraw rate (OLED does not need fast updates)
  if (!oledDirty && (now - lastOLEDUpdate < 200)) return;
  lastOLEDUpdate = now;
  oledDirty = false;

  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("Organ Encoder");

  display.print("Active: ");
  display.println(activeKeys);

  display.print("Last: ");
  if (lastKey >= 0) {
    display.print(lastKey);
    display.print(lastOn ? " ON" : " OFF");
  } else {
    display.println("--");
  }

  display.display();
}

// ============================================================
// ------------------------- Setup -----------------------------
// ============================================================

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) { }

  // ---- DROP HERE (optional): MIDI safety first ----
  // This is safe even before the MCPs are initialized.
  midiPanicOnBoot();

  // Initialize I2C bus (Teensy 4.0 defaults are fine)
  Wire.begin();
  // Wire.setClock(100000);   // Uncomment if bus stability is marginal

// ---------- OLED init ----------
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  Serial.println("SSD1306 init failed");
  while (1) delay(10);
}

display.clearDisplay();
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
display.println("Organ Encoder");
display.println("Boot OK");
display.println();
display.println("MCPs: 0x20 21 22");
display.display();

  // Initialize each MCP23017
  for (uint8_t c = 0; c < N_MCP; ++c) {
    Serial.print("Initializing MCP at 0x");
    Serial.println(MCP_ADDR[c], HEX);

    // Fail hard if any expander is missing
    if (!mcp[c].begin_I2C(MCP_ADDR[c])) {
      Serial.print("ERROR: MCP init failed at 0x");
      Serial.println(MCP_ADDR[c], HEX);
      while (1) { delay(500); }
    }

    // Configure all 16 pins as inputs with pull-ups
    // Switches pull the pin to GND when pressed
    for (uint8_t p = 0; p < 16; ++p) {
      mcp[c].pinMode(p, INPUT_PULLUP);
    }
  }

  // Initialize per-key state tracking
  for (uint8_t i = 0; i < N_KEYS; ++i) {
    stableState[i]   = 1;   // assume released
    lastReported[i]  = 1;
    debounceCount[i] = 0;
    lockoutUntil[i]  = 0;
  }

  // Prime the GPIO cache so first scan is valid
  readAllGPIO();

  // ---- DROP HERE: power-up test that prints once and then stops ----
  powerUpSelfTest();
}

// ============================================================
// -------------------------- Loop -----------------------------
// ============================================================

void loop() {

  // Read all MCP GPIOs once per scan
  readAllGPIO();

  // debugBadBits(); //turns on expander debugging

  // Scan each logical key
  for (uint8_t i = 0; i < N_KEYS; ++i) {

    // ---- Per-key lockout guard ----
    // Prevents chatter or double edges after a valid change
    uint32_t now = millis();
    if (now < lockoutUntil[i]) {
      continue;   // skip this key until lockout expires
    }

    // Sample current state (0 pressed, 1 released)
    uint8_t sample = readKey(i);

    // If sample matches the current stable state,
    // reset debounce counter
    if (sample == stableState[i]) {
      debounceCount[i] = 0;
    }
    // Otherwise, potential state change
    else {
      if (++debounceCount[i] >= STABLE_COUNT) {

        // Accept the new stable state
        stableState[i] = sample;
        debounceCount[i] = 0;

        // Start a short lockout to suppress bounce artifacts
        lockoutUntil[i] = now + 30;  // 30 ms

        // Only emit MIDI if this differs from last reported state
        if (stableState[i] != lastReported[i]) {
  uint8_t note = noteOf(i);

  if (stableState[i] == 0) {
    sendNoteOn(note, i);
    activeKeys++;
    lastOn = true;
  } else {
    sendNoteOff(note, i);
    if (activeKeys > 0) activeKeys--;
    lastOn = false;
  }

  lastKey = i;
  oledDirty = true;

  lastReported[i] = stableState[i];
}
      }
    }
  }

  // Drain incoming USB MIDI (keeps USB stack happy)
  while (usbMIDI.read()) { }

  // Control scan rate
  delay(SCAN_DELAY_MS);

  updateOLED();
}