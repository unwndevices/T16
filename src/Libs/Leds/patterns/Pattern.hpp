#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "../Palettes.hpp"

#define WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))

// Declared in LedManager.cpp
extern CRGB patternleds[16];
extern uint16_t XY(uint8_t x, uint8_t y);

void wu_pixel(uint32_t x, uint32_t y, CRGB *col);
void wu_pixel_1d(uint8_t x, uint32_t y, CRGB *col);

class Pattern
{
public:
    Pattern() {};
    virtual ~Pattern() = default;

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

    virtual void SetAmount(float amount) {};

    virtual void SetPosition(float x, float y) {};

    virtual void SetLed(uint8_t x, uint8_t y, bool state = true) {};
    virtual void SetNote(uint8_t note) {};
    virtual void SetChord(uint8_t chord) {};

    virtual void SetColor(uint8_t color) {};
    virtual void SetSpeed(uint8_t speed) {};
    virtual void SetOption(uint8_t option, uint8_t amount)
    {
        selectedOption = option;
        optionAmount = amount;
    };
    virtual void SetValue(uint8_t value, uint8_t amount)
    {
        selectedValue = value;
        valueAmount = amount;
    };

    virtual void SetStrip(uint8_t strip, float value) {};

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

    // for quick settings
    static uint8_t selectedOption;
    static uint8_t optionAmount;
    static uint8_t selectedValue;
    static uint8_t valueAmount;

    static CRGBPalette16 targetPalette;
};
