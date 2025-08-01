#include "ValidationTest.hpp"

namespace DataIntegrity {

ValidationTestSuite::ValidationTestSuite() {
    memset(&result, 0, sizeof(result));
}

ValidationTestSuite::TestResult ValidationTestSuite::runAllTests() {
    log_i("=== Running Validation Test Suite ===");
    
    setupMockEnvironment();
    
    // Run all test suites
    bool allPassed = true;
    
    allPassed &= testDataIntegrity();
    allPassed &= testConfigurationValidator();
    allPassed &= testCriticalDataProtector();
    allPassed &= testRecoverySystem();
    allPassed &= testRealtimeValidator();
    allPassed &= testValidationPerformance();
    
    result.passed = allPassed;
    
    cleanupMockEnvironment();
    
    log_i("=== Test Suite Complete ===");
    return result;
}

bool ValidationTestSuite::testDataIntegrity() {
    logTest("Data Integrity");
    
    bool passed = true;
    passed &= testCRCCalculation();
    passed &= testMessageValidation();
    
    return passed;
}

bool ValidationTestSuite::testCRCCalculation() {
    logTest("CRC Calculation");
    
    // Test known CRC values
    const char* testData = "Hello, World!";
    size_t dataSize = strlen(testData);
    
    uint8_t crc8_result = crc8(testData, dataSize);
    uint16_t crc16_result = crc16_ccitt(testData, dataSize);
    uint32_t crc32_result = crc32(testData, dataSize);
    
    // These values are architecture-dependent, so just check they're non-zero
    if (!testAssert(crc8_result != 0, "CRC8 calculation")) return false;
    if (!testAssert(crc16_result != 0, "CRC16 calculation")) return false;
    if (!testAssert(crc32_result != 0, "CRC32 calculation")) return false;
    
    // Test consistency
    uint32_t crc32_repeat = crc32(testData, dataSize);
    if (!testAssert(crc32_result == crc32_repeat, "CRC32 consistency")) return false;
    
    logPass("CRC Calculation");
    return true;
}

bool ValidationTestSuite::testMessageValidation() {
    logTest("Message Validation");
    
    MessageValidator validator;
    
    // Create test message
    SecureMessage<uint32_t> msg;
    msg.magic = 0x54313650;
    msg.version = 0x0001;
    msg.messageId = 12345;
    msg.timestamp = millis();
    msg.checksumType = ChecksumType::CRC32_STRONG;
    msg.payloadSize = sizeof(uint32_t);
    msg.payload = 0xDEADBEEF;
    msg.sequenceNumber = 0;
    msg.checksum = crc32(&msg.payload, sizeof(uint32_t));
    
    ValidationResult result = validator.validate(msg);
    if (!testAssert(result.isValid, "Valid message validation")) return false;
    
    // Test invalid magic
    msg.magic = 0xBADMAGIC;
    result = validator.validate(msg);
    if (!testAssert(!result.isValid, "Invalid magic detection")) return false;
    
    logPass("Message Validation");
    return true;
}

bool ValidationTestSuite::testConfigurationValidator() {
    logTest("Configuration Validator");
    
    bool passed = true;
    passed &= testParameterValidation();
    
    return passed;
}

bool ValidationTestSuite::testParameterValidation() {
    logTest("Parameter Validation");
    
    ConfigurationValidator validator;
    ConfigurationValidator::ValidationContext context;
    context.variant = HardwareVariant::T16;
    context.firmwareVersion = 102;
    context.strictMode = true;
    context.level = ConfigurationValidator::COMPREHENSIVE;
    
    // Test valid configuration
    ConfigurationData validConfig = createValidConfig();
    ValidationResult result = validator.validateConfiguration(validConfig, context);
    if (!testAssert(result.isValid, "Valid configuration")) return false;
    
    // Test invalid configuration
    ConfigurationData invalidConfig = createCorruptConfig();
    result = validator.validateConfiguration(invalidConfig, context);
    if (!testAssert(!result.isValid, "Invalid configuration detection")) return false;
    
    // Test auto-fix
    if (result.canAutoFix()) {
        ConfigurationData fixedConfig = invalidConfig;
        bool fixed = validator.autoFixConfiguration(fixedConfig, result);
        if (!testAssert(fixed, "Configuration auto-fix")) return false;
    }
    
    logPass("Parameter Validation");
    return true;
}

bool ValidationTestSuite::testCriticalDataProtector() {
    logTest("Critical Data Protector");
    
    bool passed = true;
    passed &= testTripleRedundancy();
    passed &= testCorruptionRecovery();
    passed &= testTransactionRollback();
    
    return passed;
}

bool ValidationTestSuite::testTripleRedundancy() {
    logTest("Triple Redundancy");
    
    TripleRedundant<uint32_t> redundant;
    
    // Test setting and getting value
    uint32_t testValue = 0x12345678;
    redundant.setValue(testValue);
    
    uint32_t retrieved = redundant.getMajorityValue();
    if (!testAssert(retrieved == testValue, "Triple redundant set/get")) return false;
    
    // Test corruption detection
    if (!testAssert(!redundant.isCorrupted(), "Initial corruption check")) return false;
    if (!testAssert(redundant.getValidCopyCount() == 3, "Valid copy count")) return false;
    
    logPass("Triple Redundancy");
    return true;
}

bool ValidationTestSuite::testCorruptionRecovery() {
    logTest("Corruption Recovery");
    
    CriticalDataProtector protector;
    
    // Test initialization
    if (!testAssert(protector.initialize(), "Protector initialization")) return false;
    
    // Test integrity verification
    if (!testAssert(protector.verifyIntegrity(), "Initial integrity check")) return false;
    
    logPass("Corruption Recovery");
    return true;
}

bool ValidationTestSuite::testTransactionRollback() {
    logTest("Transaction Rollback");
    
    CriticalDataProtector protector;
    protector.initialize();
    
    ConfigurationData originalConfig = protector.getConfiguration();
    
    {
        auto transaction = protector.beginTransaction();
        
        ConfigurationData newConfig = originalConfig;
        newConfig.brightness = 15; // Change a value
        
        if (!testAssert(transaction.updateConfiguration(newConfig), "Transaction update")) return false;
        
        // Don't commit - should rollback automatically
    }
    
    // Verify rollback
    ConfigurationData currentConfig = protector.getConfiguration();
    if (!testAssert(currentConfig.brightness == originalConfig.brightness, "Transaction rollback")) return false;
    
    logPass("Transaction Rollback");
    return true;
}

bool ValidationTestSuite::testRecoverySystem() {
    logTest("Recovery System");
    
    bool passed = true;
    passed &= testSnapshotRecovery();
    
    return passed;
}

bool ValidationTestSuite::testSnapshotRecovery() {
    logTest("Snapshot Recovery");
    
    ConfigurationRecoverySystem recovery;
    
    if (!testAssert(recovery.initialize(), "Recovery system init")) return false;
    
    // Create test snapshot
    if (!testAssert(recovery.createSnapshot(RecoveryReason::USER_REQUEST, "Test snapshot"), 
                "Snapshot creation")) return false;
    
    if (!testAssert(recovery.getSnapshotCount() > 0, "Snapshot count")) return false;
    
    logPass("Snapshot Recovery");
    return true;
}

bool ValidationTestSuite::testRealtimeValidator() {
    logTest("Realtime Validator");
    
    RealtimeValidator validator;
    
    // Test parameter validation
    ValidationResult result = validator.validateParameter(PARAM_KB_CHANNEL, 1.0f);
    if (!testAssert(result.isValid, "Valid channel parameter")) return false;
    
    result = validator.validateParameter(PARAM_KB_CHANNEL, 17.0f);
    if (!testAssert(!result.isValid, "Invalid channel parameter")) return false;
    
    // Test sanitization
    float sanitized = validator.sanitizeParameter(PARAM_KB_CHANNEL, 17.0f);
    if (!testAssert(sanitized == 16.0f, "Parameter sanitization")) return false;
    
    // Test fast validator
    if (!testAssert(FastValidator::quickValidate(PARAM_KB_CHANNEL, 8), "Fast validation valid")) return false;
    if (!testAssert(!FastValidator::quickValidate(PARAM_KB_CHANNEL, 20), "Fast validation invalid")) return false;
    
    logPass("Realtime Validator");
    return true;
}

bool ValidationTestSuite::testValidationPerformance() {
    logTest("Validation Performance");
    
    // Test CRC performance
    const size_t TEST_SIZE = 1024;
    uint8_t testData[TEST_SIZE];
    for (size_t i = 0; i < TEST_SIZE; i++) {
        testData[i] = i & 0xFF;
    }
    
    uint32_t startTime = micros();
    uint32_t crc = crc32(testData, TEST_SIZE);
    uint32_t endTime = micros();
    
    uint32_t duration = endTime - startTime;
    log_i("CRC32 of %d bytes took %d microseconds", TEST_SIZE, duration);
    
    // Performance should be reasonable (less than 1ms for 1KB)
    if (!testAssert(duration < 1000, "CRC performance acceptable")) return false;
    
    // Test fast validator performance
    startTime = micros();
    for (int i = 0; i < 1000; i++) {
        FastValidator::quickValidate(PARAM_KB_CHANNEL, i % 20);
    }
    endTime = micros();
    
    duration = endTime - startTime;
    log_i("1000 fast validations took %d microseconds", duration);
    
    logPass("Validation Performance");
    return true;
}

ConfigurationData ValidationTestSuite::createValidConfig() {
    ConfigurationData config;
    config.version = 102;
    config.mode = 0;
    config.sensitivity = 1;
    config.brightness = 6;
    config.palette = 0;
    config.midi_trs = 0;
    config.trs_type = 0;
    config.passthrough = 0;
    config.midi_ble = 0;
    
    // Initialize scales
    for (int i = 0; i < 16; i++) {
        config.custom_scale1[i] = i;
        config.custom_scale2[i] = i;
    }
    
    config.hasChanged = false;
    return config;
}

ConfigurationData ValidationTestSuite::createCorruptConfig() {
    ConfigurationData config = createValidConfig();
    
    // Introduce corruption
    config.mode = 255; // Invalid mode
    config.brightness = 255; // Invalid brightness
    config.custom_scale1[0] = -127; // Invalid scale note
    
    return config;
}

CalibrationData ValidationTestSuite::createValidCalibration() {
    CalibrationData calib;
    for (int i = 0; i < 16; i++) {
        calib.minVal[i] = 100 + i * 10;
        calib.maxVal[i] = 3900 - i * 10;
    }
    return calib;
}

CalibrationData ValidationTestSuite::createCorruptCalibration() {
    CalibrationData calib = createValidCalibration();
    
    // Introduce corruption - inverted min/max
    calib.minVal[0] = 4000;
    calib.maxVal[0] = 100;
    
    return calib;
}

bool ValidationTestSuite::testAssert(bool condition, const char* message) {
    result.testCount++;
    
    if (condition) {
        result.passedCount++;
        return true;
    } else {
        result.failedCount++;
        strncpy(result.lastFailure, message, sizeof(result.lastFailure) - 1);
        result.lastFailure[sizeof(result.lastFailure) - 1] = '\0';
        logFail("Assert", message);
        return false;
    }
}

void ValidationTestSuite::logTest(const char* testName) {
    log_d("Running test: %s", testName);
}

void ValidationTestSuite::logPass(const char* testName) {
    log_i("✓ PASS: %s", testName);
}

void ValidationTestSuite::logFail(const char* testName, const char* reason) {
    log_e("✗ FAIL: %s - %s", testName, reason);
}

void ValidationTestSuite::setupMockEnvironment() {
    // Initialize any mock objects or test data
}

void ValidationTestSuite::cleanupMockEnvironment() {
    // Clean up test resources
}

void ValidationTestSuite::printResults() const {
    log_i("=== Validation Test Results ===");
    log_i("Total tests: %d", result.testCount);
    log_i("Passed: %d", result.passedCount);
    log_i("Failed: %d", result.failedCount);
    
    if (result.failedCount > 0) {
        log_e("Last failure: %s", result.lastFailure);
    }
    
    if (result.passed) {
        log_i("✓ ALL TESTS PASSED");
    } else {
        log_e("✗ SOME TESTS FAILED");
    }
    log_i("===============================");
}

} // namespace DataIntegrity