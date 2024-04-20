#ifndef SEA_HPP
#define SEA_HPP

#include "Pattern.hpp"

class Sea : public Pattern
{
public:
    Sea()
    {
        // initialize the x/y and time values
        random16_set_seed(8934);
        random16_add_entropy(analogRead(3));

        hxy = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
        x = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
        y = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
        v_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
        hue_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
    };
    bool RunPattern() override
    {
        fadeToBlackBy(matrixleds, 16, 8);
        if (state)
        {
            fill_2dnoise8(matrixleds, kMatrixWidth, kMatrixHeight, true, octaves, x, xscale, y, yscale, v_time,
                          hue_octaves, hxy, hue_scale, hxy, hue_scale, hue_time, true);
        }

        EVERY_N_MILLIS(100)
        {
            x += x_speed;
            y += y_speed;
            v_time += time_speed;
        }
        EVERY_N_MILLIS(300)
        {
            hue_time += hue_speed;
        }
        return true;
    };

private:
    // x,y, & time values
    uint32_t x, y, v_time, hue_time, hxy;

    // Play with the values of the variables below and see what kinds of effects they
    // have!  More octaves will make things slower.

    // how many octaves to use for the brightness and hue functions
    uint8_t octaves = 2;
    uint8_t hue_octaves = 1;

    // the 'distance' between points on the x and y axis
    int xscale = 4000;
    int yscale = 4000;

    // the 'distance' between x/y points for the hue noise
    int hue_scale = 1;

    // how fast we move through time & hue noise
    int time_speed = 3;
    int hue_speed = 1;

    // adjust these values to move along the x or y axis between frames
    int x_speed = 3;
    int y_speed = 3;
};
#endif // SEA_HPP
