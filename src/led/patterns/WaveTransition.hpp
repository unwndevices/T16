#ifndef WAVETRANSITION_HPP
#define WAVETRANSITION_HPP

#include "Pattern.hpp"

enum class Direction
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class WaveTransition : public Pattern
{
public:
    WaveTransition(Direction dir = Direction::UP)
    {
        step = 0;
        totalSteps = 4;
        direction = dir;
        isTransition = true;
    };

    bool RunPattern() override;

private:
    uint8_t step;
    uint8_t totalSteps;
    Direction direction;
};

bool WaveTransition::RunPattern()
{
    blur2d(patternleds, 4, 4, 200);

    int8_t xStep = 0, yStep = 0;
    switch (direction)
    {
    case Direction::UP:
        yStep = 1;
        break;
    case Direction::DOWN:
        yStep = -1;
        break;
    case Direction::LEFT:
        xStep = 1;
        break;
    case Direction::RIGHT:
        xStep = -1;
        break;
    }

    int x = (xStep < 0) ? kMatrixWidth - 1 - step : step;
    int y = (yStep < 0) ? kMatrixHeight - 1 - step : step;

    for (int i = 0; i < kMatrixWidth; i++)
    {
        int xx = xStep ? x : i;
        patternleds[XY(xx, y)] = ColorFromPalette(currentPalette, 255, 255);
    }

    EVERY_N_MILLIS(50) { step++; }
    if (step >= (xStep ? kMatrixWidth : kMatrixHeight))
    {
        step = 0;
        return false;
    }

    return true;
}

#endif // WAVETRANSITION_HPP
