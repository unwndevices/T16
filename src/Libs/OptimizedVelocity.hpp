#ifndef OPTIMIZED_VELOCITY_HPP
#define OPTIMIZED_VELOCITY_HPP

#include <Arduino.h>
#include <cmath>
#include <algorithm>

class FastVelocityEngine
{
public:
    struct CompactConfig
    {
        uint8_t rateWeight = 179;        // 0.7 * 255
        uint8_t peakWeight = 77;         // 0.3 * 255
        uint8_t historySize = 8;         // Reduced from 16 for performance
        uint8_t noiseThreshold = 1;      // 0.005 * 255
        uint8_t tempComp = 250;          // 0.98 * 255
        uint8_t driftComp = 254;         // 0.995 * 255
    };

    struct FastHistory
    {
        uint16_t values[8];              // Use 16-bit for memory efficiency
        uint16_t timestamps[8];          // Relative timestamps in ms
        uint8_t index = 0;
        uint8_t count = 0;
    };

    FastVelocityEngine() = default;

    void Init(const CompactConfig& config = CompactConfig{})
    {
        _config = config;
        Reset();
        
        _recipLookup[0] = 0;
        for (uint16_t i = 1; i < 256; i++)
        {
            _recipLookup[i] = (uint16_t)(65536.0f / i);
        }
    }

    void Reset()
    {
        _history.index = 0;
        _history.count = 0;
        _peakPressure = 0;
        _lastTimestamp = 0;
        _velocityResult = 0;
        
        for (uint8_t i = 0; i < 8; i++)
        {
            _history.values[i] = 0;
            _history.timestamps[i] = 0;
        }
    }

    uint8_t AnalyzeVelocity(uint16_t pressure, uint16_t timestamp)
    {
        AddPressureSample(pressure, timestamp);

        if (_history.count < 3)
        {
            return _velocityResult;
        }

        uint8_t rateVelocity = AnalyzeRateOfChangeFast();
        uint8_t peakVelocity = AnalyzePeakPressureFast();

        uint16_t rawVelocity = ((uint16_t)rateVelocity * _config.rateWeight + 
                               (uint16_t)peakVelocity * _config.peakWeight) >> 8;

        _velocityResult = ApplyCalibrationFast((uint8_t)rawVelocity);

        return _velocityResult;
    }

    uint8_t GetLastVelocity() const
    {
        return _velocityResult;
    }

private:
    CompactConfig _config;
    FastHistory _history;
    uint16_t _peakPressure = 0;
    uint16_t _lastTimestamp = 0;
    uint8_t _velocityResult = 0;
    uint16_t _recipLookup[256];          // Precomputed reciprocals for division

    void AddPressureSample(uint16_t pressure, uint16_t timestamp)
    {
        _history.values[_history.index] = pressure;
        _history.timestamps[_history.index] = timestamp - _lastTimestamp;
        
        _history.index = (_history.index + 1) & 0x07;  // Fast modulo 8
        
        if (_history.count < 8)
        {
            _history.count++;
        }

        if (pressure > _peakPressure)
        {
            _peakPressure = pressure;
        }

        _lastTimestamp = timestamp;
    }

    uint8_t AnalyzeRateOfChangeFast()
    {
        if (_history.count < 3)
        {
            return 0;
        }

        uint16_t maxRate = 0;
        uint8_t samplesUsed = _history.count;

        for (uint8_t i = 1; i < samplesUsed && i < 4; i++)  // Limit iterations for performance
        {
            uint8_t currentIdx = (_history.index - 1 - i) & 0x07;
            uint8_t prevIdx = (_history.index - 2 - i) & 0x07;

            uint16_t timeDelta = _history.timestamps[currentIdx];
            if (timeDelta == 0)
            {
                continue;
            }

            int16_t pressureDelta = (int16_t)_history.values[currentIdx] - (int16_t)_history.values[prevIdx];
            
            if (abs(pressureDelta) < _config.noiseThreshold)
            {
                continue;
            }

            if (pressureDelta > 0)
            {
                uint16_t rate = ((uint32_t)pressureDelta * _recipLookup[min(timeDelta, (uint16_t)255)]) >> 8;
                maxRate = max(maxRate, rate);
            }
        }

        return min((uint8_t)((maxRate * 77) >> 8), (uint8_t)255);  // Scale by 0.3
    }

    uint8_t AnalyzePeakPressureFast()
    {
        if (_peakPressure < _config.noiseThreshold)
        {
            return 0;
        }

        uint16_t normalizedPeak = (_peakPressure * 320) >> 8;  // Scale by 1.25 for 0.8 max
        normalizedPeak = min(normalizedPeak, (uint16_t)255);
        
        return (uint8_t)((normalizedPeak * normalizedPeak) >> 8);  // Square for curve
    }

    uint8_t ApplyCalibrationFast(uint8_t rawVelocity)
    {
        uint16_t calibrated = rawVelocity;
        
        calibrated = (calibrated * _config.tempComp) >> 8;
        calibrated = (calibrated * _config.driftComp) >> 8;
        
        return min((uint8_t)calibrated, (uint8_t)255);
    }
};

class FastStyleDetector
{
public:
    enum PlayingStyle : uint8_t
    {
        GENTLE = 0,
        NORMAL = 1,
        AGGRESSIVE = 2
    };

    struct StyleMetrics
    {
        uint8_t avgVelocity = 128;       // 0.5 * 255
        uint8_t velocityVariance = 26;   // 0.1 * 255
        uint8_t attackRate = 77;         // 0.3 * 255
        uint8_t confidence = 0;
    };

    FastStyleDetector() = default;

    void Init()
    {
        Reset();
    }

    void Reset()
    {
        _currentStyle = NORMAL;
        _styleConfidence = 0;
        _sampleCount = 0;
        _velocitySum = 0;
        _varianceSum = 0;
        _attackSum = 0;
        
        for (uint8_t i = 0; i < 8; i++)
        {
            _velocityHistory[i] = 128;
        }
    }

    PlayingStyle AnalyzeStyle(uint8_t velocity, uint8_t attackRate)
    {
        UpdateMetrics(velocity, attackRate);
        
        if (_sampleCount < 5)
        {
            return _currentStyle;
        }

        PlayingStyle detectedStyle = ClassifyStyleFast();
        
        if (detectedStyle != _currentStyle)
        {
            _styleConfidence++;
            if (_styleConfidence > 2)  // Reduced threshold for faster adaptation
            {
                _currentStyle = detectedStyle;
                _styleConfidence = 0;
            }
        }
        else
        {
            _styleConfidence = 0;
        }

        return _currentStyle;
    }

    PlayingStyle GetCurrentStyle() const
    {
        return _currentStyle;
    }

    uint8_t GetStyleAdaptationFactor() const
    {
        switch (_currentStyle)
        {
            case GENTLE:
                return 204;      // 0.8 * 255
            case AGGRESSIVE:
                return 306;      // 1.2 * 255 (clamped to 255)
            default:
                return 255;      // 1.0 * 255
        }
    }

private:
    PlayingStyle _currentStyle = NORMAL;
    uint8_t _styleConfidence = 0;
    uint8_t _sampleCount = 0;
    uint16_t _velocitySum = 0;
    uint16_t _varianceSum = 0;
    uint16_t _attackSum = 0;
    uint8_t _velocityHistory[8];
    uint8_t _historyIndex = 0;

    void UpdateMetrics(uint8_t velocity, uint8_t attackRate)
    {
        _velocityHistory[_historyIndex] = velocity;
        _historyIndex = (_historyIndex + 1) & 0x07;  // Fast modulo 8
        
        if (_sampleCount < 8)
        {
            _sampleCount++;
        }
        
        _velocitySum += velocity;
        _attackSum += attackRate;
        
        // Simple variance calculation
        if (_sampleCount > 1)
        {
            uint8_t avgVel = _velocitySum / _sampleCount;
            int16_t diff = velocity - avgVel;
            _varianceSum += (diff * diff) >> 4;  // Scale down to prevent overflow
        }
    }

    PlayingStyle ClassifyStyleFast()
    {
        uint8_t avgVel = _velocitySum / _sampleCount;
        uint8_t variance = _varianceSum / max(_sampleCount - 1, (uint8_t)1);
        uint8_t avgAttack = _attackSum / _sampleCount;

        // Simple threshold-based classification for speed
        if (avgVel < 100 && variance < 20 && avgAttack < 60)
        {
            return GENTLE;
        }
        else if (avgVel > 180 && (variance > 40 || avgAttack > 150))
        {
            return AGGRESSIVE;
        }
        
        return NORMAL;
    }
};

class OptimizedHybridAnalyzer
{
public:
    struct OptimizedConfig
    {
        FastVelocityEngine::CompactConfig velocityConfig;
        uint8_t styleAdaptationRate = 26;    // 0.1 * 255
        bool enableStyleDetection = true;
        bool enableTemperatureCompensation = true;
    };

    OptimizedHybridAnalyzer() = default;

    void Init(const OptimizedConfig& config = OptimizedConfig{})
    {
        _config = config;
        _velocityEngine.Init(_config.velocityConfig);
        
        if (_config.enableStyleDetection)
        {
            _styleDetector.Init();
        }
    }

    uint8_t AnalyzeVelocity(uint16_t pressure, uint16_t timestamp = 0)
    {
        if (timestamp == 0)
        {
            timestamp = (uint16_t)(micros() >> 10);  // Convert to ~ms resolution
        }

        uint8_t baseVelocity = _velocityEngine.AnalyzeVelocity(pressure, timestamp);

        if (_config.enableStyleDetection)
        {
            uint8_t attackRate = EstimateAttackRate(pressure, timestamp);
            FastStyleDetector::PlayingStyle style = _styleDetector.AnalyzeStyle(baseVelocity, attackRate);
            
            uint8_t styleAdaptation = _styleDetector.GetStyleAdaptationFactor();
            
            // Fast fixed-point multiplication
            uint16_t adapted = (uint16_t)baseVelocity * styleAdaptation;
            adapted >>= 8;  // Divide by 256 instead of 255 for speed
            
            baseVelocity = min((uint8_t)adapted, (uint8_t)255);
        }

        return max(baseVelocity, (uint8_t)3);  // Minimum velocity of ~0.01
    }

    void Reset()
    {
        _velocityEngine.Reset();
        if (_config.enableStyleDetection)
        {
            _styleDetector.Reset();
        }
        _lastPressure = 0;
        _lastTimestamp = 0;
    }

    FastStyleDetector::PlayingStyle GetPlayingStyle() const
    {
        return _styleDetector.GetCurrentStyle();
    }

    void UpdateConfig(const OptimizedConfig& config)
    {
        _config = config;
        _velocityEngine.Init(_config.velocityConfig);
    }

private:
    OptimizedConfig _config;
    FastVelocityEngine _velocityEngine;
    FastStyleDetector _styleDetector;
    
    uint16_t _lastPressure = 0;
    uint16_t _lastTimestamp = 0;

    uint8_t EstimateAttackRate(uint16_t pressure, uint16_t timestamp)
    {
        if (_lastTimestamp == 0 || timestamp <= _lastTimestamp)
        {
            _lastPressure = pressure;
            _lastTimestamp = timestamp;
            return 0;
        }

        int16_t pressureDelta = pressure - _lastPressure;
        uint16_t timeDelta = timestamp - _lastTimestamp;

        _lastPressure = pressure;
        _lastTimestamp = timestamp;

        if (timeDelta == 0 || pressureDelta <= 0)
        {
            return 0;
        }

        // Fast division using bit shifts for common time deltas
        uint8_t rate = 0;
        if (timeDelta <= 4)
        {
            rate = min(pressureDelta << (2 - timeDelta), 255);
        }
        else if (timeDelta <= 8)
        {
            rate = pressureDelta >> (timeDelta - 4);
        }
        else
        {
            rate = pressureDelta >> 4;  // Very slow attack
        }

        return min(rate, (uint8_t)255);
    }
};

// Memory-efficient calibration data structure
struct CompactCalibrationData
{
    uint8_t minPressure = 13;            // 0.05 * 255
    uint8_t maxPressure = 242;           // 0.95 * 255
    uint8_t sensitivityFactor = 255;     // 1.0 * 255
    uint8_t temperatureCoeff = 250;      // 0.98 * 255
    uint8_t driftCompensation = 254;     // 0.995 * 255
    uint32_t calibrationTimestamp = 0;
    bool isValid = false;
};

class FastCalibrationSystem
{
public:
    FastCalibrationSystem() = default;

    bool LoadFromJson(const String& jsonStr)
    {
        // Simple JSON parsing for known structure
        int startIdx = jsonStr.indexOf("\"minPressure\":");
        if (startIdx == -1) return false;
        
        _calibration.minPressure = (uint8_t)(jsonStr.substring(startIdx + 14).toFloat() * 255);
        
        startIdx = jsonStr.indexOf("\"maxPressure\":");
        if (startIdx != -1)
        {
            _calibration.maxPressure = (uint8_t)(jsonStr.substring(startIdx + 14).toFloat() * 255);
        }
        
        startIdx = jsonStr.indexOf("\"isValid\":");
        if (startIdx != -1)
        {
            _calibration.isValid = jsonStr.substring(startIdx + 9).indexOf("true") != -1;
        }

        return _calibration.isValid;
    }

    OptimizedHybridAnalyzer::OptimizedConfig ApplyCalibration(const OptimizedHybridAnalyzer::OptimizedConfig& baseConfig) const
    {
        if (!_calibration.isValid)
        {
            return baseConfig;
        }

        OptimizedHybridAnalyzer::OptimizedConfig calibratedConfig = baseConfig;
        
        calibratedConfig.velocityConfig.tempComp = _calibration.temperatureCoeff;
        calibratedConfig.velocityConfig.driftComp = _calibration.driftCompensation;
        calibratedConfig.velocityConfig.noiseThreshold = _calibration.minPressure >> 4;  // Divide by 16
        
        uint8_t pressureRange = _calibration.maxPressure - _calibration.minPressure;
        if (pressureRange > 25)  // 0.1 * 255
        {
            calibratedConfig.velocityConfig.rateWeight = (calibratedConfig.velocityConfig.rateWeight * _calibration.sensitivityFactor) >> 8;
            calibratedConfig.velocityConfig.peakWeight = (calibratedConfig.velocityConfig.peakWeight * (512 - _calibration.sensitivityFactor)) >> 9;
        }

        return calibratedConfig;
    }

    CompactCalibrationData GetCalibration() const
    {
        return _calibration;
    }

private:
    CompactCalibrationData _calibration;
};

#endif // OPTIMIZED_VELOCITY_HPP