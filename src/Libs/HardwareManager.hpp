#ifndef HARDWARE_MANAGER_HPP
#define HARDWARE_MANAGER_HPP

#include <Arduino.h>
#include "../hardware_config.hpp"
#include "../pinout.h"
#include "KeyMatrix.hpp"
#include "LedMatrix.hpp"
#include "TouchSlider.hpp"

template<HardwareVariant variant>
class HardwareManager {
private:
    static constexpr uint8_t NUM_KEYS = HardwareConfig<variant>::NUM_KEYS;

public:
    HardwareManager() : _keyMatrix(), _ledMatrix(), _touchSlider() {}

    void Init() {
        _keyMatrix.Init();
        _ledMatrix.Init();
        _touchSlider.init();
    }

    void Update() {
        _keyMatrix.Update();
        _ledMatrix.Update();
        _touchSlider.update();
        
        _updateLedFeedback();
    }

    KeyMatrix<variant>& GetKeyMatrix() { return _keyMatrix; }
    const KeyMatrix<variant>& GetKeyMatrix() const { return _keyMatrix; }

    LedMatrix<variant>& GetLedMatrix() { return _ledMatrix; }
    const LedMatrix<variant>& GetLedMatrix() const { return _ledMatrix; }

    TouchSlider& GetTouchSlider() { return _touchSlider; }
    const TouchSlider& GetTouchSlider() const { return _touchSlider; }

    void SetCalibration(const uint16_t* min, const uint16_t* max) {
        _keyMatrix.SetCalibration(min, max);
    }

    void GetCalibration(uint16_t* min, uint16_t* max) const {
        _keyMatrix.GetCalibration(min, max);
    }

    uint16_t CalibrateMin(uint8_t keyIndex) {
        return _keyMatrix.CalibrateMin(keyIndex);
    }

    uint16_t CalibrateMax(uint8_t keyIndex) {
        return _keyMatrix.CalibrateMax(keyIndex);
    }

    void CalibrationRoutine() {
        _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::CALIBRATION_MODE);
        
        for (uint8_t i = 0; i < NUM_KEYS; i++) {
            _keyMatrix.CalibrateMin(i);
            _keyMatrix.CalibrateMax(i);
        }
    }

    void SetMode(uint8_t mode) {
        switch (mode) {
            case 0: // Keyboard mode
                _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::KEYBOARD_MODE);
                break;
            case 1: // XY Pad mode
                _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::XY_PAD_MODE);
                break;
            case 2: // Strips mode
                _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::STRIPS_MODE);
                break;
            case 3: // Strum mode
                _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::STRUM_MODE);
                break;
            case 4: // Quick Settings
                _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::MATRIX_SCAN);
                break;
            default:
                _ledMatrix.SetPattern(LedMatrix<variant>::Pattern::SOLID);
                break;
        }
    }

    static constexpr uint8_t GetNumKeys() { return NUM_KEYS; }
    static constexpr HardwareVariant GetVariant() { return variant; }

private:
    KeyMatrix<variant> _keyMatrix;
    LedMatrix<variant> _ledMatrix;
    TouchSlider _touchSlider;

    void _updateLedFeedback() {
        for (uint8_t i = 0; i < NUM_KEYS; i++) {
            const auto& key = _keyMatrix.GetKey(i);
            _ledMatrix.SetKeyPressure(i, key.pressure);
            _ledMatrix.SetKeyPressed(i, key.isPressed);
        }
    }
};

using CurrentHardwareManager = HardwareManager<CURRENT_VARIANT>;

#endif // HARDWARE_MANAGER_HPP