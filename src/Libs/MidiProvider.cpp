#include "MidiProvider.hpp"

// ---- Concrete transport implementations (private to this translation unit) ----

class MidiProvider::UsbMidiTransport : public t16::MidiTransport
{
public:
    UsbMidiTransport(decltype(MidiProvider::MIDI_USB)& midi) : midi_(midi) {}

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        midi_.sendNoteOn(note, velocity, channel);
    }
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        midi_.sendNoteOff(note, velocity, channel);
    }
    void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override
    {
        midi_.sendControlChange(cc, value, channel);
    }
    void sendPitchBend(int bend, uint8_t channel) override
    {
        midi_.sendPitchBend(bend, channel);
    }
    void sendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel) override
    {
        midi_.sendAfterTouch(note, pressure, channel);
    }
    void sendSysEx(size_t size, const byte* data) override
    {
        midi_.sendSysEx(size, data);
    }
    void read() override
    {
        midi_.read();
    }
    void setHandleSystemExclusive(void (*function)(byte*, unsigned)) override
    {
        midi_.setHandleSystemExclusive(function);
    }

private:
    decltype(MidiProvider::MIDI_USB)& midi_;
};

class MidiProvider::BleMidiTransport : public t16::MidiTransport
{
public:
    BleMidiTransport(decltype(MidiProvider::MIDI_BLE)& midi) : midi_(midi) {}

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        midi_.sendNoteOn(note, velocity, channel);
    }
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        midi_.sendNoteOff(note, velocity, channel);
    }
    void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override
    {
        midi_.sendControlChange(cc, value, channel);
    }
    void sendPitchBend(int bend, uint8_t channel) override
    {
        midi_.sendPitchBend(bend, channel);
    }
    void sendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel) override
    {
        midi_.sendAfterTouch(note, pressure, channel);
    }
    void sendSysEx(size_t size, const byte* data) override
    {
        midi_.sendSysEx(size, data);
    }
    void read() override
    {
        midi_.read();
    }
    void setHandleSystemExclusive(void (*function)(byte*, unsigned)) override
    {
        midi_.setHandleSystemExclusive(function);
    }

private:
    decltype(MidiProvider::MIDI_BLE)& midi_;
};

class MidiProvider::SerialMidiTransport : public t16::MidiTransport
{
public:
    SerialMidiTransport(decltype(MidiProvider::MIDI_SERIAL)& midi) : midi_(midi) {}

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        midi_.sendNoteOn(note, velocity, channel);
    }
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        midi_.sendNoteOff(note, velocity, channel);
    }
    void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override
    {
        midi_.sendControlChange(cc, value, channel);
    }
    void sendPitchBend(int bend, uint8_t channel) override
    {
        midi_.sendPitchBend(bend, channel);
    }
    void sendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel) override
    {
        midi_.sendAfterTouch(note, pressure, channel);
    }
    void sendSysEx(size_t size, const byte* data) override
    {
        midi_.sendSysEx(size, data);
    }
    void read() override
    {
        midi_.read();
    }
    void setHandleSystemExclusive(void (*function)(byte*, unsigned)) override
    {
        midi_.setHandleSystemExclusive(function);
    }

private:
    decltype(MidiProvider::MIDI_SERIAL)& midi_;
};

// ---- MidiProvider implementation ----

MidiProvider::MidiProvider() : serialMIDI_USB(usb_midi),
                               MIDI_USB(serialMIDI_USB),
                               serialMIDI_SERIAL(Serial1),
                               MIDI_SERIAL(serialMIDI_SERIAL),
                               BLEMIDI("Topo T16"),
                               MIDI_BLE((bleMidi::BLEMIDI_Transport<bleMidi::BLEMIDI_ESP32_NimBLE> &)BLEMIDI),
                               usbTransport_(nullptr),
                               bleTransport_(nullptr),
                               serialTransport_(nullptr),
                               transports_{{nullptr, nullptr, nullptr}},
                               activeCount_(0),
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

    // Create transport wrappers
    usbTransport_ = new UsbMidiTransport(MIDI_USB);
    bleTransport_ = new BleMidiTransport(MIDI_BLE);
    serialTransport_ = new SerialMidiTransport(MIDI_SERIAL);
}

void MidiProvider::RebuildTransportList()
{
    activeCount_ = 0;
    // USB is always active
    transports_[activeCount_++] = usbTransport_;
    if (midiBle)
    {
        transports_[activeCount_++] = bleTransport_;
    }
    if (midiOut)
    {
        transports_[activeCount_++] = serialTransport_;
    }
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

    if (midiBle)
    {
        MIDI_BLE.begin();
    }
    MIDI_USB.begin();

    RebuildTransportList();
}

void MidiProvider::Read()
{
    // USB and BLE are always read when active
    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->read();
    }
    // MIDI thru reads serial even when not in the output transport list
    if (midiThru && !midiOut)
    {
        MIDI_SERIAL.read();
    }
}

void MidiProvider::SendNoteOn(uint8_t key, uint8_t note, uint8_t velocity, uint8_t channel)
{
    note_pool[key] = note;

    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendNoteOn(note, velocity, channel);
    }
}

void MidiProvider::SendNoteOff(uint8_t key, uint8_t channel)
{
    uint8_t note = note_pool[key];

    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendNoteOff(note, 0, channel);
    }
    note_pool[key] = -1;
}

void MidiProvider::SendChordOn(uint8_t key, uint8_t note, int8_t (*chord)[4], uint8_t velocity, uint8_t channel)
{
    if (note_pool[0] != -1)
    {
        SendChordOff(key, channel);
    }
    note_pool[0] = key;
    for (int j = 1; j < 5; j++)
    {
        chord_pool[j] = note + (*chord)[j];

        for (uint8_t i = 0; i < activeCount_; i++)
        {
            transports_[i]->sendNoteOn(chord_pool[j], velocity, channel);
        }
    }
}

void MidiProvider::SendChordOff(uint8_t key, uint8_t channel)
{
    if (note_pool[0] == key)
    {
        for (int j = 1; j < 5; j++)
        {
            for (uint8_t i = 0; i < activeCount_; i++)
            {
                transports_[i]->sendNoteOff(chord_pool[j], 0, channel);
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
        for (int j = 1; j < 5; j++)
        {
            for (uint8_t i = 0; i < activeCount_; i++)
            {
                transports_[i]->sendAfterTouch(chord_pool[j], pressure, channel);
            }
        }
    }
}

void MidiProvider::SendChordNoteOn(uint8_t idx, uint8_t note, uint8_t velocity, uint8_t channel)
{
    strum_pool[idx] = note;

    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendNoteOn(note, velocity, channel);
    }
}

void MidiProvider::SendChordNoteOff(uint8_t idx, uint8_t channel)
{
    uint8_t note = strum_pool[idx];

    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendNoteOff(note, 0, channel);
    }
    strum_pool[idx] = -1;
}

void MidiProvider::SendAfterTouch(uint8_t key, uint8_t pressure, uint8_t channel)
{
    uint8_t note = note_pool[key];

    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendAfterTouch(note, pressure, channel);
    }
}

void MidiProvider::SendPitchBend(int bend, uint8_t channel)
{
    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendPitchBend(bend, channel);
    }
}

void MidiProvider::SendControlChange(uint8_t controller, uint8_t value, uint8_t channel)
{
    for (uint8_t i = 0; i < activeCount_; i++)
    {
        transports_[i]->sendControlChange(controller, value, channel);
    }
}

void MidiProvider::SendSysEx(size_t size, const byte *data)
{
    // SysEx goes to USB and BLE only (not serial out)
    usbTransport_->sendSysEx(size, data);
    if (midiBle)
    {
        bleTransport_->sendSysEx(size, data);
    }
}

void MidiProvider::SetHandleSystemExclusive(void (*function)(byte *, unsigned))
{
    usbTransport_->setHandleSystemExclusive(function);
    bleTransport_->setHandleSystemExclusive(function);
}

void MidiProvider::SetMidiThru(bool enabled)
{
    midiThru = enabled;
}

void MidiProvider::SetMidiOut(bool enabled)
{
    midiOut = enabled;
    RebuildTransportList();
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
    RebuildTransportList();
}

void MidiProvider::SetMidiTRSType(bool type)
{
    midiTRSType = type;
    if (midiTRSType)
    {
        // Type B
        Serial1.begin(31250, SERIAL_8N1, pin_rx, pin_tx);
        pinMode(pin_tx2, OUTPUT);
        digitalWrite(pin_tx2, HIGH);
    }
    else
    {
        Serial1.begin(31250, SERIAL_8N1, pin_rx, pin_tx2);
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
