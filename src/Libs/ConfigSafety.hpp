#ifndef CONFIG_SAFETY_HPP
#define CONFIG_SAFETY_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include "../Configuration.hpp"

class ConfigSafety {
private:
    static constexpr const char* GOOD_CONFIG_FILE = "/last_good_config.json";
    static constexpr const char* GOOD_CALIB_FILE = "/last_good_calib.json";

public:
    // Initialize system
    static bool initialize() {
        return LittleFS.begin();
    }
    
    // Simple validator - fix obvious problems and return if anything was fixed
    static bool validateAndFix(ConfigurationData& cfg) {
        bool fixed = false;
        
        // Fix basic parameters
        if (cfg.mode > 4) { cfg.mode = 0; fixed = true; }
        if (cfg.sensitivity > 4) { cfg.sensitivity = 1; fixed = true; }
        if (cfg.brightness > 15) { cfg.brightness = 6; fixed = true; }
        if (cfg.palette >= BANK_AMT) { cfg.palette = 0; fixed = true; }
        
        // Fix boolean flags
        if (cfg.midi_trs > 1) { cfg.midi_trs = 0; fixed = true; }
        if (cfg.trs_type > 1) { cfg.trs_type = 0; fixed = true; }
        if (cfg.passthrough > 1) { cfg.passthrough = 0; fixed = true; }
        if (cfg.midi_ble > 1) { cfg.midi_ble = 0; fixed = true; }
        
        if (fixed) {
            cfg.hasChanged = true;
            log_w("Main config auto-fixed");
        }
        
        return fixed;
    }
    
    // Validate bank configs
    static bool validateAndFixBanks(KeyModeData kb_cfg[BANK_AMT], ControlChangeData cc_cfg[BANK_AMT]) {
        bool fixed = false;
        
        for (int bank = 0; bank < BANK_AMT; bank++) {
            // Keyboard config
            if (kb_cfg[bank].channel < 1 || kb_cfg[bank].channel > 16) {
                kb_cfg[bank].channel = 1; fixed = true;
            }
            if (kb_cfg[bank].scale > 15) {
                kb_cfg[bank].scale = 0; fixed = true;
            }
            if (kb_cfg[bank].base_octave > 7) {
                kb_cfg[bank].base_octave = 2; fixed = true;
            }
            if (kb_cfg[bank].base_note > 127) {
                kb_cfg[bank].base_note = 0; fixed = true;
            }
            if (kb_cfg[bank].velocity_curve > 3) {
                kb_cfg[bank].velocity_curve = 1; fixed = true;
            }
            if (kb_cfg[bank].aftertouch_curve > 3) {
                kb_cfg[bank].aftertouch_curve = 1; fixed = true;
            }
            if (kb_cfg[bank].flip_x > 1) {
                kb_cfg[bank].flip_x = 0; fixed = true;
            }
            if (kb_cfg[bank].flip_y > 1) {
                kb_cfg[bank].flip_y = 0; fixed = true;
            }
            if (kb_cfg[bank].koala_mode > 1) {
                kb_cfg[bank].koala_mode = 0; fixed = true;
            }
            
            // CC config
            for (int cc = 0; cc < CC_AMT; cc++) {
                if (cc_cfg[bank].channel[cc] < 1 || cc_cfg[bank].channel[cc] > 16) {
                    cc_cfg[bank].channel[cc] = 1; fixed = true;
                }
                if (cc_cfg[bank].id[cc] > 127) {
                    cc_cfg[bank].id[cc] = 13 + cc; fixed = true;
                }
            }
            
            if (fixed) {
                kb_cfg[bank].hasChanged = true;
                cc_cfg[bank].hasChanged = true;
            }
        }
        
        if (fixed) {
            log_w("Bank configs auto-fixed");
        }
        
        return fixed;
    }
    
    // Validate calibration
    static bool validateAndFixCalibration(CalibrationData& calib) {
        bool fixed = false;
        
        for (int i = 0; i < 16; i++) {
            // Fix obviously wrong values
            if (calib.minVal[i] > calib.maxVal[i]) {
                // Swap them
                uint16_t temp = calib.minVal[i];
                calib.minVal[i] = calib.maxVal[i];
                calib.maxVal[i] = temp;
                fixed = true;
            }
            
            // Fix out-of-range ADC values (12-bit = 0-4095)
            if (calib.maxVal[i] > 4095) {
                calib.maxVal[i] = 4095; fixed = true;
            }
            
            // Fix suspiciously identical values (probably corrupted)
            if (calib.minVal[i] == calib.maxVal[i]) {
                calib.minVal[i] = 0;
                calib.maxVal[i] = 4095;
                fixed = true;
            }
        }
        
        if (fixed) {
            log_w("Calibration auto-fixed");
        }
        
        return fixed;
    }
    
    // Store current config as "last known good"
    static bool storeGoodConfig() {
        // Copy current config file to "good" version
        if (!copyFile("/configuration_data.json", GOOD_CONFIG_FILE)) {
            log_e("Failed to store good config");
            return false;
        }
        
        if (!copyFile("/calibration_data.json", GOOD_CALIB_FILE)) {
            log_e("Failed to store good calibration");
            return false;
        }
        
        log_i("Good config stored");
        return true;
    }
    
    // Restore from "last known good"
    static bool restoreGoodConfig() {
        bool success = true;
        
        if (LittleFS.exists(GOOD_CONFIG_FILE)) {
            success &= copyFile(GOOD_CONFIG_FILE, "/configuration_data.json");
        } else {
            log_w("No good config found");
            success = false;
        }
        
        if (LittleFS.exists(GOOD_CALIB_FILE)) {
            success &= copyFile(GOOD_CALIB_FILE, "/calibration_data.json");
        } else {
            log_w("No good calibration found");
            success = false;
        }
        
        if (success) {
            log_i("Good config restored");
        } else {
            log_e("Failed to restore good config");
        }
        
        return success;
    }
    
    // Check if we have a good config stored
    static bool hasGoodConfig() {
        return LittleFS.exists(GOOD_CONFIG_FILE) && LittleFS.exists(GOOD_CALIB_FILE);
    }
    
    // Get info about stored config
    static String getGoodConfigInfo() {
        if (!hasGoodConfig()) {
            return "No stored config";
        }
        
        File configFile = LittleFS.open(GOOD_CONFIG_FILE, "r");
        File calibFile = LittleFS.open(GOOD_CALIB_FILE, "r");
        
        String info = "Stored config available\n";
        if (configFile) {
            info += "Config: " + String(configFile.size()) + " bytes\n";
            configFile.close();
        }
        if (calibFile) {
            info += "Calibration: " + String(calibFile.size()) + " bytes";
            calibFile.close();
        }
        
        return info;
    }
    
    // Full validation and safety check on boot
    static bool bootCheck() {
        extern ConfigurationData cfg;
        extern KeyModeData kb_cfg[BANK_AMT];
        extern ControlChangeData cc_cfg[BANK_AMT];
        extern CalibrationData calibration_data;
        extern DataManager config;
        
        bool anyFixed = false;
        
        // Try to load configuration normally first
        LoadConfiguration(config, false);
        
        // Validate and fix what we loaded
        anyFixed |= validateAndFix(cfg);
        anyFixed |= validateAndFixBanks(kb_cfg, cc_cfg);
        anyFixed |= validateAndFixCalibration(calibration_data);
        
        // If we had to fix things, save the fixed version
        if (anyFixed) {
            log_w("Config had issues - fixed and saving");
            SaveConfiguration(config, true);
        }
        
        // Store this as our new "good" version
        storeGoodConfig();
        
        return !anyFixed; // Return true if no fixes were needed
    }
    
    // Emergency recovery - restore good config or factory reset
    static void emergencyRecovery() {
        log_w("Emergency recovery initiated");
        
        if (hasGoodConfig()) {
            log_i("Attempting to restore last good config");
            if (restoreGoodConfig()) {
                log_i("Good config restored - restarting");
                ESP.restart();
                return;
            }
        }
        
        log_w("No good config available - factory reset");
        factoryReset();
    }
    
    // Factory reset with calibration preservation
    static void factoryReset() {
        extern ConfigurationData cfg;
        extern KeyModeData kb_cfg[BANK_AMT];
        extern ControlChangeData cc_cfg[BANK_AMT];
        extern DataManager config;
        
        log_w("Factory reset - preserving calibration");
        
        // Reset to defaults
        cfg = ConfigurationData{};
        cfg.version = 102;
        cfg.mode = 0;
        cfg.sensitivity = 1;
        cfg.brightness = 6;
        cfg.palette = 0;
        
        // Initialize custom scales to chromatic
        for (int i = 0; i < 16; i++) {
            cfg.custom_scale1[i] = i;
            cfg.custom_scale2[i] = i;
        }
        
        // Reset all banks
        for (int bank = 0; bank < BANK_AMT; bank++) {
            kb_cfg[bank] = KeyModeData{};
            kb_cfg[bank].palette = bank;
            kb_cfg[bank].channel = 1;
            kb_cfg[bank].scale = 0;
            kb_cfg[bank].base_octave = 2;
            kb_cfg[bank].velocity_curve = 1;
            kb_cfg[bank].aftertouch_curve = 1;
            
            cc_cfg[bank] = ControlChangeData{};
            for (int cc = 0; cc < CC_AMT; cc++) {
                cc_cfg[bank].channel[cc] = 1;
                cc_cfg[bank].id[cc] = 13 + cc;
            }
        }
        
        // Save factory config (calibration is left alone)
        SaveConfiguration(config, true);
        
        // Store this as our new good config
        storeGoodConfig();
        
        log_i("Factory reset complete");
    }

private:
    // Helper to copy files
    static bool copyFile(const char* from, const char* to) {
        File source = LittleFS.open(from, "r");
        if (!source) {
            return false;
        }
        
        File dest = LittleFS.open(to, "w");
        if (!dest) {
            source.close();
            return false;
        }
        
        // Copy data
        while (source.available()) {
            dest.write(source.read());
        }
        
        source.close();
        dest.close();
        return true;
    }
};

#endif // CONFIG_SAFETY_HPP