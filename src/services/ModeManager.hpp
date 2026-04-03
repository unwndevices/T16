#pragma once

#include "../Types.hpp"
#include <stddef.h>

namespace t16
{

class ModeManager
{
public:
    ModeManager() = default;

    void setMode(Mode mode);
    Mode getMode() const;

    SliderMode getSliderMode() const;
    void cycleSliderMode();

    SliderMode getDefaultSliderMode(Mode mode) const;
    const SliderMode* getAllowedSliderModes(size_t& count) const;

private:
    Mode currentMode_ = KEYBOARD;
    SliderMode currentSliderMode_ = BEND;
};

} // namespace t16
