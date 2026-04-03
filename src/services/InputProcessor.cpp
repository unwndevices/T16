#include "InputProcessor.hpp"
#include "../ConfigManager.hpp"
#include "../Libs/MidiProvider.hpp"
#include "../Libs/Leds/LedManager.hpp"
#include "../Libs/Keyboard.hpp"
#include "../Scales.hpp"
#include <FastLED.h>

namespace t16
{

InputProcessor::InputProcessor(ConfigManager& config, MidiProvider& midi, LedManager& leds,
                               Keyboard& keyboard, Parameters& params, CRGBPalette16* palettes)
    : configManager_(config)
    , midiProvider_(midi)
    , ledManager_(leds)
    , keyboard_(keyboard)
    , params_(params)
    , palettes_(palettes)
{
}

void InputProcessor::setMarkerCallback(uint8_t index, bool isRootNote)
{
    ledManager_.SetMarker(index, isRootNote);
}

void InputProcessor::setCustomScale(int8_t* scale, int8_t* customScale, int size)
{
    for (int i = 0; i < size; i++)
    {
        scale[i] = customScale[i];
    }
}

void InputProcessor::processKey(int idx, Key::State state)
{
    uint8_t bank = params_.bank;
    KeyModeData& kb = configManager_.Bank(bank);

    uint8_t base_octave = kb.base_octave * 12;

    if (kb.koala_mode)
    {
        base_octave = kb.base_octave * 16;
    }

    uint8_t note = note_map[idx] + base_octave + 24;

    if (state == Key::State::PRESSED)
    {
        uint8_t velocity = keyboard_.GetVelocity(idx);
        midiProvider_.SendNoteOn(idx, note, velocity, kb.channel);
        ledManager_.SetPosition((uint8_t)(idx % 4), (uint8_t)(idx / 4));
        ledManager_.SetColor(255 - velocity * 2);
        ledManager_.SetSpeed((127 - velocity) / 2);
    }
    else if (state == Key::State::RELEASED)
    {
        midiProvider_.SendNoteOff(idx, kb.channel);
    }
    else if (state == Key::State::AFTERTOUCH)
    {
        uint8_t pressure = keyboard_.GetAftertouch(idx);
        log_d("Aftertouch: %d", pressure);
        midiProvider_.SendAfterTouch(idx, (midi::DataByte)pressure, kb.channel);
    }
}

void InputProcessor::processStrum(int idx, Key::State state)
{
    uint8_t bank = params_.bank;
    KeyModeData& kb = configManager_.Bank(bank);

    if (state == Key::State::PRESSED)
    {
        if (idx < 12)
        {
            currentKeyIdx_ = idx;
            currentBaseNote_ = note_map[idx] + (kb.base_octave * 12);
            ledManager_.SetNote(idx);
            currentChord_ = current_chord_mapping[idx];
            ledManager_.SetChord(currentChord_);
        }
        else
        {
            currentChord_ = idx - 12;
            current_chord_mapping[currentKeyIdx_] = currentChord_;
            ledManager_.SetChord(currentChord_);
        }
    }
}

void InputProcessor::processQuickSettings(int idx, Key::State state)
{
    if (state == Key::State::PRESSED)
    {
        log_d("Quick Settings: %d", idx);
        if (idx < 4)
        {
            currentQsOption_ = params_.quickSettingsPage * 4 + idx;
            ledManager_.SetOption(idx, 4);
            log_d("Current QS Option: %d", currentQsOption_);
            uint8_t current_value = qs.settings[currentQsOption_].value;
            currentValueLength_ = qs.settings[currentQsOption_].numOptions;
            ledManager_.SetValue(current_value, currentValueLength_);
            log_d("Current Value: %d", current_value);
            log_d("Current Value Length: %d", currentValueLength_);
        }
        else if (idx <= currentValueLength_ + 3)
        {
            ledManager_.SetValue(idx - 4, currentValueLength_);
            qs.settings[currentQsOption_].value = idx - 4;
        }
    }
}

void InputProcessor::onBankChange()
{
    uint8_t bank = params_.bank;
    KeyModeData& kb = configManager_.Bank(bank);

    ledManager_.SetPalette(palettes_[kb.palette]);
    uint8_t base_note = kb.base_note;

    auto markerCb = [this](uint8_t index, bool isRootNote) {
        this->setMarkerCallback(index, isRootNote);
    };

    if (kb.koala_mode)
    {
        SetNoteMap(kb.scale, base_note, kb.flip_x, kb.flip_y, markerCb, kb.base_octave);
    }
    else
    {
        SetNoteMap(kb.scale, base_note, kb.flip_x, kb.flip_y, markerCb);
    }
    SetChordMapping(kb.scale);
    keyboard_.SetBank(bank);
    keyboard_.SetVelocityLut((Keyboard::Lut)kb.velocity_curve);
    keyboard_.SetAftertouchLut((Keyboard::Lut)kb.aftertouch_curve);
    ledManager_.UpdateTransition();
}

void InputProcessor::applyConfiguration()
{
    ConfigurationData& global = configManager_.Global();
    uint8_t bank = params_.bank;
    KeyModeData& kb = configManager_.Bank(bank);

    float sensitivity = (7 - (global.sensitivity - 1)) * 0.02f + 0.08f;
    keyboard_.SetPressThreshold(sensitivity);

    midiProvider_.SetMidiBle((bool)global.midi_ble);
    midiProvider_.SetMidiOut((bool)global.midi_trs);
    midiProvider_.SetMidiTRSType((bool)global.trs_type);
    midiProvider_.SetMidiThru((bool)global.passthrough);
    uint8_t brightness = (global.brightness - 1) * 20 + 10;
    ledManager_.SetBrightness(brightness);
    log_d("Brightness: %d", brightness);

    setCustomScale(scales[CUSTOM1], global.custom_scale1, 16);
    setCustomScale(scales[CUSTOM2], global.custom_scale2, 16);

    auto markerCb = [this](uint8_t index, bool isRootNote) {
        this->setMarkerCallback(index, isRootNote);
    };

    uint8_t base_note = kb.base_note;
    if (kb.koala_mode)
    {
        SetNoteMap(kb.scale, base_note, kb.flip_x, kb.flip_y, markerCb, kb.base_octave);
    }
    else
    {
        SetNoteMap(kb.scale, base_note, kb.flip_x, kb.flip_y, markerCb);
    }
    SetChordMapping(kb.scale);
    keyboard_.SetVelocityLut((Keyboard::Lut)kb.velocity_curve);
    keyboard_.SetAftertouchLut((Keyboard::Lut)kb.aftertouch_curve);

    ledManager_.UpdateTransition();
}

} // namespace t16
