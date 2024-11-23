#ifndef CALIBRATION_HPP
#define CALIBRATION_HPP

#include "Pattern.hpp"

class Calibration : public Pattern
{
public:
    Calibration()
    {
        currentPalette = topo_gp;
    }
    bool RunPattern() override;

    void SetLed(uint8_t x, uint8_t y, bool state = true) override
    {
    }

    void SetKey(uint8_t key)
    {
        current_key = key;
    }

private:
    uint8_t current_key = 0;
};

bool Calibration::RunPattern()
{
    fill_solid(patternleds, NUM_KEYS, CRGB::Black);
    if (state)
    {
        CRGB color = ColorFromPalette(currentPalette, 255, 255, LINEARBLEND_NOWRAP);
        matrixleds[current_key] = color;
    };

    return true;
}

#endif// CALIBRATION_HPP
