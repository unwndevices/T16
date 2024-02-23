#ifndef SEA2_HPP
#define SEA2_HPP

#include "Pattern.hpp"

class Sea2 : public Pattern
{
public:
    Sea2()
    {
        // initialize the x/y and time values
        x = random16();
        y = random16();
        z = random16();
        currentPalette = unwn_gp;
    };
    bool RunPattern() override
    {
        fillnoise8();
        mapNoiseToLEDsUsingPalette();
        return true;
    };

    void fillnoise8()
    {
        // If we're runing at a low "speed", some 8-bit artifacts become visible
        // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
        // The amount of data smoothing we're doing depends on "speed".
        uint8_t dataSmoothing = 0;
        if (speed < 50)
        {
            dataSmoothing = 254 - (speed * 4);
        }

        for (int i = 0; i < 4; i++)
        {
            int ioffset = scale * i;
            for (int j = 0; j < 4; j++)
            {
                int joffset = scale * j;

                uint8_t data = inoise8(x + ioffset, y + joffset, z);

                // The range of the inoise8 function is roughly 16-238.
                // These two operations expand those values out to roughly 0..255
                // You can comment them out if you want the raw noise data.
                data = qsub8(data, 16);
                data = qadd8(data, scale8(data, 39));

                if (dataSmoothing)
                {
                    uint8_t olddata = noise[i][j];
                    uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
                    data = newdata;
                }

                noise[i][j] = data;
            }
        }
        z += speed;
        x += speed / 2;
        y -= speed / 4;

        // apply slow drift to X and Y, just for visual variation.
    }

    void mapNoiseToLEDsUsingPalette()
    {
        static uint8_t ihue = 0;

        for (int i = 0; i < kMatrixHeight; i++)
        {
            for (int j = 0; j < kMatrixWidth; j++)
            {
                // We use the value at the (i,j) coordinate in the noise
                // array for our brightness, and the flipped value from (j,i)
                // for our pixel's index into the color palette.

                uint8_t index = noise[j][i];
                uint8_t bri = noise[i][j];

                // if this palette is a 'loop', add a slowly-changing base value
                if (colorLoop)
                {
                    index += ihue;
                }

                // brighten up, as the color palette itself often contains the
                // light/dark dynamic range desired

                if (bri > 127)
                {
                    bri = 255;
                }
                else
                {
                    bri = dim8_raw(bri);
                }

                CRGB color = ColorFromPalette(currentPalette, index >> 2, bri >> 4);
                patternleds[XY(i, j)] = color;
            }
        }

        ihue += 1;
    }

private:
    uint16_t x;
    uint16_t y;
    uint16_t z;

    uint16_t speed = 1;
    uint16_t scale = 90;
    uint8_t noise[4][4];
    uint8_t colorLoop = 0;
};

#endif // SEA2_HPP