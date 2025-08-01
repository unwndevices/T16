#ifndef ENHANCED_CONFIGURATION_HPP
#define ENHANCED_CONFIGURATION_HPP

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Configuration.hpp"
#include "Libs/EnhancedKey.hpp"
#include "Libs/VelocityCalibration.hpp"

struct EnhancedVelocityConfig
{
    bool useEnhancedVelocity = true;
    bool enableStyleDetection = true;
    bool enableTemperatureCompensation = true;
    float legacyVelocityWeight = 0.3f;
    float enhancedVelocityWeight = 0.7f;
    float rateWeight = 0.7f;
    float peakWeight = 0.3f;
    float styleAdaptationRate = 0.1f;
    uint8_t historySize = 8;
    float noiseThreshold = 0.005f;
    float temperatureComp = 0.98f;
    float sensorDriftComp = 0.995f;
    bool hasChanged = false;
};

struct EnhancedConfigurationData : public ConfigurationData
{
    uint8_t version = 103;  // Increment version for enhanced features
    EnhancedVelocityConfig velocityConfig;
    
    // Preserve all existing fields from ConfigurationData
    using ConfigurationData::mode;
    using ConfigurationData::sensitivity;
    using ConfigurationData::brightness;
    using ConfigurationData::palette;
    using ConfigurationData::midi_trs;
    using ConfigurationData::trs_type;
    using ConfigurationData::passthrough;
    using ConfigurationData::midi_ble;
    using ConfigurationData::custom_scale1;
    using ConfigurationData::custom_scale2;
    using ConfigurationData::hasChanged;
};

struct EnhancedQuickSettingsData : public QuickSettingsData
{
    static const uint8_t NUM_ENHANCED_SETTINGS = 16;  // Add 4 more settings

    SettingPair enhancedSettings[NUM_ENHANCED_SETTINGS] = {
        // Existing settings (Page 1-3)
        {8, 8},   // brightness
        {0, 2},   // midi_trs
        {0, 2},   // trs_type
        {0, 2},   // midi_ble
        {1, 12},  // channel
        {0, 12},  // scale
        {0, 8},   // base_octave
        {0, 12},  // base_note
        {1, 4},   // velocity_curve
        {1, 4},   // aftertouch_curve
        {0, 2},   // flip_x
        {0, 2},   // flip_y
        
        // New enhanced velocity settings (Page 4)
        {1, 2},   // useEnhancedVelocity: 0-1 (2 options)
        {1, 2},   // enableStyleDetection: 0-1 (2 options)
        {7, 10},  // rateWeight: 0.1-1.0 in 0.1 steps (10 options)
        {3, 10}   // peakWeight: 0.1-1.0 in 0.1 steps (10 options)
    };
};

class EnhancedConfigurationManager
{
public:
    EnhancedConfigurationManager() = default;

    void Init(DataManager& dataManager)
    {
        _dataManager = &dataManager;
        LoadConfiguration();
    }

    void LoadConfiguration()
    {
        if (!_dataManager)
        {
            log_e("DataManager not initialized");
            return;
        }

        String configJson = _dataManager->ReadFile("/configuration_data.json");
        if (configJson.isEmpty())
        {
            log_w("No enhanced configuration found, using defaults");
            SetDefaultConfiguration();
            return;
        }

        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, configJson);
        
        if (error)
        {
            log_e("Failed to parse enhanced configuration: %s", error.c_str());
            SetDefaultConfiguration();
            return;
        }

        ParseConfiguration(doc);
        log_d("Enhanced configuration loaded successfully");
    }

    void SaveConfiguration()
    {
        if (!_dataManager)
        {
            log_e("DataManager not initialized");
            return;
        }

        DynamicJsonDocument doc(2048);
        SerializeConfiguration(doc);

        String configJson;
        serializeJson(doc, configJson);

        if (_dataManager->WriteFile("/configuration_data.json", configJson))
        {
            _config.hasChanged = false;
            _config.velocityConfig.hasChanged = false;
            log_d("Enhanced configuration saved successfully");
        }
        else
        {
            log_e("Failed to save enhanced configuration");
        }
    }

    void LoadVelocityCalibration()
    {
        if (!_dataManager)
        {
            return;
        }

        String calibrationJson = _dataManager->ReadFile("/velocity_calibration.json");
        if (!calibrationJson.isEmpty())
        {
            _velocityCalibration.LoadCalibrationFromJson(calibrationJson);
            log_d("Velocity calibration loaded");
        }
    }

    void SaveVelocityCalibration()
    {
        if (!_dataManager)
        {
            return;
        }

        String calibrationJson = _velocityCalibration.SaveCalibrationToJson();
        if (_dataManager->WriteFile("/velocity_calibration.json", calibrationJson))
        {
            log_d("Velocity calibration saved");
        }
        else
        {
            log_e("Failed to save velocity calibration");
        }
    }

    EnhancedConfigurationData& GetConfig()
    {
        return _config;
    }

    const EnhancedConfigurationData& GetConfig() const
    {
        return _config;
    }

    EnhancedQuickSettingsData& GetQuickSettings()
    {
        return _quickSettings;
    }

    VelocityCalibrationSystem& GetVelocityCalibration()
    {
        return _velocityCalibration;
    }

    EnhancedKey::KeyConfig GetKeyConfig() const
    {
        EnhancedKey::KeyConfig keyConfig;
        keyConfig.useEnhancedVelocity = _config.velocityConfig.useEnhancedVelocity;
        keyConfig.enableStyleDetection = _config.velocityConfig.enableStyleDetection;
        keyConfig.enableTemperatureCompensation = _config.velocityConfig.enableTemperatureCompensation;
        keyConfig.legacyVelocityWeight = _config.velocityConfig.legacyVelocityWeight;
        keyConfig.enhancedVelocityWeight = _config.velocityConfig.enhancedVelocityWeight;
        return keyConfig;
    }

    HybridVelocityAnalyzer::AnalyzerConfig GetAnalyzerConfig() const
    {
        HybridVelocityAnalyzer::AnalyzerConfig analyzerConfig;
        
        analyzerConfig.velocityConfig.rateWeight = _config.velocityConfig.rateWeight;
        analyzerConfig.velocityConfig.peakWeight = _config.velocityConfig.peakWeight;
        analyzerConfig.velocityConfig.historySize = _config.velocityConfig.historySize;
        analyzerConfig.velocityConfig.noiseThreshold = _config.velocityConfig.noiseThreshold;
        analyzerConfig.velocityConfig.temperatureComp = _config.velocityConfig.temperatureComp;
        analyzerConfig.velocityConfig.sensorDriftComp = _config.velocityConfig.sensorDriftComp;
        
        analyzerConfig.styleAdaptationRate = _config.velocityConfig.styleAdaptationRate;
        analyzerConfig.enableStyleDetection = _config.velocityConfig.enableStyleDetection;
        analyzerConfig.enableTemperatureCompensation = _config.velocityConfig.enableTemperatureCompensation;

        return analyzerConfig;
    }

    void UpdateVelocityConfig(const EnhancedVelocityConfig& velocityConfig)
    {
        _config.velocityConfig = velocityConfig;
        _config.velocityConfig.hasChanged = true;
        _config.hasChanged = true;
    }

    void UpdateQuickSettingsFromEnhanced(uint8_t setting, uint8_t value)
    {
        if (setting >= EnhancedQuickSettingsData::NUM_ENHANCED_SETTINGS)
        {
            return;
        }

        _quickSettings.enhancedSettings[setting].value = value;

        // Map enhanced settings back to velocity config
        switch (setting)
        {
            case 12: // useEnhancedVelocity
                _config.velocityConfig.useEnhancedVelocity = (value > 0);
                break;
            case 13: // enableStyleDetection
                _config.velocityConfig.enableStyleDetection = (value > 0);
                break;
            case 14: // rateWeight
                _config.velocityConfig.rateWeight = (value + 1) * 0.1f;
                break;
            case 15: // peakWeight
                _config.velocityConfig.peakWeight = (value + 1) * 0.1f;
                break;
        }

        _config.velocityConfig.hasChanged = true;
        _config.hasChanged = true;
    }

    String GetQuickSettingName(uint8_t setting) const
    {
        switch (setting)
        {
            case 0: return "Brightness";
            case 1: return "MIDI TRS";
            case 2: return "TRS Type";
            case 3: return "MIDI BLE";
            case 4: return "Channel";
            case 5: return "Scale";
            case 6: return "Base Octave";
            case 7: return "Base Note";
            case 8: return "Velocity Curve";
            case 9: return "Aftertouch Curve";
            case 10: return "Flip X";
            case 11: return "Flip Y";
            case 12: return "Enhanced Velocity";
            case 13: return "Style Detection";
            case 14: return "Rate Weight";
            case 15: return "Peak Weight";
            default: return "Unknown";
        }
    }

    uint8_t GetQuickSettingPage(uint8_t setting) const
    {
        if (setting < 4) return 0;      // Page 1
        if (setting < 8) return 1;      // Page 2
        if (setting < 12) return 2;     // Page 3
        return 3;                       // Page 4 (Enhanced velocity settings)
    }

    bool IsConfigurationChanged() const
    {
        return _config.hasChanged || _config.velocityConfig.hasChanged;
    }

    void MarkConfigurationSaved()
    {
        _config.hasChanged = false;
        _config.velocityConfig.hasChanged = false;
    }

private:
    DataManager* _dataManager = nullptr;
    EnhancedConfigurationData _config;
    EnhancedQuickSettingsData _quickSettings;
    VelocityCalibrationSystem _velocityCalibration;

    void SetDefaultConfiguration()
    {
        _config = EnhancedConfigurationData{};
        _config.velocityConfig = EnhancedVelocityConfig{};
        _config.hasChanged = true;
        log_d("Default enhanced configuration set");
    }

    void ParseConfiguration(const DynamicJsonDocument& doc)
    {
        // Parse base configuration
        _config.version = doc["version"] | 103;
        _config.mode = doc["mode"] | 0;
        _config.sensitivity = doc["sensitivity"] | 1;
        _config.brightness = doc["brightness"] | 6;
        _config.palette = doc["palette"] | 0;
        _config.midi_trs = doc["midi_trs"] | 0;
        _config.trs_type = doc["trs_type"] | 0;
        _config.passthrough = doc["passthrough"] | 0;
        _config.midi_ble = doc["midi_ble"] | 0;

        // Parse custom scales
        JsonArray scale1 = doc["custom_scale1"];
        JsonArray scale2 = doc["custom_scale2"];
        
        for (uint8_t i = 0; i < 16; i++)
        {
            _config.custom_scale1[i] = scale1[i] | i;
            _config.custom_scale2[i] = scale2[i] | i;
        }

        // Parse enhanced velocity configuration
        JsonObject velocityObj = doc["velocityConfig"];
        if (!velocityObj.isNull())
        {
            _config.velocityConfig.useEnhancedVelocity = velocityObj["useEnhanced"] | true;
            _config.velocityConfig.enableStyleDetection = velocityObj["styleDetection"] | true;
            _config.velocityConfig.enableTemperatureCompensation = velocityObj["tempCompensation"] | true;
            _config.velocityConfig.legacyVelocityWeight = velocityObj["legacyWeight"] | 0.3f;
            _config.velocityConfig.enhancedVelocityWeight = velocityObj["enhancedWeight"] | 0.7f;
            _config.velocityConfig.rateWeight = velocityObj["rateWeight"] | 0.7f;
            _config.velocityConfig.peakWeight = velocityObj["peakWeight"] | 0.3f;
            _config.velocityConfig.styleAdaptationRate = velocityObj["styleRate"] | 0.1f;
            _config.velocityConfig.historySize = velocityObj["historySize"] | 8;
            _config.velocityConfig.noiseThreshold = velocityObj["noiseThreshold"] | 0.005f;
            _config.velocityConfig.temperatureComp = velocityObj["tempComp"] | 0.98f;
            _config.velocityConfig.sensorDriftComp = velocityObj["driftComp"] | 0.995f;
        }

        _config.hasChanged = false;
        _config.velocityConfig.hasChanged = false;
    }

    void SerializeConfiguration(DynamicJsonDocument& doc)
    {
        // Serialize base configuration
        doc["version"] = _config.version;
        doc["mode"] = _config.mode;
        doc["sensitivity"] = _config.sensitivity;
        doc["brightness"] = _config.brightness;
        doc["palette"] = _config.palette;
        doc["midi_trs"] = _config.midi_trs;
        doc["trs_type"] = _config.trs_type;
        doc["passthrough"] = _config.passthrough;
        doc["midi_ble"] = _config.midi_ble;

        // Serialize custom scales
        JsonArray scale1 = doc.createNestedArray("custom_scale1");
        JsonArray scale2 = doc.createNestedArray("custom_scale2");
        
        for (uint8_t i = 0; i < 16; i++)
        {
            scale1.add(_config.custom_scale1[i]);
            scale2.add(_config.custom_scale2[i]);
        }

        // Serialize enhanced velocity configuration
        JsonObject velocityObj = doc.createNestedObject("velocityConfig");
        velocityObj["useEnhanced"] = _config.velocityConfig.useEnhancedVelocity;
        velocityObj["styleDetection"] = _config.velocityConfig.enableStyleDetection;
        velocityObj["tempCompensation"] = _config.velocityConfig.enableTemperatureCompensation;
        velocityObj["legacyWeight"] = _config.velocityConfig.legacyVelocityWeight;
        velocityObj["enhancedWeight"] = _config.velocityConfig.enhancedVelocityWeight;
        velocityObj["rateWeight"] = _config.velocityConfig.rateWeight;
        velocityObj["peakWeight"] = _config.velocityConfig.peakWeight;
        velocityObj["styleRate"] = _config.velocityConfig.styleAdaptationRate;
        velocityObj["historySize"] = _config.velocityConfig.historySize;
        velocityObj["noiseThreshold"] = _config.velocityConfig.noiseThreshold;
        velocityObj["tempComp"] = _config.velocityConfig.temperatureComp;
        velocityObj["driftComp"] = _config.velocityConfig.sensorDriftComp;
    }
};

// Global enhanced configuration instance
extern EnhancedConfigurationManager enhancedConfig;

#endif // ENHANCED_CONFIGURATION_HPP