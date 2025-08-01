#ifndef KEY_MATRIX_HPP
#define KEY_MATRIX_HPP

#include <Arduino.h>
#include <array>
#include "../hardware_config.hpp"
#include "MultiMuxAdcManager.hpp"

template<HardwareVariant variant>
class KeyMatrix {
private:
    static constexpr uint8_t NUM_KEYS = HardwareConfig<variant>::NUM_KEYS;
    static constexpr uint8_t MATRIX_ROWS = HardwareConfig<variant>::MATRIX_ROWS;
    static constexpr uint8_t MATRIX_COLS = HardwareConfig<variant>::MATRIX_COLS;

public:
    enum class KeyState : uint8_t {
        RELEASED = 0,
        PRESSED = 1,
        HELD = 2
    };

    struct Key {
        float pressure = 0.0f;
        float velocity = 0.0f;
        float previousPressure = 0.0f;
        KeyState state = KeyState::RELEASED;
        uint32_t pressTime = 0;
        uint32_t releaseTime = 0;
        bool isPressed = false;
        bool wasPressed = false;
        uint8_t note = 60;
        uint8_t channel = 1;
        
        float rateOfChange = 0.0f;
        float peakPressure = 0.0f;
        uint32_t lastUpdateTime = 0;
    };

    KeyMatrix() : _keys(), _adcManager() {
        static_assert(NUM_KEYS <= 64, "Maximum 64 keys supported");
        _initializeKeys();
    }

    void Init() {
        _adcManager.Init();
        
        for (auto& key : _keys) {
            key = Key{};
        }
        
        _setupKeyMapping();
    }

    void SetCalibration(const uint16_t* min, const uint16_t* max) {
        _adcManager.SetCalibration(min, max);
    }

    void GetCalibration(uint16_t* min, uint16_t* max) const {
        _adcManager.GetCalibration(min, max);
    }

    void Update() {
        _adcManager.ReadAllChannels();
        
        uint32_t currentTime = millis();
        
        for (uint8_t i = 0; i < NUM_KEYS; i++) {
            _updateKey(i, currentTime);
        }
    }

    const Key& GetKey(uint8_t keyIndex) const {
        if (keyIndex >= NUM_KEYS) {
            static const Key emptyKey{};
            return emptyKey;
        }
        return _keys[keyIndex];
    }

    Key& GetKey(uint8_t keyIndex) {
        if (keyIndex >= NUM_KEYS) {
            static Key emptyKey{};
            return emptyKey;
        }
        return _keys[keyIndex];
    }

    float GetPressure(uint8_t keyIndex) const {
        if (keyIndex >= NUM_KEYS) return 0.0f;
        return _keys[keyIndex].pressure;
    }

    bool IsPressed(uint8_t keyIndex) const {
        if (keyIndex >= NUM_KEYS) return false;
        return _keys[keyIndex].isPressed;
    }

    bool WasJustPressed(uint8_t keyIndex) const {
        if (keyIndex >= NUM_KEYS) return false;
        const auto& key = _keys[keyIndex];
        return key.isPressed && !key.wasPressed;
    }

    bool WasJustReleased(uint8_t keyIndex) const {
        if (keyIndex >= NUM_KEYS) return false;
        const auto& key = _keys[keyIndex];
        return !key.isPressed && key.wasPressed;
    }

    uint16_t CalibrateMin(uint8_t keyIndex) {
        if (keyIndex >= NUM_KEYS) return 0;
        return _adcManager.CalibrateMin(keyIndex);
    }

    uint16_t CalibrateMax(uint8_t keyIndex) {
        if (keyIndex >= NUM_KEYS) return 0;
        return _adcManager.CalibrateMax(keyIndex);
    }

    void SetFilterWindowSize(uint8_t size) {
        _adcManager.SetFilterWindowSize(size);
    }

    uint8_t GetKeyIndex(uint8_t row, uint8_t col) const {
        if (row >= MATRIX_ROWS || col >= MATRIX_COLS) return 0xFF;
        return row * MATRIX_COLS + col;
    }

    void GetKeyPosition(uint8_t keyIndex, uint8_t& row, uint8_t& col) const {
        if (keyIndex >= NUM_KEYS) {
            row = col = 0xFF;
            return;
        }
        row = keyIndex / MATRIX_COLS;
        col = keyIndex % MATRIX_COLS;
    }

    static constexpr uint8_t GetNumKeys() { return NUM_KEYS; }
    static constexpr uint8_t GetMatrixRows() { return MATRIX_ROWS; }
    static constexpr uint8_t GetMatrixCols() { return MATRIX_COLS; }

private:
    std::array<Key, NUM_KEYS> _keys;
    MultiMuxAdcManager<variant> _adcManager;
    
    static constexpr float PRESS_THRESHOLD = 0.1f;
    static constexpr float RELEASE_THRESHOLD = 0.05f;
    static constexpr uint32_t DEBOUNCE_TIME = 5;

    void _initializeKeys() {
        for (auto& key : _keys) {
            key = Key{};
        }
    }

    void _setupKeyMapping() {
        for (uint8_t i = 0; i < NUM_KEYS; i++) {
            _keys[i].note = 60 + i;
            _keys[i].channel = 1;
        }
    }

    void _updateKey(uint8_t keyIndex, uint32_t currentTime) {
        if (keyIndex >= NUM_KEYS) return;
        
        auto& key = _keys[keyIndex];
        key.wasPressed = key.isPressed;
        key.previousPressure = key.pressure;
        key.pressure = _adcManager.Get(keyIndex);
        
        uint32_t deltaTime = currentTime - key.lastUpdateTime;
        if (deltaTime > 0) {
            key.rateOfChange = (key.pressure - key.previousPressure) / float(deltaTime);
            key.lastUpdateTime = currentTime;
        }
        
        if (key.pressure > key.peakPressure) {
            key.peakPressure = key.pressure;
        }
        
        bool shouldBePressed = key.pressure > PRESS_THRESHOLD;
        bool shouldBeReleased = key.pressure < RELEASE_THRESHOLD;
        
        if (!key.isPressed && shouldBePressed) {
            if (currentTime - key.releaseTime >= DEBOUNCE_TIME) {
                key.isPressed = true;
                key.pressTime = currentTime;
                key.state = KeyState::PRESSED;
                key.peakPressure = key.pressure;
                key.velocity = _calculateVelocity(key);
            }
        } else if (key.isPressed && shouldBeReleased) {
            if (currentTime - key.pressTime >= DEBOUNCE_TIME) {
                key.isPressed = false;
                key.releaseTime = currentTime;
                key.state = KeyState::RELEASED;
                key.peakPressure = 0.0f;
            }
        } else if (key.isPressed) {
            key.state = KeyState::HELD;
        }
    }

    float _calculateVelocity(const Key& key) {
        float velocityFromRate = constrain(abs(key.rateOfChange) * 100, 0.0f, 1.0f);
        float velocityFromPeak = constrain(key.peakPressure, 0.0f, 1.0f);
        
        float weight = 0.7f;
        return weight * velocityFromRate + (1.0f - weight) * velocityFromPeak;
    }
};

#endif // KEY_MATRIX_HPP