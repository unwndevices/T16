#ifndef TOUCHBLUR_HPP
#define TOUCHBLUR_HPP

#include "Pattern.hpp"

class TouchBlur : public Pattern
{
public:
    TouchBlur()
    {
        currentPalette = unwn_gp;
        GenerateLut();
        state = true;
    };
    bool RunPattern() override;

    void SetPosition(uint8_t x, uint8_t y) override {};

    void SetPosition(float x, float y) override
    {
        pos_x = x;
        pos_y = y;
        state = true;
    };

    void SetAmount(float amount) override
    {
        this->amount = amount;
        step = static_cast<uint8_t>(amount * 9.0f);
    };

    void SetColor(uint8_t color) override
    {
        colorIndex = color;
    };

    void SetSpeed(uint8_t speed) override {};

private:
    uint8_t luma[10][16] = {0};

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
                luma[9 - j][i] = brighten8_raw(lumaleds[i].getLuma());
            }
        }
    };
    uint8_t colorIndex = 255;
    uint8_t step = 0;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
};

bool TouchBlur::RunPattern()
{
    fill_solid(patternleds, 16, CRGB::Black);
    if (state)
    {
        // Assuming step is set externally before calling this function

        for (int x = 0; x < 4; x++)
        {
            for (int y = 0; y < 4; y++)
            {
                // Calculate the coordinates of the neighboring pixels
                float new_x = (pos_x - (uint8_t)pos_x) + (float)x;
                float new_y = (pos_y - (uint8_t)pos_y) + (float)y;

                // Calculate the weight for the center pixel
                uint8_t luma_value = luma[step][abs(x - (uint8_t)pos_x) + abs(y - (uint8_t)pos_y) * 4];
                CRGB color = ColorFromPalette(currentPalette, colorIndex, luma_value, LINEARBLEND_NOWRAP);
                wu_pixel(static_cast<int32_t>(new_x * (1 << 8)), static_cast<int32_t>(new_y * (1 << 8)), &color);
            }
        }
    }

    return true;
}

#endif // TOUCHBLUR_HPP
