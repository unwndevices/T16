#ifndef SIMPLE_BACKUP_HPP
#define SIMPLE_BACKUP_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include "../Configuration.hpp"

class SimpleBackup {
public:
    // Initialize backup system
    static bool initialize() {
        return LittleFS.begin();
    }
    
    // Create backup of current configuration and calibration
    static bool createBackup() {
        bool success = true;
        
        // Backup configuration
        success &= backupConfiguration();
        
        // Backup calibration
        success &= backupCalibration();
        
        if (success) {
            log_i("Backup created successfully");
        } else {
            log_e("Backup creation failed");
        }
        
        return success;
    }
    
    // Restore from backup
    static bool restoreBackup() {
        bool success = true;
        
        // Restore configuration
        success &= restoreConfiguration();
        
        // Restore calibration  
        success &= restoreCalibration();
        
        if (success) {
            log_i("Backup restored successfully");
        } else {
            log_e("Backup restore failed");
        }
        
        return success;
    }
    
    // Check if backup exists
    static bool hasBackup() {
        return LittleFS.exists("/config_backup.json") && 
               LittleFS.exists("/calib_backup.json");
    }
    
    // Delete backups (free space)
    static void deleteBackups() {
        LittleFS.remove("/config_backup.json");
        LittleFS.remove("/calib_backup.json");
        log_i("Backups deleted");
    }
    
    // Get backup info
    static String getBackupInfo() {
        if (!hasBackup()) {
            return "No backup available";
        }
        
        File configFile = LittleFS.open("/config_backup.json", "r");
        File calibFile = LittleFS.open("/calib_backup.json", "r");
        
        String info = "Backup available:\n";
        info += "Config: " + String(configFile.size()) + " bytes\n";
        info += "Calibration: " + String(calibFile.size()) + " bytes";
        
        configFile.close();
        calibFile.close();
        
        return info;
    }
    
    // Simple validation - fix obvious problems
    static bool validateAndFixConfig(ConfigurationData& cfg) {
        bool fixed = false;
        
        // Fix mode
        if (cfg.mode > 4) {
            cfg.mode = 0;
            fixed = true;
        }
        
        // Fix brightness  
        if (cfg.brightness > 15) {
            cfg.brightness = 6;
            fixed = true;
        }
        
        // Fix sensitivity
        if (cfg.sensitivity > 4) {
            cfg.sensitivity = 1;
            fixed = true;
        }
        
        // Fix MIDI settings
        if (cfg.midi_trs > 1) cfg.midi_trs = 0;
        if (cfg.trs_type > 1) cfg.trs_type = 0;
        if (cfg.passthrough > 1) cfg.passthrough = 0;
        if (cfg.midi_ble > 1) cfg.midi_ble = 0;
        
        if (fixed) {
            cfg.hasChanged = true;
            log_w("Configuration auto-fixed");
        }
        
        return fixed;
    }
    
    // Validate bank configurations
    static bool validateAndFixBanks(KeyModeData kb_cfg[BANK_AMT], ControlChangeData cc_cfg[BANK_AMT]) {
        bool fixed = false;
        
        for (int bank = 0; bank < BANK_AMT; bank++) {
            // Fix keyboard config
            if (kb_cfg[bank].channel < 1 || kb_cfg[bank].channel > 16) {
                kb_cfg[bank].channel = 1;
                fixed = true;
            }
            
            if (kb_cfg[bank].scale > 15) {
                kb_cfg[bank].scale = 0;
                fixed = true;
            }
            
            if (kb_cfg[bank].base_octave > 7) {
                kb_cfg[bank].base_octave = 2;
                fixed = true;
            }
            
            if (kb_cfg[bank].velocity_curve > 3) {
                kb_cfg[bank].velocity_curve = 1;
                fixed = true;
            }
            
            if (kb_cfg[bank].aftertouch_curve > 3) {
                kb_cfg[bank].aftertouch_curve = 1;
                fixed = true;
            }
            
            // Fix CC config
            for (int cc = 0; cc < CC_AMT; cc++) {
                if (cc_cfg[bank].channel[cc] < 1 || cc_cfg[bank].channel[cc] > 16) {
                    cc_cfg[bank].channel[cc] = 1;
                    fixed = true;
                }
                
                if (cc_cfg[bank].id[cc] > 127) {
                    cc_cfg[bank].id[cc] = 13 + cc; // Default mapping
                    fixed = true;
                }
            }
            
            if (fixed) {
                kb_cfg[bank].hasChanged = true;
                cc_cfg[bank].hasChanged = true;
            }
        }
        
        if (fixed) {
            log_w("Bank configurations auto-fixed");
        }
        
        return fixed;
    }
    
    // Validate calibration data
    static bool validateCalibration(CalibrationData& calib) {
        bool fixed = false;
        
        for (int i = 0; i < 16; i++) {
            // Fix inverted min/max
            if (calib.minVal[i] > calib.maxVal[i]) {
                calib.minVal[i] = 0;
                calib.maxVal[i] = 4095;
                fixed = true;
            }
            
            // Fix out-of-range values
            if (calib.maxVal[i] > 4095) {
                calib.maxVal[i] = 4095;
                fixed = true;
            }
        }
        
        if (fixed) {
            log_w("Calibration data auto-fixed");
        }
        
        return fixed;
    }

private:
    static bool backupConfiguration() {
        // Copy current config file to backup
        File source = LittleFS.open("/configuration_data.json", "r");
        if (!source) {
            log_e("Could not open config file for backup");
            return false;
        }
        
        File backup = LittleFS.open("/config_backup.json", "w");
        if (!backup) {
            source.close();
            log_e("Could not create config backup file");
            return false;
        }
        
        // Copy file contents
        while (source.available()) {
            backup.write(source.read());
        }
        
        source.close();
        backup.close();
        
        return true;
    }
    
    static bool backupCalibration() {
        // Copy calibration file to backup
        File source = LittleFS.open("/calibration_data.json", "r");
        if (!source) {
            log_e("Could not open calibration file for backup");
            return false;
        }
        
        File backup = LittleFS.open("/calib_backup.json", "w");
        if (!backup) {
            source.close();
            log_e("Could not create calibration backup file");
            return false;
        }
        
        // Copy file contents
        while (source.available()) {
            backup.write(source.read());
        }
        
        source.close();
        backup.close();
        
        return true;
    }
    
    static bool restoreConfiguration() {
        if (!LittleFS.exists("/config_backup.json")) {
            log_e("No config backup found");
            return false;
        }
        
        // Copy backup to main file
        File backup = LittleFS.open("/config_backup.json", "r");
        File dest = LittleFS.open("/configuration_data.json", "w");
        
        if (!backup || !dest) {
            if (backup) backup.close();
            if (dest) dest.close();
            return false;
        }
        
        while (backup.available()) {
            dest.write(backup.read());
        }
        
        backup.close();
        dest.close();
        
        return true;
    }
    
    static bool restoreCalibration() {
        if (!LittleFS.exists("/calib_backup.json")) {
            log_e("No calibration backup found");
            return false;
        }
        
        // Copy backup to main file
        File backup = LittleFS.open("/calib_backup.json", "r");
        File dest = LittleFS.open("/calibration_data.json", "w");
        
        if (!backup || !dest) {
            if (backup) backup.close();
            if (dest) dest.close();
            return false;
        }
        
        while (backup.available()) {
            dest.write(backup.read());
        }
        
        backup.close();
        dest.close();
        
        return true;
    }
};

#endif // SIMPLE_BACKUP_HPP