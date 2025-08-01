// Example integration of SimpleBackup system
// Add these to your main.cpp or appropriate places

#include "Libs/SimpleBackup.hpp"

// In setup() - initialize backup system
void setup() {
    // ... existing setup code ...
    
    // Initialize backup system
    if (!SimpleBackup::initialize()) {
        log_e("Backup system failed to initialize");
    }
    
    // Load and validate configuration
    extern ConfigurationData cfg;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT];
    extern CalibrationData calibration_data;
    extern DataManager config;
    
    // Load configuration normally
    LoadConfiguration(config, false);
    
    // Validate and auto-fix if needed
    bool configFixed = SimpleBackup::validateAndFixConfig(cfg);
    bool banksFixed = SimpleBackup::validateAndFixBanks(kb_cfg, cc_cfg);
    bool calibFixed = SimpleBackup::validateCalibration(calibration_data);
    
    if (configFixed || banksFixed || calibFixed) {
        log_w("Configuration issues found and auto-fixed");
        SaveConfiguration(config, true); // Save the fixes
    }
    
    // Create initial backup if none exists
    if (!SimpleBackup::hasBackup()) {
        log_i("Creating initial backup...");
        SimpleBackup::createBackup();
    }
    
    // ... rest of setup ...
}

// Before major configuration changes (e.g., firmware update, factory reset)
void beforeMajorChange() {
    log_i("Creating safety backup before major change...");
    if (SimpleBackup::createBackup()) {
        log_i("Backup created successfully");
    } else {
        log_e("Failed to create backup - proceeding anyway");
    }
}

// In web interface or MIDI command handlers
void handleBackupCommand(const String& command) {
    if (command == "backup_create") {
        if (SimpleBackup::createBackup()) {
            // Send success response to web interface
            Serial.println("Backup created successfully");
        } else {
            Serial.println("Backup creation failed");
        }
    }
    else if (command == "backup_restore") {
        if (SimpleBackup::restoreBackup()) {
            Serial.println("Backup restored - please restart device");
            // Could trigger automatic restart here
            ESP.restart();
        } else {
            Serial.println("Backup restore failed");
        }
    }
    else if (command == "backup_info") {
        Serial.println(SimpleBackup::getBackupInfo());
    }
    else if (command == "backup_delete") {
        SimpleBackup::deleteBackups();
        Serial.println("Backups deleted");
    }
}

// Factory reset with calibration preservation
void factoryResetSafe() {
    log_w("Performing safe factory reset...");
    
    // Create backup first
    SimpleBackup::createBackup();
    
    // Reset configuration to defaults
    extern ConfigurationData cfg;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT]; 
    extern DataManager config;
    
    // Reset main config
    cfg = ConfigurationData{}; // This initializes with defaults
    cfg.version = 102;
    cfg.brightness = 6;
    cfg.sensitivity = 1;
    
    // Reset all banks to defaults
    for (int bank = 0; bank < BANK_AMT; bank++) {
        kb_cfg[bank] = KeyModeData{};
        kb_cfg[bank].palette = bank;
        kb_cfg[bank].channel = 1;
        kb_cfg[bank].base_octave = 2;
        kb_cfg[bank].velocity_curve = 1;
        kb_cfg[bank].aftertouch_curve = 1;
        
        cc_cfg[bank] = ControlChangeData{};
        for (int cc = 0; cc < CC_AMT; cc++) {
            cc_cfg[bank].channel[cc] = 1;
            cc_cfg[bank].id[cc] = 13 + cc;
        }
    }
    
    // Save factory configuration
    SaveConfiguration(config, true);
    
    // Note: Calibration data is NOT reset - it's preserved
    log_i("Factory reset complete - calibration preserved");
}

// Emergency recovery function
void emergencyRecovery() {
    log_w("Attempting emergency recovery...");
    
    if (SimpleBackup::hasBackup()) {
        log_i("Backup found - attempting restore...");
        if (SimpleBackup::restoreBackup()) {
            log_i("Emergency recovery successful");
            ESP.restart();
            return;
        }
    }
    
    log_w("No backup available - performing factory reset");
    factoryResetSafe();
    ESP.restart();
}