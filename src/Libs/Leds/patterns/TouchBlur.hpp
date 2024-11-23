#ifndef TOUCHBLUR_HPP
#define TOUCHBLUR_HPP

#ifdef T32
#define PAD_COUNT 2
#else
#define PAD_COUNT 1
#endif // TOUCHBLUR_HPP

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
        pos_[0].x = x;
        pos_[0].y = y;
        state = true;
    };

    void SetPosition(uint8_t id, float x, float y)
    {
        pos_[id].x = x;
        pos_[id].y = y;
        state = true;
    }

    void SetAmount(uint8_t id, float amt)
    {
        step[id] = static_cast<uint8_t>(amt * 9.0f);
    };

    void SetColor(uint8_t id, uint8_t color)
    {
        colorIndex[id] = color;
    };

    void SetSpeed(uint8_t speed) override {};

private:
    static constexpr uint8_t PAD_SIZE = 4; // Each pad is 4x4
    uint8_t luma[10][16] = {0};            // Keep original 4x4 luma table (16 values)

    void GenerateLut()
    {
        // Generate brightness lookup table for the blur (using blur2d), in a 4x4 grid
        CRGB lumaleds[16];
        fill_solid(lumaleds, 16, CRGB::Black);
        lumaleds[0] = CRGB::White;

        // blur, then store luma values inside the luma array for each led
        for (int j = 0; j < 10; j++)
        {
            blur2d(lumaleds, PAD_SIZE, PAD_SIZE, 2 + j * 4, xy_map);
            lumaleds[0] = CRGB::White;
            for (int i = 0; i < 16; i++)
            {
                luma[9 - j][i] = brighten8_raw(lumaleds[i].getLuma());
            }
        }
    };
    uint8_t colorIndex[PAD_COUNT] = {255};
    uint8_t step[PAD_COUNT] = {0};
    Vec2 pos_[PAD_COUNT] = {{0.0f, 0.0f}};
};

bool TouchBlur::RunPattern()
{
    fill_solid(patternleds, NUM_KEYS, CRGB::Black);
    if (state)
    {
        for (uint8_t id = 0; id < PAD_COUNT; id++)
        {
            // Calculate which pad we're in
            uint8_t pad_offset_x = (id * PAD_SIZE); // For T32, second pad starts at x=4

            for (int x = 0; x < PAD_SIZE; x++)
            {
                for (int y = 0; y < PAD_SIZE; y++)
                {
                    // Calculate the coordinates relative to the touch position within the pad
                    float new_x = (pos_[id].x - (uint8_t)pos_[id].x) + (float)x;
                    // Add the pad offset to the y coordinate
                    float new_y = (pos_[id].y - (uint8_t)pos_[id].y) + (float)y;
                    EVERY_N_MILLISECONDS(250)
                    {
                        log_d("new_x: %f, new_y: %f", new_x, new_y);
                    }

                    // Calculate luma index based on distance from touch within the 4x4 pad
                    uint8_t dx = abs(x - (uint8_t)pos_[id].x);
                    uint8_t dy = abs(y - (uint8_t)pos_[id].y) * PAD_SIZE;
                    uint8_t luma_idx = dx + dy;

                    uint8_t luma_value = luma[step[id]][luma_idx];
                    CRGB color = ColorFromPalette(currentPalette, colorIndex[id], luma_value, LINEARBLEND_NOWRAP);
                    wu_pixel_multipad(static_cast<int32_t>(new_x * (1 << 8)),
                                      static_cast<int32_t>(new_y * (1 << 8)),
                                      &color, id, PAD_SIZE);
                }
            }
        }
    }

    return true;
}

#endif // TOUCHBLUR_HPP
