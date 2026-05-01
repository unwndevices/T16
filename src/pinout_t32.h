#pragma once

// T32 pin assignments — validated on hardware via the proven 3dot0 firmware.

#include <cstdint>

namespace pinout::t32 {

// Multiplexer (dual mux: shared S0..S3 select lines, separate common pins)
constexpr uint8_t S0 = 4;
constexpr uint8_t S1 = 5;
constexpr uint8_t S2 = 6;
constexpr uint8_t S3 = 7;
constexpr uint8_t COM  = 8;   // Mux 0 common pin (ADC1_CH7).
constexpr uint8_t COM2 = 2;   // Mux 1 common pin (ADC1_CH1) — confirmed on T32 hardware 2026-05-01.

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
