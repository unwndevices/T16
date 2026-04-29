#pragma once

#include <cstdint>

namespace pinout::t16 {

#if defined(REV_A)

// Multiplexer
constexpr uint8_t S0 = 5;
constexpr uint8_t S1 = 4;
constexpr uint8_t S2 = 7;
constexpr uint8_t S3 = 6;
constexpr uint8_t COM = 8;

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
constexpr uint8_t TOUCH = 2;
constexpr uint8_t EXT3  = 41;
constexpr uint8_t EXT4  = 40;
constexpr uint8_t MODE  = 0;

#elif defined(REV_B)

// Multiplexer
constexpr uint8_t S0 = 4;
constexpr uint8_t S1 = 5;
constexpr uint8_t S2 = 6;
constexpr uint8_t S3 = 7;
constexpr uint8_t COM = 8;

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

#else
#error "T16 build requires either -DREV_A or -DREV_B"
#endif

}  // namespace pinout::t16
