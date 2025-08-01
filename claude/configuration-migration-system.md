# Configuration Migration System for T16 Firmware Updates

## Overview

Firmware updates should never destroy user configurations. This document outlines a comprehensive migration system that automatically converts configurations from old firmware versions to new formats, preserving user presets and settings across all firmware updates.

## Current Migration Challenges

### Existing Format Analysis (Configuration.hpp)
```cpp
// Current JSON-based configuration (v103)
struct ConfigurationData {
    uint8_t version = 102;  // Version field exists but not used for migration
    uint8_t mode = 0;
    uint8_t sensitivity = 1;
    uint8_t brightness = 6;
    // ... other fields
};

// Problems:
// 1. No structured migration system
// 2. Version check only triggers factory reset (main.cpp:786-791)
// 3. JSON field changes break compatibility
// 4. No backward compatibility
// 5. Manual reconfiguration required after updates
```

### Migration Scenarios
```cpp
enum MigrationScenario {
    SAME_VERSION,           // No migration needed
    MINOR_UPDATE,           // Compatible changes (v1.0.0 → v1.0.1)
    MAJOR_UPDATE,           // Breaking changes (v1.0.x → v1.1.0)
    ARCHITECTURE_CHANGE,    // JSON → Binary protocol
    HARDWARE_VARIANT,       // T16 → T32/T64 config adaptation
    LEGACY_RECOVERY,        // Very old firmware (emergency migration)
    DOWNGRADE,              // Newer → Older firmware (limited support)
    CORRUPTED_MIGRATION     // Partial/corrupted configuration recovery
};
```

## Comprehensive Migration Architecture

### 1. Configuration Schema Versioning

```cpp
namespace ConfigMigration {
    // Semantic versioning for configuration schemas
    struct SchemaVersion {
        uint8_t major;      // Breaking changes
        uint8_t minor;      // New features, backward compatible
        uint8_t patch;      // Bug fixes, fully compatible
        uint8_t build;      // Development builds
        
        uint32_t toUint32() const {
            return (major << 24) | (minor << 16) | (patch << 8) | build;
        }
        
        static SchemaVersion fromUint32(uint32_t version) {
            return {
                .major = (version >> 24) & 0xFF,
                .minor = (version >> 16) & 0xFF,
                .patch = (version >> 8) & 0xFF,
                .build = version & 0xFF
            };
        }
        
        bool isCompatible(const SchemaVersion& other) const {
            // Same major version = compatible
            return major == other.major;
        }
        
        bool requiresMigration(const SchemaVersion& other) const {
            return toUint32() != other.toUint32();
        }
    };
    
    // Schema evolution tracking
    static constexpr SchemaVersion SCHEMA_VERSIONS[] = {
        {1, 0, 0, 0},   // v1.0.0: Original JSON format
        {1, 0, 1, 0},   // v1.0.1: Added koala_mode field
        {1, 0, 2, 0},   // v1.0.2: Added custom scales
        {1, 1, 0, 0},   // v1.1.0: Added velocity detection config
        {2, 0, 0, 0},   // v2.0.0: Binary protocol introduction
        {2, 0, 1, 0},   // v2.0.1: Multi-variant support
        {2, 1, 0, 0},   // v2.1.0: Enhanced LED configuration
        // ... future versions
    };
    
    static constexpr SchemaVersion CURRENT_SCHEMA = {2, 1, 0, 0};
}
```

### 2. Universal Configuration Container

```cpp
// Abstract configuration that can hold any version
class UniversalConfiguration {
private:
    ConfigMigration::SchemaVersion version;
    std::unique_ptr<uint8_t[]> rawData;
    size_t dataSize;
    std::unordered_map<std::string, ConfigValue> namedFields;
    
public:
    // Configuration value that can hold any type
    struct ConfigValue {
        enum Type { INT8, UINT8, INT16, UINT16, INT32, UINT32, FLOAT, STRING, ARRAY };
        
        Type type;
        union {
            int8_t i8;
            uint8_t u8;
            int16_t i16;
            uint16_t u16;
            int32_t i32;
            uint32_t u32;
            float f32;
        } value;
        std::string stringValue;
        std::vector<ConfigValue> arrayValue;
        
        // Type-safe getters with defaults
        template<typename T>
        T get(T defaultValue = T{}) const;
        
        // Automatic conversion between compatible types
        uint8_t asUint8(uint8_t defaultValue = 0) const {
            switch (type) {
                case UINT8: return value.u8;
                case INT8: return std::max(0, (int)value.i8);
                case UINT16: return std::min(255u, (uint32_t)value.u16);
                case INT16: return std::clamp((int)value.i16, 0, 255);
                case UINT32: return std::min(255u, value.u32);
                case INT32: return std::clamp(value.i32, 0, 255);
                case FLOAT: return std::clamp((int)(value.f32 * 255), 0, 255);
                default: return defaultValue;
            }
        }
        
        float asFloat(float defaultValue = 0.0f) const {
            switch (type) {
                case FLOAT: return value.f32;
                case UINT8: return value.u8 / 255.0f;
                case INT8: return std::max(0, value.i8) / 127.0f;
                case UINT16: return value.u16 / 65535.0f;
                // ... other conversions
                default: return defaultValue;
            }
        }
    };
    
    // Load from any format
    static UniversalConfiguration fromJSON(const std::string& json);
    static UniversalConfiguration fromBinary(const uint8_t* data, size_t size);
    static UniversalConfiguration fromLegacyStruct(const void* data, size_t size, 
                                                  ConfigMigration::SchemaVersion version);
    
    // Access fields by name (schema-independent)
    ConfigValue getField(const std::string& fieldName, ConfigValue defaultValue = {}) const;
    void setField(const std::string& fieldName, const ConfigValue& value);
    
    // Schema information
    ConfigMigration::SchemaVersion getSchemaVersion() const { return version; }
    bool hasField(const std::string& fieldName) const;
    std::vector<std::string> getFieldNames() const;
    
    // Export to specific format
    std::string toJSON() const;
    std::vector<uint8_t> toBinary() const;
    CompactConfiguration toCompactBinary() const;
};
```

### 3. Migration Rule Engine

```cpp
class ConfigurationMigrator {
private:
    // Migration rule definition
    struct MigrationRule {
        ConfigMigration::SchemaVersion fromVersion;
        ConfigMigration::SchemaVersion toVersion;
        std::function<bool(UniversalConfiguration&)> migrationFunc;
        std::string description;
        bool isReversible;
        std::function<bool(UniversalConfiguration&)> reverseMigrationFunc;
    };
    
    std::vector<MigrationRule> migrationRules;
    
public:
    ConfigurationMigrator() {
        setupMigrationRules();
    }
    
    // Main migration function
    MigrationResult migrate(const UniversalConfiguration& sourceConfig, 
                          ConfigMigration::SchemaVersion targetVersion) {
        MigrationResult result;
        result.sourceVersion = sourceConfig.getSchemaVersion();
        result.targetVersion = targetVersion;
        
        // No migration needed
        if (sourceConfig.getSchemaVersion().toUint32() == targetVersion.toUint32()) {
            result.migratedConfig = sourceConfig;
            result.success = true;
            result.migrationPath = {"No migration required"};
            return result;
        }
        
        // Find migration path
        auto path = findMigrationPath(sourceConfig.getSchemaVersion(), targetVersion);
        if (path.empty()) {
            result.success = false;
            result.error = "No migration path found";
            return result;
        }
        
        // Execute migration steps
        UniversalConfiguration current = sourceConfig;
        for (const auto& step : path) {
            if (!executeMigrationStep(current, step)) {
                result.success = false;
                result.error = "Migration step failed: " + step.description;
                return result;
            }
            result.migrationPath.push_back(step.description);
        }
        
        result.migratedConfig = current;
        result.success = true;
        return result;
    }
    
private:
    void setupMigrationRules() {
        // v1.0.0 → v1.0.1: Add koala_mode field
        migrationRules.push_back({
            .fromVersion = {1, 0, 0, 0},
            .toVersion = {1, 0, 1, 0},
            .migrationFunc = [](UniversalConfiguration& config) {
                // Add koala_mode field to each bank (default: false)
                for (int bank = 0; bank < 4; bank++) {
                    std::string fieldName = "banks[" + std::to_string(bank) + "].koala_mode";
                    if (!config.hasField(fieldName)) {
                        config.setField(fieldName, ConfigValue{.type = ConfigValue::UINT8, .value = {.u8 = 0}});
                    }
                }
                return true;
            },
            .description = "Add koala_mode support",
            .isReversible = true,
            .reverseMigrationFunc = [](UniversalConfiguration& config) {
                // Remove koala_mode fields for backward compatibility
                for (int bank = 0; bank < 4; bank++) {
                    std::string fieldName = "banks[" + std::to_string(bank) + "].koala_mode";
                    config.removeField(fieldName);
                }
                return true;
            }
        });
        
        // v1.0.1 → v1.0.2: Add custom scales
        migrationRules.push_back({
            .fromVersion = {1, 0, 1, 0},
            .toVersion = {1, 0, 2, 0},
            .migrationFunc = [](UniversalConfiguration& config) {
                // Add custom scale arrays if missing
                if (!config.hasField("custom_scale1")) {
                    std::vector<ConfigValue> defaultScale;
                    for (int i = 0; i < 16; i++) {
                        defaultScale.push_back({.type = ConfigValue::INT8, .value = {.i8 = (int8_t)i}});
                    }
                    config.setField("custom_scale1", {.type = ConfigValue::ARRAY, .arrayValue = defaultScale});
                }
                if (!config.hasField("custom_scale2")) {
                    std::vector<ConfigValue> defaultScale;
                    for (int i = 0; i < 16; i++) {
                        defaultScale.push_back({.type = ConfigValue::INT8, .value = {.i8 = (int8_t)i}});
                    }
                    config.setField("custom_scale2", {.type = ConfigValue::ARRAY, .arrayValue = defaultScale});
                }
                return true;
            },
            .description = "Add custom scale support",
            .isReversible = true
        });
        
        // v1.1.0 → v2.0.0: JSON → Binary protocol (major breaking change)
        migrationRules.push_back({
            .fromVersion = {1, 1, 0, 0},
            .toVersion = {2, 0, 0, 0},
            .migrationFunc = [](UniversalConfiguration& config) {
                return migrateJsonToBinaryProtocol(config);
            },
            .description = "Migrate to binary protocol",
            .isReversible = false // Binary → JSON conversion is possible but lossy
        });
        
        // v2.0.0 → v2.0.1: Add multi-variant support
        migrationRules.push_back({
            .fromVersion = {2, 0, 0, 0},
            .toVersion = {2, 0, 1, 0},
            .migrationFunc = [](UniversalConfiguration& config) {
                // Add hardware variant field
                if (!config.hasField("hardware_variant")) {
                    config.setField("hardware_variant", {.type = ConfigValue::UINT8, .value = {.u8 = 0}}); // T16
                }
                
                // Add variant-specific calibration arrays
                return addVariantSpecificFields(config);
            },
            .description = "Add multi-variant support",
            .isReversible = true
        });
        
        // Add more migration rules for future versions...
    }
    
    // Dijkstra-like algorithm to find optimal migration path
    std::vector<MigrationRule> findMigrationPath(ConfigMigration::SchemaVersion from,
                                                ConfigMigration::SchemaVersion to) {
        struct PathNode {
            ConfigMigration::SchemaVersion version;
            std::vector<MigrationRule> path;
            int cost;
        };
        
        std::queue<PathNode> queue;
        std::set<uint32_t> visited;
        
        queue.push({from, {}, 0});
        
        while (!queue.empty()) {
            PathNode current = queue.front();
            queue.pop();
            
            if (current.version.toUint32() == to.toUint32()) {
                return current.path; // Found path
            }
            
            uint32_t currentVersion = current.version.toUint32();
            if (visited.contains(currentVersion)) continue;
            visited.insert(currentVersion);
            
            // Explore possible migration steps
            for (const auto& rule : migrationRules) {
                if (rule.fromVersion.toUint32() == currentVersion) {
                    PathNode next = current;
                    next.version = rule.toVersion;
                    next.path.push_back(rule);
                    next.cost++;
                    
                    // Prefer direct paths and avoid excessive hops
                    if (next.cost < 10) { // Reasonable limit
                        queue.push(next);
                    }
                }
            }
        }
        
        return {}; // No path found
    }
    
    bool executeMigrationStep(UniversalConfiguration& config, const MigrationRule& rule) {
        try {
            return rule.migrationFunc(config);
        } catch (const std::exception& e) {
            log_e("Migration step failed: %s", e.what());
            return false;
        }
    }
    
    // Complex migration: JSON → Binary protocol
    bool migrateJsonToBinaryProtocol(UniversalConfiguration& config) {
        // Create new binary configuration structure
        CompactConfiguration binaryConfig = {};
        
        // Migrate global settings
        binaryConfig.version = 2;
        binaryConfig.global_flags = 0;
        
        // Pack brightness (3 bits)
        uint8_t brightness = config.getField("brightness").asUint8(6);
        binaryConfig.global_flags |= (std::min(brightness, 7u) << 0);
        
        // Pack MIDI settings (3 bits)
        uint8_t midiTrs = config.getField("midi_trs").asUint8(0);
        uint8_t midiBle = config.getField("midi_ble").asUint8(0);
        uint8_t trsType = config.getField("trs_type").asUint8(0);
        binaryConfig.global_flags |= (midiTrs << 3) | (midiBle << 4) | (trsType << 5);
        
        // Migrate bank configurations
        for (int bank = 0; bank < 4; bank++) {
            std::string bankPrefix = "banks[" + std::to_string(bank) + "]";
            
            auto& bankConfig = binaryConfig.banks[bank];
            bankConfig.channel = config.getField(bankPrefix + ".ch").asUint8(1);
            bankConfig.scale = config.getField(bankPrefix + ".scale").asUint8(0);
            bankConfig.octave = config.getField(bankPrefix + ".oct").asUint8(2);
            bankConfig.velocity_curve = config.getField(bankPrefix + ".vel").asUint8(1);
            bankConfig.aftertouch_curve = config.getField(bankPrefix + ".at").asUint8(1);
            bankConfig.flip_x = config.getField(bankPrefix + ".flip_x").asUint8(0);
            bankConfig.flip_y = config.getField(bankPrefix + ".flip_y").asUint8(0);
            bankConfig.koala_mode = config.getField(bankPrefix + ".koala_mode").asUint8(0);
            bankConfig.base_note = config.getField(bankPrefix + ".note").asUint8(0);
            bankConfig.palette = config.getField(bankPrefix + ".palette").asUint8(0);
            
            // Migrate MIDI routing
            for (int cc = 0; cc < 8; cc++) {
                std::string ccPrefix = bankPrefix + ".chs[" + std::to_string(cc) + "]";
                std::string idPrefix = bankPrefix + ".ids[" + std::to_string(cc) + "]";
                
                binaryConfig.midi_config[bank].channels[cc] = config.getField(ccPrefix).asUint8(1);
                binaryConfig.midi_config[bank].cc_ids[cc] = config.getField(idPrefix).asUint8(13 + cc);
            }
        }
        
        // Migrate custom scales
        auto customScale1 = config.getField("custom_scale1");
        auto customScale2 = config.getField("custom_scale2");
        
        if (customScale1.type == ConfigValue::ARRAY) {
            for (size_t i = 0; i < std::min(customScale1.arrayValue.size(), 16UL); i++) {
                binaryConfig.custom_scales[i] = customScale1.arrayValue[i].asUint8(i);
            }
        }
        if (customScale2.type == ConfigValue::ARRAY) {
            for (size_t i = 0; i < std::min(customScale2.arrayValue.size(), 16UL); i++) {
                binaryConfig.custom_scales[16 + i] = customScale2.arrayValue[i].asUint8(i);
            }
        }
        
        // Calculate checksum
        binaryConfig.checksum = crc8(&binaryConfig, sizeof(binaryConfig) - 1);
        
        // Replace configuration with binary version
        config = UniversalConfiguration::fromBinary(reinterpret_cast<uint8_t*>(&binaryConfig), 
                                                  sizeof(binaryConfig));
        
        return true;
    }
};
```

### 4. Automatic Migration Detection and Execution

```cpp
class AutoMigrationManager {
private:
    ConfigurationMigrator migrator;
    ConfigurationRecoverySystem recovery;
    
public:
    struct MigrationStatus {
        bool migrationRequired;
        bool migrationPossible;
        ConfigMigration::SchemaVersion currentVersion;
        ConfigMigration::SchemaVersion targetVersion;
        std::vector<std::string> migrationSteps;
        MigrationRisk riskLevel;
        size_t estimatedTimeMs;
    };
    
    enum MigrationRisk {
        SAFE,           // Fully reversible, no data loss risk
        LOW,            // Minor risk, mostly reversible
        MODERATE,       // Some features might be lost
        HIGH,           // Significant changes, backup recommended
        DANGEROUS       // Major breaking changes, full backup required
    };
    
    // Called during firmware boot
    MigrationStatus checkMigrationNeeded() {
        MigrationStatus status = {};
        
        // Try to load existing configuration
        auto existingConfig = loadExistingConfiguration();
        if (!existingConfig) {
            // No existing configuration - use factory defaults
            status.migrationRequired = false;
            return status;
        }
        
        status.currentVersion = existingConfig->getSchemaVersion();
        status.targetVersion = ConfigMigration::CURRENT_SCHEMA;
        status.migrationRequired = status.currentVersion.requiresMigration(status.targetVersion);
        
        if (!status.migrationRequired) {
            return status; // No migration needed
        }
        
        // Analyze migration requirements
        auto migrationResult = migrator.migrate(*existingConfig, status.targetVersion);
        status.migrationPossible = migrationResult.success;
        status.migrationSteps = migrationResult.migrationPath;
        status.riskLevel = assessMigrationRisk(status.currentVersion, status.targetVersion);
        status.estimatedTimeMs = estimateMigrationTime(status.migrationSteps);
        
        return status;
    }
    
    // Execute automatic migration with user consent
    bool performAutoMigration(bool userConsent = true) {
        auto status = checkMigrationNeeded();
        
        if (!status.migrationRequired) {
            log_i("No migration required");
            return true;
        }
        
        if (!status.migrationPossible) {
            log_e("Migration not possible - using factory defaults");
            return initializeFactoryDefaults();
        }
        
        // Risk assessment
        if (status.riskLevel >= HIGH && !userConsent) {
            log_w("High-risk migration requires user consent");
            return false;
        }
        
        log_i("Starting configuration migration from %d.%d.%d to %d.%d.%d",
              status.currentVersion.major, status.currentVersion.minor, status.currentVersion.patch,
              status.targetVersion.major, status.targetVersion.minor, status.targetVersion.patch);
        
        // Create backup before migration
        recovery.createSnapshot(RecoveryReason::BEFORE_MIGRATION, "Pre-migration backup");
        
        // Load and migrate configuration
        auto existingConfig = loadExistingConfiguration();
        auto migrationResult = migrator.migrate(*existingConfig, status.targetVersion);
        
        if (!migrationResult.success) {
            log_e("Migration failed: %s", migrationResult.error.c_str());
            return false;
        }
        
        // Validate migrated configuration
        ConfigurationValidator validator;
        ValidationContext context = getCurrentContext();
        auto validation = validator.validateConfiguration(
            migrationResult.migratedConfig.toCompactBinary(), context
        );
        
        if (!validation.isValid) {
            log_e("Migrated configuration validation failed");
            
            // Attempt auto-fix
            if (validation.canAutoFix()) {
                log_i("Attempting to auto-fix migration issues");
                // Apply fixes and re-validate
                // ... auto-fix logic ...
            } else {
                return false;
            }
        }
        
        // Apply migrated configuration
        if (!applyMigratedConfiguration(migrationResult.migratedConfig)) {
            log_e("Failed to apply migrated configuration");
            return false;
        }
        
        log_i("Configuration migration completed successfully");
        logMigrationSuccess(status, migrationResult);
        
        return true;
    }
    
private:
    std::unique_ptr<UniversalConfiguration> loadExistingConfiguration() {
        // Try multiple sources in order of preference
        
        // 1. Try current binary format
        if (auto config = tryLoadBinaryConfiguration()) {
            return config;
        }
        
        // 2. Try JSON format (legacy)
        if (auto config = tryLoadJSONConfiguration()) {
            return config;
        }
        
        // 3. Try even older formats
        if (auto config = tryLoadLegacyFormats()) {
            return config;
        }
        
        return nullptr; // No configuration found
    }
    
    std::unique_ptr<UniversalConfiguration> tryLoadBinaryConfiguration() {
        // Load from flash memory
        CompactConfiguration binaryConfig;
        if (!loadFromFlash(CONFIGURATION_FLASH_ADDRESS, &binaryConfig, sizeof(binaryConfig))) {
            return nullptr;
        }
        
        // Verify checksum
        uint8_t expectedChecksum = crc8(&binaryConfig, sizeof(binaryConfig) - 1);
        if (binaryConfig.checksum != expectedChecksum) {
            log_w("Binary configuration checksum mismatch");
            return nullptr;
        }
        
        return std::make_unique<UniversalConfiguration>(
            UniversalConfiguration::fromBinary(reinterpret_cast<uint8_t*>(&binaryConfig), 
                                              sizeof(binaryConfig))
        );
    }
    
    std::unique_ptr<UniversalConfiguration> tryLoadJSONConfiguration() {
        // Load JSON from DataManager (existing system)
        DataManager config("/configuration_data.json");
        if (!config.Init()) {
            return nullptr;
        }
        
        // Serialize to string
        char buffer[4096];
        size_t size = config.SerializeToBuffer(buffer, sizeof(buffer));
        if (size == 0) {
            return nullptr;
        }
        
        std::string jsonString(buffer, size);
        
        try {
            auto universalConfig = UniversalConfiguration::fromJSON(jsonString);
            
            // Infer version from JSON structure
            auto version = inferVersionFromJSON(universalConfig);
            universalConfig.setSchemaVersion(version);
            
            return std::make_unique<UniversalConfiguration>(std::move(universalConfig));
        } catch (const std::exception& e) {
            log_e("Failed to parse JSON configuration: %s", e.what());
            return nullptr;
        }
    }
    
    ConfigMigration::SchemaVersion inferVersionFromJSON(const UniversalConfiguration& config) {
        // Use feature detection to infer version
        if (config.hasField("velocity_detection")) {
            return {1, 1, 0, 0}; // Has velocity detection config
        } else if (config.hasField("custom_scale1")) {
            return {1, 0, 2, 0}; // Has custom scales
        } else if (config.hasField("banks[0].koala_mode")) {
            return {1, 0, 1, 0}; // Has koala mode
        } else {
            return {1, 0, 0, 0}; // Original version
        }
    }
    
    MigrationRisk assessMigrationRisk(ConfigMigration::SchemaVersion from, 
                                    ConfigMigration::SchemaVersion to) {
        // Major version changes are risky
        if (from.major != to.major) {
            return DANGEROUS;
        }
        
        // Minor version changes have moderate risk
        if (from.minor != to.minor) {
            return MODERATE;
        }
        
        // Patch level changes are safe
        return SAFE;
    }
    
    size_t estimateMigrationTime(const std::vector<std::string>& steps) {
        // Estimate based on number and complexity of steps
        size_t baseTime = 100; // ms
        size_t timePerStep = 50; // ms
        
        return baseTime + (steps.size() * timePerStep);
    }
    
    void logMigrationSuccess(const MigrationStatus& status, const MigrationResult& result) {
        log_i("Migration completed:");
        log_i("  From: %d.%d.%d.%d", 
              status.currentVersion.major, status.currentVersion.minor, 
              status.currentVersion.patch, status.currentVersion.build);
        log_i("  To: %d.%d.%d.%d",
              status.targetVersion.major, status.targetVersion.minor,
              status.targetVersion.patch, status.targetVersion.build);
        log_i("  Steps: %d", result.migrationPath.size());
        
        for (const auto& step : result.migrationPath) {
            log_i("    - %s", step.c_str());
        }
    }
};
```

### 5. Web Interface Migration Support

```typescript
// Web interface migration handling
class ConfigurationMigrationUI {
    private device: DeviceConnection;
    private migrationStatus: MigrationStatus | null = null;
    
    async checkForMigration(): Promise<void> {
        try {
            // Query device for migration status
            this.migrationStatus = await this.device.getMigrationStatus();
            
            if (this.migrationStatus.migrationRequired) {
                this.showMigrationDialog();
            }
            
        } catch (error) {
            console.error('Failed to check migration status:', error);
        }
    }
    
    private showMigrationDialog(): void {
        const dialog = new MigrationDialog({
            status: this.migrationStatus!,
            onAccept: () => this.performMigration(),
            onDecline: () => this.handleMigrationDecline(),
            onBackup: () => this.createBackup()
        });
        
        dialog.show();
    }
    
    private async performMigration(): Promise<void> {
        if (!this.migrationStatus) return;
        
        try {
            // Show progress dialog
            const progressDialog = new ProgressDialog({
                title: 'Migrating Configuration',
                steps: this.migrationStatus.migrationSteps,
                estimatedTime: this.migrationStatus.estimatedTimeMs
            });
            
            progressDialog.show();
            
            // Start migration on device
            const result = await this.device.performMigration({
                progressCallback: (step: string, progress: number) => {
                    progressDialog.updateProgress(step, progress);
                }
            });
            
            if (result.success) {
                progressDialog.close();
                this.showMigrationSuccess(result);
                
                // Reload configuration from device
                await this.reloadConfiguration();
                
            } else {
                progressDialog.close();
                this.showMigrationError(result.error);
            }
            
        } catch (error) {
            console.error('Migration failed:', error);
            this.showMigrationError(error.message);
        }
    }
    
    private async createBackup(): Promise<void> {
        try {
            // Download current configuration as backup
            const config = await this.device.downloadConfiguration();
            const timestamp = new Date().toISOString().slice(0, 19).replace(/:/g, '-');
            const filename = `t16-backup-${timestamp}.topo`;
            
            this.downloadFile(config, filename);
            
            this.showNotification({
                type: 'success',
                message: `Configuration backup saved as ${filename}`
            });
            
        } catch (error) {
            this.showNotification({
                type: 'error',
                message: `Failed to create backup: ${error.message}`
            });
        }
    }
    
    // Handle cases where user declines migration
    private handleMigrationDecline(): void {
        if (!this.migrationStatus) return;
        
        if (this.migrationStatus.riskLevel >= MigrationRisk.HIGH) {
            // Show warning about potential compatibility issues
            this.showCompatibilityWarning();
        }
        
        // Continue with factory defaults or limited functionality
        this.initializeLimitedMode();
    }
    
    private showCompatibilityWarning(): void {
        const warning = new WarningDialog({
            title: 'Configuration Compatibility Warning',
            message: `
                Your device configuration is from an older firmware version and may not be 
                fully compatible. Some features may not work correctly without migration.
                
                Recommended actions:
                • Create a backup of your current settings
                • Perform the migration to ensure full compatibility
                • Contact support if you experience issues
            `,
            actions: [
                { label: 'Create Backup', action: () => this.createBackup() },
                { label: 'Migrate Now', action: () => this.performMigration() },
                { label: 'Continue Anyway', action: () => this.initializeLimitedMode() }
            ]
        });
        
        warning.show();
    }
    
    // Migration progress visualization
    private renderMigrationProgress(): JSX.Element {
        if (!this.migrationStatus) return null;
        
        return (
            <div className="migration-progress">
                <h3>Configuration Migration in Progress</h3>
                
                <div className="migration-info">
                    <div className="version-info">
                        <span>From: v{this.formatVersion(this.migrationStatus.currentVersion)}</span>
                        <span>To: v{this.formatVersion(this.migrationStatus.targetVersion)}</span>
                    </div>
                    
                    <div className="risk-level">
                        <span className={`risk-${this.migrationStatus.riskLevel.toLowerCase()}`}>
                            Risk Level: {this.migrationStatus.riskLevel}
                        </span>
                    </div>
                </div>
                
                <div className="migration-steps">
                    <h4>Migration Steps:</h4>
                    <ul>
                        {this.migrationStatus.migrationSteps.map((step, index) => (
                            <li key={index} className="migration-step">
                                <span className="step-number">{index + 1}</span>
                                <span className="step-description">{step}</span>
                            </li>
                        ))}
                    </ul>
                </div>
                
                <div className="migration-actions">
                    <button onClick={() => this.createBackup()} className="backup-button">
                        Create Backup First
                    </button>
                    <button onClick={() => this.performMigration()} className="migrate-button">
                        Start Migration
                    </button>
                    <button onClick={() => this.handleMigrationDecline()} className="decline-button">
                        Skip Migration
                    </button>
                </div>
            </div>
        );
    }
}

// Migration-aware configuration loader
class MigrationAwareConfigLoader {
    async loadConfiguration(device: DeviceConnection): Promise<UniversalConfiguration> {
        // Check if migration is needed
        const migrationStatus = await device.getMigrationStatus();
        
        if (migrationStatus.migrationRequired) {
            // Handle migration UI flow
            const migrationUI = new ConfigurationMigrationUI();
            await migrationUI.handleMigration(device, migrationStatus);
        }
        
        // Load current configuration (post-migration)
        return await device.loadUniversalConfiguration();
    }
}
```

### 6. Testing and Validation

```cpp
// Comprehensive migration testing
class MigrationTestSuite {
public:
    struct TestCase {
        std::string name;
        ConfigMigration::SchemaVersion fromVersion;
        ConfigMigration::SchemaVersion toVersion;
        UniversalConfiguration inputConfig;
        std::function<bool(const UniversalConfiguration&)> validator;
    };
    
    bool runAllTests() {
        std::vector<TestCase> testCases = generateTestCases();
        
        bool allPassed = true;
        for (const auto& testCase : testCases) {
            if (!runSingleTest(testCase)) {
                allPassed = false;
            }
        }
        
        return allPassed;
    }
    
private:
    std::vector<TestCase> generateTestCases() {
        return {
            // Test v1.0.0 → v1.0.1 migration
            {
                .name = "Add koala_mode field",
                .fromVersion = {1, 0, 0, 0},
                .toVersion = {1, 0, 1, 0},
                .inputConfig = createV100Config(),
                .validator = [](const UniversalConfiguration& config) {
                    return config.hasField("banks[0].koala_mode") &&
                           config.getField("banks[0].koala_mode").asUint8() == 0;
                }
            },
            
            // Test JSON → Binary migration
            {
                .name = "JSON to Binary protocol",
                .fromVersion = {1, 1, 0, 0},
                .toVersion = {2, 0, 0, 0},
                .inputConfig = createV110JSONConfig(),
                .validator = [](const UniversalConfiguration& config) {
                    // Verify binary format and data integrity
                    auto binary = config.toBinary();
                    return binary.size() <= 128 && // Compact size
                           config.getField("version").asUint8() == 2;
                }
            },
            
            // Test complex multi-step migration
            {
                .name = "Multi-step migration v1.0.0 → v2.1.0",
                .fromVersion = {1, 0, 0, 0},
                .toVersion = {2, 1, 0, 0},
                .inputConfig = createV100Config(),
                .validator = validateCompleteModernConfig
            },
            
            // Test edge cases and error handling
            {
                .name = "Corrupted configuration recovery",
                .fromVersion = {1, 0, 0, 0},
                .toVersion = {2, 1, 0, 0},
                .inputConfig = createCorruptedConfig(),
                .validator = [](const UniversalConfiguration& config) {
                    // Should fall back to defaults but preserve what's salvageable
                    return config.getField("version").asUint8() > 0;
                }
            }
        };
    }
    
    bool runSingleTest(const TestCase& testCase) {
        log_i("Running migration test: %s", testCase.name.c_str());
        
        ConfigurationMigrator migrator;
        auto result = migrator.migrate(testCase.inputConfig, testCase.toVersion);
        
        if (!result.success) {
            log_e("Migration failed: %s", result.error.c_str());
            return false;
        }
        
        if (!testCase.validator(result.migratedConfig)) {
            log_e("Migration validation failed");
            return false;
        }
        
        log_i("Migration test passed: %s", testCase.name.c_str());
        return true;
    }
};
```

## Integration with Existing System

### Boot Sequence Integration
```cpp
// Modified main.cpp boot sequence
void setup() {
    // ... existing hardware initialization ...
    
    // Initialize migration system
    AutoMigrationManager migrationManager;
    
    // Check if migration is needed
    auto migrationStatus = migrationManager.checkMigrationNeeded();
    
    if (migrationStatus.migrationRequired) {
        log_i("Configuration migration required");
        
        // Show migration status on LEDs
        showMigrationStatus(migrationStatus);
        
        // Perform automatic migration for safe cases
        if (migrationStatus.riskLevel <= MODERATE) {
            if (migrationManager.performAutoMigration(true)) {
                log_i("Automatic migration completed");
            } else {
                log_e("Automatic migration failed - using factory defaults");
                initializeFactoryDefaults();
            }
        } else {
            // High-risk migration requires user interaction via web interface
            log_w("High-risk migration requires user confirmation");
            setMigrationPendingFlag(true);
        }
    }
    
    // ... continue with normal initialization ...
}
```

## Benefits and Results

This comprehensive migration system provides:

**Seamless Updates:**
- **Zero configuration loss** during firmware updates
- **Automatic detection** and migration of legacy formats
- **Multi-step migration** support for major version jumps
- **Backward compatibility** where possible

**Risk Management:**
- **Risk assessment** before migration (SAFE → DANGEROUS)
- **Automatic backups** before risky operations  
- **Rollback capability** for failed migrations
- **Factory reset** as last resort with data preservation

**User Experience:**
- **Transparent migration** for low-risk updates
- **Clear communication** about migration requirements
- **Progress indication** for complex migrations
- **Backup creation** tools for user peace of mind

**Developer Benefits:**
- **Schema versioning** system for organized evolution
- **Migration testing** framework
- **Universal configuration** format for cross-version compatibility
- **Rule-based migration** engine for maintainable updates

Users can confidently update firmware knowing their carefully crafted configurations will be preserved and automatically converted to the new format, maintaining full functionality while gaining access to new features.