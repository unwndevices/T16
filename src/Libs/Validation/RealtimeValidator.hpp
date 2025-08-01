#ifndef REALTIME_VALIDATOR_HPP
#define REALTIME_VALIDATOR_HPP

#include <Arduino.h>
#include "DataIntegrity.hpp"
#include <unordered_map>
#include <functional>

namespace DataIntegrity {

// Parameter IDs for validation
enum ParameterID : uint32_t {
    // Global settings
    PARAM_MODE = 0x00000001,
    PARAM_SENSITIVITY = 0x00000002,
    PARAM_BRIGHTNESS = 0x00000003,
    PARAM_PALETTE = 0x00000004,
    PARAM_MIDI_TRS = 0x00000005,
    PARAM_TRS_TYPE = 0x00000006,
    PARAM_PASSTHROUGH = 0x00000007,
    PARAM_MIDI_BLE = 0x00000008,
    
    // Keyboard settings (use bank offset: 0x0100 * bank)
    PARAM_KB_CHANNEL = 0x00000100,
    PARAM_KB_SCALE = 0x00000101,
    PARAM_KB_OCTAVE = 0x00000102,
    PARAM_KB_BASE_NOTE = 0x00000103,
    PARAM_KB_VELOCITY_CURVE = 0x00000104,
    PARAM_KB_AFTERTOUCH_CURVE = 0x00000105,
    PARAM_KB_FLIP_X = 0x00000106,
    PARAM_KB_FLIP_Y = 0x00000107,
    PARAM_KB_KOALA_MODE = 0x00000108,
    
    // CC settings (use bank offset: 0x0200 * bank + cc index)
    PARAM_CC_CHANNEL = 0x00000200,
    PARAM_CC_ID = 0x00000201,
    
    // System parameters
    PARAM_PRESSURE_THRESHOLD = 0x00001000,
    PARAM_AFTERTOUCH_THRESHOLD = 0x00001001,
    PARAM_VELOCITY_SENSITIVITY = 0x00001002,
    PARAM_DEBOUNCE_TIME = 0x00001003,
    
    // Calibration (use key index offset)
    PARAM_CALIBRATION_MIN = 0x00002000,
    PARAM_CALIBRATION_MAX = 0x00002001
};

class RealtimeValidator {
public:
    struct ParameterConstraints {
        float minValue;
        float maxValue;
        float defaultValue;
        std::function<bool(float)> customValidator;
        std::function<float(float)> sanitizer;
        const char* description;
    };
    
    RealtimeValidator();
    
    // Validate parameter in real-time
    ValidationResult validateParameter(uint32_t parameterId, float value);
    
    // Auto-sanitize parameter values
    float sanitizeParameter(uint32_t parameterId, float value);
    
    // Check if parameter needs special handling
    bool isCriticalParameter(uint32_t parameterId) const;
    
    // Get parameter constraints
    const ParameterConstraints* getConstraints(uint32_t parameterId) const;
    
    // Helper to create parameter ID with bank/index
    static uint32_t makeParameterId(ParameterID base, uint8_t bank = 0, uint8_t index = 0) {
        return base | (bank << 8) | (index << 16);
    }
    
    // Extract components from parameter ID
    static void parseParameterId(uint32_t id, ParameterID& base, uint8_t& bank, uint8_t& index) {
        base = static_cast<ParameterID>(id & 0xFF);
        bank = (id >> 8) & 0xFF;
        index = (id >> 16) & 0xFF;
    }
    
    // Update dynamic constraints (e.g., aftertouch threshold based on pressure threshold)
    void updateDynamicConstraints(uint32_t parameterId, float newValue);
    
private:
    std::unordered_map<uint32_t, ParameterConstraints> constraints;
    
    // Current values for cross-parameter validation
    float currentPressureThreshold = 0.18f;
    float currentAftertouchThreshold = 0.25f;
    
    void setupConstraints();
    void setupGlobalConstraints();
    void setupKeyboardConstraints();
    void setupCCConstraints();
    void setupSystemConstraints();
    void setupCalibrationConstraints();
    
    // Helper functions for validators
    static bool isInteger(float value) {
        return fmod(value, 1.0f) == 0.0f;
    }
    
    static float roundToInteger(float value) {
        return round(value);
    }
    
    static float clampValue(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

// Fast validation helper for critical path
class FastValidator {
public:
    // Quick range check only
    static bool quickValidate(uint32_t parameterId, uint32_t value) {
        switch (parameterId & 0xFF) {
            case PARAM_KB_CHANNEL:
            case PARAM_CC_CHANNEL:
                return value >= 1 && value <= 16;
            
            case PARAM_KB_SCALE:
                return value <= 15;
            
            case PARAM_KB_OCTAVE:
                return value <= 7;
            
            case PARAM_CC_ID:
                return value <= 127;
            
            case PARAM_MODE:
                return value <= 4;
            
            case PARAM_SENSITIVITY:
                return value <= 4;
            
            case PARAM_BRIGHTNESS:
                return value <= 15;
            
            default:
                return true; // Unknown parameters pass through
        }
    }
    
    // Quick sanitize for common parameters
    static uint32_t quickSanitize(uint32_t parameterId, uint32_t value) {
        switch (parameterId & 0xFF) {
            case PARAM_KB_CHANNEL:
            case PARAM_CC_CHANNEL:
                if (value < 1) return 1;
                if (value > 16) return 16;
                return value;
            
            case PARAM_KB_SCALE:
                if (value > 15) return 15;
                return value;
            
            case PARAM_KB_OCTAVE:
                if (value > 7) return 7;
                return value;
            
            case PARAM_CC_ID:
                if (value > 127) return 127;
                return value;
            
            default:
                return value;
        }
    }
};

} // namespace DataIntegrity

#endif // REALTIME_VALIDATOR_HPP