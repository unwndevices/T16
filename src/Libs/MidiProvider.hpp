
#ifndef MIDIPROVIDER_HPP
#define MIDIPROVIDER_HPP

#include <Arduino.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>

struct CustomSettings : public midi::DefaultSettings
{
    static const unsigned SysExMaxSize = 2048; // to allow configuration transfers
    static const unsigned BaudRate = 31250;
};

class MidiProvider
{
public:
    MidiProvider();
    void Init(int pin_rx, int pin_tx);
    void Read();
    void SendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void SendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
    void SendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel);
    void SendPitchBend(int bend, uint8_t channel);
    void SendControlChange(uint8_t controller, uint8_t value, uint8_t channel);
    void SendSysEx(size_t size, const byte *data);
    void SetHandleSystemExclusive(void (*function)(byte *, unsigned));
    void SetMidiThru(bool enabled);
    void SetMidiOut(bool enabled);

private:
    Adafruit_USBD_MIDI usb_midi;
    midi::SerialMIDI<Adafruit_USBD_MIDI, CustomSettings> serialMIDI_USB;
    midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI, CustomSettings>> MIDI_USB;
    midi::SerialMIDI<HardwareSerial, CustomSettings> serialMIDI_SERIAL;
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial, CustomSettings>> MIDI_SERIAL;
    bool midiThru;
    bool midiOut;
};

#endif // MIDIPROVIDER_HPP
