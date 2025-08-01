#ifndef LED_MATRIX_HPP
#define LED_MATRIX_HPP

#include <Arduino.h>
#include <FastLED.h>
#include <array>
#include "../hardware_config.hpp"
#include "../pinout.h"

template<HardwareVariant variant>
class LedMatrix {
private:
    static constexpr uint8_t NUM_LEDS = HardwareConfig<variant>::NUM_LEDS;
    static constexpr uint8_t MATRIX_ROWS = HardwareConfig<variant>::MATRIX_ROWS;
    static constexpr uint8_t MATRIX_COLS = HardwareConfig<variant>::MATRIX_COLS;
    
    using PinoutType = HardwarePinout<variant>;

public:
    enum class Pattern : uint8_t {
        OFF = 0,
        SOLID,
        PULSE,
        RAINBOW,
        WAVE,
        REACTIVE,
        MATRIX_SCAN,
        KEYBOARD_MODE,
        XY_PAD_MODE,
        STRIPS_MODE,
        STRUM_MODE,
        CALIBRATION_MODE
    };

    struct LedConfig {
        uint8_t brightness = 128;
        CRGB color = CRGB::White;
        Pattern pattern = Pattern::OFF;
        uint16_t speed = 100;
        bool enabled = true;
    };

    LedMatrix() : _leds(), _config(), _lastUpdate(0), _animationStep(0) {
        static_assert(NUM_LEDS <= 64, "Maximum 64 LEDs supported");
    }

    void Init() {
        FastLED.addLeds<WS2812B, PinoutType::Led::data, GRB>(_leds.data(), NUM_LEDS);
        FastLED.setBrightness(_config.brightness);
        FastLED.clear();
        FastLED.show();
    }

    void Update() {
        uint32_t currentTime = millis();
        if (currentTime - _lastUpdate < (1000 / 60)) {
            return;
        }
        _lastUpdate = currentTime;

        switch (_config.pattern) {
            case Pattern::OFF:
                _patternOff();
                break;
            case Pattern::SOLID:
                _patternSolid();
                break;
            case Pattern::PULSE:
                _patternPulse();
                break;
            case Pattern::RAINBOW:
                _patternRainbow();
                break;
            case Pattern::WAVE:
                _patternWave();
                break;
            case Pattern::REACTIVE:
                _patternReactive();
                break;
            case Pattern::MATRIX_SCAN:
                _patternMatrixScan();
                break;
            case Pattern::KEYBOARD_MODE:
                _patternKeyboardMode();
                break;
            case Pattern::XY_PAD_MODE:
                _patternXYPadMode();
                break;
            case Pattern::STRIPS_MODE:
                _patternStripsMode();
                break;
            case Pattern::STRUM_MODE:
                _patternStrumMode();
                break;
            case Pattern::CALIBRATION_MODE:
                _patternCalibrationMode();
                break;
        }

        FastLED.show();
        _animationStep++;
    }

    void SetLed(uint8_t ledIndex, CRGB color) {
        if (ledIndex < NUM_LEDS) {
            _leds[ledIndex] = color;
        }
    }

    void SetLed(uint8_t row, uint8_t col, CRGB color) {
        uint8_t ledIndex = GetLedIndex(row, col);
        SetLed(ledIndex, color);
    }

    CRGB GetLed(uint8_t ledIndex) const {
        if (ledIndex < NUM_LEDS) {
            return _leds[ledIndex];
        }
        return CRGB::Black;
    }

    void SetBrightness(uint8_t brightness) {
        _config.brightness = brightness;
        FastLED.setBrightness(brightness);
    }

    void SetPattern(Pattern pattern) {
        _config.pattern = pattern;
        _animationStep = 0;
    }

    void SetColor(CRGB color) {
        _config.color = color;
    }

    void SetSpeed(uint16_t speed) {
        _config.speed = constrain(speed, 1, 1000);
    }

    void Clear() {
        FastLED.clear();
    }

    void Show() {
        FastLED.show();
    }

    uint8_t GetLedIndex(uint8_t row, uint8_t col) const {
        if (row >= MATRIX_ROWS || col >= MATRIX_COLS) return 0xFF;
        return row * MATRIX_COLS + col;
    }

    void GetLedPosition(uint8_t ledIndex, uint8_t& row, uint8_t& col) const {
        if (ledIndex >= NUM_LEDS) {
            row = col = 0xFF;
            return;
        }
        row = ledIndex / MATRIX_COLS;
        col = ledIndex % MATRIX_COLS;
    }

    void SetKeyPressure(uint8_t keyIndex, float pressure) {
        if (keyIndex < NUM_LEDS) {
            _keyPressures[keyIndex] = constrain(pressure, 0.0f, 1.0f);
        }
    }

    void SetKeyPressed(uint8_t keyIndex, bool pressed) {
        if (keyIndex < NUM_LEDS) {
            _keyPressed[keyIndex] = pressed;
        }
    }

    static constexpr uint8_t GetNumLeds() { return NUM_LEDS; }
    static constexpr uint8_t GetMatrixRows() { return MATRIX_ROWS; }
    static constexpr uint8_t GetMatrixCols() { return MATRIX_COLS; }

private:
    std::array<CRGB, NUM_LEDS> _leds;
    LedConfig _config;
    uint32_t _lastUpdate;
    uint16_t _animationStep;
    
    std::array<float, NUM_LEDS> _keyPressures{};
    std::array<bool, NUM_LEDS> _keyPressed{};

    void _patternOff() {
        FastLED.clear();
    }

    void _patternSolid() {
        fill_solid(_leds.data(), NUM_LEDS, _config.color);
    }

    void _patternPulse() {
        uint8_t pulse = beatsin8(60000 / _config.speed, 0, 255);
        CRGB color = _config.color;
        color.fadeToBlackBy(255 - pulse);
        fill_solid(_leds.data(), NUM_LEDS, color);
    }

    void _patternRainbow() {
        uint8_t hue = (_animationStep * _config.speed / 100) % 256;
        fill_rainbow(_leds.data(), NUM_LEDS, hue, 255 / NUM_LEDS);
    }

    void _patternWave() {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            uint8_t wave = sin8((i * 20) + (_animationStep * _config.speed / 10));
            _leds[i] = ColorFromPalette(RainbowColors_p, wave, wave, LINEARBLEND);
        }
    }

    void _patternReactive() {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (_keyPressed[i]) {
                uint8_t intensity = _keyPressures[i] * 255;
                _leds[i] = ColorFromPalette(RainbowColors_p, (i * 16) % 256, intensity, LINEARBLEND);
            } else {
                _leds[i].fadeToBlackBy(50);
            }
        }
    }

    void _patternMatrixScan() {
        FastLED.clear();
        uint8_t scanPos = (_animationStep * _config.speed / 100) % NUM_LEDS;
        _leds[scanPos] = _config.color;
        
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (i != scanPos) {
                _leds[i].fadeToBlackBy(30);
            }
        }
    }

    void _patternKeyboardMode() {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (_keyPressed[i]) {
                uint8_t intensity = _keyPressures[i] * 255;
                _leds[i] = CHSV(120, 255, intensity);
            } else {
                _leds[i] = CRGB(10, 10, 10);
            }
        }
    }

    void _patternXYPadMode() {
        FastLED.clear();
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (_keyPressed[i]) {
                uint8_t row, col;
                GetLedPosition(i, row, col);
                uint8_t hue = (row * 64) + (col * 32);
                uint8_t intensity = _keyPressures[i] * 255;
                _leds[i] = CHSV(hue, 255, intensity);
            }
        }
    }

    void _patternStripsMode() {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            bool columnActive = false;
            float maxPressure = 0.0f;
            
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                uint8_t ledIndex = GetLedIndex(row, col);
                if (ledIndex < NUM_LEDS && _keyPressed[ledIndex]) {
                    columnActive = true;
                    maxPressure = max(maxPressure, _keyPressures[ledIndex]);
                }
            }
            
            CRGB color = columnActive ? CHSV(col * (255 / MATRIX_COLS), 255, maxPressure * 255) : CRGB::Black;
            
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                uint8_t ledIndex = GetLedIndex(row, col);
                if (ledIndex < NUM_LEDS) {
                    _leds[ledIndex] = color;
                }
            }
        }
    }

    void _patternStrumMode() {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (_keyPressed[i]) {
                uint8_t hue = 60;
                uint8_t intensity = _keyPressures[i] * 255;
                _leds[i] = CHSV(hue, 255, intensity);
            } else {
                _leds[i].fadeToBlackBy(100);
            }
        }
    }

    void _patternCalibrationMode() {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (_keyPressed[i]) {
                _leds[i] = CRGB::Red;
            } else {
                uint8_t intensity = _keyPressures[i] * 128 + 20;
                _leds[i] = CRGB(intensity, intensity, 0);
            }
        }
    }
};

#endif // LED_MATRIX_HPP