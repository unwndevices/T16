#ifndef PATTERN_HPP
#define PATTERN_HPP

#include <Arduino.h>
#include <FastLED.h>

#define WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))

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

class Pattern
{
public:
    Pattern(){};

    virtual bool RunPattern() = 0;
    virtual void SetPosition(uint8_t x, uint8_t y)
    {
        pos_x = x;
        pos_y = y;
        state = true;
    };

    virtual void SetState(bool state)
    {
        this->state = state;
    };

    virtual void SetAmount(float amount){};

    virtual void SetPosition(float x, float y){};

    virtual void SetLed(uint8_t x, uint8_t y, bool state = true){};
    virtual void SetNote(uint8_t note){};
    virtual void SetChord(uint8_t chord){};

    virtual void SetColor(uint8_t color){};
    virtual void SetSpeed(uint8_t speed){};

    virtual void SetStrip(uint8_t strip, float value){};

    void SetPalette(CRGBPalette16 palette)
    {
        currentPalette = palette;
    }

    static CRGBPalette16 currentPalette;

    bool isTransition = false;

protected:
    uint8_t pos_x = 0;
    uint8_t pos_y = 0;
    float amount = 0.0f;
    bool state = false;

    static CRGBPalette16 targetPalette;
};

CRGBPalette16 Pattern::currentPalette(CRGB::Black);
CRGBPalette16 Pattern::targetPalette(CRGB::Black);

#endif // PATTERN_HPP
