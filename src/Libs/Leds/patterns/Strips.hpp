#ifndef STRIPS_HPP
#define STRIPS_HPP

#include "Pattern.hpp"

class Strips : public Pattern
{
public:
    Strips()
    {
        currentPalette = acid_gp;
        state = true;
    };
    bool RunPattern() override;

    void SetStrip(uint8_t strip, float value) override
    {
        strips[strip] = value;
        state = true;
    };

    void SetColor(uint8_t color) override
    {
        colorIndex = color;
    };

private:
    uint8_t colorIndex = 255;
    float strips[kMatrixWidth] = {0.0f};
};

bool Strips::RunPattern()
{
    fill_solid(patternleds, NUM_KEYS, CRGB::Black);
    if (state)
    {
        CRGB color = ColorFromPalette(currentPalette, colorIndex, 255, LINEARBLEND_NOWRAP);
        for (uint8_t i = 0; i < kMatrixWidth; i++)
        {
            wu_pixel_1d(i, static_cast<uint32_t>(strips[i] * (1 << 8)), &color);
        }
    };

    return true;
}

#endif// STRIPS_HPP
