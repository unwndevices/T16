#include "LedManager.hpp"
#include "patterns/Pattern.hpp"
#include "patterns/Droplet.hpp"
#include "patterns/Sea.hpp"
#include "patterns/Sea2.hpp"
#include "patterns/NoBlur.hpp"
#include "patterns/TouchBlur.hpp"
#include "patterns/WaveTransition.hpp"
#include "patterns/Strips.hpp"
#include "patterns/Strum.hpp"
#include "patterns/QuickSettings.hpp"

// --- LED array definitions ---

#ifndef REV_B

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);
CRGBSet leds_set(leds, NUM_LEDS);
CRGBSet stateled(leds_set(0, 0));
CRGBSet matrixleds(leds_set(1, 16));
CRGB patternleds[16];
CRGBSet sliderleds(leds_set(17, 23));

#endif

#ifdef REV_B

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);
CRGBSet leds_set(leds, NUM_LEDS);
CRGBSet stateled(leds_set(0, 0));
CRGBSet sliderleds(leds_set(1, 7));
CRGBSet matrixleds(leds_set(8, 16));
CRGB patternleds[16];

#endif

// --- XY helper ---

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

// --- LedManager implementation ---

LedManager::LedManager() = default;
LedManager::~LedManager() = default;

void LedManager::Init()
{
    FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS);
    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setTemperature(Candle);
    FastLED.setDither(0);
    FastLED.setBrightness(170);
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    FastLED.show();
    currentPattern_ = std::make_unique<TouchBlur>();
}

void LedManager::SetMarker(uint8_t idx, bool state)
{
    is_marker[idx] = state;
    if (state)
        log_d("Marker added at %d", idx);
}

void LedManager::SetLed(uint8_t idx, bool state)
{
    if (state)
        matrixleds[idx] = CHSV(HUE_ORANGE, 240, 70);
    else
        matrixleds[idx] = CRGB::Black;
}

void LedManager::DrawMarkers()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        if (is_marker[i])
        {
            matrixleds[i] = ColorFromPalette(Pattern::currentPalette, 0, 64);
        }

        else
        {
            matrixleds[i] = CRGB::Black;
        }
    }
}

void LedManager::SetBrightness(uint8_t brightness)
{
    FastLED.setBrightness(brightness);
}

void LedManager::RunPattern()
{
    if (!currentPattern_->RunPattern())
    {
        currentPattern_ = std::move(nextPattern_);
    }
    CombineBuffers();
}

void LedManager::SetPosition(uint8_t x, uint8_t y)
{
    currentPattern_->SetPosition(x, y);
}

void LedManager::SetPosition(float x, float y)
{
    currentPattern_->SetPosition(x, y);
}

void LedManager::SetStrip(uint8_t strip, float value)
{
    currentPattern_->SetStrip(strip, value);
}

void LedManager::SetState(bool state)
{
    currentPattern_->SetState(state);
}

void LedManager::SetColor(uint8_t color)
{
    currentPattern_->SetColor(color);
}

void LedManager::SetSpeed(uint8_t speed)
{
    currentPattern_->SetSpeed(speed);
}

void LedManager::SetAmount(float amount)
{
    currentPattern_->SetAmount(amount);
}

void LedManager::SetNote(uint8_t note)
{
    currentPattern_->SetNote(note);
}

void LedManager::SetChord(uint8_t chord)
{
    currentPattern_->SetChord(chord);
}

void LedManager::SetOption(uint8_t option, uint8_t amount)
{
    currentPattern_->SetOption(option, amount);
}

void LedManager::SetValue(uint8_t value, uint8_t amount)
{
    currentPattern_->SetValue(value, amount);
}

void LedManager::SetSlider(float value, bool fill, uint8_t fade)
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

void LedManager::SetSlider(uint8_t position, bool fill, uint8_t fade)
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

void LedManager::SetSliderOff()
{
    sliderleds = CRGB::Black;
}

void LedManager::SetSliderLed(uint8_t idx, uint8_t intensity, uint8_t steps)
{
    sliderleds[min(6 - idx * steps, 6)] = CHSV(slider_color, 240, intensity);
}

void LedManager::SetPattern(std::unique_ptr<Pattern> pattern)
{
    currentPattern_ = std::move(pattern);
}

void LedManager::TransitionToPattern(std::unique_ptr<Pattern> pattern)
{
    nextPattern_ = std::move(pattern);
    currentPattern_ = std::make_unique<WaveTransition>();
}

void LedManager::TransitionToModePattern(Mode mode)
{
    switch (mode)
    {
    case KEYBOARD:
        TransitionToPattern(std::make_unique<NoBlur>());
        break;
    case XY_PAD:
        TransitionToPattern(std::make_unique<TouchBlur>());
        break;
    case STRIPS:
        TransitionToPattern(std::make_unique<Strips>());
        break;
    case STRUM:
        TransitionToPattern(std::make_unique<Strum>());
        break;
    case QUICK_SETTINGS:
        TransitionToPattern(std::make_unique<QuickSettings>());
        break;
    default:
        TransitionToPattern(std::make_unique<NoBlur>());
        break;
    }
}

void LedManager::UpdateTransition()
{
    if (nextPattern_ && !nextPattern_->isTransition)
    {
        currentPattern_ = std::move(nextPattern_);
    }
    currentPattern_ = std::make_unique<WaveTransition>(Direction::DOWN);
}

void LedManager::SetSliderHue(uint8_t hue)
{
    slider_color = hue;
}

void LedManager::SetPalette(CRGBPalette16 palette)
{
    currentPattern_->SetPalette(palette);
}

void LedManager::SetStatus(bool state)
{
    stateled = state ? CRGB::White : CRGB::Black;
    FastLED.show();
}

void LedManager::TestAll(uint8_t color)
{
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        leds_set[i] = CHSV(color, 240, 255);
    }
    FastLED.show();
}

void LedManager::OffAll()
{
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void LedManager::CombineBuffers()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        matrixleds[i] |= patternleds[i];
    }
}

void LedManager::StartupAnimation()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        matrixleds[i] = CHSV(HUE_ORANGE, 240, 70);
        FastLED.show();
        delay(10);
    }
}
