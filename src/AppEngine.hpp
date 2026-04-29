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

#if defined(T32)
    // T32: 32 keys. variant::t32::kMuxes[].keyMapping translates electrical
    // channel -> logical key index inside Adc::ReadValues, so _channels[] is
    // already indexed by logical key. Each Key reads logical index = position.
    Key keys_[variant::CurrentVariant::kConfig.TOTAL_KEYS] = {
         0,  1,  2,  3,  4,  5,  6,  7,
         8,  9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31
    };
#elif defined(REV_B)
    // T16 REV_B: identity keyMapping in variant_t16, but keys_ stores the
    // electrical-channel-to-physical-position permutation directly (each Key
    // reads adc->GetMux(0, mux_idx) = _channels[mux_idx]).
    Key keys_[variant::CurrentVariant::kConfig.TOTAL_KEYS] = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6};
#else
    Key keys_[variant::CurrentVariant::kConfig.TOTAL_KEYS] = {6, 7, 15, 11, 5, 2, 14, 9, 4, 1, 13, 8, 3, 0, 12, 10};
#endif

    uint8_t sliderSensors_[7] = {CurrentPinout::T1, CurrentPinout::T2, CurrentPinout::T3, CurrentPinout::T4, CurrentPinout::T5, CurrentPinout::T6, CurrentPinout::T7};
    Keyboard keyboard_;
    TouchSlider slider_;
    Button touchBtn_{CurrentPinout::TOUCH};
    Button modeBtn_{CurrentPinout::MODE};
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
