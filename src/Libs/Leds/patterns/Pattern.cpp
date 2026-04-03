#include "Pattern.hpp"

// --- Pattern static member definitions ---

CRGBPalette16 Pattern::currentPalette(CRGB::Black);
CRGBPalette16 Pattern::targetPalette(CRGB::Black);

uint8_t Pattern::selectedOption = 0;
uint8_t Pattern::optionAmount = 1;
uint8_t Pattern::selectedValue = 0;
uint8_t Pattern::valueAmount = 1;

// --- wu_pixel free functions ---

void wu_pixel(uint32_t x, uint32_t y, CRGB *col)
{
    // extract the fractional parts and derive their inverses
    uint8_t xx = x & 0xff, yy = y & 0xff, ix = 255 - xx, iy = 255 - yy;
    uint8_t wu[4] = {WEIGHT(ix, iy), WEIGHT(xx, iy),
                     WEIGHT(ix, yy), WEIGHT(xx, yy)};
    // multiply the intensities by the colour, and saturating-add them to the pixels
    for (uint8_t i = 0; i < 4; i++)
    {
        uint16_t xy = XY((x >> 8) + (i & 1), (y >> 8) + ((i >> 1) & 1));

        patternleds[xy].r = qadd8(patternleds[xy].r, (col->r * wu[i]) >> 8);
        patternleds[xy].g = qadd8(patternleds[xy].g, (col->g * wu[i]) >> 8);
        patternleds[xy].b = qadd8(patternleds[xy].b, (col->b * wu[i]) >> 8);
    }
}

void wu_pixel_1d(uint8_t x, uint32_t y, CRGB *col)
{
    // Extract the integer and fractional parts of y
    uint32_t iy = y >> 8;  // Integer part
    uint8_t fy = y & 0xFF; // Fractional part

    // Calculate the inverse of the fractional part
    uint8_t ify = 255 - fy;

    // Calculate the weights for interpolation
    uint8_t w[2] = {WEIGHT(ify, 255), WEIGHT(fy, 255)};

    // Interpolate between two pixels
    for (uint8_t i = 0; i < 2; i++)
    {
        uint16_t xy = XY(x, iy + i); // Calculate the pixel index
        patternleds[xy].r = qadd8(patternleds[xy].r, (col->r * w[i]) >> 8);
        patternleds[xy].g = qadd8(patternleds[xy].g, (col->g * w[i]) >> 8);
        patternleds[xy].b = qadd8(patternleds[xy].b, (col->b * w[i]) >> 8);
    }
}
