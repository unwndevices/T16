
#ifndef MIDIPROVIDER_HPP
#define MIDIPROVIDER_HPP

#include <Arduino.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
struct CustomSettings : public midi::DefaultSettings
{
    // static const bool Use1ByteParsing = false;
    static const unsigned SysExMaxSize = 2048; // to allow configuration transfers
    static const unsigned BaudRate = 31250;
};

class MidiProvider
{
public:
    MidiProvider();
    void Init(int pin_rx, int pin_tx, int pin_tx2);
    void Read();
    void SendNoteOn(uint8_t key, uint8_t note, uint8_t velocity, uint8_t channel);
    void SendNoteOff(uint8_t key, uint8_t channel);
    void SendAfterTouch(uint8_t key, uint8_t pressure, uint8_t channel);
    void SendChordOn(uint8_t key, uint8_t note, int8_t (*chord)[4], uint8_t velocity, uint8_t channel);
    void SendChordOff(uint8_t key, uint8_t channel);
    void SendChordPressure(uint8_t key, uint8_t pressure, uint8_t channel);

    void SendChordNoteOn(uint8_t idx, uint8_t note, uint8_t velocity, uint8_t channel);
    void SendChordNoteOff(uint8_t idx, uint8_t channel);

    void SendPitchBend(int bend, uint8_t channel);
    void SendControlChange(uint8_t controller, uint8_t value, uint8_t channel);
    void SendSysEx(size_t size, const byte *data);
    void SetHandleSystemExclusive(void (*function)(byte *, unsigned));
    void SetMidiThru(bool enabled);
    void SetMidiOut(bool enabled);
    void SetMidiBle(bool enabled);

    void ClearChordPool(uint8_t channel);

    void SetMidiTRSType(bool type);

private:
    Adafruit_USBD_MIDI usb_midi;
    midi::SerialMIDI<Adafruit_USBD_MIDI, CustomSettings> serialMIDI_USB;
    midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI, CustomSettings>> MIDI_USB;
    midi::SerialMIDI<HardwareSerial, CustomSettings> serialMIDI_SERIAL;
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial, CustomSettings>> MIDI_SERIAL;
    midi::MidiInterface<bleMidi::BLEMIDI_Transport<bleMidi::BLEMIDI_ESP32_NimBLE>, CustomSettings> MIDI_BLE;
    bleMidi::BLEMIDI_Transport<bleMidi::BLEMIDI_ESP32_NimBLE> BLEMIDI;

    bool midiBle;
    bool midiThru;
    bool midiOut;
    bool midiTRSType;

    int8_t note_pool[16];
    int8_t chord_pool[5]; // 1 for the key, 4 for the notes
    int8_t strum_pool[7];
};

#endif // MIDIPROVIDER_HPP
