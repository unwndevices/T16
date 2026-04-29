#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <memory>
#include "pinout.h"
#include "../../variant.hpp"
#include "../../Types.hpp"

inline constexpr uint8_t kMatrixWidth  = variant::CurrentVariant::kConfig.MATRIX_WIDTH;
inline constexpr uint8_t kMatrixHeight = variant::CurrentVariant::kConfig.MATRIX_HEIGHT;
inline constexpr uint8_t sliderLength  = variant::CurrentVariant::kConfig.SLIDER_LENGTH;
inline constexpr uint8_t NUM_LEDS      = variant::CurrentVariant::kConfig.LED_COUNT;

// LED arrays -- defined in LedManager.cpp
extern CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
extern CRGB *const leds;
extern CRGBSet leds_set;
extern CRGBSet stateled;
extern CRGBSet matrixleds;
extern CRGB patternleds[kMatrixWidth * kMatrixHeight];
extern CRGBSet sliderleds;

uint16_t XY(uint8_t x, uint8_t y);

// Forward declare Pattern to avoid including all pattern headers
class Pattern;

class LedManager
{
public:
    LedManager();
    ~LedManager();

    void Init();

    void SetMarker(uint8_t idx, bool state);
    void SetLed(uint8_t idx, bool state);
    void DrawMarkers();
    void SetBrightness(uint8_t brightness);

    void RunPattern();
    void SetPosition(uint8_t x, uint8_t y);
    void SetPosition(float x, float y);
    void SetStrip(uint8_t strip, float value);
    void SetState(bool state);
    void SetColor(uint8_t color);
    void SetSpeed(uint8_t speed);
    void SetAmount(float amount);
    void SetNote(uint8_t note);
    void SetChord(uint8_t chord);
    void SetOption(uint8_t option, uint8_t amount);
    void SetValue(uint8_t value, uint8_t amount);

    void SetSlider(float value, bool fill = true, uint8_t fade = 1);
    void SetSlider(uint8_t position, bool fill = false, uint8_t fade = 1);
    void SetSliderOff();
    void SetSliderLed(uint8_t idx, uint8_t intensity, uint8_t steps = 1);

    void SetPattern(std::unique_ptr<Pattern> pattern);
    void TransitionToPattern(std::unique_ptr<Pattern> pattern);
    void TransitionToModePattern(Mode mode);
    void UpdateTransition();

    void SetSliderHue(uint8_t hue);
    void SetPalette(CRGBPalette16 palette);
    void SetStatus(bool state);
    void TestAll(uint8_t color = HUE_BLUE);
    void OffAll();

private:
    uint8_t pos_x = 0;
    uint8_t pos_y = 0;

    float sliderValue = 0.0f;

    bool state = false;

    std::unique_ptr<Pattern> currentPattern_;
    std::unique_ptr<Pattern> nextPattern_;

    void CombineBuffers();
    void StartupAnimation();

    uint8_t slider_color = HUE_ORANGE;
    bool is_marker[16] = {false};
};
