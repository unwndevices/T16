#include "ModeManager.hpp"

namespace t16
{

// Allowed slider modes per mode
static constexpr SliderMode keyboardSliderModes[] = {BEND, MOD, OCTAVE, BANK};
static constexpr SliderMode strumSliderModes[] = {STRUMMING, OCTAVE, BANK};
static constexpr SliderMode xyPadSliderModes[] = {SLEW, BANK};
static constexpr SliderMode stripsSliderModes[] = {SLEW, BANK};
static constexpr SliderMode quickSettingsSliderModes[] = {QUICK};

void ModeManager::setMode(Mode mode)
{
    currentMode_ = mode;
    currentSliderMode_ = getDefaultSliderMode(mode);
}

Mode ModeManager::getMode() const
{
    return currentMode_;
}

SliderMode ModeManager::getSliderMode() const
{
    return currentSliderMode_;
}

SliderMode ModeManager::getDefaultSliderMode(Mode mode) const
{
    switch (mode)
    {
    case KEYBOARD:
        return keyboardSliderModes[0];
    case STRUM:
        return strumSliderModes[0];
    case XY_PAD:
        return xyPadSliderModes[0];
    case STRIPS:
        return stripsSliderModes[0];
    case QUICK_SETTINGS:
        return quickSettingsSliderModes[0];
    default:
        return BEND;
    }
}

const SliderMode* ModeManager::getAllowedSliderModes(size_t& count) const
{
    switch (currentMode_)
    {
    case KEYBOARD:
        count = sizeof(keyboardSliderModes) / sizeof(keyboardSliderModes[0]);
        return keyboardSliderModes;
    case STRUM:
        count = sizeof(strumSliderModes) / sizeof(strumSliderModes[0]);
        return strumSliderModes;
    case XY_PAD:
        count = sizeof(xyPadSliderModes) / sizeof(xyPadSliderModes[0]);
        return xyPadSliderModes;
    case STRIPS:
        count = sizeof(stripsSliderModes) / sizeof(stripsSliderModes[0]);
        return stripsSliderModes;
    case QUICK_SETTINGS:
        count = sizeof(quickSettingsSliderModes) / sizeof(quickSettingsSliderModes[0]);
        return quickSettingsSliderModes;
    default:
        count = 0;
        return nullptr;
    }
}

void ModeManager::cycleSliderMode()
{
    size_t count = 0;
    const SliderMode* allowed = getAllowedSliderModes(count);

    if (count == 0 || allowed == nullptr)
    {
        return;
    }

    // Find current slider mode in allowed list
    int currentIndex = -1;
    for (size_t i = 0; i < count; i++)
    {
        if (allowed[i] == currentSliderMode_)
        {
            currentIndex = static_cast<int>(i);
            break;
        }
    }

    // Advance to next, wrapping around
    currentIndex = (currentIndex + 1) % static_cast<int>(count);
    currentSliderMode_ = allowed[currentIndex];
}

} // namespace t16
