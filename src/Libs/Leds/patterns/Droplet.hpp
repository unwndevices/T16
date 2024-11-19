#ifndef DROPLET_HPP
#define DROPLET_HPP

#include "Pattern.hpp"

class Drops : public Pattern
{
public:
    Drops();
    bool RunPattern() override;

    void SetPosition(uint8_t x, uint8_t y) override
    {
        pos_x = x;
        pos_y = y;
        state = true;
    };

    void SetColor(uint8_t color) override
    {
        colorIndex = color;
    };

private:
    void DrawCircle(int16_t x0, int16_t y0, int16_t r, CRGB color);

    uint8_t radius = 0;
    uint8_t colorIndex = 255;
};

Drops::Drops()
{
    currentPalette = unwn_gp;
}

bool Drops::RunPattern()
{

    int8_t spawnX = pos_x;
    int8_t spawnY = pos_y;

    CRGB color = ColorFromPalette(currentPalette, colorIndex, 255, LINEARBLEND_NOWRAP);

    fadeToBlackBy(patternleds, NUM_LEDS, 2);

    if (state)
    {
        if (radius == 0)
        {
            patternleds[XY(spawnX, spawnY)] = color;
            radius++;
        }
        else if (radius < 4)
        {
            patternleds[XY(spawnX, spawnY)] = color;
            EVERY_N_MILLIS(50)
            {
                colorIndex -= 2;
                for (uint8_t i = 1; i <= radius; i++)
                {
                    patternleds[XY(spawnX - i, spawnY)] = color;
                    patternleds[XY(spawnX + i, spawnY)] = color;
                    patternleds[XY(spawnX, spawnY - i)] = color;
                    patternleds[XY(spawnX, spawnY + i)] = color;
                }
                radius++;
            }
        }
        else if (radius >= 4)
        {
            radius = 0;
            state = false;
        }
    }

    return state;
}

void Drops::DrawCircle(int16_t x0, int16_t y0, int16_t r, CRGB color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    patternleds[XY(x0, y0 + r)] = color;
    patternleds[XY(x0, y0 - r)] = color;
    patternleds[XY(x0 + r, y0)] = color;
    patternleds[XY(x0 - r, y0)] = color;

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        patternleds[XY(x0 + x, y0 + y)] = color;
        patternleds[XY(x0 - x, y0 + y)] = color;
        patternleds[XY(x0 + x, y0 - y)] = color;
        leds[XY(x0 - x, y0 - y)] = color;
        leds[XY(x0 + y, y0 + x)] = color;
        leds[XY(x0 - y, y0 + x)] = color;
        leds[XY(x0 + y, y0 - x)] = color;
        leds[XY(x0 - y, y0 - x)] = color;
    }
}

#endif // DROPLET_HPP
