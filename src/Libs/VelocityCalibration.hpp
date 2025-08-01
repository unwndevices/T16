#ifndef VELOCITY_CALIBRATION_HPP
#define VELOCITY_CALIBRATION_HPP

#include <Arduino.h>
#include <ArduinoJson.h>
#include "EnhancedVelocity.hpp"

class VelocityCalibrationSystem
{
public:
    struct CalibrationData
    {
        float minPressure = 0.05f;
        float maxPressure = 0.95f;
        float sensitivityFactor = 1.0f;
        float temperatureCoeff = 0.98f;
        float driftCompensation = 0.995f;
        uint32_t calibrationTimestamp = 0;
        bool isValid = false;
    };

    struct CalibrationSample
    {
        float pressure;
        float expectedVelocity;
        uint32_t timestamp;
    };

    VelocityCalibrationSystem() = default;

    void Init()
    {
        Reset();
    }

    void Reset()
    {
        _calibrationData = CalibrationData{};
        _samples.clear();
        _isCalibrating = false;
        _currentStep = CALIBRATION_IDLE;
    }

    bool StartCalibration()
    {
        if (_isCalibrating)
        {
            return false;
        }

        _isCalibrating = true;
        _currentStep = CALIBRATION_MIN_PRESSURE;
        _samples.clear();
        _sampleCount = 0;
        _stepStartTime = millis();
        
        log_d("Velocity calibration started");
        return true;
    }

    bool UpdateCalibration(float pressure, float userInputVelocity = -1.0f)
    {
        if (!_isCalibrating)
        {
            return false;
        }

        uint32_t currentTime = millis();
        bool stepComplete = false;

        switch (_currentStep)
        {
            case CALIBRATION_MIN_PRESSURE:
                stepComplete = CalibrateMinPressure(pressure, currentTime);
                break;
                
            case CALIBRATION_MAX_PRESSURE:
                stepComplete = CalibrateMaxPressure(pressure, currentTime);
                break;
                
            case CALIBRATION_VELOCITY_MAPPING:
                stepComplete = CalibrateVelocityMapping(pressure, userInputVelocity, currentTime);
                break;
                
            case CALIBRATION_VALIDATION:
                stepComplete = ValidateCalibration(pressure, currentTime);
                break;
                
            default:
                break;
        }

        if (stepComplete)
        {
            AdvanceCalibrationStep();
        }

        return _currentStep != CALIBRATION_COMPLETE;
    }

    bool IsCalibrating() const
    {
        return _isCalibrating;
    }

    float GetCalibrationProgress() const
    {
        if (!_isCalibrating)
        {
            return _calibrationData.isValid ? 1.0f : 0.0f;
        }

        float stepProgress = 0.0f;
        switch (_currentStep)
        {
            case CALIBRATION_MIN_PRESSURE:
                stepProgress = 0.0f + (std::min(_sampleCount, SAMPLES_PER_STEP) / (float)SAMPLES_PER_STEP) * 0.25f;
                break;
            case CALIBRATION_MAX_PRESSURE:
                stepProgress = 0.25f + (std::min(_sampleCount, SAMPLES_PER_STEP) / (float)SAMPLES_PER_STEP) * 0.25f;
                break;
            case CALIBRATION_VELOCITY_MAPPING:
                stepProgress = 0.5f + (std::min(_sampleCount, VELOCITY_SAMPLES_NEEDED) / (float)VELOCITY_SAMPLES_NEEDED) * 0.25f;
                break;
            case CALIBRATION_VALIDATION:
                stepProgress = 0.75f + (std::min(_sampleCount, VALIDATION_SAMPLES) / (float)VALIDATION_SAMPLES) * 0.25f;
                break;
            case CALIBRATION_COMPLETE:
                stepProgress = 1.0f;
                break;
            default:
                stepProgress = 0.0f;
                break;
        }

        return stepProgress;
    }

    String GetCalibrationStepName() const
    {
        switch (_currentStep)
        {
            case CALIBRATION_MIN_PRESSURE:
                return "Minimum Pressure";
            case CALIBRATION_MAX_PRESSURE:
                return "Maximum Pressure";
            case CALIBRATION_VELOCITY_MAPPING:
                return "Velocity Mapping";
            case CALIBRATION_VALIDATION:
                return "Validation";
            case CALIBRATION_COMPLETE:
                return "Complete";
            default:
                return "Idle";
        }
    }

    CalibrationData GetCalibrationData() const
    {
        return _calibrationData;
    }

    bool LoadCalibrationFromJson(const String& jsonStr)
    {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, jsonStr);
        
        if (error)
        {
            log_e("Failed to parse velocity calibration JSON: %s", error.c_str());
            return false;
        }

        _calibrationData.minPressure = doc["minPressure"] | 0.05f;
        _calibrationData.maxPressure = doc["maxPressure"] | 0.95f;
        _calibrationData.sensitivityFactor = doc["sensitivityFactor"] | 1.0f;
        _calibrationData.temperatureCoeff = doc["temperatureCoeff"] | 0.98f;
        _calibrationData.driftCompensation = doc["driftCompensation"] | 0.995f;
        _calibrationData.calibrationTimestamp = doc["timestamp"] | 0;
        _calibrationData.isValid = doc["isValid"] | false;

        return _calibrationData.isValid;
    }

    String SaveCalibrationToJson() const
    {
        DynamicJsonDocument doc(512);
        
        doc["minPressure"] = _calibrationData.minPressure;
        doc["maxPressure"] = _calibrationData.maxPressure;
        doc["sensitivityFactor"] = _calibrationData.sensitivityFactor;
        doc["temperatureCoeff"] = _calibrationData.temperatureCoeff;
        doc["driftCompensation"] = _calibrationData.driftCompensation;
        doc["timestamp"] = _calibrationData.calibrationTimestamp;
        doc["isValid"] = _calibrationData.isValid;

        String jsonStr;
        serializeJson(doc, jsonStr);
        return jsonStr;
    }

    HybridVelocityAnalyzer::AnalyzerConfig ApplyCalibrationToConfig(const HybridVelocityAnalyzer::AnalyzerConfig& baseConfig) const
    {
        if (!_calibrationData.isValid)
        {
            return baseConfig;
        }

        HybridVelocityAnalyzer::AnalyzerConfig calibratedConfig = baseConfig;
        
        calibratedConfig.velocityConfig.temperatureComp = _calibrationData.temperatureCoeff;
        calibratedConfig.velocityConfig.sensorDriftComp = _calibrationData.driftCompensation;
        calibratedConfig.velocityConfig.noiseThreshold = _calibrationData.minPressure * 0.1f;
        
        float pressureRange = _calibrationData.maxPressure - _calibrationData.minPressure;
        if (pressureRange > 0.1f)
        {
            calibratedConfig.velocityConfig.rateWeight *= _calibrationData.sensitivityFactor;
            calibratedConfig.velocityConfig.peakWeight *= (2.0f - _calibrationData.sensitivityFactor);
        }

        return calibratedConfig;
    }

private:
    enum CalibrationStep
    {
        CALIBRATION_IDLE,
        CALIBRATION_MIN_PRESSURE,
        CALIBRATION_MAX_PRESSURE,
        CALIBRATION_VELOCITY_MAPPING,
        CALIBRATION_VALIDATION,
        CALIBRATION_COMPLETE
    };

    static constexpr uint8_t SAMPLES_PER_STEP = 50;
    static constexpr uint8_t VELOCITY_SAMPLES_NEEDED = 20;
    static constexpr uint8_t VALIDATION_SAMPLES = 10;
    static constexpr uint32_t STEP_TIMEOUT_MS = 30000;

    CalibrationData _calibrationData;
    std::vector<CalibrationSample> _samples;
    bool _isCalibrating = false;
    CalibrationStep _currentStep = CALIBRATION_IDLE;
    uint8_t _sampleCount = 0;
    uint32_t _stepStartTime = 0;

    bool CalibrateMinPressure(float pressure, uint32_t currentTime)
    {
        if (currentTime - _stepStartTime < 2000)
        {
            return false;
        }

        if (pressure < 0.15f)
        {
            _samples.push_back({pressure, 0.0f, currentTime});
            _sampleCount++;
        }

        return _sampleCount >= SAMPLES_PER_STEP || (currentTime - _stepStartTime > STEP_TIMEOUT_MS);
    }

    bool CalibrateMaxPressure(float pressure, uint32_t currentTime)
    {
        if (currentTime - _stepStartTime < 1000)
        {
            return false;
        }

        if (pressure > 0.7f)
        {
            _samples.push_back({pressure, 1.0f, currentTime});
            _sampleCount++;
        }

        return _sampleCount >= SAMPLES_PER_STEP || (currentTime - _stepStartTime > STEP_TIMEOUT_MS);
    }

    bool CalibrateVelocityMapping(float pressure, float userInputVelocity, uint32_t currentTime)
    {
        if (userInputVelocity >= 0.0f && userInputVelocity <= 1.0f)
        {
            _samples.push_back({pressure, userInputVelocity, currentTime});
            _sampleCount++;
        }

        return _sampleCount >= VELOCITY_SAMPLES_NEEDED || (currentTime - _stepStartTime > STEP_TIMEOUT_MS);
    }

    bool ValidateCalibration(float pressure, uint32_t currentTime)
    {
        _sampleCount++;
        return _sampleCount >= VALIDATION_SAMPLES || (currentTime - _stepStartTime > 10000);
    }

    void AdvanceCalibrationStep()
    {
        switch (_currentStep)
        {
            case CALIBRATION_MIN_PRESSURE:
                ProcessMinPressureSamples();
                _currentStep = CALIBRATION_MAX_PRESSURE;
                break;
                
            case CALIBRATION_MAX_PRESSURE:
                ProcessMaxPressureSamples();
                _currentStep = CALIBRATION_VELOCITY_MAPPING;
                break;
                
            case CALIBRATION_VELOCITY_MAPPING:
                ProcessVelocityMappingSamples();
                _currentStep = CALIBRATION_VALIDATION;
                break;
                
            case CALIBRATION_VALIDATION:
                FinalizeCalibration();
                _currentStep = CALIBRATION_COMPLETE;
                _isCalibrating = false;
                break;
                
            default:
                break;
        }

        _sampleCount = 0;
        _stepStartTime = millis();
    }

    void ProcessMinPressureSamples()
    {
        if (_samples.empty())
        {
            _calibrationData.minPressure = 0.05f;
            return;
        }

        float sum = 0.0f;
        for (const auto& sample : _samples)
        {
            sum += sample.pressure;
        }
        
        _calibrationData.minPressure = std::max(0.01f, sum / _samples.size());
        _samples.clear();
        
        log_d("Min pressure calibrated: %.3f", _calibrationData.minPressure);
    }

    void ProcessMaxPressureSamples()
    {
        if (_samples.empty())
        {
            _calibrationData.maxPressure = 0.95f;
            return;
        }

        float sum = 0.0f;
        for (const auto& sample : _samples)
        {
            sum += sample.pressure;
        }
        
        _calibrationData.maxPressure = std::min(1.0f, sum / _samples.size());
        _samples.clear();
        
        log_d("Max pressure calibrated: %.3f", _calibrationData.maxPressure);
    }

    void ProcessVelocityMappingSamples()
    {
        if (_samples.size() < 5)
        {
            _calibrationData.sensitivityFactor = 1.0f;
            _samples.clear();
            return;
        }

        float totalSensitivity = 0.0f;
        uint8_t validSamples = 0;

        for (const auto& sample : _samples)
        {
            if (sample.pressure > _calibrationData.minPressure && 
                sample.pressure < _calibrationData.maxPressure)
            {
                float normalizedPressure = (sample.pressure - _calibrationData.minPressure) / 
                                         (_calibrationData.maxPressure - _calibrationData.minPressure);
                
                if (normalizedPressure > 0.01f)
                {
                    totalSensitivity += sample.expectedVelocity / normalizedPressure;
                    validSamples++;
                }
            }
        }

        if (validSamples > 0)
        {
            _calibrationData.sensitivityFactor = std::clamp(totalSensitivity / validSamples, 0.5f, 2.0f);
        }
        else
        {
            _calibrationData.sensitivityFactor = 1.0f;
        }

        _samples.clear();
        log_d("Sensitivity factor calibrated: %.3f", _calibrationData.sensitivityFactor);
    }

    void FinalizeCalibration()
    {
        _calibrationData.calibrationTimestamp = millis();
        _calibrationData.isValid = true;
        
        log_d("Velocity calibration completed successfully");
        log_d("  Min Pressure: %.3f", _calibrationData.minPressure);
        log_d("  Max Pressure: %.3f", _calibrationData.maxPressure);
        log_d("  Sensitivity: %.3f", _calibrationData.sensitivityFactor);
    }
};

#endif // VELOCITY_CALIBRATION_HPP