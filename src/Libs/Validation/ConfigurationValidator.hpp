#ifndef CONFIGURATION_VALIDATOR_HPP
#define CONFIGURATION_VALIDATOR_HPP

#include <Arduino.h>
#include "DataIntegrity.hpp"
#include "../../Configuration.hpp"
#include "../../hardware_config.hpp"
#include <unordered_set>

namespace DataIntegrity {

class ConfigurationValidator {
public:
    enum ValidationLevel {
        BASIC,           // Essential checks only
        STANDARD,        // Normal validation
        COMPREHENSIVE,   // Full validation with cross-checks
        PARANOID        // Maximum validation + redundancy checks
    };
    
    struct ValidationContext {
        HardwareVariant variant = HardwareVariant::T16;
        uint8_t firmwareVersion = 102;
        bool strictMode = true;
        ValidationLevel level = COMPREHENSIVE;
    };
    
    ValidationResult validateConfiguration(const ConfigurationData& config, 
                                         const ValidationContext& context);
    
    ValidationResult validateKeyModeData(const KeyModeData& kb_cfg, uint8_t bank,
                                       const ValidationContext& context);
    
    ValidationResult validateControlChangeData(const ControlChangeData& cc_cfg, uint8_t bank,
                                             const ValidationContext& context);
    
    ValidationResult validateCalibrationData(const CalibrationData& calibration,
                                           const ValidationContext& context);
    
    ValidationResult validateCustomScale(const int8_t* scale, uint8_t scaleNum,
                                       const ValidationContext& context);
    
    // Quick validation for real-time parameter updates
    ValidationResult validateParameter(const char* paramName, uint32_t value,
                                     const ValidationContext& context);
    
    // Auto-fix functions
    bool autoFixConfiguration(ConfigurationData& config, const ValidationResult& result);
    bool autoFixKeyModeData(KeyModeData& kb_cfg, const ValidationResult& result);
    bool autoFixControlChangeData(ControlChangeData& cc_cfg, const ValidationResult& result);
    
private:
    static constexpr uint8_t MAX_SCALE_INDEX = 15;
    static constexpr uint8_t MAX_OCTAVE = 7;
    static constexpr uint8_t MAX_CC_ID = 127;
    static constexpr uint8_t MIN_FIRMWARE_VERSION = 102;
    static constexpr int8_t MAX_SCALE_NOTE_OFFSET = 24;
    
    void validateBasicRanges(const ConfigurationData& config, ValidationResult& result);
    void validateSemanticConsistency(const ConfigurationData& config, 
                                   const KeyModeData kb_cfg[BANK_AMT],
                                   const ControlChangeData cc_cfg[BANK_AMT],
                                   ValidationResult& result);
    void validateHardwareCompatibility(const ConfigurationData& config,
                                     const KeyModeData kb_cfg[BANK_AMT],
                                     const ValidationContext& context,
                                     ValidationResult& result);
    void validateCrossParameterConstraints(const ConfigurationData& config,
                                         const KeyModeData kb_cfg[BANK_AMT],
                                         const ControlChangeData cc_cfg[BANK_AMT],
                                         ValidationResult& result);
    void validatePerformanceConstraints(const ConfigurationData& config,
                                      ValidationResult& result);
    
    // Helper functions
    bool isValidMidiChannel(uint8_t channel) const {
        return channel >= 1 && channel <= 16;
    }
    
    bool isValidScale(uint8_t scale) const {
        return scale <= MAX_SCALE_INDEX;
    }
    
    bool isValidOctave(uint8_t octave) const {
        return octave <= MAX_OCTAVE;
    }
    
    bool isValidCCId(uint8_t ccId) const {
        return ccId <= MAX_CC_ID;
    }
    
    bool isValidScaleNote(int8_t note) const {
        return note >= -MAX_SCALE_NOTE_OFFSET && note <= MAX_SCALE_NOTE_OFFSET;
    }
    
    bool isValidVelocityCurve(uint8_t curve) const {
        return curve <= 3; // Assuming 4 velocity curves (0-3)
    }
    
    bool isValidAftertouchCurve(uint8_t curve) const {
        return curve <= 3; // Assuming 4 aftertouch curves (0-3)
    }
    
    uint8_t clampToRange(uint8_t value, uint8_t min, uint8_t max) const {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

} // namespace DataIntegrity

#endif // CONFIGURATION_VALIDATOR_HPP