#ifndef LED_HPP
#define LED_HPP

#include <Arduino.h>
#include <FastLED.h>
#include "pinout.h"

#define kMatrixWidth 4  // Matrix width [4]
#define kMatrixHeight 4 // Matrix height [4]
#define sliderLength 7
#define NUM_LEDS (kMatrixWidth * kMatrixHeight + sliderLength + 1)

uint16_t XY(uint8_t x, uint8_t y)
{
    if (x >= kMatrixWidth)
        return -1;
    if (y >= kMatrixHeight)
        return -1;

    uint16_t i;

    i = (y * kMatrixWidth) + x;

    return i;
}

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);
CRGBSet leds_set(leds, NUM_LEDS);
CRGBSet stateled(leds_set(0, 0));
CRGBSet matrixleds(leds_set(1, 16));
CRGB patternleds[16];
CRGBSet sliderleds(leds_set(17, 23));
CRGBSet markerleds(leds_set(24, 24));

#include "led/patterns/Droplet.hpp"
#include "led/patterns/Sea.hpp"
#include "led/patterns/Sea2.hpp"
#include "led/patterns/NoBlur.hpp"
#include "led/patterns/TouchBlur.hpp"
#include "led/patterns/WaveTransition.hpp"
#include "led/patterns/Strips.hpp"

class LedManager
{
public:
    LedManager(){};
    void Init()
    {
        FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS);
        FastLED.setCorrection(TypicalLEDStrip);
        // FastLED.setBrightness(170);
        fill_solid(leds, NUM_LEDS, CRGB::Black);

        FastLED.show();
        // STARTUP ANIMATION
        // StartupAnimation();
        // currentPattern = new NoBlur();
        currentPattern = new TouchBlur();
        // basePattern = new Sea2();
    }

    void SetMarker(uint8_t idx)
    {
        matrixleds[idx] = ColorFromPalette(Pattern::currentPalette, 0, 64);
    }

    void RunPattern()
    {
        if (!currentPattern->RunPattern())
        {
            currentPattern = nextPattern;
        }
        CombineBuffers();
    };
    void SetPosition(uint8_t x, uint8_t y)
    {
        currentPattern->SetPosition(x, y);
    }

    void SetPosition(float x, float y)
    {
        currentPattern->SetPosition(x, y);
    }

    void SetStrip(uint8_t strip, float value)
    {
        currentPattern->SetStrip(strip, value);
    }

    void SetState(bool state)
    {
        currentPattern->SetState(state);
    }

    void SetColor(uint8_t color)
    {
        currentPattern->SetColor(color);
    }

    void SetSpeed(uint8_t speed)
    {
        currentPattern->SetSpeed(speed);
    }

    void SetAmount(float amount)
    {
        currentPattern->SetAmount(amount);
    }

    void SetSlider(float value, bool fill = true, uint8_t fade = 1)
    {
        uint8_t numLedsToLight = static_cast<uint8_t>(value * sliderLength + 1);
        fadeToBlackBy(sliderleds, 7, fade);

        for (uint8_t i = 0; i < sliderLength; i++)
        {
            if (fill)
            {
                if (i < numLedsToLight)
                {
                    sliderleds[i] = CHSV(slider_color, 230, 100);
                }
            }
            else
            {
                if (i == numLedsToLight - 1)
                {
                    sliderleds[i] = CHSV(slider_color, 230, 100);
                }
            }
        }
    }

    void SetPattern(Pattern *pattern)
    {
        currentPattern = pattern;
    }

    void TransitionToPattern(Pattern *pattern)
    {
        nextPattern = pattern;
        currentPattern = new WaveTransition();
    }

    void SetSliderHue(uint8_t hue)
    {
        slider_color = hue;
    }

private:
    uint8_t pos_x = 0;
    uint8_t pos_y = 0;

    float sliderValue = 0.0f;

    bool state = false;

    Pattern *currentPattern;
    Pattern *nextPattern;

    void CombineBuffers()
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            matrixleds[i] |= patternleds[i];
        }
    }

    void StartupAnimation()
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            matrixleds[i] = CHSV(HUE_ORANGE, 230, 70);
            FastLED.show();
            delay(10);
        }
    }

    uint8_t slider_color = HUE_ORANGE;
};

#endif// LED_HPP
