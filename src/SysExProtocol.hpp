#pragma once
#include <Arduino.h>

namespace SysEx {
    // Manufacturer ID — 0x7D is non-commercial per MIDI spec (per PROTO-01)
    constexpr uint8_t MANUFACTURER_ID = 0x7D;

    // Command bytes
    constexpr uint8_t CMD_VERSION     = 0x01;
    constexpr uint8_t CMD_CONFIG      = 0x02;
    constexpr uint8_t CMD_PARAM       = 0x03;
    constexpr uint8_t CMD_CALIBRATION = 0x04;
    constexpr uint8_t CMD_BOOTLOADER     = 0x05;
    constexpr uint8_t CMD_FACTORY_RESET  = 0x06;

    // Sub-command bytes
    constexpr uint8_t SUB_REQUEST  = 0x01;
    constexpr uint8_t SUB_RESPONSE = 0x02;
    constexpr uint8_t SUB_LOAD     = 0x03;
    constexpr uint8_t SUB_ACK      = 0x04;

    // Protocol version (for PROTO-05 handshake)
    constexpr uint8_t PROTOCOL_VERSION = 1;

    // Per-parameter addressing domains (for PROTO-02)
    constexpr uint8_t DOMAIN_GLOBAL  = 0x00;
    constexpr uint8_t DOMAIN_BANK_KB = 0x01;
    constexpr uint8_t DOMAIN_BANK_CC = 0x02;

    // Minimum valid message length (after MIDI library strips F0/F7):
    // data[0]=F0, data[1]=manufacturer_id, data[2]=cmd, data[3]=sub
    constexpr size_t MIN_MESSAGE_LENGTH = 4;

    // Status codes for ACK responses
    constexpr uint8_t STATUS_OK    = 0x00;
    constexpr uint8_t STATUS_ERROR = 0x01;

    // Field IDs for DOMAIN_GLOBAL (sequential, matching schema key order)
    constexpr uint8_t FIELD_GLOBAL_MODE         = 0;
    constexpr uint8_t FIELD_GLOBAL_SENSITIVITY   = 1;
    constexpr uint8_t FIELD_GLOBAL_BRIGHTNESS    = 2;
    constexpr uint8_t FIELD_GLOBAL_MIDI_TRS      = 3;
    constexpr uint8_t FIELD_GLOBAL_TRS_TYPE      = 4;
    constexpr uint8_t FIELD_GLOBAL_PASSTHROUGH   = 5;
    constexpr uint8_t FIELD_GLOBAL_MIDI_BLE      = 6;

    // Field IDs for DOMAIN_BANK_KB
    constexpr uint8_t FIELD_BANK_CHANNEL          = 0;
    constexpr uint8_t FIELD_BANK_SCALE            = 1;
    constexpr uint8_t FIELD_BANK_OCTAVE           = 2;
    constexpr uint8_t FIELD_BANK_NOTE             = 3;
    constexpr uint8_t FIELD_BANK_VELOCITY_CURVE   = 4;
    constexpr uint8_t FIELD_BANK_AFTERTOUCH_CURVE = 5;
    constexpr uint8_t FIELD_BANK_FLIP_X           = 6;
    constexpr uint8_t FIELD_BANK_FLIP_Y           = 7;
    constexpr uint8_t FIELD_BANK_KOALA_MODE       = 8;
}
