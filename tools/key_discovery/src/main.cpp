// =============================================================================
// T32 Key Discovery Firmware
//
// Single-purpose tool: empirically map matrix key index (0..31, top-left →
// bottom-right) to electrical mux channel (mux*16 + ch). Lights one LED red,
// waits for the user to press the corresponding physical key, locks in the
// channel that responded, advances. After all 32 keys, prints a copy-paste-
// ready k3dot0Keys[32] block to Serial and blinks all 32 LEDs slow green
// forever to indicate completion.
//
// MODE handling:
//   - Held at boot for kModeHoldBootMs: print message + ESP.restart().
//   - Pressed during a key-wait: re-arm the current index (debounced ~30 ms).
//
// This firmware does NOT touch MIDI, BLE, USB, the filesystem, or any
// configuration. It only drives the mux select pins, reads the two mux
// commons, and runs the LED strip.
// =============================================================================

#include <Arduino.h>
#include <FastLED.h>

#include "pinout_t32.h"

namespace {

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

constexpr uint8_t  kNumKeys        = 32;
constexpr uint8_t  kNumLeds        = 32;     // matrix only
constexpr uint8_t  kNumMuxChannels = 16;
constexpr uint8_t  kNumMuxes       = 2;
constexpr uint8_t  kNumChannels    = kNumMuxes * kNumMuxChannels;  // 32

constexpr uint16_t kPressDelta     = 800;    // ADC delta over baseline = press
constexpr uint32_t kBaselineMs     = 500;
constexpr uint32_t kPressHoldMs    = 50;     // sustain threshold this long
constexpr uint32_t kReleaseMs      = 30;     // sustain release this long
constexpr uint16_t kReleaseDelta   = 200;    // within this delta = released
constexpr uint16_t kSelectSettleUs = 8;
constexpr uint32_t kModeHoldBootMs = 50;     // boot-time MODE debounce
constexpr uint32_t kModeDebounceMs = 30;     // in-loop MODE debounce

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

CRGB     leds[kNumLeds];
uint16_t baseline[kNumChannels]   = {0};
uint8_t  discovered[kNumKeys]     = {0};

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void selectChannel(uint8_t c) {
    digitalWrite(pinout::t32::S0, (c >> 0) & 0x1);
    digitalWrite(pinout::t32::S1, (c >> 1) & 0x1);
    digitalWrite(pinout::t32::S2, (c >> 2) & 0x1);
    digitalWrite(pinout::t32::S3, (c >> 3) & 0x1);
    delayMicroseconds(kSelectSettleUs);
}

uint16_t readMuxRaw(uint8_t mux) {
    const uint8_t pin = (mux == 0) ? pinout::t32::COM : pinout::t32::COM2;
    return static_cast<uint16_t>(analogRead(pin));
}

uint16_t readMux(uint8_t mux, uint8_t c) {
    selectChannel(c);
    return readMuxRaw(mux);
}

// Scan all 32 channels into out[]. Cycles select lines once for both muxes.
void scanAll(uint16_t out[kNumChannels]) {
    for (uint8_t c = 0; c < kNumMuxChannels; ++c) {
        selectChannel(c);
        out[0 * kNumMuxChannels + c] = readMuxRaw(0);
        out[1 * kNumMuxChannels + c] = readMuxRaw(1);
    }
}

bool modePressed() {
    return digitalRead(pinout::t32::MODE) == LOW;
}

bool modePressedDebounced() {
    if (!modePressed()) return false;
    const uint32_t start = millis();
    while (millis() - start < kModeDebounceMs) {
        if (!modePressed()) return false;
        delay(2);
    }
    return modePressed();
}

void setAllLeds(CRGB c) {
    for (uint8_t i = 0; i < kNumLeds; ++i) leds[i] = c;
}

void showOnlyLed(uint8_t i, CRGB c) {
    setAllLeds(CRGB::Black);
    if (i < kNumLeds) leds[i] = c;
    FastLED.show();
}

// -----------------------------------------------------------------------------
// Baseline
// -----------------------------------------------------------------------------

void captureBaseline() {
    Serial.println(F("Capturing baseline (do not touch keys)..."));

    uint32_t sum[kNumChannels]   = {0};
    uint32_t count               = 0;
    uint16_t scratch[kNumChannels];

    const uint32_t start = millis();
    while (millis() - start < kBaselineMs) {
        scanAll(scratch);
        for (uint8_t k = 0; k < kNumChannels; ++k) sum[k] += scratch[k];
        ++count;
    }

    if (count == 0) count = 1;  // defensive
    uint16_t bMin = 0xFFFF;
    uint16_t bMax = 0;
    for (uint8_t k = 0; k < kNumChannels; ++k) {
        baseline[k] = static_cast<uint16_t>(sum[k] / count);
        if (baseline[k] < bMin) bMin = baseline[k];
        if (baseline[k] > bMax) bMax = baseline[k];
    }

    Serial.print(F("Baseline captured. samples="));
    Serial.print(count);
    Serial.print(F(" min="));
    Serial.print(bMin);
    Serial.print(F(" max="));
    Serial.println(bMax);
}

// -----------------------------------------------------------------------------
// Discovery state machine (per-key)
// -----------------------------------------------------------------------------

// Wait for a single channel to sustain a press for kPressHoldMs and return its
// electrical index. Honors MODE (debounced) as a re-arm signal: if MODE is
// pressed, this function returns 0xFF and the caller should retry.
uint8_t waitForPress(uint8_t keyIdx) {
    uint16_t scratch[kNumChannels];

    int16_t  candidate         = -1;
    uint32_t candidateStart    = 0;

    while (true) {
        if (modePressedDebounced()) {
            Serial.print(F("Re-arming key "));
            Serial.println(keyIdx);
            // Wait for MODE release before returning so we don't immediately
            // re-trigger on the next call.
            while (modePressed()) delay(5);
            return 0xFF;
        }

        scanAll(scratch);

        // Find the channel with the largest delta over baseline that exceeds
        // kPressDelta. Picking the max delta makes neighbour-crosstalk
        // cases unambiguous.
        int16_t  bestCh    = -1;
        int32_t  bestDelta = 0;
        for (uint8_t k = 0; k < kNumChannels; ++k) {
            const int32_t delta =
                static_cast<int32_t>(scratch[k]) -
                static_cast<int32_t>(baseline[k]);
            if (delta > kPressDelta && delta > bestDelta) {
                bestDelta = delta;
                bestCh    = k;
            }
        }

        if (bestCh < 0) {
            // No channel exceeds threshold: clear any pending candidate.
            candidate = -1;
            continue;
        }

        if (bestCh != candidate) {
            // New (or first) candidate: start hold timer.
            candidate      = bestCh;
            candidateStart = millis();
            continue;
        }

        if (millis() - candidateStart >= kPressHoldMs) {
            return static_cast<uint8_t>(candidate);
        }
    }
}

// Wait for `electrical` channel to return to within kReleaseDelta of baseline
// for kReleaseMs continuously. Prevents the next-key wait from being polluted
// by a still-held key.
void waitForRelease(uint8_t electrical) {
    uint32_t releasedSince = 0;
    while (true) {
        const uint16_t v     = readMux(electrical / kNumMuxChannels,
                                       electrical % kNumMuxChannels);
        const int32_t  delta =
            static_cast<int32_t>(v) - static_cast<int32_t>(baseline[electrical]);
        const bool released = (delta < kReleaseDelta);
        if (released) {
            if (releasedSince == 0) releasedSince = millis();
            if (millis() - releasedSince >= kReleaseMs) return;
        } else {
            releasedSince = 0;
        }
    }
}

void runDiscovery() {
    for (uint8_t i = 0; i < kNumKeys; ++i) {
        uint8_t electrical = 0xFF;
        while (electrical == 0xFF) {
            showOnlyLed(i, CRGB::Red);
            Serial.print(F("Waiting for key "));
            Serial.println(i);
            electrical = waitForPress(i);
        }

        discovered[i] = electrical;
        showOnlyLed(i, CRGB::Green);

        const uint8_t mux = electrical / kNumMuxChannels;
        const uint8_t ch  = electrical % kNumMuxChannels;
        Serial.print(F("key "));
        Serial.print(i);
        Serial.print(F(" -> mux="));
        Serial.print(mux);
        Serial.print(F(" ch="));
        Serial.print(ch);
        Serial.print(F(" (electrical="));
        Serial.print(electrical);
        Serial.println(F(")"));

        waitForRelease(electrical);
    }
}

// -----------------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------------

void emitArray() {
    Serial.println();
    Serial.println(F("inline constexpr uint8_t k3dot0Keys[32] = {"));
    for (uint8_t row = 0; row < 4; ++row) {
        Serial.print(F("    "));
        for (uint8_t col = 0; col < 8; ++col) {
            const uint8_t idx = row * 8 + col;
            Serial.print(discovered[idx]);
            Serial.print(F(","));
            if (col != 7) Serial.print(F(" "));
        }
        Serial.println();
    }
    Serial.println(F("};"));
    Serial.println();
}

}  // namespace

// =============================================================================
// Arduino entry points
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println(F("=== T32 Key Discovery Firmware ==="));

    // MODE setup + boot-time held check.
    pinMode(pinout::t32::MODE, INPUT_PULLUP);
    {
        const uint32_t start = millis();
        bool stillHeld = modePressed();
        while (stillHeld && (millis() - start < kModeHoldBootMs)) {
            if (!modePressed()) { stillHeld = false; break; }
            delay(2);
        }
        if (stillHeld && modePressed()) {
            Serial.println(F("MODE held at boot — restarting"));
            delay(50);
            ESP.restart();
        }
    }

    // Mux select lines + commons.
    pinMode(pinout::t32::S0, OUTPUT);
    pinMode(pinout::t32::S1, OUTPUT);
    pinMode(pinout::t32::S2, OUTPUT);
    pinMode(pinout::t32::S3, OUTPUT);
    digitalWrite(pinout::t32::S0, LOW);
    digitalWrite(pinout::t32::S1, LOW);
    digitalWrite(pinout::t32::S2, LOW);
    digitalWrite(pinout::t32::S3, LOW);

    pinMode(pinout::t32::COM,  INPUT);
    pinMode(pinout::t32::COM2, INPUT);
    analogReadResolution(12);

    // LED strip.
    FastLED.addLeds<WS2812, pinout::t32::LED_PIN, GRB>(leds, kNumLeds);
    FastLED.setBrightness(64);
    setAllLeds(CRGB::Black);
    FastLED.show();

    // Baseline.
    captureBaseline();

    Serial.println(F("Press each lit (red) key in turn. MODE re-arms."));
}

void loop() {
    static bool ran = false;
    if (!ran) {
        ran = true;
        runDiscovery();
        emitArray();
        Serial.println(F("Discovery complete. Paste the block into "
                         "src/variant_t32.hpp (k3dot0Keys[32])."));
    }

    // Done indicator: slow green blink on all LEDs forever.
    static uint32_t last = 0;
    static bool     on   = false;
    if (millis() - last >= 700) {
        last = millis();
        on   = !on;
        setAllLeds(on ? CRGB(0, 60, 0) : CRGB::Black);
        FastLED.show();
    }
}
