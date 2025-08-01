#ifndef CRITICAL_DATA_PROTECTOR_HPP
#define CRITICAL_DATA_PROTECTOR_HPP

#include <Arduino.h>
#include "DataIntegrity.hpp"
#include "ConfigurationValidator.hpp"
#include "../../Configuration.hpp"
#include <LittleFS.h>

namespace DataIntegrity {

template<typename T>
class TripleRedundant {
private:
    T primary;
    T backup1;  
    T backup2;
    uint32_t primaryChecksum;
    uint32_t backup1Checksum;
    uint32_t backup2Checksum;
    uint32_t generationCounter;
    
public:
    TripleRedundant() : generationCounter(0) {
        primary = T{};
        backup1 = T{};
        backup2 = T{};
        updateChecksums();
    }
    
    T getMajorityValue() const {
        uint32_t primaryCrc = crc32(&primary, sizeof(T));
        uint32_t backup1Crc = crc32(&backup1, sizeof(T));
        uint32_t backup2Crc = crc32(&backup2, sizeof(T));
        
        bool primaryValid = (primaryCrc == primaryChecksum);
        bool backup1Valid = (backup1Crc == backup1Checksum);
        bool backup2Valid = (backup2Crc == backup2Checksum);
        
        // Count valid copies
        int validCount = (primaryValid ? 1 : 0) + (backup1Valid ? 1 : 0) + (backup2Valid ? 1 : 0);
        
        if (validCount == 0) {
            log_e("CRITICAL: All redundant copies corrupted!");
            return T{}; // Return default
        }
        
        // All valid and matching
        if (primaryValid && backup1Valid && backup2Valid) {
            if (memcmp(&primary, &backup1, sizeof(T)) == 0 && 
                memcmp(&primary, &backup2, sizeof(T)) == 0) {
                return primary;
            }
        }
        
        // Majority voting
        if (primaryValid && backup1Valid && memcmp(&primary, &backup1, sizeof(T)) == 0) {
            return primary;
        }
        if (primaryValid && backup2Valid && memcmp(&primary, &backup2, sizeof(T)) == 0) {
            return primary;
        }
        if (backup1Valid && backup2Valid && memcmp(&backup1, &backup2, sizeof(T)) == 0) {
            return backup1;
        }
        
        // Single valid copy
        if (primaryValid) {
            log_w("Only primary copy valid, repairing backups");
            const_cast<TripleRedundant*>(this)->repairFromPrimary();
            return primary;
        }
        if (backup1Valid) {
            log_w("Only backup1 valid, repairing others");
            const_cast<TripleRedundant*>(this)->repairFromBackup1();
            return backup1;
        }
        if (backup2Valid) {
            log_w("Only backup2 valid, repairing others");
            const_cast<TripleRedundant*>(this)->repairFromBackup2();
            return backup2;
        }
        
        // Should never reach here
        return T{};
    }
    
    void setValue(const T& value) {
        primary = backup1 = backup2 = value;
        generationCounter++;
        updateChecksums();
    }
    
    bool isCorrupted() const {
        uint32_t primaryCrc = crc32(&primary, sizeof(T));
        uint32_t backup1Crc = crc32(&backup1, sizeof(T));
        uint32_t backup2Crc = crc32(&backup2, sizeof(T));
        
        bool primaryValid = (primaryCrc == primaryChecksum);
        bool backup1Valid = (backup1Crc == backup1Checksum);
        bool backup2Valid = (backup2Crc == backup2Checksum);
        
        return !(primaryValid && backup1Valid && backup2Valid);
    }
    
    uint8_t getValidCopyCount() const {
        uint32_t primaryCrc = crc32(&primary, sizeof(T));
        uint32_t backup1Crc = crc32(&backup1, sizeof(T));
        uint32_t backup2Crc = crc32(&backup2, sizeof(T));
        
        uint8_t count = 0;
        if (primaryCrc == primaryChecksum) count++;
        if (backup1Crc == backup1Checksum) count++;
        if (backup2Crc == backup2Checksum) count++;
        
        return count;
    }
    
    uint32_t getGeneration() const {
        return generationCounter;
    }
    
private:
    void updateChecksums() {
        primaryChecksum = crc32(&primary, sizeof(T));
        backup1Checksum = crc32(&backup1, sizeof(T));
        backup2Checksum = crc32(&backup2, sizeof(T));
    }
    
    void repairFromPrimary() {
        backup1 = backup2 = primary;
        updateChecksums();
    }
    
    void repairFromBackup1() {
        primary = backup2 = backup1;
        updateChecksums();
    }
    
    void repairFromBackup2() {
        primary = backup1 = backup2;
        updateChecksums();
    }
};

class CriticalDataProtector {
public:
    struct CriticalData {
        TripleRedundant<CalibrationData> calibration;
        TripleRedundant<ConfigurationData> configuration;
        TripleRedundant<uint32_t> deviceSerial;
        TripleRedundant<uint8_t> firmwareVersion;
        
        // Checksums for the entire structure
        uint32_t structureChecksum;
        uint32_t magic = 0x54313643; // "T16C"
    };
    
    class ConfigurationTransaction {
    private:
        CriticalDataProtector* protector;
        ConfigurationData originalConfig;
        CalibrationData originalCalibration;
        bool committed = false;
        bool configModified = false;
        bool calibrationModified = false;
        
    public:
        ConfigurationTransaction(CriticalDataProtector* p) : protector(p) {
            originalConfig = protector->getConfiguration();
            originalCalibration = protector->getCalibration();
        }
        
        ~ConfigurationTransaction() {
            if (!committed) {
                rollback();
            }
        }
        
        bool updateConfiguration(const ConfigurationData& newConfig) {
            // Validate new configuration
            ConfigurationValidator validator;
            ConfigurationValidator::ValidationContext context = protector->getCurrentContext();
            ValidationResult validation = validator.validateConfiguration(newConfig, context);
            
            if (!validation.isValid && !validation.canAutoFix()) {
                log_e("Configuration validation failed with unfixable errors");
                return false;
            }
            
            ConfigurationData fixedConfig = newConfig;
            if (!validation.isValid && validation.canAutoFix()) {
                if (validator.autoFixConfiguration(fixedConfig, validation)) {
                    log_w("Configuration auto-fixed");
                } else {
                    log_e("Configuration auto-fix failed");
                    return false;
                }
            }
            
            // Apply configuration atomically
            protector->criticalData.configuration.setValue(fixedConfig);
            configModified = true;
            return true;
        }
        
        bool updateCalibration(const CalibrationData& newCalibration) {
            // Validate calibration
            ConfigurationValidator validator;
            ConfigurationValidator::ValidationContext context = protector->getCurrentContext();
            ValidationResult validation = validator.validateCalibrationData(newCalibration, context);
            
            if (!validation.isValid) {
                log_e("Calibration validation failed");
                return false;
            }
            
            protector->criticalData.calibration.setValue(newCalibration);
            calibrationModified = true;
            return true;
        }
        
        void commit() {
            committed = true;
            protector->updateStructureChecksum();
            protector->persistCriticalData();
            log_i("Configuration transaction committed");
        }
        
        void rollback() {
            if (configModified) {
                protector->criticalData.configuration.setValue(originalConfig);
            }
            if (calibrationModified) {
                protector->criticalData.calibration.setValue(originalCalibration);
            }
            log_w("Configuration transaction rolled back");
        }
        
        bool isModified() const {
            return configModified || calibrationModified;
        }
    };
    
    CriticalDataProtector();
    
    // Initialize and load critical data
    bool initialize();
    
    // Get data with integrity checking
    ConfigurationData getConfiguration() const;
    CalibrationData getCalibration() const;
    uint32_t getDeviceSerial() const;
    uint8_t getFirmwareVersion() const;
    
    // Direct setters (use transactions for configuration/calibration)
    void setDeviceSerial(uint32_t serial);
    void setFirmwareVersion(uint8_t version);
    
    // Transaction management
    ConfigurationTransaction beginTransaction();
    
    // Check data integrity
    bool verifyIntegrity() const;
    uint8_t getCorruptionLevel() const;
    bool repairCorruption();
    
    // Persistence
    bool persistCriticalData();
    bool loadCriticalData();
    
    // Factory reset
    void factoryReset(bool preserveCalibration = true);
    
private:
    static constexpr const char* CRITICAL_DATA_FILE = "/critical_data.dat";
    static constexpr const char* CRITICAL_BACKUP1_FILE = "/critical_backup1.dat";
    static constexpr const char* CRITICAL_BACKUP2_FILE = "/critical_backup2.dat";
    
    CriticalData criticalData;
    
    ConfigurationValidator::ValidationContext getCurrentContext() const;
    void updateStructureChecksum();
    bool verifyStructureChecksum() const;
    
    ConfigurationData getFactoryConfiguration() const;
    CalibrationData getFactoryCalibration() const;
    
    bool writeToFile(const char* filename, const CriticalData& data);
    bool readFromFile(const char* filename, CriticalData& data);
    
    bool performWearLeveling();
};

} // namespace DataIntegrity

#endif // CRITICAL_DATA_PROTECTOR_HPP