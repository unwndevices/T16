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
    
    // Validation result for web app feedback
    struct ValidationResult {
        bool isValid = true;
        String warnings = "";
        String errors = "";
        int fixesApplied = 0;
        
        void addWarning(const String& msg) {
            if (warnings.length() > 0) warnings += "; ";
            warnings += msg;
        }
        
        void addError(const String& msg) {
            if (errors.length() > 0) errors += "; ";
            errors += msg;
            isValid = false;
        }
        
        void addFix(const String& msg) {
            addWarning("Fixed: " + msg);
            fixesApplied++;
        }
    };
    
    // Detailed validation for web app with feedback
    static ValidationResult validateConfigDetailed(ConfigurationData& cfg, 
                                                  KeyModeData kb_cfg[BANK_AMT], 
                                                  ControlChangeData cc_cfg[BANK_AMT]) {
        ValidationResult result;
        
        // Validate main config with detailed feedback
        if (cfg.mode > 4) {
            result.addFix("Mode reset from " + String(cfg.mode) + " to 0");
            cfg.mode = 0;
        }
        
        if (cfg.sensitivity > 4) {
            result.addFix("Sensitivity reset from " + String(cfg.sensitivity) + " to 1");
            cfg.sensitivity = 1;
        }
        
        if (cfg.brightness > 15) {
            result.addFix("Brightness clamped from " + String(cfg.brightness) + " to 15");
            cfg.brightness = 15;
        }
        
        if (cfg.palette >= BANK_AMT) {
            result.addFix("Palette reset from " + String(cfg.palette) + " to 0");
            cfg.palette = 0;
        }
        
        // Boolean flags
        if (cfg.midi_trs > 1) {
            result.addFix("MIDI TRS reset to disabled");
            cfg.midi_trs = 0;
        }
        
        if (cfg.trs_type > 1) {
            result.addFix("TRS type reset to Type A");
            cfg.trs_type = 0;
        }
        
        if (cfg.midi_ble > 1) {
            result.addFix("MIDI BLE reset to disabled");
            cfg.midi_ble = 0;
        }
        
        // Validate banks with detailed feedback
        for (int bank = 0; bank < BANK_AMT; bank++) {
            String bankName = "Bank " + String(bank) + ": ";
            
            // Keyboard config
            if (kb_cfg[bank].channel < 1 || kb_cfg[bank].channel > 16) {
                result.addFix(bankName + "MIDI channel reset to 1");
                kb_cfg[bank].channel = 1;
            }
            
            if (kb_cfg[bank].scale > 15) {
                result.addFix(bankName + "Scale reset to chromatic");
                kb_cfg[bank].scale = 0;
            }
            
            if (kb_cfg[bank].base_octave > 7) {
                result.addFix(bankName + "Octave reset to 2");
                kb_cfg[bank].base_octave = 2;
            }
            
            if (kb_cfg[bank].base_note > 127) {
                result.addFix(bankName + "Base note reset to 0");
                kb_cfg[bank].base_note = 0;
            }
            
            if (kb_cfg[bank].velocity_curve > 3) {
                result.addFix(bankName + "Velocity curve reset to 1");
                kb_cfg[bank].velocity_curve = 1;
            }
            
            if (kb_cfg[bank].aftertouch_curve > 3) {
                result.addFix(bankName + "Aftertouch curve reset to 1");
                kb_cfg[bank].aftertouch_curve = 1;
            }
            
            // CC config validation with specific feedback
            for (int cc = 0; cc < CC_AMT; cc++) {
                if (cc_cfg[bank].channel[cc] < 1 || cc_cfg[bank].channel[cc] > 16) {
                    result.addFix(bankName + "CC" + String(cc) + " channel reset to 1");
                    cc_cfg[bank].channel[cc] = 1;
                }
                
                if (cc_cfg[bank].id[cc] > 127) {
                    result.addFix(bankName + "CC" + String(cc) + " ID reset to " + String(13 + cc));
                    cc_cfg[bank].id[cc] = 13 + cc;
                }
            }
            
            // Check for duplicate CC IDs in same bank
            for (int cc1 = 0; cc1 < CC_AMT; cc1++) {
                for (int cc2 = cc1 + 1; cc2 < CC_AMT; cc2++) {
                    if (cc_cfg[bank].id[cc1] == cc_cfg[bank].id[cc2]) {
                        result.addWarning(bankName + "Duplicate CC ID " + String(cc_cfg[bank].id[cc1]) + 
                                        " on CC" + String(cc1) + " and CC" + String(cc2));
                    }
                }
            }
        }
        
        // Mark as changed if fixes were applied
        if (result.fixesApplied > 0) {
            cfg.hasChanged = true;
            for (int bank = 0; bank < BANK_AMT; bank++) {
                kb_cfg[bank].hasChanged = true;
                cc_cfg[bank].hasChanged = true;
            }
        }
        
        return result;
    }
    
    // Validate and apply incoming configuration from web app
    static ValidationResult validateAndApplyConfig(const String& jsonConfig) {
        extern ConfigurationData cfg;
        extern KeyModeData kb_cfg[BANK_AMT];
        extern ControlChangeData cc_cfg[BANK_AMT];
        extern DataManager config;
        
        log_i("Validating incoming configuration from web app");
        
        // Store current good config as backup before applying new one
        if (!storeGoodConfig()) {
            log_w("Failed to backup current config - proceeding anyway");
        }
        
        // Parse the JSON configuration
        // (This would integrate with your existing JSON parsing)
        // For now, assuming the config is already parsed into the structs
        
        // Perform detailed validation with feedback
        ValidationResult result = validateConfigDetailed(cfg, kb_cfg, cc_cfg);
        
        // Save the validated configuration
        if (result.fixesApplied > 0 || result.isValid) {
            SaveConfiguration(config, true);
            log_i("Configuration applied and validated - " + String(result.fixesApplied) + " fixes applied");
        }
        
        return result;
    }
    
    // Simple boot check - just validate existing config
    static bool bootCheck() {
        extern ConfigurationData cfg;
        extern KeyModeData kb_cfg[BANK_AMT];
        extern ControlChangeData cc_cfg[BANK_AMT];
        extern CalibrationData calibration_data;
        extern DataManager config;
        
        // Load existing configuration
        LoadConfiguration(config, false);
        
        // Quick validation - only fix critical issues
        bool anyFixed = false;
        anyFixed |= validateAndFix(cfg);
        anyFixed |= validateAndFixBanks(kb_cfg, cc_cfg);
        anyFixed |= validateAndFixCalibration(calibration_data);
        
        if (anyFixed) {
            log_w("Boot: Config had issues - fixed and saving");
            SaveConfiguration(config, true);
        }
        
        // Store as good config if we don't have one
        if (!hasGoodConfig()) {
            storeGoodConfig();
            log_i("Initial good config stored");
        }
        
        return !anyFixed;
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