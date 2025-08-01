#ifndef ENHANCED_VELOCITY_HPP
#define ENHANCED_VELOCITY_HPP

#include <Arduino.h>
#include <cmath>
#include <algorithm>

class VelocityDetectionEngine
{
public:
    struct VelocityConfig
    {
        float rateWeight = 0.7f;           // Weight for rate-of-change analysis
        float peakWeight = 0.3f;           // Weight for peak pressure analysis
        float minVelocity = 0.01f;         // Minimum velocity output
        float maxVelocity = 1.0f;          // Maximum velocity output
        float noiseThreshold = 0.005f;     // Minimum pressure change to consider
        uint8_t historySize = 8;           // Number of samples to analyze
        float temperatureComp = 0.98f;     // Temperature compensation factor
        float sensorDriftComp = 0.995f;    // Sensor drift compensation factor
    };

    struct PressureHistory
    {
        float values[16];
        uint32_t timestamps[16];
        uint8_t index = 0;
        uint8_t count = 0;
        bool filled = false;
    };

    VelocityDetectionEngine() = default;

    void Init(const VelocityConfig& config = VelocityConfig{})
    {
        _config = config;
        Reset();
    }

    void Reset()
    {
        _history.index = 0;
        _history.count = 0;
        _history.filled = false;
        _peakPressure = 0.0f;
        _lastPressure = 0.0f;
        _attackPhase = false;
        _velocityResult = 0.0f;
        
        for (uint8_t i = 0; i < 16; i++)
        {
            _history.values[i] = 0.0f;
            _history.timestamps[i] = 0;
        }
    }

    float AnalyzeVelocity(float pressure, uint32_t timestamp)
    {
        AddPressureSample(pressure, timestamp);

        if (_history.count < 3)
        {
            return _velocityResult;
        }

        float rateVelocity = AnalyzeRateOfChange();
        float peakVelocity = AnalyzePeakPressure();

        float rawVelocity = (rateVelocity * _config.rateWeight) + 
                           (peakVelocity * _config.peakWeight);

        _velocityResult = ApplyCalibration(rawVelocity);

        return std::clamp(_velocityResult, _config.minVelocity, _config.maxVelocity);
    }

    float GetLastVelocity() const
    {
        return _velocityResult;
    }

    void UpdateConfig(const VelocityConfig& config)
    {
        _config = config;
    }

    VelocityConfig GetConfig() const
    {
        return _config;
    }

private:
    VelocityConfig _config;
    PressureHistory _history;
    float _peakPressure = 0.0f;
    float _lastPressure = 0.0f;
    bool _attackPhase = false;
    float _velocityResult = 0.0f;

    void AddPressureSample(float pressure, uint32_t timestamp)
    {
        _history.values[_history.index] = pressure;
        _history.timestamps[_history.index] = timestamp;
        
        _history.index = (_history.index + 1) % _config.historySize;
        
        if (_history.count < _config.historySize)
        {
            _history.count++;
        }
        else
        {
            _history.filled = true;
        }

        if (pressure > _peakPressure)
        {
            _peakPressure = pressure;
        }

        _lastPressure = pressure;
    }

    float AnalyzeRateOfChange()
    {
        if (_history.count < 3)
        {
            return 0.0f;
        }

        float maxRate = 0.0f;
        uint8_t samplesUsed = std::min((uint8_t)_config.historySize, _history.count);

        for (uint8_t i = 1; i < samplesUsed; i++)
        {
            uint8_t currentIdx = (_history.index - 1 - i + _config.historySize) % _config.historySize;
            uint8_t prevIdx = (_history.index - 2 - i + _config.historySize) % _config.historySize;

            if (_history.timestamps[currentIdx] == _history.timestamps[prevIdx])
            {
                continue;
            }

            float pressureDelta = _history.values[currentIdx] - _history.values[prevIdx];
            uint32_t timeDelta = _history.timestamps[currentIdx] - _history.timestamps[prevIdx];

            if (std::abs(pressureDelta) < _config.noiseThreshold)
            {
                continue;
            }

            if (timeDelta > 0 && pressureDelta > 0)
            {
                float rate = pressureDelta / (timeDelta * 0.001f);
                maxRate = std::max(maxRate, rate);
            }
        }

        return std::clamp(maxRate * 0.3f, 0.0f, 1.0f);
    }

    float AnalyzePeakPressure()
    {
        if (_peakPressure < _config.noiseThreshold)
        {
            return 0.0f;
        }

        float normalizedPeak = std::clamp(_peakPressure / 0.8f, 0.0f, 1.0f);
        
        return normalizedPeak * normalizedPeak;
    }

    float ApplyCalibration(float rawVelocity)
    {
        float calibrated = rawVelocity;
        
        calibrated *= _config.temperatureComp;
        calibrated *= _config.sensorDriftComp;
        
        return calibrated;
    }
};

class PlayingStyleDetector
{
public:
    enum PlayingStyle
    {
        GENTLE,
        NORMAL,
        AGGRESSIVE,
        STYLE_COUNT
    };

    struct StyleProfile
    {
        float avgVelocity = 0.5f;
        float velocityVariance = 0.1f;
        float attackRate = 0.3f;
        uint32_t sampleCount = 0;
    };

    PlayingStyleDetector() = default;

    void Init()
    {
        Reset();
    }

    void Reset()
    {
        _currentStyle = NORMAL;
        _adaptationRate = 0.95f;
        
        for (uint8_t i = 0; i < STYLE_COUNT; i++)
        {
            _styleProfiles[i] = StyleProfile{};
        }
        
        _styleProfiles[GENTLE] = {0.3f, 0.05f, 0.2f, 0};
        _styleProfiles[NORMAL] = {0.5f, 0.1f, 0.3f, 0};
        _styleProfiles[AGGRESSIVE] = {0.8f, 0.2f, 0.6f, 0};
        
        _velocityHistory.clear();
    }

    PlayingStyle AnalyzeStyle(float velocity, float attackRate)
    {
        UpdateHistory(velocity, attackRate);
        
        if (_velocityHistory.size() < 5)
        {
            return _currentStyle;
        }

        float avgVel = CalculateAverageVelocity();
        float variance = CalculateVelocityVariance(avgVel);
        float avgAttack = CalculateAverageAttackRate();

        PlayingStyle detectedStyle = ClassifyStyle(avgVel, variance, avgAttack);
        
        if (detectedStyle != _currentStyle)
        {
            _styleChangePending = true;
            _pendingStyle = detectedStyle;
            _styleChangeConfidence++;
            
            if (_styleChangeConfidence > 3)
            {
                _currentStyle = detectedStyle;
                _styleChangePending = false;
                _styleChangeConfidence = 0;
                AdaptToStyle();
            }
        }
        else
        {
            _styleChangeConfidence = 0;
        }

        return _currentStyle;
    }

    PlayingStyle GetCurrentStyle() const
    {
        return _currentStyle;
    }

    StyleProfile GetStyleProfile(PlayingStyle style) const
    {
        if (style < STYLE_COUNT)
        {
            return _styleProfiles[style];
        }
        return _styleProfiles[NORMAL];
    }

    float GetStyleAdaptationFactor() const
    {
        switch (_currentStyle)
        {
            case GENTLE:
                return 0.8f;
            case AGGRESSIVE:
                return 1.2f;
            default:
                return 1.0f;
        }
    }

private:
    PlayingStyle _currentStyle = NORMAL;
    StyleProfile _styleProfiles[STYLE_COUNT];
    float _adaptationRate = 0.95f;
    
    struct PlayingData
    {
        float velocity;
        float attackRate;
        uint32_t timestamp;
    };
    
    std::vector<PlayingData> _velocityHistory;
    static constexpr uint8_t MAX_HISTORY = 20;
    
    bool _styleChangePending = false;
    PlayingStyle _pendingStyle = NORMAL;
    uint8_t _styleChangeConfidence = 0;

    void UpdateHistory(float velocity, float attackRate)
    {
        PlayingData data = {velocity, attackRate, (uint32_t)millis()};
        _velocityHistory.push_back(data);
        
        if (_velocityHistory.size() > MAX_HISTORY)
        {
            _velocityHistory.erase(_velocityHistory.begin());
        }
    }

    float CalculateAverageVelocity()
    {
        float sum = 0.0f;
        for (const auto& data : _velocityHistory)
        {
            sum += data.velocity;
        }
        return sum / _velocityHistory.size();
    }

    float CalculateVelocityVariance(float avgVel)
    {
        float variance = 0.0f;
        for (const auto& data : _velocityHistory)
        {
            float diff = data.velocity - avgVel;
            variance += diff * diff;
        }
        return variance / _velocityHistory.size();
    }

    float CalculateAverageAttackRate()
    {
        float sum = 0.0f;
        for (const auto& data : _velocityHistory)
        {
            sum += data.attackRate;
        }
        return sum / _velocityHistory.size();
    }

    PlayingStyle ClassifyStyle(float avgVel, float variance, float avgAttack)
    {
        float gentleScore = CalculateStyleScore(avgVel, variance, avgAttack, GENTLE);
        float normalScore = CalculateStyleScore(avgVel, variance, avgAttack, NORMAL);
        float aggressiveScore = CalculateStyleScore(avgVel, variance, avgAttack, AGGRESSIVE);

        if (gentleScore > normalScore && gentleScore > aggressiveScore)
        {
            return GENTLE;
        }
        else if (aggressiveScore > normalScore)
        {
            return AGGRESSIVE;
        }
        
        return NORMAL;
    }

    float CalculateStyleScore(float avgVel, float variance, float avgAttack, PlayingStyle style)
    {
        const StyleProfile& profile = _styleProfiles[style];
        
        float velDiff = std::abs(avgVel - profile.avgVelocity);
        float varDiff = std::abs(variance - profile.velocityVariance);
        float attackDiff = std::abs(avgAttack - profile.attackRate);
        
        return 1.0f / (1.0f + velDiff + varDiff + attackDiff);
    }

    void AdaptToStyle()
    {
        _styleProfiles[_currentStyle].sampleCount++;
    }
};

class HybridVelocityAnalyzer
{
public:
    struct AnalyzerConfig
    {
        VelocityDetectionEngine::VelocityConfig velocityConfig;
        float styleAdaptationRate = 0.1f;
        bool enableStyleDetection = true;
        bool enableTemperatureCompensation = true;
    };

    HybridVelocityAnalyzer() = default;

    void Init(const AnalyzerConfig& config = AnalyzerConfig{})
    {
        _config = config;
        _velocityEngine.Init(_config.velocityConfig);
        
        if (_config.enableStyleDetection)
        {
            _styleDetector.Init();
        }
    }

    float AnalyzeVelocity(float pressure, uint32_t timestamp = 0)
    {
        if (timestamp == 0)
        {
            timestamp = micros();
        }

        float baseVelocity = _velocityEngine.AnalyzeVelocity(pressure, timestamp);

        if (_config.enableStyleDetection)
        {
            float attackRate = EstimateAttackRate(pressure, timestamp);
            PlayingStyleDetector::PlayingStyle style = _styleDetector.AnalyzeStyle(baseVelocity, attackRate);
            
            float styleAdaptation = _styleDetector.GetStyleAdaptationFactor();
            baseVelocity *= (1.0f + (_config.styleAdaptationRate * (styleAdaptation - 1.0f)));
        }

        return std::clamp(baseVelocity, 0.01f, 1.0f);
    }

    void Reset()
    {
        _velocityEngine.Reset();
        if (_config.enableStyleDetection)
        {
            _styleDetector.Reset();
        }
        _lastPressure = 0.0f;
        _lastTimestamp = 0;
    }

    PlayingStyleDetector::PlayingStyle GetPlayingStyle() const
    {
        return _styleDetector.GetCurrentStyle();
    }

    void UpdateConfig(const AnalyzerConfig& config)
    {
        _config = config;
        _velocityEngine.UpdateConfig(_config.velocityConfig);
    }

    AnalyzerConfig GetConfig() const
    {
        return _config;
    }

private:
    AnalyzerConfig _config;
    VelocityDetectionEngine _velocityEngine;
    PlayingStyleDetector _styleDetector;
    
    float _lastPressure = 0.0f;
    uint32_t _lastTimestamp = 0;

    float EstimateAttackRate(float pressure, uint32_t timestamp)
    {
        if (_lastTimestamp == 0 || timestamp <= _lastTimestamp)
        {
            _lastPressure = pressure;
            _lastTimestamp = timestamp;
            return 0.0f;
        }

        float pressureDelta = pressure - _lastPressure;
        uint32_t timeDelta = timestamp - _lastTimestamp;

        _lastPressure = pressure;
        _lastTimestamp = timestamp;

        if (timeDelta == 0 || pressureDelta <= 0)
        {
            return 0.0f;
        }

        return std::clamp(pressureDelta / (timeDelta * 0.001f), 0.0f, 1.0f);
    }
};

#endif // ENHANCED_VELOCITY_HPP