#include "ConfigurationValidator.hpp"

namespace DataIntegrity {

// Helper function for clamping values
static uint8_t clampToRange(uint8_t value, uint8_t min, uint8_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

ValidationResult ConfigurationValidator::validateConfiguration(const ConfigurationData& config, 
                                                             const ValidationContext& context) {
    ValidationResult result;
    
    // Skip validation if not in proper mode
    if (context.level == BASIC) {
        // Just check version
        if (config.version < MIN_FIRMWARE_VERSION) {
            result.addError({
                .type = ValidationError::FIRMWARE_TOO_OLD,
                .currentValue = config.version,
                .validRange = {MIN_FIRMWARE_VERSION, 255},
                .isAutoFixable = false,
                .message = "Configuration requires newer firmware"
            });
        }
        return result;
    }
    
    // Layer 1: Basic range validation
    validateBasicRanges(config, result);
    
    // For comprehensive validation, we need the full configuration arrays
    // Since we only have ConfigurationData here, we'll validate what we can
    
    // Validate mode
    if (config.mode > 4) { // Assuming 5 modes (0-4)
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "mode"},
            .currentValue = config.mode,
            .validRange = {0, 4},
            .isAutoFixable = true,
            .suggestedFix = "Reset to keyboard mode (0)"
        });
    }
    
    // Validate sensitivity
    if (config.sensitivity > 4) { // Assuming 5 sensitivity levels (0-4)
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "sensitivity"},
            .currentValue = config.sensitivity,
            .validRange = {0, 4},
            .isAutoFixable = true,
            .suggestedFix = "Reset to default sensitivity (1)"
        });
    }
    
    // Validate brightness
    if (config.brightness > 15) { // Assuming 16 brightness levels (0-15)
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "brightness"},
            .currentValue = config.brightness,
            .validRange = {0, 15},
            .isAutoFixable = true,
            .suggestedFix = "Clamp to maximum brightness (15)"
        });
    }
    
    // Validate MIDI settings
    if (config.midi_trs > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "midi_trs"},
            .currentValue = config.midi_trs,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to disabled (0)"
        });
    }
    
    if (config.trs_type > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "trs_type"},
            .currentValue = config.trs_type,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to Type A (0)"
        });
    }
    
    if (config.passthrough > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "passthrough"},
            .currentValue = config.passthrough,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to disabled (0)"
        });
    }
    
    if (config.midi_ble > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "midi_ble"},
            .currentValue = config.midi_ble,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to disabled (0)"
        });
    }
    
    // Validate custom scales
    result = validateCustomScale(config.custom_scale1, 1, context);
    result = validateCustomScale(config.custom_scale2, 2, context);
    
    return result;
}

ValidationResult ConfigurationValidator::validateKeyModeData(const KeyModeData& kb_cfg, uint8_t bank,
                                                           const ValidationContext& context) {
    ValidationResult result;
    
    // Validate MIDI channel
    if (!isValidMidiChannel(kb_cfg.channel)) {
        result.addError({
            .type = ValidationError::INVALID_MIDI_CHANNEL,
            .location = {.bank = bank, .parameter = "channel"},
            .currentValue = kb_cfg.channel,
            .validRange = {1, 16},
            .isAutoFixable = true,
            .suggestedFix = "Clamp to valid range [1-16]"
        });
    }
    
    // Validate scale
    if (!isValidScale(kb_cfg.scale)) {
        result.addError({
            .type = ValidationError::INVALID_SCALE,
            .location = {.bank = bank, .parameter = "scale"},
            .currentValue = kb_cfg.scale,
            .validRange = {0, MAX_SCALE_INDEX},
            .isAutoFixable = true,
            .suggestedFix = "Reset to chromatic scale (0)"
        });
    }
    
    // Validate octave
    if (!isValidOctave(kb_cfg.base_octave)) {
        result.addError({
            .type = ValidationError::INVALID_OCTAVE,
            .location = {.bank = bank, .parameter = "base_octave"},
            .currentValue = kb_cfg.base_octave,
            .validRange = {0, MAX_OCTAVE},
            .isAutoFixable = true,
            .suggestedFix = "Clamp to middle octaves [0-7]"
        });
    }
    
    // Validate base note
    if (kb_cfg.base_note > 127) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.bank = bank, .parameter = "base_note"},
            .currentValue = kb_cfg.base_note,
            .validRange = {0, 127},
            .isAutoFixable = true,
            .suggestedFix = "Clamp to MIDI note range [0-127]"
        });
    }
    
    // Validate velocity curve
    if (!isValidVelocityCurve(kb_cfg.velocity_curve)) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.bank = bank, .parameter = "velocity_curve"},
            .currentValue = kb_cfg.velocity_curve,
            .validRange = {0, 3},
            .isAutoFixable = true,
            .suggestedFix = "Reset to default curve (1)"
        });
    }
    
    // Validate aftertouch curve
    if (!isValidAftertouchCurve(kb_cfg.aftertouch_curve)) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.bank = bank, .parameter = "aftertouch_curve"},
            .currentValue = kb_cfg.aftertouch_curve,
            .validRange = {0, 3},
            .isAutoFixable = true,
            .suggestedFix = "Reset to default curve (1)"
        });
    }
    
    // Validate flip settings
    if (kb_cfg.flip_x > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.bank = bank, .parameter = "flip_x"},
            .currentValue = kb_cfg.flip_x,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to non-flipped (0)"
        });
    }
    
    if (kb_cfg.flip_y > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.bank = bank, .parameter = "flip_y"},
            .currentValue = kb_cfg.flip_y,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to non-flipped (0)"
        });
    }
    
    // Validate koala mode
    if (kb_cfg.koala_mode > 1) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.bank = bank, .parameter = "koala_mode"},
            .currentValue = kb_cfg.koala_mode,
            .validRange = {0, 1},
            .isAutoFixable = true,
            .suggestedFix = "Disable koala mode (0)"
        });
    }
    
    // Hardware compatibility check for koala mode
    if (kb_cfg.koala_mode && context.variant != HardwareVariant::T16) {
        result.addError({
            .type = ValidationError::HARDWARE_INCOMPATIBLE,
            .location = {.bank = bank, .parameter = "koala_mode"},
            .isAutoFixable = true,
            .message = "Koala mode only available on T16 variant"
        });
    }
    
    return result;
}

ValidationResult ConfigurationValidator::validateControlChangeData(const ControlChangeData& cc_cfg, uint8_t bank,
                                                                 const ValidationContext& context) {
    ValidationResult result;
    std::unordered_set<uint8_t> usedCCs;
    
    for (int i = 0; i < CC_AMT; i++) {
        // Validate channel
        if (!isValidMidiChannel(cc_cfg.channel[i])) {
            result.addError({
                .type = ValidationError::INVALID_MIDI_CHANNEL,
                .location = {.bank = bank, .parameter = "cc_channel", .index = static_cast<uint8_t>(i)},
                .currentValue = cc_cfg.channel[i],
                .validRange = {1, 16},
                .isAutoFixable = true,
                .suggestedFix = "Reset to channel 1"
            });
        }
        
        // Validate CC ID
        if (!isValidCCId(cc_cfg.id[i])) {
            result.addError({
                .type = ValidationError::INVALID_CC_ID,
                .location = {.bank = bank, .parameter = "cc_id", .index = static_cast<uint8_t>(i)},
                .currentValue = cc_cfg.id[i],
                .validRange = {0, MAX_CC_ID},
                .isAutoFixable = true,
                .suggestedFix = "Reset to default CC mapping"
            });
        }
        
        // Check for duplicate CC IDs
        if (context.level >= COMPREHENSIVE) {
            if (usedCCs.find(cc_cfg.id[i]) != usedCCs.end()) {
                result.addWarning({
                    .type = ValidationWarning::DUPLICATE_CC_ID,
                    .message = "Multiple CCs assigned to same ID",
                    .location = {.bank = bank, .parameter = "cc_id", .index = static_cast<uint8_t>(i)},
                    .severity = WarningLevel::WARN_HIGH
                });
            } else {
                usedCCs.insert(cc_cfg.id[i]);
            }
        }
    }
    
    return result;
}

ValidationResult ConfigurationValidator::validateCalibrationData(const CalibrationData& calibration,
                                                               const ValidationContext& context) {
    ValidationResult result;
    
    uint8_t numKeys = 16;
    if (context.variant == HardwareVariant::T32) numKeys = 32;
    else if (context.variant == HardwareVariant::T64) numKeys = 64;
    
    for (uint8_t i = 0; i < numKeys && i < 16; i++) { // CalibrationData only has 16 entries
        // Check if min > max (inverted calibration)
        if (calibration.minVal[i] > calibration.maxVal[i]) {
            result.addError({
                .type = ValidationError::OUT_OF_RANGE,
                .location = {.parameter = "calibration", .index = i},
                .currentValue = calibration.minVal[i],
                .validRange = {0, calibration.maxVal[i]},
                .isAutoFixable = false,
                .message = "Calibration min value exceeds max value"
            });
        }
        
        // Check for reasonable ADC values (12-bit ADC = 0-4095)
        if (calibration.maxVal[i] > 4095) {
            result.addError({
                .type = ValidationError::OUT_OF_RANGE,
                .location = {.parameter = "calibration_max", .index = i},
                .currentValue = calibration.maxVal[i],
                .validRange = {0, 4095},
                .isAutoFixable = true,
                .suggestedFix = "Clamp to ADC maximum (4095)"
            });
        }
        
        // Check for suspiciously narrow calibration range
        uint16_t range = calibration.maxVal[i] - calibration.minVal[i];
        if (range < 100 && calibration.maxVal[i] > 0) {
            result.addWarning({
                .type = ValidationWarning::UNKNOWN_PARAMETER,
                .message = "Suspiciously narrow calibration range",
                .location = {.parameter = "calibration", .index = i},
                .severity = WarningLevel::MODERATE
            });
        }
    }
    
    return result;
}

ValidationResult ConfigurationValidator::validateCustomScale(const int8_t* scale, uint8_t scaleNum,
                                                           const ValidationContext& context) {
    ValidationResult result;
    
    // Check if scale is defined (first note != 0xFF)
    if (scale[0] == static_cast<int8_t>(0xFF)) {
        return result; // Scale not defined, skip validation
    }
    
    for (int i = 0; i < 16; i++) {
        if (!isValidScaleNote(scale[i])) {
            result.addError({
                .type = ValidationError::INVALID_SCALE_NOTE,
                .location = {.parameter = (scaleNum == 1) ? "custom_scale1" : "custom_scale2", .index = static_cast<uint8_t>(i)},
                .currentValue = static_cast<uint32_t>(scale[i]),
                .validRange = {static_cast<uint32_t>(-MAX_SCALE_NOTE_OFFSET), MAX_SCALE_NOTE_OFFSET},
                .isAutoFixable = true,
                .suggestedFix = "Clamp to reasonable range [-24, +24]"
            });
        }
    }
    
    return result;
}

ValidationResult ConfigurationValidator::validateParameter(const char* paramName, uint32_t value,
                                                         const ValidationContext& context) {
    ValidationResult result;
    
    // Quick parameter validation for real-time updates
    if (strcmp(paramName, "channel") == 0) {
        if (!isValidMidiChannel(value)) {
            result.addError({
                .type = ValidationError::INVALID_MIDI_CHANNEL,
                .location = {.parameter = paramName},
                .currentValue = value,
                .validRange = {1, 16},
                .isAutoFixable = true,
                .suggestedFix = "Clamp to valid MIDI channel range [1-16]"
            });
        }
    }
    else if (strcmp(paramName, "scale") == 0) {
        if (!isValidScale(value)) {
            result.addError({
                .type = ValidationError::INVALID_SCALE,
                .location = {.parameter = paramName},
                .currentValue = value,
                .validRange = {0, MAX_SCALE_INDEX},
                .isAutoFixable = true,
                .suggestedFix = "Reset to chromatic scale (0)"
            });
        }
    }
    else if (strcmp(paramName, "octave") == 0 || strcmp(paramName, "base_octave") == 0) {
        if (!isValidOctave(value)) {
            result.addError({
                .type = ValidationError::INVALID_OCTAVE,
                .location = {.parameter = paramName},
                .currentValue = value,
                .validRange = {0, MAX_OCTAVE},
                .isAutoFixable = true,
                .suggestedFix = "Clamp to octave range [0-7]"
            });
        }
    }
    else if (strcmp(paramName, "cc_id") == 0) {
        if (!isValidCCId(value)) {
            result.addError({
                .type = ValidationError::INVALID_CC_ID,
                .location = {.parameter = paramName},
                .currentValue = value,
                .validRange = {0, MAX_CC_ID},
                .isAutoFixable = true,
                .suggestedFix = "Clamp to CC range [0-127]"
            });
        }
    }
    else {
        // Unknown parameter
        result.addWarning({
            .type = ValidationWarning::UNKNOWN_PARAMETER,
            .message = "Parameter not recognized for validation",
            .location = {.parameter = paramName},
            .severity = WarningLevel::LOW
        });
    }
    
    return result;
}

void ConfigurationValidator::validateBasicRanges(const ConfigurationData& config, ValidationResult& result) {
    // This is called from validateConfiguration, so we validate basic ConfigurationData fields
    // The full validation of banks would require access to kb_cfg and cc_cfg arrays
    
    // Validate palette
    if (config.palette >= BANK_AMT) {
        result.addError({
            .type = ValidationError::OUT_OF_RANGE,
            .location = {.parameter = "palette"},
            .currentValue = config.palette,
            .validRange = {0, BANK_AMT - 1},
            .isAutoFixable = true,
            .suggestedFix = "Reset to bank 0"
        });
    }
}

bool ConfigurationValidator::autoFixConfiguration(ConfigurationData& config, const ValidationResult& result) {
    if (!result.canAutoFix()) {
        return false;
    }
    
    bool fixed = false;
    
    for (const auto& error : result.errors) {
        if (!error.isAutoFixable) continue;
        
        if (strcmp(error.location.parameter, "mode") == 0) {
            config.mode = clampToRange(config.mode, (uint8_t)0, (uint8_t)4);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "sensitivity") == 0) {
            config.sensitivity = clampToRange(config.sensitivity, (uint8_t)0, (uint8_t)4);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "brightness") == 0) {
            config.brightness = clampToRange(config.brightness, (uint8_t)0, (uint8_t)15);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "palette") == 0) {
            config.palette = clampToRange(config.palette, (uint8_t)0, (uint8_t)(BANK_AMT - 1));
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "midi_trs") == 0) {
            config.midi_trs = clampToRange(config.midi_trs, (uint8_t)0, (uint8_t)1);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "trs_type") == 0) {
            config.trs_type = clampToRange(config.trs_type, (uint8_t)0, (uint8_t)1);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "passthrough") == 0) {
            config.passthrough = clampToRange(config.passthrough, (uint8_t)0, (uint8_t)1);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "midi_ble") == 0) {
            config.midi_ble = clampToRange(config.midi_ble, (uint8_t)0, (uint8_t)1);
            fixed = true;
        }
    }
    
    config.hasChanged = fixed;
    return fixed;
}

bool ConfigurationValidator::autoFixKeyModeData(KeyModeData& kb_cfg, const ValidationResult& result) {
    if (!result.canAutoFix()) {
        return false;
    }
    
    bool fixed = false;
    
    for (const auto& error : result.errors) {
        if (!error.isAutoFixable) continue;
        
        if (strcmp(error.location.parameter, "channel") == 0) {
            kb_cfg.channel = clampToRange(kb_cfg.channel, (uint8_t)1, (uint8_t)16);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "scale") == 0) {
            kb_cfg.scale = clampToRange(kb_cfg.scale, (uint8_t)0, (uint8_t)15);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "base_octave") == 0) {
            kb_cfg.base_octave = clampToRange(kb_cfg.base_octave, (uint8_t)0, (uint8_t)7);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "base_note") == 0) {
            kb_cfg.base_note = clampToRange(kb_cfg.base_note, (uint8_t)0, (uint8_t)127);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "velocity_curve") == 0) {
            kb_cfg.velocity_curve = clampToRange(kb_cfg.velocity_curve, (uint8_t)0, (uint8_t)3);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "aftertouch_curve") == 0) {
            kb_cfg.aftertouch_curve = clampToRange(kb_cfg.aftertouch_curve, (uint8_t)0, (uint8_t)3);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "flip_x") == 0) {
            kb_cfg.flip_x = clampToRange(kb_cfg.flip_x, (uint8_t)0, (uint8_t)1);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "flip_y") == 0) {
            kb_cfg.flip_y = clampToRange(kb_cfg.flip_y, (uint8_t)0, (uint8_t)1);
            fixed = true;
        }
        else if (strcmp(error.location.parameter, "koala_mode") == 0) {
            kb_cfg.koala_mode = 0; // Disable for incompatible hardware
            fixed = true;
        }
    }
    
    kb_cfg.hasChanged = fixed;
    return fixed;
}

bool ConfigurationValidator::autoFixControlChangeData(ControlChangeData& cc_cfg, const ValidationResult& result) {
    if (!result.canAutoFix()) {
        return false;
    }
    
    bool fixed = false;
    
    for (const auto& error : result.errors) {
        if (!error.isAutoFixable) continue;
        
        if (strcmp(error.location.parameter, "cc_channel") == 0) {
            uint8_t idx = error.location.index;
            if (idx < CC_AMT) {
                cc_cfg.channel[idx] = clampToRange(cc_cfg.channel[idx], (uint8_t)1, (uint8_t)16);
                fixed = true;
            }
        }
        else if (strcmp(error.location.parameter, "cc_id") == 0) {
            uint8_t idx = error.location.index;
            if (idx < CC_AMT) {
                cc_cfg.id[idx] = clampToRange(cc_cfg.id[idx], (uint8_t)0, (uint8_t)127);
                fixed = true;
            }
        }
    }
    
    cc_cfg.hasChanged = fixed;
    return fixed;
}

} // namespace DataIntegrity