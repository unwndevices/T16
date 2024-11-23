#ifndef LEDMANAGER_HPP
#define LEDMANAGER_HPP

#include <Arduino.h>
#include <FastLED.h>
#include "pinout.h"
#include "Configuration.hpp"

#define kMatrixWidth COLS  // Matrix width [4]
#define kMatrixHeight ROWS // Matrix height [4]
#define sliderLength 7
#define NUM_LEDS (kMatrixWidth * kMatrixHeight + sliderLength + 1)

#ifdef T32
uint16_t xy_lut[NUM_KEYS]{0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23, 8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31};
#else
uint16_t xy_lut[kMatrixHeight * kMatrixWidth]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
#endif// LEDMANAGER_HPP

int16_t remap(int16_t id)
{
    return xy_lut[id];
}

int16_t XY(uint8_t x, uint8_t y)
{
    if (x < kMatrixWidth && y < kMatrixHeight)
    {
        return xy_lut[y * kMatrixWidth + x];
    }
    else
    {
        // Return an invalid ID if out of bounds
        return -1;
    }
}

#ifndef REV_B

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);
CRGBSet leds_set(leds, NUM_LEDS);
CRGBSet stateled(leds_set(0, 0));
CRGBSet matrixleds(leds_set(1, 16));
CRGB patternleds[16];
CRGBSet sliderleds(leds_set(17, 23));
#endif// LEDMANAGER_HPP

#ifdef REV_B
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);
CRGBSet leds_set(leds, NUM_LEDS);
CRGBSet stateled(leds_set(0, 0));
CRGBSet sliderleds(leds_set(1, 7));
CRGBSet matrixleds(leds_set(8, NUM_KEYS));
CRGB patternleds[NUM_KEYS];
#endif// LEDMANAGER_HPP

XYMap xy_map = XYMap::constructWithLookUpTable(kMatrixWidth, kMatrixHeight, xy_lut);

#include "patterns/Droplet.hpp"
#include "patterns/Sea.hpp"
#include "patterns/Sea2.hpp"
#include "patterns/NoBlur.hpp"
#include "patterns/TouchBlur.hpp"
#include "patterns/WaveTransition.hpp"
#include "patterns/Strips.hpp"
#include "patterns/Strum.hpp"
#include "patterns/QuickSettings.hpp"
#include "patterns/Calibration.hpp"

class LedManager
{
public:
    LedManager() {};
    void Init()
    {
        FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS);
        FastLED.setCorrection(TypicalLEDStrip);
        FastLED.setTemperature(Candle);
        FastLED.setDither(0);
        FastLED.setBrightness(170);
        fill_solid(leds, NUM_LEDS, CRGB::Black);

        FastLED.show();
        // STARTUP ANIMATION
        // StartupAnimation();
        // currentPattern = new NoBlur();
        currentPattern = new TouchBlur();
        // basePattern = new Sea2();
    }

    void SetMarker(uint8_t idx, bool state)
    {
        is_marker[idx] = state;
        if (state)
            log_d("Marker added at %d", idx);
    }

    void SetLed(uint8_t idx, bool state)
    {
        if (state)
            matrixleds[remap(idx)] = CHSV(HUE_ORANGE, 240, 70);
        else
            matrixleds[remap(idx)] = CRGB::Black;
    }

    void DrawMarkers()
    {
        for (uint8_t i = 0; i < NUM_KEYS; i++)
        {
            if (is_marker[i])
            {

                matrixleds[remap(i)] = ColorFromPalette(Pattern::currentPalette, 0, 64);
            }

            else
            {
                matrixleds[remap(i)] = CRGB::Black;
            }
        }
    }

    void SetBrightness(uint8_t brightness)
    {
        FastLED.setBrightness(brightness);
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

    void SetNote(uint8_t note)
    {
        currentPattern->SetNote(note);
    }

    void SetChord(uint8_t chord)
    {
        currentPattern->SetChord(chord);
    }

    void SetOption(uint8_t option, uint8_t amount)
    {
        currentPattern->SetOption(option, amount);
    }

    void SetValue(uint8_t value, uint8_t amount)
    {
        currentPattern->SetValue(value, amount);
    }

    void SetSlider(float value, bool fill = true, uint8_t fade = 1)
    {
        uint8_t numLedsToLight = static_cast<uint8_t>(value * (sliderLength - 1));
        fadeToBlackBy(sliderleds, 7, fade);

        for (uint8_t i = 0; i < sliderLength; i++)
        {
            if (fill)
            {
                if (i <= numLedsToLight)
                {
                    sliderleds[6 - i] = CHSV(slider_color, 240, 140);
                }
            }
            else
            {
                if (i == numLedsToLight)
                {
                    sliderleds[6 - i] = CHSV(slider_color, 240, 140);
                }
            }
        }
    }

    void SetSlider(uint8_t position, bool fill = false, uint8_t fade = 1)
    {
        fadeToBlackBy(sliderleds, 7, fade);
        if (fill)
        {
            for (uint8_t i = 0; i <= position; i++)
            {
                sliderleds[6 - i] = CHSV(slider_color, 240, 140);
            }
        }
        else
        {
            sliderleds[6 - position] = CHSV(slider_color, 240, 140);
        }
    }

    void SetSliderOff()
    {
        sliderleds = CRGB::Black;
    }

    void SetSliderLed(uint8_t idx, uint8_t intensity, uint8_t steps = 1)
    {
        sliderleds[min(6 - idx * steps, 6)] = CHSV(slider_color, 240, intensity);
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

    void UpdateTransition()
    {
        if (!nextPattern->isTransition)
        {
            currentPattern = nextPattern;
        }
        currentPattern = new WaveTransition(Direction::DOWN);
    }

    void SetSliderHue(uint8_t hue)
    {
        slider_color = hue;
    }

    void SetPalette(CRGBPalette16 palette)
    {
        currentPattern->SetPalette(palette);
    }

    void SetStatus(bool state)
    {
        stateled = state ? CRGB::White : CRGB::Black;
        FastLED.show();
    }

    void TestAll(uint8_t color = HUE_BLUE)
    {
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        {
            leds_set[i] = CHSV(color, 240, 255);
        }
        FastLED.show();
    }

    void OffAll()
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
    }

    void CombineBuffers()
    {
        for (uint8_t i = 0; i < NUM_KEYS; i++)
        {
            matrixleds[i] |= patternleds[i];
        }
    }

private:
    uint8_t pos_x = 0;
    uint8_t pos_y = 0;

    float sliderValue = 0.0f;

    bool state = false;

    Pattern *currentPattern;
    Pattern *nextPattern;

    void StartupAnimation()
    {
        for (uint8_t i = 0; i < NUM_KEYS; i++)
        {
            matrixleds[i] = CHSV(HUE_ORANGE, 240, 70);
            FastLED.show();
            delay(10);
        }
    }

    uint8_t slider_color = HUE_ORANGE;
    bool is_marker[NUM_KEYS] = {false};
};

#endif// LEDMANAGER_HPP
