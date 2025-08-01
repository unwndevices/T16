#include "Validation.hpp"

namespace DataIntegrity {

bool ValidationSystem::initialize() {
    log_i("Initializing T16 Validation System v%04X", VALIDATION_SYSTEM_VERSION);
    
    status = ValidationSystemStatus::INITIALIZING;
    
    // Initialize all components
    if (!initializeComponents()) {
        log_e("Failed to initialize validation components");
        status = ValidationSystemStatus::FAILED;
        return false;
    }
    
    // Verify system integrity
    if (!verifySystemIntegrity()) {
        log_w("System integrity issues detected, entering recovery mode");
        status = ValidationSystemStatus::RECOVERY_MODE;
        
        // Attempt recovery
        if (!performEmergencyRecovery()) {
            log_e("Recovery failed, entering safe mode");
            status = ValidationSystemStatus::SAFE_MODE;
            enterSafeMode();
        }
    }
    
    status = ValidationSystemStatus::ACTIVE;
    log_i("Validation system initialized successfully");
    return true;
}

bool ValidationSystem::initializeComponents() {
    // Initialize data protector first (critical for everything else)
    if (!dataProtector.initialize()) {
        log_e("Critical data protector initialization failed");
        return false;
    }
    
    // Initialize recovery system
    if (!recoverySystem.initialize()) {
        log_e("Recovery system initialization failed");
        return false;
    }
    
    // Boot manager initialization
    BootResult bootResult = bootManager.performBootSequence();
    if (bootResult == BootResult::CRITICAL_FAILURE) {
        log_e("Boot sequence failed critically");
        return false;
    }
    
    if (bootResult == BootResult::SAFE_MODE_REQUIRED) {
        log_w("Boot sequence requires safe mode");
        status = ValidationSystemStatus::SAFE_MODE;
    }
    
    log_i("All validation components initialized");
    return true;
}

bool ValidationSystem::verifySystemIntegrity() {
    ValidationResult result = validateSystem();
    
    if (!result.isValid) {
        log_w("System integrity check failed with %d errors", result.errors.size());
        for (const auto& error : result.errors) {
            log_w("Integrity error: %s", error.message ? error.message : "Unknown");
        }
        return false;
    }
    
    log_i("System integrity verified");
    return true;
}

bool ValidationSystem::isHealthy() const {
    if (status != ValidationSystemStatus::ACTIVE) {
        return false;
    }
    
    // Check data protector integrity
    if (!dataProtector.verifyIntegrity()) {
        return false;
    }
    
    // Check if boot manager is in safe mode
    if (bootManager.isInSafeMode()) {
        return false;
    }
    
    return true;
}

ValidationResult ValidationSystem::validateSystem() {
    ValidationResult result;
    
    // Validate current configuration
    extern ConfigurationData cfg;
    extern CalibrationData calibration_data;
    
    ConfigurationValidator::ValidationContext context;
    context.variant = HardwareVariant::T16;
    context.firmwareVersion = cfg.version;
    context.strictMode = true;
    context.level = ConfigurationValidator::COMPREHENSIVE;
    
    // Validate main configuration
    ValidationResult configResult = configValidator.validateConfiguration(cfg, context);
    if (!configResult.isValid) {
        result.errors.insert(result.errors.end(), 
                           configResult.errors.begin(), 
                           configResult.errors.end());
        result.isValid = false;
    }
    
    // Validate calibration
    ValidationResult calibResult = configValidator.validateCalibrationData(calibration_data, context);
    if (!calibResult.isValid) {
        result.errors.insert(result.errors.end(), 
                           calibResult.errors.begin(), 
                           calibResult.errors.end());
        result.isValid = false;
    }
    
    // Check data protector integrity
    if (!dataProtector.verifyIntegrity()) {
        result.addError({
            .type = ValidationError::CHECKSUM_FAILURE,
            .message = "Critical data integrity check failed"
        });
    }
    
    return result;
}

bool ValidationSystem::performEmergencyRecovery() {
    log_w("Performing emergency system recovery");
    
    // Try boot manager emergency recovery first
    if (bootManager.performEmergencyRecovery()) {
        log_i("Boot manager emergency recovery successful");
        return true;
    }
    
    // Try recovery system
    if (recoverySystem.attemptAutoRecovery()) {
        log_i("Recovery system emergency recovery successful"); 
        return true;
    }
    
    // Try data protector repair
    if (dataProtector.repairCorruption()) {
        log_i("Data protector repair successful");
        return true;
    }
    
    log_e("All emergency recovery attempts failed");
    return false;
}

void ValidationSystem::enterSafeMode() {
    log_w("=== VALIDATION SYSTEM SAFE MODE ===");
    status = ValidationSystemStatus::SAFE_MODE;
    
    // Let boot manager handle safe mode
    bootManager.enterSafeMode();
    
    // Minimal validation only
    // Disable advanced features
    log_w("System running in safe mode - limited validation active");
}

void ValidationSystem::printSystemDiagnostics() const {
    log_i("=== Validation System Diagnostics ===");
    log_i("System version: v%04X", VALIDATION_SYSTEM_VERSION);
    
    const char* statusStr = "";
    switch (status) {
        case ValidationSystemStatus::UNINITIALIZED: statusStr = "UNINITIALIZED"; break;
        case ValidationSystemStatus::INITIALIZING: statusStr = "INITIALIZING"; break;
        case ValidationSystemStatus::ACTIVE: statusStr = "ACTIVE"; break;
        case ValidationSystemStatus::RECOVERY_MODE: statusStr = "RECOVERY_MODE"; break;
        case ValidationSystemStatus::SAFE_MODE: statusStr = "SAFE_MODE"; break;
        case ValidationSystemStatus::FAILED: statusStr = "FAILED"; break;
    }
    log_i("System status: %s", statusStr);
    log_i("System healthy: %s", isHealthy() ? "YES" : "NO");
    
    // Print component diagnostics
    bootManager.printBootDiagnostics();
    
    // Print recovery statistics
    auto recoveryStats = recoverySystem.getStats();
    log_i("Recovery stats - Total: %d, Success: %d, Failed: %d", 
          recoveryStats.totalRecoveries,
          recoveryStats.successfulRecoveries, 
          recoveryStats.failedRecoveries);
    
    // Print corruption level
    uint8_t corruptionLevel = dataProtector.getCorruptionLevel();
    log_i("Data corruption level: %d/12", corruptionLevel);
    
    log_i("====================================");
}

bool ValidationSystem::runSystemTests() {
    log_i("Running validation system test suite");
    
    ValidationTestSuite::TestResult result = testSuite.runAllTests();
    testSuite.printResults();
    
    if (!result.passed) {
        log_e("System tests failed - system may be unreliable");
        return false;
    }
    
    log_i("All system tests passed");
    return true;
}

} // namespace DataIntegrity