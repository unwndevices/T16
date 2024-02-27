#ifndef NOBLUR_HPP
#define NOBLUR_HPP

#include "Pattern.hpp"

class NoBlur : public Pattern
{
public:
    NoBlur()
    {
        currentPalette = topo_gp;
        GenerateLut();
    };
    bool RunPattern() override;

    void SetPosition(uint8_t x, uint8_t y) override
    {
        pos_x = x;
        pos_y = y;
        state = true;
        step = 0;
    };

    void SetColor(uint8_t color) override
    {
        colorIndex = color;
    };

    void SetSpeed(uint8_t speed) override
    {
        this->speed = speed;
    };

private:
    uint8_t colorIndex = 255;
    uint8_t step = 0;

    void GenerateLut()
    {
        // generate brightness lookup table for the blur (using blur2d), in a 4x4 grid.
        CRGB lumaleds[16];
        fill_solid(lumaleds, 16, CRGB::Black);
        lumaleds[0] = CRGB::White;

        // blur, then store luma values inside the luma array for each led
        for (int j = 0; j < 10; j++)
        {
            blur2d(lumaleds, 4, 4, 2 + j * 4);
            lumaleds[0] = CRGB::White;
            for (int i = 0; i < 16; i++)
            {
                luma[j][i] = brighten8_raw(lumaleds[i].getLuma());
            }
        }
    };

    uint8_t luma[10][16] = {0};
    uint8_t speed = 10;
    ulong lastMillis = 0;
};

bool NoBlur::RunPattern()
{
    EVERY_N_MILLIS(45)
    {
        blur2d(patternleds, 4, 4, 40);
    }
    // based on the current position, get the luma value from the lookup table and use it to set the color brightness for the LED
    // the luma array has its center on 0,0, so we need to treat it as the center (pos_x, pos_y) and then mirror it in every direction.
    if (state)
    {
        if (step == 10)
        {
            step = 0;
            state = false;
        }
        else
        {
            for (int x = 0; x < 4; x++)
            {
                for (int y = 0; y < 4; y++)
                {
                    uint8_t luma_value = luma[step][abs(x - (uint8_t)pos_x) + abs(y - (uint8_t)pos_y) * 4];
                    patternleds[XY(x, y)] |= ColorFromPalette(currentPalette, colorIndex, luma_value, LINEARBLEND_NOWRAP);
                }
            }

            if (millis() - lastMillis > speed)
            {
                step++;
                patternleds[XY(pos_x, pos_y)] = ColorFromPalette(currentPalette, colorIndex, 255, LINEARBLEND_NOWRAP);
                lastMillis = millis();
            }
        }
    }
    return true;
}

#endif // NOBLUR_HPP
