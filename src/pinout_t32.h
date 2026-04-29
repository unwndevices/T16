#pragma once

// =============================================================================
// PHASE 10 PLACEHOLDER
// Values mirror T16 REV_B for the sole purpose of letting -DT32 builds compile
// during Phase 10 (Build System & Variant Selection). Phase 12 (T32 Hardware
// Bring-Up) will replace these with validated T32 dual-mux pin assignments.
// DO NOT flash a -DT32 binary onto T32 hardware until Phase 12 lands.
// =============================================================================

#include <cstdint>

namespace pinout::t32 {

// Multiplexer (T32 uses dual mux — Phase 12 will add second mux constants)
constexpr uint8_t S0 = 4;
constexpr uint8_t S1 = 5;
constexpr uint8_t S2 = 6;
constexpr uint8_t S3 = 7;
constexpr uint8_t COM  = 8;   // Mux 0 common pin (ADC1_CH7).
constexpr uint8_t COM2 = 17;  // Mux 1 common pin.
                              // PLACEHOLDER GPIO — confirm against T32 schematic
                              // before flashing real hardware (validation_deferred,
                              // Plan 12.03.5.A). ESP32-S3 ADC2_CH6.

// LED strip
constexpr uint8_t LED_PIN = 1;

// Touch slider sensors
constexpr uint8_t T1 = 3;
constexpr uint8_t T2 = 9;
constexpr uint8_t T3 = 10;
constexpr uint8_t T4 = 11;
constexpr uint8_t T5 = 12;
constexpr uint8_t T6 = 13;
constexpr uint8_t T7 = 14;

// MIDI / serial
constexpr uint8_t TX  = 43;
constexpr uint8_t RX  = 44;
constexpr uint8_t TX2_PIN = 42;

// Buttons / extras
constexpr uint8_t TOUCH = 21;
constexpr uint8_t EXT3  = 41;
constexpr uint8_t EXT4  = 40;
constexpr uint8_t MODE  = 0;

}  // namespace pinout::t32
