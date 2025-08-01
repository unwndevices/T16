#include "RealtimeValidator.hpp"

namespace DataIntegrity {

RealtimeValidator::RealtimeValidator() {
    setupConstraints();
}

void RealtimeValidator::setupConstraints() {
    setupGlobalConstraints();
    setupKeyboardConstraints();
    setupCCConstraints();
    setupSystemConstraints();
    setupCalibrationConstraints();
}

void RealtimeValidator::setupGlobalConstraints() {
    constraints[PARAM_MODE] = {
        .minValue = 0.0f,
        .maxValue = 4.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Device mode (0=Keyboard, 1=XY, 2=Strips, 3=Strum, 4=Settings)"
    };
    
    constraints[PARAM_SENSITIVITY] = {
        .minValue = 0.0f,
        .maxValue = 4.0f,
        .defaultValue = 1.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Global sensitivity level"
    };
    
    constraints[PARAM_BRIGHTNESS] = {
        .minValue = 0.0f,
        .maxValue = 15.0f,
        .defaultValue = 6.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "LED brightness level"
    };
    
    constraints[PARAM_PALETTE] = {
        .minValue = 0.0f,
        .maxValue = 3.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Color palette selection"
    };
    
    constraints[PARAM_MIDI_TRS] = {
        .minValue = 0.0f,
        .maxValue = 1.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "TRS MIDI enabled"
    };
    
    constraints[PARAM_TRS_TYPE] = {
        .minValue = 0.0f,
        .maxValue = 1.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "TRS type (0=Type A, 1=Type B)"
    };
    
    constraints[PARAM_PASSTHROUGH] = {
        .minValue = 0.0f,
        .maxValue = 1.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "MIDI passthrough enabled"
    };
    
    constraints[PARAM_MIDI_BLE] = {
        .minValue = 0.0f,
        .maxValue = 1.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "BLE MIDI enabled"
    };
}

void RealtimeValidator::setupKeyboardConstraints() {
    // Base keyboard constraints (will be used for all banks)
    ParameterConstraints channelConstraint = {
        .minValue = 1.0f,
        .maxValue = 16.0f,
        .defaultValue = 1.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "MIDI channel"
    };
    
    ParameterConstraints scaleConstraint = {
        .minValue = 0.0f,
        .maxValue = 15.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Scale selection"
    };
    
    ParameterConstraints octaveConstraint = {
        .minValue = 0.0f,
        .maxValue = 7.0f,
        .defaultValue = 2.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Base octave"
    };
    
    ParameterConstraints noteConstraint = {
        .minValue = 0.0f,
        .maxValue = 127.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Base note"
    };
    
    ParameterConstraints curveConstraint = {
        .minValue = 0.0f,
        .maxValue = 3.0f,
        .defaultValue = 1.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Velocity/aftertouch curve"
    };
    
    ParameterConstraints boolConstraint = {
        .minValue = 0.0f,
        .maxValue = 1.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Boolean parameter"
    };
    
    // Apply constraints for all banks (0-3)
    for (uint8_t bank = 0; bank < 4; bank++) {
        constraints[makeParameterId(PARAM_KB_CHANNEL, bank)] = channelConstraint;
        constraints[makeParameterId(PARAM_KB_SCALE, bank)] = scaleConstraint;
        constraints[makeParameterId(PARAM_KB_OCTAVE, bank)] = octaveConstraint;
        constraints[makeParameterId(PARAM_KB_BASE_NOTE, bank)] = noteConstraint;
        constraints[makeParameterId(PARAM_KB_VELOCITY_CURVE, bank)] = curveConstraint;
        constraints[makeParameterId(PARAM_KB_AFTERTOUCH_CURVE, bank)] = curveConstraint;
        constraints[makeParameterId(PARAM_KB_FLIP_X, bank)] = boolConstraint;
        constraints[makeParameterId(PARAM_KB_FLIP_Y, bank)] = boolConstraint;
        constraints[makeParameterId(PARAM_KB_KOALA_MODE, bank)] = boolConstraint;
    }
}

void RealtimeValidator::setupCCConstraints() {
    ParameterConstraints ccChannelConstraint = {
        .minValue = 1.0f,
        .maxValue = 16.0f,
        .defaultValue = 1.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "CC MIDI channel"
    };
    
    ParameterConstraints ccIdConstraint = {
        .minValue = 0.0f,
        .maxValue = 127.0f,
        .defaultValue = 1.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "CC number"
    };
    
    // Apply constraints for all banks and CC indices
    for (uint8_t bank = 0; bank < 4; bank++) {
        for (uint8_t cc = 0; cc < 8; cc++) {
            constraints[makeParameterId(PARAM_CC_CHANNEL, bank, cc)] = ccChannelConstraint;
            constraints[makeParameterId(PARAM_CC_ID, bank, cc)] = ccIdConstraint;
        }
    }
}

void RealtimeValidator::setupSystemConstraints() {
    constraints[PARAM_PRESSURE_THRESHOLD] = {
        .minValue = 0.05f,
        .maxValue = 0.5f,
        .defaultValue = 0.18f,
        .customValidator = [this](float v) { 
            return v < currentAftertouchThreshold;
        },
        .sanitizer = [this](float v) { 
            return clampValue(v, 0.05f, min(0.5f, currentAftertouchThreshold - 0.05f));
        },
        .description = "Pressure detection threshold"
    };
    
    constraints[PARAM_AFTERTOUCH_THRESHOLD] = {
        .minValue = 0.1f,
        .maxValue = 0.7f,
        .defaultValue = 0.25f,
        .customValidator = [this](float v) { 
            return v > currentPressureThreshold;
        },
        .sanitizer = [this](float v) { 
            return clampValue(v, max(0.1f, currentPressureThreshold + 0.05f), 0.7f);
        },
        .description = "Aftertouch threshold"
    };
    
    constraints[PARAM_VELOCITY_SENSITIVITY] = {
        .minValue = 0.1f,
        .maxValue = 2.0f,
        .defaultValue = 1.0f,
        .customValidator = [](float v) { return v > 0; },
        .sanitizer = [](float v) { return v <= 0 ? 0.1f : v; },
        .description = "Velocity sensitivity multiplier"
    };
    
    constraints[PARAM_DEBOUNCE_TIME] = {
        .minValue = 0.0f,
        .maxValue = 50.0f,
        .defaultValue = 10.0f,
        .customValidator = [](float v) { return v >= 0; },
        .sanitizer = [](float v) { return v < 0 ? 0 : v; },
        .description = "Key debounce time in ms"
    };
}

void RealtimeValidator::setupCalibrationConstraints() {
    ParameterConstraints calibMinConstraint = {
        .minValue = 0.0f,
        .maxValue = 4095.0f,
        .defaultValue = 0.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Calibration minimum value"
    };
    
    ParameterConstraints calibMaxConstraint = {
        .minValue = 0.0f,
        .maxValue = 4095.0f,
        .defaultValue = 4095.0f,
        .customValidator = isInteger,
        .sanitizer = roundToInteger,
        .description = "Calibration maximum value"
    };
    
    // Apply constraints for all keys (up to 64 for T64)
    for (uint8_t key = 0; key < 64; key++) {
        constraints[makeParameterId(PARAM_CALIBRATION_MIN, 0, key)] = calibMinConstraint;
        constraints[makeParameterId(PARAM_CALIBRATION_MAX, 0, key)] = calibMaxConstraint;
    }
}

ValidationResult RealtimeValidator::validateParameter(uint32_t parameterId, float value) {
    ValidationResult result;
    
    auto constraintIt = constraints.find(parameterId);
    if (constraintIt == constraints.end()) {
        // Check if it's a parameterized constraint (bank/index specific)
        ParameterID base;
        uint8_t bank, index;
        parseParameterId(parameterId, base, bank, index);
        
        // Try to find base constraint
        constraintIt = constraints.find(base);
        if (constraintIt == constraints.end()) {
            result.addWarning({
                .type = ValidationWarning::UNKNOWN_PARAMETER,
                .message = "Parameter not recognized",
                .severity = WarningLevel::WARN_LOW
            });
            return result;
        }
    }
    
    const auto& constraint = constraintIt->second;
    
    // Range check
    if (value < constraint.minValue || value > constraint.maxValue) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .currentValue = static_cast<uint32_t>(value),
            .validRange = {static_cast<uint32_t>(constraint.minValue), 
                          static_cast<uint32_t>(constraint.maxValue)},
            .isAutoFixable = true,
            .suggestedFix = "Clamp to valid range"
        });
    }
    
    // Custom validation
    if (constraint.customValidator && !constraint.customValidator(value)) {
        result.addError({
            .type = ValidationError::CUSTOM_VALIDATION_FAILED,
            .currentValue = static_cast<uint32_t>(value),
            .isAutoFixable = constraint.sanitizer != nullptr,
            .suggestedFix = "Check parameter-specific requirements"
        });
    }
    
    return result;
}

float RealtimeValidator::sanitizeParameter(uint32_t parameterId, float value) {
    auto constraintIt = constraints.find(parameterId);
    if (constraintIt == constraints.end()) {
        // Check if it's a parameterized constraint
        ParameterID base;
        uint8_t bank, index;
        parseParameterId(parameterId, base, bank, index);
        
        constraintIt = constraints.find(base);
        if (constraintIt == constraints.end()) {
            return value; // Unknown parameter - pass through
        }
    }
    
    const auto& constraint = constraintIt->second;
    
    // Apply sanitizer if available
    if (constraint.sanitizer) {
        value = constraint.sanitizer(value);
    }
    
    // Clamp to valid range
    return clampValue(value, constraint.minValue, constraint.maxValue);
}

bool RealtimeValidator::isCriticalParameter(uint32_t parameterId) const {
    ParameterID base;
    uint8_t bank, index;
    parseParameterId(parameterId, base, bank, index);
    
    switch (base) {
        case PARAM_CALIBRATION_MIN:
        case PARAM_CALIBRATION_MAX:
        case PARAM_KB_CHANNEL:
        case PARAM_PRESSURE_THRESHOLD:
        case PARAM_AFTERTOUCH_THRESHOLD:
            return true;
        default:
            return false;
    }
}

const RealtimeValidator::ParameterConstraints* RealtimeValidator::getConstraints(uint32_t parameterId) const {
    auto it = constraints.find(parameterId);
    if (it != constraints.end()) {
        return &it->second;
    }
    
    // Check base parameter
    ParameterID base;
    uint8_t bank, index;
    parseParameterId(parameterId, base, bank, index);
    
    it = constraints.find(base);
    if (it != constraints.end()) {
        return &it->second;
    }
    
    return nullptr;
}

void RealtimeValidator::updateDynamicConstraints(uint32_t parameterId, float newValue) {
    ParameterID base;
    uint8_t bank, index;
    parseParameterId(parameterId, base, bank, index);
    
    // Update dependent constraints
    if (base == PARAM_PRESSURE_THRESHOLD) {
        currentPressureThreshold = newValue;
        // Aftertouch threshold must be higher than pressure threshold
        auto& atConstraint = constraints[PARAM_AFTERTOUCH_THRESHOLD];
        atConstraint.minValue = newValue + 0.05f;
    }
    else if (base == PARAM_AFTERTOUCH_THRESHOLD) {
        currentAftertouchThreshold = newValue;
        // Pressure threshold must be lower than aftertouch threshold
        auto& pressureConstraint = constraints[PARAM_PRESSURE_THRESHOLD];
        pressureConstraint.maxValue = newValue - 0.05f;
    }
}

} // namespace DataIntegrity