#ifndef CONFIGURATION_RECOVERY_HPP
#define CONFIGURATION_RECOVERY_HPP

#include <Arduino.h>
#include "DataIntegrity.hpp"
#include "ConfigurationValidator.hpp"
#include "CriticalDataProtector.hpp"
#include "../../Configuration.hpp"

namespace DataIntegrity {

enum class RecoveryReason : uint8_t {
    BEFORE_FIRMWARE_UPDATE,
    BEFORE_CRITICAL_CHANGE,
    PERIODIC_BACKUP,
    USER_REQUEST,
    CORRUPTION_DETECTED,
    VALIDATION_FAILURE
};

enum class RecoveryLevel : uint8_t {
    PARAMETER_RESET,     // Reset single parameter to default
    BANK_RESET,          // Reset entire bank to defaults
    FACTORY_RESET,       // Reset everything to factory defaults
    RECOVERY_SNAPSHOT,   // Restore from automatic snapshot
    CALIBRATION_PRESERVE // Factory reset but preserve calibration
};

struct RecoverySnapshot {
    ConfigurationData config;
    CalibrationData calibration;
    uint32_t timestamp;
    uint32_t checksum;
    RecoveryReason reason;
    char description[64];
    uint8_t firmwareVersion;
    bool isValid;
};

class ConfigurationRecoverySystem {
public:
    static constexpr uint8_t MAX_SNAPSHOTS = 5;
    static constexpr uint32_t SNAPSHOT_INTERVAL = 3600000; // 1 hour in ms
    
    ConfigurationRecoverySystem();
    
    // Initialize recovery system
    bool initialize();
    
    // Create recovery snapshot
    bool createSnapshot(RecoveryReason reason, const char* description);
    bool createSnapshot(const ConfigurationData& config, const CalibrationData& calib,
                       RecoveryReason reason, const char* description);
    
    // Automatic recovery
    bool attemptAutoRecovery(const ValidationResult& validation);
    bool attemptAutoRecovery(RecoveryLevel level = RecoveryLevel::RECOVERY_SNAPSHOT);
    
    // Manual recovery options
    bool recoverFromSnapshot(uint8_t snapshotIndex);
    bool recoverToFactoryDefaults(bool preserveCalibration = true);
    bool recoverSingleParameter(const char* paramName, uint8_t bank = 0);
    bool recoverBank(uint8_t bank);
    
    // Snapshot management
    uint8_t getSnapshotCount() const;
    const RecoverySnapshot* getSnapshot(uint8_t index) const;
    void listSnapshots() const;
    bool deleteSnapshot(uint8_t index);
    void deleteAllSnapshots();
    
    // Periodic snapshot management
    void checkPeriodicSnapshot();
    
    // Get recovery statistics
    struct RecoveryStats {
        uint32_t totalRecoveries;
        uint32_t successfulRecoveries;
        uint32_t failedRecoveries;
        uint32_t lastRecoveryTimestamp;
        RecoveryLevel lastRecoveryLevel;
    };
    RecoveryStats getStats() const { return stats; }
    
private:
    RecoverySnapshot snapshots[MAX_SNAPSHOTS];
    uint8_t currentSnapshotIndex;
    uint8_t snapshotCount;
    uint32_t lastPeriodicSnapshot;
    RecoveryStats stats;
    
    CriticalDataProtector* dataProtector;
    ConfigurationValidator validator;
    
    // Recovery methods
    bool tryParameterRecovery(const ValidationResult& validation);
    bool trySnapshotRecovery();
    bool tryFactoryRecovery(bool preserveCalibration);
    
    // Helper methods
    void saveSnapshotToStorage(uint8_t index);
    bool loadSnapshotFromStorage(uint8_t index);
    bool loadAllSnapshots();
    bool saveAllSnapshots();
    
    bool validateSnapshot(const RecoverySnapshot& snapshot) const;
    void updateSnapshotChecksum(RecoverySnapshot& snapshot);
    bool verifySnapshotChecksum(const RecoverySnapshot& snapshot) const;
    
    // Apply recovery
    bool applyConfiguration(const ConfigurationData& config);
    bool applyCalibration(const CalibrationData& calib);
    
    // Default value getters
    uint8_t getDefaultValue(const char* paramName) const;
    void getDefaultBankConfig(uint8_t bank, KeyModeData& kb_cfg, ControlChangeData& cc_cfg) const;
    
    // Logging
    void logRecoveryAttempt(RecoveryLevel level, bool success);
    
    // File management
    static constexpr const char* SNAPSHOT_FILE_PREFIX = "/snapshot_";
    static constexpr const char* RECOVERY_STATS_FILE = "/recovery_stats.dat";
    
    String getSnapshotFilename(uint8_t index) const;
};

// Global recovery system instance
extern ConfigurationRecoverySystem* recoverySystem;

// Helper class for automatic snapshot creation
class AutoSnapshot {
private:
    bool created;
    
public:
    AutoSnapshot(RecoveryReason reason, const char* description) : created(false) {
        if (recoverySystem) {
            created = recoverySystem->createSnapshot(reason, description);
        }
    }
    
    ~AutoSnapshot() {
        if (!created) {
            log_w("AutoSnapshot failed to create snapshot");
        }
    }
    
    bool wasCreated() const { return created; }
};

// Macro for easy snapshot creation before risky operations
#define CREATE_RECOVERY_SNAPSHOT(reason, desc) \
    AutoSnapshot _snapshot(reason, desc)

} // namespace DataIntegrity

#endif // CONFIGURATION_RECOVERY_HPP