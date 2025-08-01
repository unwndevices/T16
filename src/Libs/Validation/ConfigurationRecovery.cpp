#include "ConfigurationRecovery.hpp"
#include <LittleFS.h>

namespace DataIntegrity {

// Global instance
ConfigurationRecoverySystem* recoverySystem = nullptr;

ConfigurationRecoverySystem::ConfigurationRecoverySystem() 
    : currentSnapshotIndex(0), snapshotCount(0), lastPeriodicSnapshot(0) {
    memset(&stats, 0, sizeof(stats));
    memset(snapshots, 0, sizeof(snapshots));
    dataProtector = nullptr;
}

bool ConfigurationRecoverySystem::initialize() {
    // Create global instance if needed
    if (!recoverySystem) {
        recoverySystem = this;
    }
    
    // Load existing snapshots
    if (!loadAllSnapshots()) {
        log_w("No existing snapshots found");
    }
    
    // Load recovery statistics
    File statsFile = LittleFS.open(RECOVERY_STATS_FILE, "r");
    if (statsFile) {
        statsFile.read((uint8_t*)&stats, sizeof(stats));
        statsFile.close();
    }
    
    // Create initial snapshot
    return createSnapshot(RecoveryReason::PERIODIC_BACKUP, "System initialization");
}

bool ConfigurationRecoverySystem::createSnapshot(RecoveryReason reason, const char* description) {
    if (!dataProtector) {
        // Get current configuration from global variables
        extern ConfigurationData cfg;
        extern CalibrationData calibration_data;
        return createSnapshot(cfg, calibration_data, reason, description);
    }
    
    ConfigurationData config = dataProtector->getConfiguration();
    CalibrationData calib = dataProtector->getCalibration();
    return createSnapshot(config, calib, reason, description);
}

bool ConfigurationRecoverySystem::createSnapshot(const ConfigurationData& config, 
                                                const CalibrationData& calib,
                                                RecoveryReason reason, 
                                                const char* description) {
    RecoverySnapshot& snapshot = snapshots[currentSnapshotIndex];
    
    snapshot.config = config;
    snapshot.calibration = calib;
    snapshot.timestamp = millis();
    snapshot.reason = reason;
    snapshot.firmwareVersion = config.version;
    snapshot.isValid = true;
    
    strncpy(snapshot.description, description, sizeof(snapshot.description) - 1);
    snapshot.description[sizeof(snapshot.description) - 1] = '\0';
    
    updateSnapshotChecksum(snapshot);
    
    // Save to storage
    saveSnapshotToStorage(currentSnapshotIndex);
    
    // Update indices
    currentSnapshotIndex = (currentSnapshotIndex + 1) % MAX_SNAPSHOTS;
    if (snapshotCount < MAX_SNAPSHOTS) {
        snapshotCount++;
    }
    
    log_i("Recovery snapshot created: %s", description);
    return true;
}

bool ConfigurationRecoverySystem::attemptAutoRecovery(const ValidationResult& validation) {
    log_w("Configuration corruption detected, attempting recovery...");
    
    // Update statistics
    stats.totalRecoveries++;
    stats.lastRecoveryTimestamp = millis();
    
    // Try progressively more aggressive recovery methods
    if (tryParameterRecovery(validation)) {
        log_i("Parameter-level recovery successful");
        stats.successfulRecoveries++;
        stats.lastRecoveryLevel = RecoveryLevel::PARAMETER_RESET;
        logRecoveryAttempt(RecoveryLevel::PARAMETER_RESET, true);
        return true;
    }
    
    if (trySnapshotRecovery()) {
        log_i("Snapshot recovery successful");
        stats.successfulRecoveries++;
        stats.lastRecoveryLevel = RecoveryLevel::RECOVERY_SNAPSHOT;
        logRecoveryAttempt(RecoveryLevel::RECOVERY_SNAPSHOT, true);
        return true;
    }
    
    if (tryFactoryRecovery(true)) {
        log_w("Factory reset recovery successful");
        stats.successfulRecoveries++;
        stats.lastRecoveryLevel = RecoveryLevel::CALIBRATION_PRESERVE;
        logRecoveryAttempt(RecoveryLevel::CALIBRATION_PRESERVE, true);
        return true;
    }
    
    log_e("All recovery attempts failed!");
    stats.failedRecoveries++;
    logRecoveryAttempt(RecoveryLevel::FACTORY_RESET, false);
    return false;
}

bool ConfigurationRecoverySystem::tryParameterRecovery(const ValidationResult& validation) {
    bool recovered = false;
    
    extern ConfigurationData cfg;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT];
    
    for (const auto& error : validation.errors) {
        if (!error.isAutoFixable) continue;
        
        // Apply fixes based on error location
        if (error.location.parameter) {
            if (strcmp(error.location.parameter, "mode") == 0) {
                cfg.mode = 0;
                recovered = true;
            }
            else if (strcmp(error.location.parameter, "sensitivity") == 0) {
                cfg.sensitivity = 1;
                recovered = true;
            }
            else if (strcmp(error.location.parameter, "brightness") == 0) {
                cfg.brightness = 6;
                recovered = true;
            }
            else if (strcmp(error.location.parameter, "channel") == 0) {
                if (error.location.bank < BANK_AMT) {
                    kb_cfg[error.location.bank].channel = 1;
                    recovered = true;
                }
            }
            else if (strcmp(error.location.parameter, "scale") == 0) {
                if (error.location.bank < BANK_AMT) {
                    kb_cfg[error.location.bank].scale = 0;
                    recovered = true;
                }
            }
            else if (strcmp(error.location.parameter, "cc_id") == 0) {
                if (error.location.bank < BANK_AMT && error.location.index < CC_AMT) {
                    cc_cfg[error.location.bank].id[error.location.index] = 
                        13 + error.location.index; // Default CC mapping
                    recovered = true;
                }
            }
        }
    }
    
    if (recovered) {
        cfg.hasChanged = true;
    }
    
    return recovered;
}

bool ConfigurationRecoverySystem::trySnapshotRecovery() {
    // Find most recent valid snapshot
    for (int i = 0; i < snapshotCount; i++) {
        int idx = (currentSnapshotIndex - 1 - i + MAX_SNAPSHOTS) % MAX_SNAPSHOTS;
        const RecoverySnapshot& snapshot = snapshots[idx];
        
        if (!snapshot.isValid) continue;
        
        // Verify snapshot integrity
        if (!verifySnapshotChecksum(snapshot)) {
            log_w("Snapshot %d corrupted, trying next", idx);
            continue;
        }
        
        // Validate snapshot configuration
        if (!validateSnapshot(snapshot)) {
            log_w("Snapshot %d validation failed, trying next", idx);
            continue;
        }
        
        log_i("Restoring from snapshot: %s", snapshot.description);
        
        // Apply configuration
        if (applyConfiguration(snapshot.config) && 
            applyCalibration(snapshot.calibration)) {
            return true;
        }
    }
    
    return false;
}

bool ConfigurationRecoverySystem::tryFactoryRecovery(bool preserveCalibration) {
    log_w("Performing factory reset%s...", preserveCalibration ? " (preserving calibration)" : "");
    
    // Get current calibration if preserving
    extern CalibrationData calibration_data;
    CalibrationData savedCalibration = calibration_data;
    
    // Reset to factory defaults
    extern ConfigurationData cfg;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT];
    
    // Reset global configuration
    cfg = ConfigurationData{};
    cfg.version = 102;
    cfg.mode = 0;
    cfg.sensitivity = 1;
    cfg.brightness = 6;
    cfg.palette = 0;
    cfg.midi_trs = 0;
    cfg.trs_type = 0;
    cfg.passthrough = 0;
    cfg.midi_ble = 0;
    
    // Initialize custom scales
    for (int i = 0; i < 16; i++) {
        cfg.custom_scale1[i] = i;
        cfg.custom_scale2[i] = i;
    }
    
    // Reset all banks
    for (int bank = 0; bank < BANK_AMT; bank++) {
        getDefaultBankConfig(bank, kb_cfg[bank], cc_cfg[bank]);
    }
    
    // Restore calibration if requested
    if (preserveCalibration) {
        calibration_data = savedCalibration;
    } else {
        // Reset calibration to defaults
        for (int i = 0; i < 16; i++) {
            calibration_data.minVal[i] = 0;
            calibration_data.maxVal[i] = 4095;
        }
    }
    
    cfg.hasChanged = true;
    return true;
}

bool ConfigurationRecoverySystem::recoverFromSnapshot(uint8_t snapshotIndex) {
    if (snapshotIndex >= snapshotCount) {
        log_e("Invalid snapshot index");
        return false;
    }
    
    const RecoverySnapshot& snapshot = snapshots[snapshotIndex];
    
    if (!snapshot.isValid || !verifySnapshotChecksum(snapshot)) {
        log_e("Snapshot corrupted or invalid");
        return false;
    }
    
    return applyConfiguration(snapshot.config) && 
           applyCalibration(snapshot.calibration);
}

void ConfigurationRecoverySystem::checkPeriodicSnapshot() {
    uint32_t now = millis();
    
    if (now - lastPeriodicSnapshot >= SNAPSHOT_INTERVAL) {
        createSnapshot(RecoveryReason::PERIODIC_BACKUP, "Periodic backup");
        lastPeriodicSnapshot = now;
    }
}

bool ConfigurationRecoverySystem::validateSnapshot(const RecoverySnapshot& snapshot) const {
    ConfigurationValidator::ValidationContext context;
    context.variant = HardwareVariant::T16;
    context.firmwareVersion = snapshot.firmwareVersion;
    context.strictMode = false; // Be lenient with snapshots
    context.level = ConfigurationValidator::STANDARD;
    
    ValidationResult result = validator.validateConfiguration(snapshot.config, context);
    
    // Also validate calibration
    ValidationResult calibResult = validator.validateCalibrationData(snapshot.calibration, context);
    
    return result.isValid && calibResult.isValid;
}

void ConfigurationRecoverySystem::updateSnapshotChecksum(RecoverySnapshot& snapshot) {
    // Calculate checksum excluding the checksum field
    size_t checksumOffset = offsetof(RecoverySnapshot, checksum);
    snapshot.checksum = crc32(&snapshot, checksumOffset);
}

bool ConfigurationRecoverySystem::verifySnapshotChecksum(const RecoverySnapshot& snapshot) const {
    size_t checksumOffset = offsetof(RecoverySnapshot, checksum);
    uint32_t calculated = crc32(&snapshot, checksumOffset);
    return calculated == snapshot.checksum;
}

bool ConfigurationRecoverySystem::applyConfiguration(const ConfigurationData& config) {
    extern ConfigurationData cfg;
    cfg = config;
    cfg.hasChanged = true;
    return true;
}

bool ConfigurationRecoverySystem::applyCalibration(const CalibrationData& calib) {
    extern CalibrationData calibration_data;
    calibration_data = calib;
    return true;
}

void ConfigurationRecoverySystem::getDefaultBankConfig(uint8_t bank, 
                                                      KeyModeData& kb_cfg, 
                                                      ControlChangeData& cc_cfg) const {
    // Default keyboard configuration
    kb_cfg = KeyModeData{};
    kb_cfg.palette = bank;
    kb_cfg.channel = 1;
    kb_cfg.scale = 0;
    kb_cfg.base_octave = 2;
    kb_cfg.base_note = 0;
    kb_cfg.velocity_curve = 1;
    kb_cfg.aftertouch_curve = 1;
    kb_cfg.flip_x = 0;
    kb_cfg.flip_y = 0;
    kb_cfg.koala_mode = 0;
    
    // Default CC configuration
    cc_cfg = ControlChangeData{};
    for (int i = 0; i < CC_AMT; i++) {
        cc_cfg.channel[i] = 1;
        cc_cfg.id[i] = 13 + i; // Default CC mapping
    }
}

void ConfigurationRecoverySystem::saveSnapshotToStorage(uint8_t index) {
    String filename = getSnapshotFilename(index);
    File file = LittleFS.open(filename, "w");
    
    if (!file) {
        log_e("Failed to save snapshot %d", index);
        return;
    }
    
    file.write((const uint8_t*)&snapshots[index], sizeof(RecoverySnapshot));
    file.close();
}

bool ConfigurationRecoverySystem::loadSnapshotFromStorage(uint8_t index) {
    String filename = getSnapshotFilename(index);
    
    if (!LittleFS.exists(filename)) {
        return false;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return false;
    }
    
    size_t read = file.read((uint8_t*)&snapshots[index], sizeof(RecoverySnapshot));
    file.close();
    
    return read == sizeof(RecoverySnapshot) && verifySnapshotChecksum(snapshots[index]);
}

bool ConfigurationRecoverySystem::loadAllSnapshots() {
    snapshotCount = 0;
    
    for (uint8_t i = 0; i < MAX_SNAPSHOTS; i++) {
        if (loadSnapshotFromStorage(i)) {
            snapshotCount++;
        }
    }
    
    return snapshotCount > 0;
}

bool ConfigurationRecoverySystem::saveAllSnapshots() {
    for (uint8_t i = 0; i < snapshotCount; i++) {
        saveSnapshotToStorage(i);
    }
    
    // Save statistics
    File statsFile = LittleFS.open(RECOVERY_STATS_FILE, "w");
    if (statsFile) {
        statsFile.write((const uint8_t*)&stats, sizeof(stats));
        statsFile.close();
    }
    
    return true;
}

String ConfigurationRecoverySystem::getSnapshotFilename(uint8_t index) const {
    return String(SNAPSHOT_FILE_PREFIX) + String(index) + ".dat";
}

void ConfigurationRecoverySystem::logRecoveryAttempt(RecoveryLevel level, bool success) {
    const char* levelStr = "";
    switch (level) {
        case RecoveryLevel::PARAMETER_RESET: levelStr = "PARAMETER"; break;
        case RecoveryLevel::BANK_RESET: levelStr = "BANK"; break;
        case RecoveryLevel::FACTORY_RESET: levelStr = "FACTORY"; break;
        case RecoveryLevel::RECOVERY_SNAPSHOT: levelStr = "SNAPSHOT"; break;
        case RecoveryLevel::CALIBRATION_PRESERVE: levelStr = "FACTORY_PRESERVE"; break;
    }
    
    log_i("Recovery attempt: %s - %s", levelStr, success ? "SUCCESS" : "FAILED");
    
    // Save statistics
    saveAllSnapshots();
}

void ConfigurationRecoverySystem::listSnapshots() const {
    log_i("Recovery Snapshots:");
    for (uint8_t i = 0; i < snapshotCount; i++) {
        const RecoverySnapshot& snap = snapshots[i];
        if (snap.isValid) {
            log_i("[%d] %s - %s (FW v%d)", 
                  i, snap.description, 
                  snap.timestamp > 0 ? "Valid" : "Invalid",
                  snap.firmwareVersion);
        }
    }
}

uint8_t ConfigurationRecoverySystem::getSnapshotCount() const {
    return snapshotCount;
}

const RecoverySnapshot* ConfigurationRecoverySystem::getSnapshot(uint8_t index) const {
    if (index >= snapshotCount) return nullptr;
    return &snapshots[index];
}

} // namespace DataIntegrity