#pragma once

#include <stdint.h>
#include "../Types.hpp"
#include "../Configuration.hpp"

// Forward declarations
class ConfigManager;
class MidiProvider;
class LedManager;
class Keyboard;
class TouchSlider;

namespace t16
{

class ModeManager;
class InputProcessor;

class SliderProcessor
{
public:
    SliderProcessor(ModeManager& modeManager, ConfigManager& config, MidiProvider& midi,
                    LedManager& leds, TouchSlider& slider, Keyboard& keyboard,
                    Parameters& params, InputProcessor& input);

    void update();
    void onSliderModeChanged();

private:
    ModeManager& modeManager_;
    ConfigManager& configManager_;
    MidiProvider& midiProvider_;
    LedManager& ledManager_;
    TouchSlider& slider_;
    Keyboard& keyboard_;
    Parameters& params_;
    InputProcessor& inputProcessor_;
};

} // namespace t16
