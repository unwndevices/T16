#pragma once

#include <Arduino.h>

namespace t16 {

class MidiTransport
{
public:
    virtual ~MidiTransport() = default;
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel) = 0;
    virtual void sendPitchBend(int bend, uint8_t channel) = 0;
    virtual void sendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel) = 0;
    virtual void sendSysEx(size_t size, const byte* data) = 0;
    virtual void read() = 0;
    virtual void setHandleSystemExclusive(void (*function)(byte*, unsigned)) = 0;
};

} // namespace t16
