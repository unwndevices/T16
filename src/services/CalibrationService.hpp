#pragma once

#include <stdint.h>
#include "../Libs/Keyboard.hpp"

// Forward declarations
class Adc;
class LedManager;
class Button;
class TouchSlider;

namespace t16
{

class CalibrationService
{
public:
    CalibrationService(Adc& adc, LedManager& leds);

    void runHardwareTest(Key* keys, uint8_t numKeys);
    void runCalibration(Key* keys, uint8_t numKeys, Button& touchBtn, Button& modeBtn,
                        Keyboard& keyboard, TouchSlider& slider);

private:
    Adc& adc_;
    LedManager& ledManager_;

    static constexpr unsigned long KEY_TEST_TIMEOUT_MS = 3000;
};

} // namespace t16
