#ifndef WAVETRANSITION_HPP
#define WAVETRANSITION_HPP

#include "Pattern.hpp"

class WaveTransition : public Pattern
{
public:
    WaveTransition()
    {
        step = 0;
        totalSteps = 4;
    };
    bool RunPattern() override;

private:
    uint8_t step;
    uint8_t totalSteps;
};

bool WaveTransition::RunPattern()
{
    blur2d(patternleds, 4, 4, 160);
    for (int x = 0; x < kMatrixWidth; x++)
    {
        for (int y = 0; y < kMatrixHeight; y++)
        {
            patternleds[XY(step, y)] = ColorFromPalette(currentPalette, 255, 255);
        }
    }

    EVERY_N_MILLIS(50) { step++; }

    if (step >= totalSteps)
    {
        step = 0;
        return false;
    }
    return true;
}

#endif // WAVETRANSITION_HPP
