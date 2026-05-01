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
CRGBSet matrixleds(leds_set(1, kMatrixWidth * kMatrixHeight));
CRGB patternleds[kMatrixWidth * kMatrixHeight];
CRGBSet sliderleds(leds_set(17, 23));

#endif

#ifdef REV_B

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);
CRGBSet leds_set(leds, NUM_LEDS);
CRGBSet stateled(leds_set(0, 0));
CRGBSet sliderleds(leds_set(1, 7));
CRGBSet matrixleds(leds_set(8, kMatrixWidth * kMatrixHeight));
CRGB patternleds[kMatrixWidth * kMatrixHeight];

#endif

// --- XY helper ---

// XY() is the LOGICAL pattern-space mapping: row-major across the full
// MATRIX_WIDTH × MATRIX_HEIGHT grid. patternleds[] is laid out in this
// logical space, so FastLED 2D helpers (blur2d, fill_2dnoise8, ...) work
// correctly across both variants. The variant-specific physical strip
// translation lives in physicalLedIndex() and is applied only when
// composing onto matrixleds.
uint16_t XY(uint8_t x, uint8_t y)
{
    if (x >= kMatrixWidth)
        return -1;
    if (y >= kMatrixHeight)
        return -1;
    return y * kMatrixWidth + x;
}

// Translate a logical pattern-space index (0..kMatrixSize-1) to the
// physical position within matrixleds. T16 is a single 4×4 block and the
// mapping is the identity; T32 is two horizontally-tiled 4×4 blocks where
// the LED strip walks the leftmost block fully before continuing into the
// next block, so logical column 4 (top of the right block) maps to
// physical position 16 rather than 4.
uint16_t physicalLedIndex(uint16_t logicalIdx)
{
    if constexpr (kMatrixBlockCount == 1)
    {
        return logicalIdx;
    }
    else
    {
        const uint8_t  x          = logicalIdx % kMatrixWidth;
        const uint8_t  y          = logicalIdx / kMatrixWidth;
        const uint8_t  block      = x / kMatrixBlockWidth;
        const uint8_t  localX     = x - block * kMatrixBlockWidth;
        const uint16_t blockBase  = block * (kMatrixBlockWidth * kMatrixHeight);
        return blockBase + y * kMatrixBlockWidth + localX;
    }
}

// --- LedManager implementation ---

LedManager::LedManager() = default;
LedManager::~LedManager() = default;

void LedManager::Init()
{
    FastLED.addLeds<WS2812B, CurrentPinout::LED_PIN, GRB>(leds, NUM_LEDS);
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
    const uint16_t p = physicalLedIndex(idx);
    if (state)
        matrixleds[p] = CHSV(HUE_ORANGE, 240, 70);
    else
        matrixleds[p] = CRGB::Black;
}

void LedManager::DrawMarkers()
{
    for (uint8_t i = 0; i < kMatrixSize; i++)
    {
        const uint16_t p = physicalLedIndex(i);
        if (is_marker[i])
        {
            matrixleds[p] = ColorFromPalette(Pattern::currentPalette, 0, 64);
        }
        else
        {
            matrixleds[p] = CRGB::Black;
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
        // Only advance if a successor was queued. Without this guard a
        // transition with no queued follow-up (e.g. UpdateTransition() entering
        // the else-branch with nextPattern_ already null) leaves currentPattern_
        // null on the next frame and dereferencing it panics the device.
        if (nextPattern_)
        {
            currentPattern_ = std::move(nextPattern_);
        }
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
    else if (nextPattern_)
    {
        // A transition with a queued follow-up: kick off the wipe.
        currentPattern_ = std::make_unique<WaveTransition>(Direction::DOWN);
    }
    // else: nothing queued — leave currentPattern_ alone. Starting a
    // transition with no successor would leave RunPattern() with nothing to
    // advance to once the wipe finishes (previous behaviour: null deref).
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
    // Compose pattern-space buffer (logical row-major) onto the physical
    // strip view (variant-specific). On T16 this is a 1:1 OR; on T32 the
    // logical-to-physical translation reorders cells across the two blocks.
    for (uint8_t i = 0; i < kMatrixSize; i++)
    {
        matrixleds[physicalLedIndex(i)] |= patternleds[i];
    }
}

void LedManager::StartupAnimation()
{
    for (uint8_t i = 0; i < kMatrixSize; i++)
    {
        matrixleds[physicalLedIndex(i)] = CHSV(HUE_ORANGE, 240, 70);
        FastLED.show();
        delay(10);
    }
}
