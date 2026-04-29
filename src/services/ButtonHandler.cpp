#include "ButtonHandler.hpp"
#include "ModeManager.hpp"
#include "InputProcessor.hpp"
#include "SliderProcessor.hpp"
#include "../ConfigManager.hpp"
#include "../Libs/MidiProvider.hpp"
#include "../Configuration.hpp"
#include "../pinout.h"

namespace t16
{

ButtonHandler::ButtonHandler(ModeManager& modeManager, ConfigManager& config, MidiProvider& midi,
                             InputProcessor& input, SliderProcessor& slider, Parameters& params,
                             Button& touchBtn, Button& modeBtn)
    : modeManager_(modeManager)
    , configManager_(config)
    , midiProvider_(midi)
    , inputProcessor_(input)
    , sliderProcessor_(slider)
    , params_(params)
    , touchBtn_(touchBtn)
    , modeBtn_(modeBtn)
{
}

void ButtonHandler::handleButton(int idx, Button::State state)
{
    if (state == Button::State::CLICKED)
    {
        if (idx == CurrentPinout::TOUCH)
        {
            log_d("Touch button clicked");
            modeManager_.cycleSliderMode();
            sliderProcessor_.onSliderModeChanged();
        }

        if (idx == CurrentPinout::MODE)
        {
            log_d("Mode button clicked");
            Mode currentMode = modeManager_.getMode();
            Mode nextMode = (Mode)((currentMode + 1) % (MODE_AMOUNT - 1));
            if (nextMode == QUICK_SETTINGS)
            {
                nextMode = KEYBOARD;
            }
            modeManager_.setMode(nextMode);
            inputProcessor_.onModeChanged(nextMode);
            sliderProcessor_.onSliderModeChanged();
        }
    }

    if (state == Button::State::LONG_PRESSED)
    {
        if (idx == CurrentPinout::TOUCH)
        {
            if (modeBtn_.GetState() == Button::State::LONG_PRESSED)
            {
                if (modeManager_.getMode() != QUICK_SETTINGS)
                {
                    log_d("Both buttons long pressed");
                    log_d("Entering Quick Settings mode");
                    modeManager_.setMode(QUICK_SETTINGS);
                    inputProcessor_.onModeChanged(QUICK_SETTINGS);
                    sliderProcessor_.onSliderModeChanged();
                }
                else
                {
                    log_d("Both buttons long pressed");
                    log_d("Exiting Quick Settings mode");
                    SaveQuickSettings(params_.bank);
                    configManager_.Save(true);
                    log_d("Saved configuration");
                    modeManager_.setMode(KEYBOARD);
                    inputProcessor_.onModeChanged(KEYBOARD);
                    sliderProcessor_.onSliderModeChanged();
                }
            }
            else
            {
                log_d("Touch button long pressed");
                log_d("Midi Learn mode");
                params_.midiLearn = true;
            }
        }
        if (idx == CurrentPinout::MODE)
        {
            if (touchBtn_.GetState() == Button::State::LONG_PRESSED)
            {
                if (modeManager_.getMode() != QUICK_SETTINGS)
                {
                    log_d("Both buttons long pressed");
                    log_d("Entering Quick Settings mode");
                    modeManager_.setMode(QUICK_SETTINGS);
                    inputProcessor_.onModeChanged(QUICK_SETTINGS);
                    sliderProcessor_.onSliderModeChanged();
                }
                else
                {
                    log_d("Both buttons long pressed");
                    log_d("Exiting Quick Settings mode");
                    SaveQuickSettings(params_.bank);
                    configManager_.Save(true);
                    log_d("Saved configuration");
                    modeManager_.setMode(KEYBOARD);
                    inputProcessor_.onModeChanged(KEYBOARD);
                    sliderProcessor_.onSliderModeChanged();
                }
            }
            else
            {
                log_d("Mode button long pressed");
                log_d("MIDI panic!");
                uint8_t channel = configManager_.Bank(params_.bank).channel;
                for (uint8_t i = 0; i < 16; i++)
                {
                    midiProvider_.SendNoteOff(i, channel);
                }
            }
        }
    }

    if (state == Button::State::LONG_RELEASED)
    {
        if (idx == CurrentPinout::TOUCH)
        {
            log_d("Touch button long released");
            params_.midiLearn = false;
        }
    }
}

} // namespace t16
