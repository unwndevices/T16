#include "CriticalDataProtector.hpp"

namespace DataIntegrity {

CriticalDataProtector::CriticalDataProtector() {
    // Initialize with factory defaults
    criticalData.configuration.setValue(getFactoryConfiguration());
    criticalData.calibration.setValue(getFactoryCalibration());
    criticalData.deviceSerial.setValue(0);
    criticalData.firmwareVersion.setValue(102);
    updateStructureChecksum();
}

bool CriticalDataProtector::initialize() {
    if (!LittleFS.begin()) {
        log_e("Failed to mount LittleFS");
        return false;
    }
    
    // Try to load critical data
    if (!loadCriticalData()) {
        log_w("No valid critical data found, initializing with defaults");
        // Save initial data
        return persistCriticalData();
    }
    
    // Verify integrity
    if (!verifyIntegrity()) {
        log_w("Critical data corruption detected, attempting repair");
        if (!repairCorruption()) {
            log_e("Failed to repair corruption, resetting to factory defaults");
            factoryReset(true);
            return persistCriticalData();
        }
    }
    
    return true;
}

ConfigurationData CriticalDataProtector::getConfiguration() const {
    return criticalData.configuration.getMajorityValue();
}

CalibrationData CriticalDataProtector::getCalibration() const {
    return criticalData.calibration.getMajorityValue();
}

uint32_t CriticalDataProtector::getDeviceSerial() const {
    return criticalData.deviceSerial.getMajorityValue();
}

uint8_t CriticalDataProtector::getFirmwareVersion() const {
    return criticalData.firmwareVersion.getMajorityValue();
}

void CriticalDataProtector::setDeviceSerial(uint32_t serial) {
    criticalData.deviceSerial.setValue(serial);
    updateStructureChecksum();
}

void CriticalDataProtector::setFirmwareVersion(uint8_t version) {
    criticalData.firmwareVersion.setValue(version);
    updateStructureChecksum();
}

CriticalDataProtector::ConfigurationTransaction CriticalDataProtector::beginTransaction() {
    return ConfigurationTransaction(this);
}

bool CriticalDataProtector::verifyIntegrity() const {
    // Check structure checksum
    if (!verifyStructureChecksum()) {
        return false;
    }
    
    // Check individual redundant data
    if (criticalData.configuration.isCorrupted()) {
        log_w("Configuration data corruption detected");
        return false;
    }
    
    if (criticalData.calibration.isCorrupted()) {
        log_w("Calibration data corruption detected");
        return false;
    }
    
    if (criticalData.deviceSerial.isCorrupted()) {
        log_w("Device serial corruption detected");
        return false;
    }
    
    if (criticalData.firmwareVersion.isCorrupted()) {
        log_w("Firmware version corruption detected");
        return false;
    }
    
    return true;
}

uint8_t CriticalDataProtector::getCorruptionLevel() const {
    uint8_t corruptionScore = 0;
    
    // Check each redundant data set
    uint8_t configValid = criticalData.configuration.getValidCopyCount();
    uint8_t calibValid = criticalData.calibration.getValidCopyCount();
    uint8_t serialValid = criticalData.deviceSerial.getValidCopyCount();
    uint8_t versionValid = criticalData.firmwareVersion.getValidCopyCount();
    
    // Calculate corruption score (0 = no corruption, 12 = total corruption)
    corruptionScore += (3 - configValid);
    corruptionScore += (3 - calibValid);
    corruptionScore += (3 - serialValid);
    corruptionScore += (3 - versionValid);
    
    return corruptionScore;
}

bool CriticalDataProtector::repairCorruption() {
    bool repaired = false;
    
    // Configuration will auto-repair via getMajorityValue()
    if (criticalData.configuration.isCorrupted()) {
        ConfigurationData goodConfig = criticalData.configuration.getMajorityValue();
        criticalData.configuration.setValue(goodConfig);
        repaired = true;
    }
    
    if (criticalData.calibration.isCorrupted()) {
        CalibrationData goodCalib = criticalData.calibration.getMajorityValue();
        criticalData.calibration.setValue(goodCalib);
        repaired = true;
    }
    
    if (criticalData.deviceSerial.isCorrupted()) {
        uint32_t goodSerial = criticalData.deviceSerial.getMajorityValue();
        criticalData.deviceSerial.setValue(goodSerial);
        repaired = true;
    }
    
    if (criticalData.firmwareVersion.isCorrupted()) {
        uint8_t goodVersion = criticalData.firmwareVersion.getMajorityValue();
        criticalData.firmwareVersion.setValue(goodVersion);
        repaired = true;
    }
    
    if (repaired) {
        updateStructureChecksum();
        log_i("Critical data repaired successfully");
    }
    
    return true;
}

bool CriticalDataProtector::persistCriticalData() {
    // Write to multiple files for redundancy
    bool success = true;
    
    if (!writeToFile(CRITICAL_DATA_FILE, criticalData)) {
        log_e("Failed to write primary critical data file");
        success = false;
    }
    
    if (!writeToFile(CRITICAL_BACKUP1_FILE, criticalData)) {
        log_e("Failed to write backup1 critical data file");
        success = false;
    }
    
    if (!writeToFile(CRITICAL_BACKUP2_FILE, criticalData)) {
        log_e("Failed to write backup2 critical data file");
        success = false;
    }
    
    return success;
}

bool CriticalDataProtector::loadCriticalData() {
    CriticalData primaryData, backup1Data, backup2Data;
    bool primaryValid = false, backup1Valid = false, backup2Valid = false;
    
    // Try to read all files
    if (readFromFile(CRITICAL_DATA_FILE, primaryData)) {
        if (primaryData.magic == 0x54313643 && verifyStructureChecksum()) {
            primaryValid = true;
        }
    }
    
    if (readFromFile(CRITICAL_BACKUP1_FILE, backup1Data)) {
        if (backup1Data.magic == 0x54313643) {
            backup1Valid = true;
        }
    }
    
    if (readFromFile(CRITICAL_BACKUP2_FILE, backup2Data)) {
        if (backup2Data.magic == 0x54313643) {
            backup2Valid = true;
        }
    }
    
    // Determine which data to use
    int validCount = (primaryValid ? 1 : 0) + (backup1Valid ? 1 : 0) + (backup2Valid ? 1 : 0);
    
    if (validCount == 0) {
        log_e("No valid critical data files found");
        return false;
    }
    
    // Use majority voting if multiple valid copies
    if (validCount >= 2) {
        if (primaryValid && backup1Valid && 
            memcmp(&primaryData, &backup1Data, sizeof(CriticalData)) == 0) {
            criticalData = primaryData;
            return true;
        }
        if (primaryValid && backup2Valid && 
            memcmp(&primaryData, &backup2Data, sizeof(CriticalData)) == 0) {
            criticalData = primaryData;
            return true;
        }
        if (backup1Valid && backup2Valid && 
            memcmp(&backup1Data, &backup2Data, sizeof(CriticalData)) == 0) {
            criticalData = backup1Data;
            return true;
        }
    }
    
    // Use any valid copy
    if (primaryValid) {
        criticalData = primaryData;
        log_w("Using primary critical data file (backups invalid)");
        return true;
    }
    if (backup1Valid) {
        criticalData = backup1Data;
        log_w("Using backup1 critical data file");
        return true;
    }
    if (backup2Valid) {
        criticalData = backup2Data;
        log_w("Using backup2 critical data file");
        return true;
    }
    
    return false;
}

void CriticalDataProtector::factoryReset(bool preserveCalibration) {
    CalibrationData currentCalibration;
    uint32_t currentSerial = getDeviceSerial();
    
    if (preserveCalibration) {
        currentCalibration = getCalibration();
    } else {
        currentCalibration = getFactoryCalibration();
    }
    
    // Reset to factory defaults
    criticalData.configuration.setValue(getFactoryConfiguration());
    criticalData.calibration.setValue(currentCalibration);
    criticalData.deviceSerial.setValue(currentSerial);
    criticalData.firmwareVersion.setValue(102);
    
    updateStructureChecksum();
    log_w("Factory reset completed%s", preserveCalibration ? " (calibration preserved)" : "");
}

ConfigurationValidator::ValidationContext CriticalDataProtector::getCurrentContext() const {
    return {
        .variant = HardwareVariant::T16, // Would need to detect actual variant
        .firmwareVersion = getFirmwareVersion(),
        .strictMode = true,
        .level = ConfigurationValidator::COMPREHENSIVE
    };
}

void CriticalDataProtector::updateStructureChecksum() {
    // Calculate checksum excluding the checksum field itself
    size_t checksumOffset = offsetof(CriticalData, structureChecksum);
    criticalData.structureChecksum = crc32(&criticalData, checksumOffset);
}

bool CriticalDataProtector::verifyStructureChecksum() const {
    size_t checksumOffset = offsetof(CriticalData, structureChecksum);
    uint32_t calculated = crc32(&criticalData, checksumOffset);
    return calculated == criticalData.structureChecksum;
}

ConfigurationData CriticalDataProtector::getFactoryConfiguration() const {
    ConfigurationData factory;
    factory.version = 102;
    factory.mode = 0;
    factory.sensitivity = 1;
    factory.brightness = 6;
    factory.palette = 0;
    factory.midi_trs = 0;
    factory.trs_type = 0;
    factory.passthrough = 0;
    factory.midi_ble = 0;
    
    // Initialize custom scales to chromatic
    for (int i = 0; i < 16; i++) {
        factory.custom_scale1[i] = i;
        factory.custom_scale2[i] = i;
    }
    
    factory.hasChanged = false;
    return factory;
}

CalibrationData CriticalDataProtector::getFactoryCalibration() const {
    CalibrationData factory;
    for (int i = 0; i < 16; i++) {
        factory.minVal[i] = 0;
        factory.maxVal[i] = 4095;
    }
    return factory;
}

bool CriticalDataProtector::writeToFile(const char* filename, const CriticalData& data) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        log_e("Failed to open %s for writing", filename);
        return false;
    }
    
    size_t written = file.write((const uint8_t*)&data, sizeof(CriticalData));
    file.close();
    
    if (written != sizeof(CriticalData)) {
        log_e("Failed to write complete data to %s", filename);
        return false;
    }
    
    return true;
}

bool CriticalDataProtector::readFromFile(const char* filename, CriticalData& data) {
    if (!LittleFS.exists(filename)) {
        return false;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        log_e("Failed to open %s for reading", filename);
        return false;
    }
    
    size_t read = file.read((uint8_t*)&data, sizeof(CriticalData));
    file.close();
    
    if (read != sizeof(CriticalData)) {
        log_e("Failed to read complete data from %s", filename);
        return false;
    }
    
    return true;
}

bool CriticalDataProtector::performWearLeveling() {
    // Simple wear leveling by rotating file usage
    static uint8_t writeCounter = 0;
    writeCounter++;
    
    // Rotate which file gets written first every 10 writes
    if (writeCounter % 10 == 0) {
        // Swap file names temporarily
        const char* temp = CRITICAL_DATA_FILE;
        const_cast<const char*&>(CRITICAL_DATA_FILE) = CRITICAL_BACKUP1_FILE;
        const_cast<const char*&>(CRITICAL_BACKUP1_FILE) = temp;
    }
    
    return true;
}

} // namespace DataIntegrity