#ifndef PINOUT_T32_H
#define PINOUT_T32_H

#include "hardware_config.hpp"

namespace T32 {

struct PinDefinitions {
    static constexpr uint8_t S0 = 4;
    static constexpr uint8_t S1 = 5;
    static constexpr uint8_t S2 = 6;
    static constexpr uint8_t S3 = 7;
    
    static constexpr uint8_t COM1 = 8;
    static constexpr uint8_t COM2 = 15;
    
    static constexpr uint8_t LED = 1;
    
    static constexpr uint8_t T1 = 3;
    static constexpr uint8_t T2 = 9;
    static constexpr uint8_t T3 = 10;
    static constexpr uint8_t T4 = 11;
    static constexpr uint8_t T5 = 12;
    static constexpr uint8_t T6 = 13;
    static constexpr uint8_t T7 = 14;
    
    static constexpr uint8_t TX = 43;
    static constexpr uint8_t RX = 44;
    static constexpr uint8_t TX2 = 42;
    
    static constexpr uint8_t TOUCH = 21;
    static constexpr uint8_t EXT3 = 41;
    static constexpr uint8_t EXT4 = 40;
    static constexpr uint8_t MODE = 0;
};

struct MultiplexerPins {
    static constexpr uint8_t select_pins[3] = {PinDefinitions::S0, PinDefinitions::S1, PinDefinitions::S2};
    static constexpr uint8_t com_pins[2] = {PinDefinitions::COM1, PinDefinitions::COM2};
    static constexpr uint8_t num_mux = 2;
    static constexpr bool shared_select = true;
};

struct TouchSliderPins {
    static constexpr uint8_t pins[7] = {
        PinDefinitions::T1, PinDefinitions::T2, PinDefinitions::T3, PinDefinitions::T4,
        PinDefinitions::T5, PinDefinitions::T6, PinDefinitions::T7
    };
};

struct MidiPins {
    static constexpr uint8_t tx = PinDefinitions::TX;
    static constexpr uint8_t rx = PinDefinitions::RX;
    static constexpr uint8_t tx2 = PinDefinitions::TX2;
};

struct ControlPins {
    static constexpr uint8_t touch_button = PinDefinitions::TOUCH;
    static constexpr uint8_t mode_button = PinDefinitions::MODE;
    static constexpr uint8_t ext3 = PinDefinitions::EXT3;
    static constexpr uint8_t ext4 = PinDefinitions::EXT4;
};

struct LedPins {
    static constexpr uint8_t data = PinDefinitions::LED;
};

}

template<>
struct HardwarePinout<HardwareVariant::T32> {
    using Pins = T32::PinDefinitions;
    using Mux = T32::MultiplexerPins;
    using Touch = T32::TouchSliderPins;
    using Midi = T32::MidiPins;
    using Control = T32::ControlPins;
    using Led = T32::LedPins;
};

#endif // PINOUT_T32_H