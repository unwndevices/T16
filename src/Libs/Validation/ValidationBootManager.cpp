#include "ValidationBootManager.hpp"
#include <LittleFS.h>

namespace DataIntegrity {

// Global instance
ValidationBootManager* bootManager = nullptr;

ValidationBootManager::ValidationBootManager() 
    : inSafeMode(false), bootCount(0), failureCount(0), lastSuccessfulBoot(0) {
    memset(&status, 0, sizeof(status));
    status.state = BootState::INIT;
}

BootResult ValidationBootManager::performBootSequence() {
    uint32_t bootStartTime = millis();
    status.bootTime = bootStartTime;
    status.recoveryAttempts = 0;
    
    log_i("=== T16 Validation Boot Sequence Starting ===");
    
    // Stage 1: Initialize filesystem
    updateBootState(BootState::INIT);
    showBootProgress(BootState::INIT);
    
    if (!initializeFilesystem()) {
        logBootError("Filesystem initialization failed");
        return BootResult::CRITICAL_FAILURE;
    }
    
    // Load boot statistics
    loadBootStatistics();
    bootCount++;
    
    // Check if we need safe mode due to repeated failures
    if (failureCount >= MAX_BOOT_FAILURES) {
        log_w("Too many boot failures (%d), entering safe mode", failureCount);
        enterSafeMode();
        return BootResult::SAFE_MODE_REQUIRED;
    }
    
    // Stage 2: Check data integrity
    updateBootState(BootState::CHECKING_INTEGRITY);
    showBootProgress(BootState::CHECKING_INTEGRITY);
    
    if (!checkDataIntegrity()) {
        logBootError("Data integrity check failed");
        failureCount++;
        
        // Attempt recovery
        if (attemptRecovery()) {
            log_i("Recovery successful");
            status.recoveryAttempts++;
        } else {
            log_e("Recovery failed, entering safe mode");
            enterSafeMode();
            return BootResult::SAFE_MODE_REQUIRED;
        }
    }
    
    // Stage 3: Load configuration
    updateBootState(BootState::LOADING_CONFIG);
    showBootProgress(BootState::LOADING_CONFIG);
    
    if (!loadConfiguration()) {
        logBootError("Configuration loading failed");
        failureCount++;
        
        if (status.recoveryAttempts < MAX_RECOVERY_ATTEMPTS) {
            if (attemptRecovery()) {
                status.recoveryAttempts++;
            } else {
                enterSafeMode();
                return BootResult::SAFE_MODE_REQUIRED;
            }
        } else {
            log_e("Max recovery attempts reached");
            enterSafeMode();
            return BootResult::SAFE_MODE_REQUIRED;
        }
    }
    
    // Stage 4: Validate configuration
    updateBootState(BootState::VALIDATING_CONFIG);
    showBootProgress(BootState::VALIDATING_CONFIG);
    
    if (!validateConfiguration()) {
        logBootError("Configuration validation failed");
        
        if (status.recoveryAttempts < MAX_RECOVERY_ATTEMPTS) {
            updateBootState(BootState::RECOVERING);
            showRecoveryMode();
            
            if (attemptRecovery()) {
                status.recoveryAttempts++;
                // Re-validate after recovery
                if (!validateConfiguration()) {
                    log_e("Configuration still invalid after recovery");
                    enterSafeMode();
                    return BootResult::SAFE_MODE_REQUIRED;
                }
            } else {
                enterSafeMode();
                return BootResult::SAFE_MODE_REQUIRED;
            }
        }
    }
    
    // Boot successful
    updateBootState(BootState::READY);
    showBootProgress(BootState::READY);
    
    // Update statistics
    lastSuccessfulBoot = millis();
    if (status.recoveryAttempts > 0) {
        log_i("Boot completed with %d recovery attempts", status.recoveryAttempts);
        saveBootStatistics();
        return BootResult::RECOVERED;
    }
    
    // Reset failure counter on successful boot
    if (failureCount > 0) {
        failureCount = 0;
        saveBootStatistics();
    }
    
    uint32_t bootTime = millis() - bootStartTime;
    log_i("=== Boot Sequence Completed in %dms ===", bootTime);
    
    return BootResult::SUCCESS;
}

bool ValidationBootManager::initializeFilesystem() {
    if (!LittleFS.begin()) {
        log_e("LittleFS mount failed, attempting format");
        
        // Try formatting
        if (LittleFS.format()) {
            log_i("LittleFS formatted successfully");
            if (LittleFS.begin()) {
                log_i("LittleFS mounted after format");
                return true;
            }
        }
        
        status.integrityCheck = false;
        return false;
    }
    
    log_i("Filesystem initialized successfully");
    return true;
}

bool ValidationBootManager::checkDataIntegrity() {
    // Initialize critical data protector
    if (!dataProtector.initialize()) {
        log_e("Critical data protector initialization failed");
        status.integrityCheck = false;
        return false;
    }
    
    // Check integrity
    if (!dataProtector.verifyIntegrity()) {
        status.corruptionLevel = dataProtector.getCorruptionLevel();
        log_w("Data corruption detected, level: %d", status.corruptionLevel);
        
        // Attempt automatic repair
        if (dataProtector.repairCorruption()) {
            log_i("Data corruption repaired automatically");
            status.integrityCheck = true;
            return true;
        }
        
        status.integrityCheck = false;
        return false;
    }
    
    status.integrityCheck = true;
    status.corruptionLevel = 0;
    log_i("Data integrity check passed");
    return true;
}

bool ValidationBootManager::loadConfiguration() {
    extern ConfigurationData cfg;
    extern CalibrationData calibration_data;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT];
    extern DataManager config;
    
    // Get configuration from critical data protector
    cfg = dataProtector.getConfiguration();
    calibration_data = dataProtector.getCalibration();
    
    // Load bank configurations
    LoadConfiguration(config, false);
    
    status.configValid = true;
    log_i("Configuration loaded successfully");
    return true;
}

bool ValidationBootManager::validateConfiguration() {
    extern ConfigurationData cfg;
    extern CalibrationData calibration_data;
    extern KeyModeData kb_cfg[BANK_AMT];
    extern ControlChangeData cc_cfg[BANK_AMT];
    
    ConfigurationValidator::ValidationContext context;
    context.variant = HardwareVariant::T16;
    context.firmwareVersion = cfg.version;
    context.strictMode = true;
    context.level = ConfigurationValidator::COMPREHENSIVE;
    
    // Validate main configuration
    ValidationResult configResult = validator.validateConfiguration(cfg, context);
    if (!configResult.isValid) {
        log_e("Main configuration validation failed with %d errors", configResult.errors.size());
        status.configValid = false;
        
        // Try auto-fix
        if (configResult.canAutoFix()) {
            if (validator.autoFixConfiguration(cfg, configResult)) {
                log_i("Configuration auto-fixed");
                status.configValid = true;
            }
        }
    }
    
    // Validate calibration
    ValidationResult calibResult = validator.validateCalibrationData(calibration_data, context);
    if (!calibResult.isValid) {
        log_e("Calibration validation failed");
        status.calibrationValid = false;
        return false;
    }
    status.calibrationValid = true;
    
    // Validate each bank
    for (int bank = 0; bank < BANK_AMT; bank++) {
        ValidationResult kbResult = validator.validateKeyModeData(kb_cfg[bank], bank, context);
        if (!kbResult.isValid) {
            log_e("Bank %d keyboard config validation failed", bank);
            
            if (kbResult.canAutoFix()) {
                if (validator.autoFixKeyModeData(kb_cfg[bank], kbResult)) {
                    log_i("Bank %d keyboard config auto-fixed", bank);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        
        ValidationResult ccResult = validator.validateControlChangeData(cc_cfg[bank], bank, context);
        if (!ccResult.isValid) {
            log_e("Bank %d CC config validation failed", bank);
            
            if (ccResult.canAutoFix()) {
                if (validator.autoFixControlChangeData(cc_cfg[bank], ccResult)) {
                    log_i("Bank %d CC config auto-fixed", bank);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    
    log_i("Full configuration validation passed");
    return true;
}

bool ValidationBootManager::attemptRecovery() {
    updateBootState(BootState::RECOVERING);
    showRecoveryMode();
    
    // Initialize recovery system if needed
    if (!recoverySystem.initialize()) {
        log_e("Recovery system initialization failed");
        return false;
    }
    
    // Try automatic recovery
    if (recoverySystem.attemptAutoRecovery()) {
        log_i("Automatic recovery successful");
        
        // Re-initialize data protector with recovered data
        extern ConfigurationData cfg;
        extern CalibrationData calibration_data;
        
        auto transaction = dataProtector.beginTransaction();
        transaction.updateConfiguration(cfg);
        transaction.updateCalibration(calibration_data);
        transaction.commit();
        
        return true;
    }
    
    return false;
}

void ValidationBootManager::enterSafeMode() {
    inSafeMode = true;
    updateBootState(BootState::SAFE_MODE);
    
    log_w("=== ENTERING SAFE MODE ===");
    log_w("Limited functionality enabled");
    log_w("Please connect to configurator to repair");
    
    // Load minimal safe configuration
    extern ConfigurationData cfg;
    cfg = ConfigurationData{};
    cfg.version = 102;
    cfg.mode = 4; // Quick settings mode
    cfg.sensitivity = 1;
    cfg.brightness = 3; // Dim LEDs
    cfg.midi_trs = 0;
    cfg.midi_ble = 0;
    
    // Show safe mode indication on LEDs
    showBootProgress(BootState::SAFE_MODE);
}

bool ValidationBootManager::performEmergencyRecovery() {
    log_w("Performing emergency recovery procedure");
    
    // Try each recovery method
    if (recoverySystem.recoverToFactoryDefaults(true)) {
        log_i("Factory reset successful (calibration preserved)");
        resetFailureCounter();
        return true;
    }
    
    // Last resort - full factory reset
    if (recoverySystem.recoverToFactoryDefaults(false)) {
        log_w("Full factory reset performed");
        resetFailureCounter();
        return true;
    }
    
    log_e("Emergency recovery failed");
    return false;
}

void ValidationBootManager::updateBootState(BootState newState) {
    status.state = newState;
    
    const char* stateStr = "";
    switch (newState) {
        case BootState::INIT: stateStr = "INIT"; break;
        case BootState::CHECKING_INTEGRITY: stateStr = "CHECKING_INTEGRITY"; break;
        case BootState::LOADING_CONFIG: stateStr = "LOADING_CONFIG"; break;
        case BootState::VALIDATING_CONFIG: stateStr = "VALIDATING_CONFIG"; break;
        case BootState::RECOVERING: stateStr = "RECOVERING"; break;
        case BootState::READY: stateStr = "READY"; break;
        case BootState::FAILED: stateStr = "FAILED"; break;
        case BootState::SAFE_MODE: stateStr = "SAFE_MODE"; break;
    }
    
    log_d("Boot state: %s", stateStr);
}

void ValidationBootManager::logBootError(const char* error) {
    strncpy(status.lastError, error, sizeof(status.lastError) - 1);
    status.lastError[sizeof(status.lastError) - 1] = '\0';
    log_e("Boot error: %s", error);
}

bool ValidationBootManager::saveBootStatistics() {
    struct BootStats {
        uint32_t bootCount;
        uint32_t failureCount;
        uint32_t lastSuccessfulBoot;
    } stats = {bootCount, failureCount, lastSuccessfulBoot};
    
    File file = LittleFS.open(BOOT_STATS_FILE, "w");
    if (!file) return false;
    
    file.write((const uint8_t*)&stats, sizeof(stats));
    file.close();
    return true;
}

bool ValidationBootManager::loadBootStatistics() {
    if (!LittleFS.exists(BOOT_STATS_FILE)) {
        return false;
    }
    
    struct BootStats {
        uint32_t bootCount;
        uint32_t failureCount;
        uint32_t lastSuccessfulBoot;
    } stats;
    
    File file = LittleFS.open(BOOT_STATS_FILE, "r");
    if (!file) return false;
    
    size_t read = file.read((uint8_t*)&stats, sizeof(stats));
    file.close();
    
    if (read == sizeof(stats)) {
        bootCount = stats.bootCount;
        failureCount = stats.failureCount;
        lastSuccessfulBoot = stats.lastSuccessfulBoot;
        return true;
    }
    
    return false;
}

void ValidationBootManager::resetFailureCounter() {
    failureCount = 0;
    saveBootStatistics();
}

void ValidationBootManager::showBootProgress(BootState state) {
    // This would interface with the LED system to show boot progress
    // Different colors/patterns for different states
    // For now, just log
    switch (state) {
        case BootState::INIT:
            // Pulse white
            break;
        case BootState::CHECKING_INTEGRITY:
            // Pulse blue
            break;
        case BootState::LOADING_CONFIG:
            // Pulse green
            break;
        case BootState::VALIDATING_CONFIG:
            // Pulse yellow
            break;
        case BootState::READY:
            // Solid green briefly
            break;
        case BootState::SAFE_MODE:
            // Slow red pulse
            break;
        default:
            break;
    }
}

void ValidationBootManager::showBootError(const char* error) {
    // Flash red LEDs
    log_e("Boot error display: %s", error);
}

void ValidationBootManager::showRecoveryMode() {
    // Orange pulse pattern
    log_w("Recovery mode active");
}

void ValidationBootManager::printBootDiagnostics() const {
    log_i("=== Boot Diagnostics ===");
    log_i("Boot count: %d", bootCount);
    log_i("Failure count: %d", failureCount);
    log_i("Last successful boot: %d", lastSuccessfulBoot);
    log_i("Current state: %d", (int)status.state);
    log_i("Config valid: %s", status.configValid ? "YES" : "NO");
    log_i("Calibration valid: %s", status.calibrationValid ? "YES" : "NO");
    log_i("Integrity check: %s", status.integrityCheck ? "PASS" : "FAIL");
    log_i("Corruption level: %d", status.corruptionLevel);
    log_i("Recovery attempts: %d", status.recoveryAttempts);
    if (strlen(status.lastError) > 0) {
        log_i("Last error: %s", status.lastError);
    }
    log_i("Safe mode: %s", inSafeMode ? "ACTIVE" : "INACTIVE");
    log_i("=======================");
}

} // namespace DataIntegrity