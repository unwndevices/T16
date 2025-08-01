#ifndef VALIDATION_TEST_HPP
#define VALIDATION_TEST_HPP

#include <Arduino.h>
#include "DataIntegrity.hpp"
#include "ConfigurationValidator.hpp"
#include "CriticalDataProtector.hpp"
#include "ConfigurationRecovery.hpp"
#include "RealtimeValidator.hpp"
#include "ValidationBootManager.hpp"

namespace DataIntegrity {

class ValidationTestSuite {
public:
    struct TestResult {
        bool passed;
        uint32_t testCount;
        uint32_t passedCount;
        uint32_t failedCount;
        char lastFailure[128];
    };
    
    ValidationTestSuite();
    
    // Run all tests
    TestResult runAllTests();
    
    // Individual test suites
    bool testDataIntegrity();
    bool testConfigurationValidator();
    bool testCriticalDataProtector();
    bool testRecoverySystem();
    bool testRealtimeValidator();
    bool testBootManager();
    
    // Specific test cases
    bool testCRCCalculation();
    bool testMessageValidation();
    bool testParameterValidation();
    bool testTripleRedundancy();
    bool testCorruptionRecovery();
    bool testTransactionRollback();
    bool testSnapshotRecovery();
    bool testBootSequence();
    
    // Performance tests
    bool testValidationPerformance();
    
    // Print test results
    void printResults() const;
    
private:
    TestResult result;
    
    // Test helpers
    bool testAssert(bool condition, const char* message);
    void logTest(const char* testName);
    void logPass(const char* testName);
    void logFail(const char* testName, const char* reason);
    
    // Test data generators
    ConfigurationData createValidConfig();
    ConfigurationData createCorruptConfig();
    CalibrationData createValidCalibration();
    CalibrationData createCorruptCalibration();
    
    // Mock objects for testing
    void setupMockEnvironment();
    void cleanupMockEnvironment();
};

// Test execution macros
#define RUN_VALIDATION_TESTS() \
    do { \
        ValidationTestSuite testSuite; \
        auto result = testSuite.runAllTests(); \
        testSuite.printResults(); \
        if (!result.passed) { \
            log_e("Validation tests failed!"); \
        } \
    } while(0)

} // namespace DataIntegrity

#endif // VALIDATION_TEST_HPP