#include "MidiProvider.hpp"

MidiProvider::MidiProvider() : serialMIDI_USB(usb_midi),
                               MIDI_USB(serialMIDI_USB),
                               serialMIDI_SERIAL(Serial1),
                               MIDI_SERIAL(serialMIDI_SERIAL),
                               BLEMIDI("Topo T16"),
                               MIDI_BLE((bleMidi::BLEMIDI_Transport<bleMidi::BLEMIDI_ESP32_NimBLE> &)BLEMIDI),
                               midiBle(false),
                               midiThru(false),
                               midiOut(false),
                               midiTRSType(false),
                               pin_rx(0),
                               pin_tx(0),
                               pin_tx2(0)
{
    memset(note_pool, -1, sizeof(note_pool));
    memset(chord_pool, -1, sizeof(chord_pool));
    memset(strum_pool, -1, sizeof(strum_pool));
}
void MidiProvider::Init(int pin_rx, int pin_tx, int pin_tx2)
{
    this->pin_rx = pin_rx;
    this->pin_tx = pin_tx;
    this->pin_tx2 = pin_tx2;
    usb_midi.setStringDescriptor("Topo T16");

    MIDI_USB.turnThruOff();
    MIDI_SERIAL.turnThruOff();
    MIDI_BLE.turnThruOff();

    MIDI_USB.begin();
    if (midiBle)
    {
        MIDI_BLE.begin();
    }
}

void MidiProvider::Read()
{
    MIDI_USB.read();

    if (midiBle)
    {
        MIDI_BLE.read();
    }
    if (midiThru)
    {
        // TODO: proper midi thru implementation
        MIDI_SERIAL.read();
    }
}

void MidiProvider::SendNoteOn(uint8_t key, uint8_t note, uint8_t velocity, uint8_t channel)
{
    note_pool[key] = note; // Save the note in the note pool at the index corresponding to the key

    MIDI_USB.sendNoteOn(note, velocity, channel);
    if (midiBle)
    {
        MIDI_BLE.sendNoteOn(note, velocity, channel);
    }

    if (midiOut)
    {
        MIDI_SERIAL.sendNoteOn(note, velocity, channel);
    }
}

void MidiProvider::SendNoteOff(uint8_t key, uint8_t channel)
{
    uint8_t note = note_pool[key]; // Retrieve the note from the note pool using the key

    MIDI_USB.sendNoteOff(note, 0, channel); // Velocity is not used in NoteOff in some MIDI implementations
    if (midiBle)
    {
        MIDI_BLE.sendNoteOff(note, 0, channel);
    }

    if (midiOut)
    {
        MIDI_SERIAL.sendNoteOff(note, 0, channel);
    }
    note_pool[key] = -1; // Clear the note from the note pool
}

void MidiProvider::SendChordOn(uint8_t key, uint8_t note, int8_t (*chord)[4], uint8_t velocity, uint8_t channel)
{
    if (note_pool[0] != -1)
    {
        SendChordOff(key, channel);
    }
    note_pool[0] = key;
    for (int i = 1; i < 5; i++)
    {
        chord_pool[i] = note + (*chord)[i];

        MIDI_USB.sendNoteOn(chord_pool[i], velocity, channel);

        if (midiBle)
        {
            MIDI_BLE.sendNoteOn(chord_pool[i], velocity, channel);
        }

        if (midiOut)
        {
            MIDI_SERIAL.sendNoteOn(chord_pool[i], velocity, channel);
        }
    }
}

void MidiProvider::SendChordOff(uint8_t key, uint8_t channel)
{
    if (note_pool[0] == key)
    {
        for (int i = 1; i < 5; i++)
        {

            MIDI_USB.sendNoteOff(chord_pool[i], 0, channel);
            if (midiBle)
            {
                MIDI_BLE.sendNoteOff(chord_pool[i], 0, channel);
            }

            if (midiOut)
            {
                MIDI_SERIAL.sendNoteOff(chord_pool[i], 0, channel);
            }
        }
        note_pool[0] = -1;
        memset(chord_pool, -1, sizeof(chord_pool));
    }
}

void MidiProvider::SendChordPressure(uint8_t key, uint8_t pressure, uint8_t channel)
{
    if (note_pool[0] == key)
    {
        for (int i = 1; i < 5; i++)
        {

            MIDI_USB.sendAfterTouch(chord_pool[i], pressure, channel);

            if (midiBle)
            {
                MIDI_BLE.sendAfterTouch(chord_pool[i], pressure, channel);
            }
            if (midiOut)
            {
                MIDI_SERIAL.sendAfterTouch(chord_pool[i], pressure, channel);
            }
        }
    }
}

void MidiProvider::SendChordNoteOn(uint8_t idx, uint8_t note, uint8_t velocity, uint8_t channel)
{

    MIDI_USB.sendNoteOn(note, velocity, channel);

    if (midiBle)
    {
        MIDI_BLE.sendNoteOn(note, velocity, channel);
    }
    strum_pool[idx] = note; // Save the note in the note pool at the index corresponding to the key
    if (midiOut)
    {
        MIDI_SERIAL.sendNoteOn(note, velocity, channel);
    }
}

void MidiProvider::SendChordNoteOff(uint8_t idx, uint8_t channel)
{
    uint8_t note = strum_pool[idx]; // Retrieve the note from the note pool using the key

    MIDI_USB.sendNoteOff(note, 0, channel); // Velocity is not used in NoteOff in some MIDI implementations

    if (midiBle)
    {
        MIDI_BLE.sendNoteOff(note, 0, channel);
    }
    if (midiOut)
    {
        MIDI_SERIAL.sendNoteOff(note, 0, channel);
    }
    strum_pool[idx] = -1; // Clear the note from the note pool
}

void MidiProvider::SendAfterTouch(uint8_t key, uint8_t pressure, uint8_t channel)
{
    uint8_t note = note_pool[key]; // Retrieve the note from the note pool using the key

    MIDI_USB.sendAfterTouch(note, pressure, channel);

    if (midiBle)
    {
        MIDI_BLE.sendAfterTouch(note, pressure, channel);
    }
    if (midiOut)
    {
        MIDI_SERIAL.sendAfterTouch(note, pressure, channel);
    }
}

void MidiProvider::SendPitchBend(int bend, uint8_t channel)
{

    MIDI_USB.sendPitchBend(bend, channel);
    if (midiBle)
    {
        MIDI_BLE.sendPitchBend(bend, channel);
    }
    if (midiOut)
    {
        MIDI_SERIAL.sendPitchBend(bend, channel);
    }
}

void MidiProvider::SendControlChange(uint8_t controller, uint8_t value, uint8_t channel)
{

    MIDI_USB.sendControlChange(controller, value, channel);
    if (midiBle)
    {
        MIDI_BLE.sendControlChange(controller, value, channel);
    }
    if (midiOut)
    {
        MIDI_SERIAL.sendControlChange(controller, value, channel);
    }
}

void MidiProvider::SendSysEx(size_t size, const byte *data)
{
    MIDI_USB.sendSysEx(size, data);
    if (midiBle)
    {
        MIDI_BLE.sendSysEx(size, data);
    }
}

void MidiProvider::SetHandleSystemExclusive(void (*function)(byte *, unsigned))
{
    MIDI_USB.setHandleSystemExclusive(function);
    MIDI_BLE.setHandleSystemExclusive(function);
}

void MidiProvider::SetMidiThru(bool enabled)
{
    midiThru = enabled;
}

void MidiProvider::SetMidiOut(bool enabled)
{
    midiOut = enabled;
}

void MidiProvider::SetMidiBle(bool enabled)
{
    if (enabled && !midiBle)
    {
        MIDI_BLE.begin();
        midiBle = true;
    }
    else if (!enabled && midiBle)
    {
        midiBle = false;
    }
}

void MidiProvider::SetMidiTRSType(bool type)
{
    midiTRSType = type;
    if (midiTRSType)
    {
        // Type B
        Serial1.begin(31250, SERIAL_8N1, -1, pin_tx);
        pinMode(pin_tx2, OUTPUT);
        digitalWrite(pin_tx2, HIGH);
    }
    else
    {
        Serial1.begin(31250, SERIAL_8N1, -1, pin_tx2);
        pinMode(pin_tx, OUTPUT);
        digitalWrite(pin_tx, HIGH);
    }
    if (midiOut)
    {
        MIDI_SERIAL.begin();
    }
}

void MidiProvider::ClearChordPool(uint8_t channel)
{
    SendChordOff(chord_pool[0], channel);
    for (int i = 0; i < 5; i++)
    {
        chord_pool[i] = -1;
    }
}
