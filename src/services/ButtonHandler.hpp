#pragma once

#include <stdint.h>
#include "../Types.hpp"
#include "../Configuration.hpp"
#include "../Libs/Button.hpp"

// Forward declarations
class ConfigManager;
class MidiProvider;

namespace t16
{

class ModeManager;
class InputProcessor;
class SliderProcessor;

class ButtonHandler
{
public:
    ButtonHandler(ModeManager& modeManager, ConfigManager& config, MidiProvider& midi,
                  InputProcessor& input, SliderProcessor& slider, Parameters& params,
                  Button& touchBtn, Button& modeBtn);

    void handleButton(int idx, Button::State state);

private:
    ModeManager& modeManager_;
    ConfigManager& configManager_;
    MidiProvider& midiProvider_;
    InputProcessor& inputProcessor_;
    SliderProcessor& sliderProcessor_;
    Parameters& params_;
    Button& touchBtn_;
    Button& modeBtn_;
};

} // namespace t16
