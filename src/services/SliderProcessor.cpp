#include "SliderProcessor.hpp"
#include "ModeManager.hpp"
#include "InputProcessor.hpp"
#include "../ConfigManager.hpp"
#include "../Libs/MidiProvider.hpp"
#include "../Libs/Leds/LedManager.hpp"
#include "../Libs/Keyboard.hpp"
#include "../Libs/TouchSlider.hpp"
#include "../Scales.hpp"

namespace t16
{

SliderProcessor::SliderProcessor(ModeManager& modeManager, ConfigManager& config,
                                 MidiProvider& midi, LedManager& leds, TouchSlider& slider,
                                 Keyboard& keyboard, Parameters& params, InputProcessor& input)
    : modeManager_(modeManager)
    , configManager_(config)
    , midiProvider_(midi)
    , ledManager_(leds)
    , slider_(slider)
    , keyboard_(keyboard)
    , params_(params)
    , inputProcessor_(input)
{
}

void SliderProcessor::update()
{
    uint8_t bank = params_.bank;
    KeyModeData& kb = configManager_.Bank(bank);
    ControlChangeData& cc = configManager_.CC(bank);
    SliderMode sliderMode = modeManager_.getSliderMode();

    switch (sliderMode)
    {
    case BEND:
    {
        if (!slider_.IsTouched())
        {
            ledManager_.SetSlider(0.5f, false);
            if (params_.isBending)
            {
                params_.bend = 0.0f;
                midiProvider_.SendPitchBend(params_.bend, kb.channel);
                params_.isBending = false;
            }
        }
        else if (slider_.IsTouched())
        {
            float value = (slider_.GetPosition() - 0.5f) * 2.0f;
            params_.bend = (value * 8191.0f);
            midiProvider_.SendPitchBend((int)params_.bend, kb.channel);
            ledManager_.SetSlider(slider_.GetPosition(), false);
            params_.isBending = true;
        }
        break;
    }
    case OCTAVE:
    {
        uint8_t position = slider_.GetQuantizedPosition(7);
        ledManager_.SetSlider((uint8_t)position, false);
        if (position != kb.base_octave)
        {
            kb.base_octave = position;
            if (kb.koala_mode)
            {
                auto markerCb = [this](uint8_t index, bool isRootNote) {
                    ledManager_.SetMarker(index, isRootNote);
                };
                SetNoteMap(kb.scale, kb.base_note, kb.flip_x, kb.flip_y, markerCb, kb.base_octave);
            }
        }
        break;
    }
    case MOD:
    {
        float mod = slider_.GetPosition();
        if (params_.mod != mod)
        {
            params_.mod = mod;
            midiProvider_.SendControlChange(cc.id[3], (uint8_t)(params_.mod * 127.0f), cc.channel[3]);
        }
        ledManager_.SetSlider(slider_.GetPosition());
        break;
    }
    case SLEW:
    {
        params_.slew = slider_.GetPosition();
        ledManager_.SetSlider(params_.slew);
        keyboard_.SetSlew(params_.slew);
        break;
    }
    case BANK:
    {
        uint8_t newBank = slider_.GetQuantizedPosition(4);
        if (params_.bank != newBank)
        {
            params_.bank = newBank;
            inputProcessor_.onBankChange();
        }
        ledManager_.SetSlider((uint8_t)(slider_.GetQuantizedPosition(4) * 2), false, 30);
        break;
    }
    case STRUMMING:
    {
        uint8_t currentChord = inputProcessor_.getCurrentChord();
        uint8_t currentBaseNote = inputProcessor_.getCurrentBaseNote();
        uint8_t currentKeyIdx = inputProcessor_.getCurrentKeyIdx();

        for (int i = 0; i < 7; i++)
        {
            bool isTouched = slider_.IsTouched(i, 19000U);
            if (isTouched && !params_.active_string[i])
            {
                params_.active_string[i] = true;
                uint8_t velocity = 126;
                midiProvider_.SendChordNoteOn(i, strum_chords[currentChord][i] + currentBaseNote, velocity, kb.channel);
                ledManager_.SetSliderLed(i, 254);
            }
            else if (!isTouched && params_.active_string[i])
            {
                params_.active_string[i] = false;
                midiProvider_.SendChordNoteOff(i, kb.channel);
                ledManager_.SetSliderLed(i, 70);
            }
            else if (isTouched && params_.active_string[i])
            {
                uint8_t pressure = keyboard_.GetPressure(currentKeyIdx);
                if (params_.at_strum != pressure)
                {
                    params_.at_strum = pressure;
                    midiProvider_.SendAfterTouch(currentKeyIdx, pressure, kb.channel);
                }
            }
            else if (!params_.active_string[i])
            {
                ledManager_.SetSliderLed(i, 70);
            }
        }
        break;
    }
    case QUICK:
    {
        uint8_t page = slider_.GetQuantizedPosition(3);
        if (params_.quickSettingsPage != page)
        {
            params_.quickSettingsPage = page;
            ledManager_.UpdateTransition();
            inputProcessor_.processQuickSettings(0, Key::State::PRESSED);
        }
        ledManager_.SetSlider((uint8_t)(page * 3), false, 40);
        break;
    }
    default:
        break;
    }
}

void SliderProcessor::onSliderModeChanged()
{
    uint8_t bank = params_.bank;
    KeyModeData& kb = configManager_.Bank(bank);
    SliderMode sliderMode = modeManager_.getSliderMode();

    switch (sliderMode)
    {
    case BEND:
        log_d("Slider mode: Bend");
        ledManager_.SetSliderHue(HSVHue::HUE_PINK);
        break;
    case OCTAVE:
        log_d("Slider mode: Octave");
        slider_.SetPosition((float)kb.base_octave / 7.0f);
        ledManager_.SetSliderHue(HSVHue::HUE_BLUE - 5);
        break;
    case MOD:
        log_d("Slider mode: Mod");
        slider_.SetPosition(params_.mod);
        ledManager_.SetSliderHue(HSVHue::HUE_GREEN + 15);
        break;
    case BANK:
        log_d("Slider mode: Bank");
        slider_.SetPosition((float)params_.bank / 3.0f);
        ledManager_.SetSliderHue(HSVHue::HUE_PURPLE);
        break;
    case SLEW:
        log_d("Slider mode: Slew");
        slider_.SetPosition(params_.slew);
        ledManager_.SetSliderHue(HSVHue::HUE_ORANGE);
        break;
    case STRUMMING:
        log_d("Slider mode: Strum");
        ledManager_.SetSliderHue(HSVHue::HUE_BLUE + 10);
        break;
    case QUICK:
        log_d("Slider mode: Quick");
        slider_.SetPosition(0.0f);
        ledManager_.SetSliderHue(HSVHue::HUE_PINK);
        break;
    default:
        break;
    }
}

} // namespace t16
