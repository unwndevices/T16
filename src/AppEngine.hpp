#pragma once

#include <Arduino.h>
#include "pinout.h"
#include "Configuration.hpp"
#include "ConfigManager.hpp"
#include "SysExHandler.hpp"
#include "Libs/Adc.hpp"
#include "Libs/Keyboard.hpp"
#include "Libs/TouchSlider.hpp"
#include "Libs/Button.hpp"
#include "Libs/MidiProvider.hpp"
#include "Libs/Leds/LedManager.hpp"
#include "services/ModeManager.hpp"
#include "services/InputProcessor.hpp"
#include "services/SliderProcessor.hpp"
#include "services/ButtonHandler.hpp"
#include "services/CalibrationService.hpp"
#include "services/SerialCommandManager.hpp"

namespace t16
{

class AppEngine
{
public:
    AppEngine();

    void init();
    void update();

    // Public for trampoline callbacks
    void handleButtonEvent(int idx, Button::State state);
    void handleSysEx(byte* data, unsigned length);

private:
    // Hardware (order matters for DI)
    Adc adc_;

#ifdef REV_B
    Key keys_[16] = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6};
#else
    Key keys_[16] = {6, 7, 15, 11, 5, 2, 14, 9, 4, 1, 13, 8, 3, 0, 12, 10};
#endif

    uint8_t sliderSensors_[7] = {PIN_T1, PIN_T2, PIN_T3, PIN_T4, PIN_T5, PIN_T6, PIN_T7};
    Keyboard keyboard_;
    TouchSlider slider_;
    Button touchBtn_{PIN_TOUCH};
    Button modeBtn_{PIN_MODE};
    LedManager ledManager_;
    MidiProvider midiProvider_;

    // Palettes
    CRGBPalette16 palettes_[4];

    // Services (order matters for DI -- declared after their dependencies)
    ConfigManager configManager_;
    ModeManager modeManager_;
    Parameters params_;
    CalibrationData calibrationData_;
    SysExHandler sysExHandler_;
    InputProcessor inputProcessor_;
    SliderProcessor sliderProcessor_;
    ButtonHandler buttonHandler_;
    CalibrationService calibrationService_;
    SerialCommandManager serialCommands_;

    // Rendering
    void renderModeVisuals();
};

} // namespace t16
