#pragma once
#include <cstdint>
#include <cstddef>

namespace SysEx {
    constexpr uint8_t MANUFACTURER_ID = 0x7D;
    constexpr uint8_t CMD_VERSION = 0x01;
    constexpr uint8_t CMD_CONFIG = 0x02;
    constexpr uint8_t CMD_PARAM = 0x03;
    constexpr uint8_t CMD_CALIBRATION = 0x04;
    constexpr uint8_t SUB_REQUEST = 0x01;
    constexpr uint8_t SUB_RESPONSE = 0x02;
    constexpr uint8_t SUB_LOAD = 0x03;
    constexpr uint8_t SUB_ACK = 0x04;
    constexpr uint8_t PROTOCOL_VERSION = 1;
    constexpr size_t MIN_MESSAGE_LENGTH = 4;
    constexpr uint8_t STATUS_OK = 0x00;
    constexpr uint8_t STATUS_ERROR = 0x01;
    constexpr uint8_t DOMAIN_GLOBAL = 0x00;
    constexpr uint8_t DOMAIN_BANK_KB = 0x01;
    constexpr uint8_t DOMAIN_BANK_CC = 0x02;

    // Global field IDs (sequential 0-6)
    constexpr uint8_t FIELD_GLOBAL_MODE = 0;
    constexpr uint8_t FIELD_GLOBAL_SENSITIVITY = 1;
    constexpr uint8_t FIELD_GLOBAL_BRIGHTNESS = 2;
    constexpr uint8_t FIELD_GLOBAL_MIDI_TRS = 3;
    constexpr uint8_t FIELD_GLOBAL_TRS_TYPE = 4;
    constexpr uint8_t FIELD_GLOBAL_PASSTHROUGH = 5;
    constexpr uint8_t FIELD_GLOBAL_MIDI_BLE = 6;

    // Bank field IDs (sequential 0-8)
    constexpr uint8_t FIELD_BANK_CHANNEL = 0;
    constexpr uint8_t FIELD_BANK_SCALE = 1;
    constexpr uint8_t FIELD_BANK_OCTAVE = 2;
    constexpr uint8_t FIELD_BANK_NOTE = 3;
    constexpr uint8_t FIELD_BANK_VELOCITY = 4;
    constexpr uint8_t FIELD_BANK_AFTERTOUCH = 5;
    constexpr uint8_t FIELD_BANK_FLIP_X = 6;
    constexpr uint8_t FIELD_BANK_FLIP_Y = 7;
    constexpr uint8_t FIELD_BANK_KOALA_MODE = 8;
}
