// Simple integration example for ConfigSafety
// Replace complex validation system with this

#include "Libs/ConfigSafety.hpp"

// In setup() function
void setup() {
    // ... existing setup code ...
    
    // Initialize config safety system
    if (!ConfigSafety::initialize()) {
        log_e("Config safety system failed to initialize");
    }
    
    // Perform boot check - validates config and stores good copy
    bool configWasClean = ConfigSafety::bootCheck();
    
    if (!configWasClean) {
        log_w("Configuration issues were found and fixed on boot");
    }
    
    log_i("Config safety system active");
    log_i(ConfigSafety::getGoodConfigInfo().c_str());
    
    // ... rest of setup ...
}

// Before applying new configuration (e.g., from web interface)
bool applyNewConfiguration(const String& newConfigJson) {
    // Store current config as good before making changes
    ConfigSafety::storeGoodConfig();
    
    // Apply the new configuration
    // ... your existing config application code ...
    
    // Validate the newly applied config
    extern ConfigurationData cfg;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT];
    extern CalibrationData calibration_data;
    
    bool hadIssues = false;
    hadIssues |= ConfigSafety::validateAndFix(cfg);
    hadIssues |= ConfigSafety::validateAndFixBanks(kb_cfg, cc_cfg);
    hadIssues |= ConfigSafety::validateAndFixCalibration(calibration_data);
    
    if (hadIssues) {
        log_w("New configuration had issues - auto-fixed");
        // Save the fixed version
        extern DataManager config;
        SaveConfiguration(config, true);
    }
    
    return true; // Success
}

// Web interface or MIDI command handlers
void handleSafetyCommand(const String& command) {
    if (command == "config_info") {
        Serial.println(ConfigSafety::getGoodConfigInfo());
    }
    else if (command == "restore_good") {
        if (ConfigSafety::restoreGoodConfig()) {
            Serial.println("Good config restored - restarting");
            ESP.restart();
        } else {
            Serial.println("Failed to restore - no good config available");
        }
    }
    else if (command == "factory_reset") {
        Serial.println("Performing factory reset (keeping calibration)");
        ConfigSafety::factoryReset();
        Serial.println("Factory reset complete");
    }
    else if (command == "emergency_recovery") {
        Serial.println("Emergency recovery initiated");
        ConfigSafety::emergencyRecovery();
        // Will restart if successful
    }
}

// Example of how to use in error handling
void onConfigurationError() {
    log_e("Configuration error detected!");
    
    // Try to recover
    if (ConfigSafety::hasGoodConfig()) {
        log_i("Attempting to restore last good configuration");
        ConfigSafety::restoreGoodConfig();
        
        // Reload configuration
        extern DataManager config;
        LoadConfiguration(config, false);
        
        log_i("Configuration restored from backup");
    } else {
        log_w("No good config available - performing emergency recovery");
        ConfigSafety::emergencyRecovery();
    }
}

// Simple button handler for hardware reset button
void onResetButtonPressed() {
    static unsigned long pressTime = 0;
    static bool longPress = false;
    
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        if (pressTime == 0) {
            pressTime = millis();
        } else if (millis() - pressTime > 3000 && !longPress) {
            // Long press detected
            longPress = true;
            log_i("Long press detected - factory reset");
            ConfigSafety::factoryReset();
            
            // Visual feedback - flash LEDs
            // ... your LED feedback code ...
        }
    } else {
        // Button released
        if (pressTime > 0 && millis() - pressTime < 3000 && !longPress) {
            // Short press - restore good config
            log_i("Short press - restoring good config");
            if (ConfigSafety::hasGoodConfig()) {
                ConfigSafety::restoreGoodConfig();
                ESP.restart();
            }
        }
        
        pressTime = 0;
        longPress = false;
    }
}