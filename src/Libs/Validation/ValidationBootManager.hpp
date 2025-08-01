#ifndef VALIDATION_BOOT_MANAGER_HPP
#define VALIDATION_BOOT_MANAGER_HPP

#include <Arduino.h>
#include "DataIntegrity.hpp"
#include "ConfigurationValidator.hpp"
#include "CriticalDataProtector.hpp"
#include "ConfigurationRecovery.hpp"
#include "../../Configuration.hpp"

namespace DataIntegrity {

enum class BootState : uint8_t {
    INIT,
    CHECKING_INTEGRITY,
    LOADING_CONFIG,
    VALIDATING_CONFIG,
    RECOVERING,
    READY,
    FAILED,
    SAFE_MODE
};

enum class BootResult : uint8_t {
    SUCCESS,
    RECOVERED,
    SAFE_MODE_REQUIRED,
    CRITICAL_FAILURE
};

class ValidationBootManager {
public:
    struct BootStatus {
        BootState state;
        bool configValid;
        bool calibrationValid;
        bool integrityCheck;
        uint8_t corruptionLevel;
        uint32_t bootTime;
        uint32_t recoveryAttempts;
        char lastError[128];
    };
    
    ValidationBootManager();
    
    // Main boot sequence
    BootResult performBootSequence();
    
    // Individual boot stages
    bool initializeFilesystem();
    bool checkDataIntegrity();
    bool loadConfiguration();
    bool validateConfiguration();
    bool attemptRecovery();
    
    // Safe mode operations
    void enterSafeMode();
    bool isInSafeMode() const { return inSafeMode; }
    
    // Status and diagnostics
    BootStatus getStatus() const { return status; }
    void printBootDiagnostics() const;
    
    // Emergency procedures
    bool performEmergencyRecovery();
    bool resetToFactoryDefaults();
    
    // Boot counter and failure tracking
    uint32_t getBootCount() const { return bootCount; }
    uint32_t getFailureCount() const { return failureCount; }
    void resetFailureCounter();
    
private:
    static constexpr uint8_t MAX_RECOVERY_ATTEMPTS = 3;
    static constexpr uint8_t MAX_BOOT_FAILURES = 5;
    static constexpr const char* BOOT_STATS_FILE = "/boot_stats.dat";
    
    BootStatus status;
    CriticalDataProtector dataProtector;
    ConfigurationValidator validator;
    ConfigurationRecoverySystem recoverySystem;
    
    bool inSafeMode;
    uint32_t bootCount;
    uint32_t failureCount;
    uint32_t lastSuccessfulBoot;
    
    // Helper methods
    void updateBootState(BootState newState);
    void logBootError(const char* error);
    bool saveBootStatistics();
    bool loadBootStatistics();
    
    // Validation helpers
    bool performFullValidation();
    bool performQuickValidation();
    
    // LED feedback during boot
    void showBootProgress(BootState state);
    void showBootError(const char* error);
    void showRecoveryMode();
};

// Global boot manager instance
extern ValidationBootManager* bootManager;

// Boot sequence integration macros
#define INIT_VALIDATION_BOOT() \
    do { \
        bootManager = new ValidationBootManager(); \
        BootResult result = bootManager->performBootSequence(); \
        if (result == BootResult::CRITICAL_FAILURE) { \
            while(1) { delay(1000); } \
        } \
    } while(0)

#define CHECK_BOOT_SAFE_MODE() \
    (bootManager && bootManager->isInSafeMode())

} // namespace DataIntegrity

#endif // VALIDATION_BOOT_MANAGER_HPP