#ifndef VALIDATION_HPP
#define VALIDATION_HPP

// Master include file for the T16 Data Validation and Integrity System
// 
// This comprehensive validation system provides:
// - Multi-layer validation architecture
// - Protocol-level integrity checks (CRC32, sequence numbers)
// - Semantic validation system
// - Real-time parameter validation
// - Cross-parameter constraint validation
// - Critical data protection with triple redundancy
// - Automatic recovery mechanisms
// - Configuration transaction system with rollback
// - Boot sequence integration for corruption detection
// - Emergency recovery procedures
//
// Usage:
//   #include "Libs/Validation/Validation.hpp"
//   using namespace DataIntegrity;

// Core validation components
#include "DataIntegrity.hpp"
#include "ConfigurationValidator.hpp"
#include "RealtimeValidator.hpp"

// Data protection and recovery
#include "CriticalDataProtector.hpp"
#include "ConfigurationRecovery.hpp"

// Boot integration
#include "ValidationBootManager.hpp"

// Testing framework
#include "ValidationTest.hpp"

namespace DataIntegrity {

// Validation system version
constexpr uint16_t VALIDATION_SYSTEM_VERSION = 0x0001;

// Global validation system status
enum class ValidationSystemStatus : uint8_t {
    UNINITIALIZED,
    INITIALIZING,
    ACTIVE,
    RECOVERY_MODE,
    SAFE_MODE,
    FAILED
};

// Master validation system manager
class ValidationSystem {
public:
    static ValidationSystem& getInstance() {
        static ValidationSystem instance;
        return instance;
    }
    
    // Initialize entire validation system
    bool initialize();
    
    // Get system status
    ValidationSystemStatus getStatus() const { return status; }
    
    // Quick system health check
    bool isHealthy() const;
    
    // Get system components
    ConfigurationValidator& getConfigValidator() { return configValidator; }
    RealtimeValidator& getRealtimeValidator() { return realtimeValidator; }
    CriticalDataProtector& getDataProtector() { return dataProtector; }
    ConfigurationRecoverySystem& getRecoverySystem() { return recoverySystem; }
    ValidationBootManager& getBootManager() { return bootManager; }
    
    // System-wide validation
    ValidationResult validateSystem();
    
    // Emergency procedures
    bool performEmergencyRecovery();
    void enterSafeMode();
    
    // System diagnostics
    void printSystemDiagnostics() const;
    
    // Run comprehensive test suite
    bool runSystemTests();
    
private:
    ValidationSystem() = default;
    ValidationSystem(const ValidationSystem&) = delete;
    ValidationSystem& operator=(const ValidationSystem&) = delete;
    
    ValidationSystemStatus status = ValidationSystemStatus::UNINITIALIZED;
    
    // System components
    ConfigurationValidator configValidator;
    RealtimeValidator realtimeValidator;
    CriticalDataProtector dataProtector;
    ConfigurationRecoverySystem recoverySystem;
    ValidationBootManager bootManager;
    ValidationTestSuite testSuite;
    
    bool initializeComponents();
    bool verifySystemIntegrity();
};

// Convenience macros for system-wide validation
#define INIT_VALIDATION_SYSTEM() \
    DataIntegrity::ValidationSystem::getInstance().initialize()

#define VALIDATE_SYSTEM() \
    DataIntegrity::ValidationSystem::getInstance().validateSystem()

#define SYSTEM_HEALTH_CHECK() \
    DataIntegrity::ValidationSystem::getInstance().isHealthy()

#define EMERGENCY_RECOVERY() \
    DataIntegrity::ValidationSystem::getInstance().performEmergencyRecovery()

#define RUN_SYSTEM_TESTS() \
    DataIntegrity::ValidationSystem::getInstance().runSystemTests()

// Parameter validation shortcuts
#define VALIDATE_PARAM(id, value) \
    DataIntegrity::ValidationSystem::getInstance().getRealtimeValidator().validateParameter(id, value)

#define SANITIZE_PARAM(id, value) \
    DataIntegrity::ValidationSystem::getInstance().getRealtimeValidator().sanitizeParameter(id, value)

#define QUICK_VALIDATE(id, value) \
    DataIntegrity::FastValidator::quickValidate(id, value)

// Transaction shortcuts
#define BEGIN_CONFIG_TRANSACTION() \
    DataIntegrity::ValidationSystem::getInstance().getDataProtector().beginTransaction()

#define SYSTEM_CREATE_RECOVERY_SNAPSHOT(reason, desc) \
    DataIntegrity::ValidationSystem::getInstance().getRecoverySystem().createSnapshot(reason, desc)

} // namespace DataIntegrity

#endif // VALIDATION_HPP