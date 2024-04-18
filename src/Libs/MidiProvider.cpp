#include "MidiProvider.hpp"

MidiProvider::MidiProvider() : serialMIDI_USB(usb_midi),
                               MIDI_USB(serialMIDI_USB),
                               serialMIDI_SERIAL(Serial1),
                               MIDI_SERIAL(serialMIDI_SERIAL),
                               midiThru(false),
                               midiOut(false)
{
}
void MidiProvider::Init(int pin_rx, int pin_tx)
{
    MIDI_USB.begin();
    Serial1.begin(115200, SERIAL_8N1, pin_rx, pin_tx);
    MIDI_SERIAL.begin();
}

void MidiProvider::Read()
{
    MIDI_USB.read();
    if (midiThru)
    {
        // TODO: proper midi thru implementation
        MIDI_SERIAL.read();
    }
}

void MidiProvider::SendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    MIDI_USB.sendNoteOn(note, velocity, channel);
    if (midiOut)
    {
        MIDI_SERIAL.sendNoteOn(note, velocity, channel);
    }
}

void MidiProvider::SendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    MIDI_USB.sendNoteOff(note, velocity, channel);
    if (midiOut)
    {
        MIDI_SERIAL.sendNoteOff(note, velocity, channel);
    }
}

void MidiProvider::SendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel)
{
    MIDI_USB.sendAfterTouch(note, pressure, channel);
    if (midiOut)
    {
        MIDI_SERIAL.sendAfterTouch(note, pressure, channel);
    }
}

void MidiProvider::SendPitchBend(int bend, uint8_t channel)
{
    MIDI_USB.sendPitchBend(bend, channel);
    if (midiOut)
    {
        MIDI_SERIAL.sendPitchBend(bend, channel);
    }
}

void MidiProvider::SendControlChange(uint8_t controller, uint8_t value, uint8_t channel)
{
    MIDI_USB.sendControlChange(controller, value, channel);
    if (midiOut)
    {
        MIDI_SERIAL.sendControlChange(controller, value, channel);
    }
}

void MidiProvider::SendSysEx(size_t size, const byte *data)
{
    MIDI_USB.sendSysEx(size, data);
}

void MidiProvider::SetHandleSystemExclusive(void (*function)(byte *, unsigned))
{
    MIDI_USB.setHandleSystemExclusive(function);
}

void MidiProvider::SetMidiThru(bool enabled)
{
    midiThru = enabled;
}

void MidiProvider::SetMidiOut(bool enabled)
{
    midiOut = enabled;
}