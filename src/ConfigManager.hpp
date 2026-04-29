#pragma once

#include <ArduinoJson.h>
#include "Configuration.hpp"

class ConfigManager
{
public:
    // Initialize: mount LittleFS, load config from flash into structs
    void Init();

    // Persistence
    void Save(bool force = false);
    bool IsDirty() const;
    void MarkDirty();
    void CheckIdleFlush(unsigned long now);

    // Direct struct access for runtime reads (no flash access)
    ConfigurationData& Global();
    const ConfigurationData& Global() const;
    KeyModeData& Bank(uint8_t index);
    const KeyModeData& Bank(uint8_t index) const;
    ControlChangeData& CC(uint8_t index);
    const ControlChangeData& CC(uint8_t index) const;

    // Per-parameter mutation (marks dirty automatically, no flash write)
    void SetGlobalParam(uint8_t fieldId, uint8_t value);
    void SetBankParam(uint8_t bank, uint8_t fieldId, uint8_t value);
    void SetCCParam(uint8_t bank, uint8_t ccIndex, uint8_t channel, uint8_t id);

    // Full config serialization for SysEx dump/load
    size_t SerializeToBuffer(char* buffer, size_t maxSize);
    bool DeserializeFromBuffer(const char* buffer, size_t length);

    // Migration (called during Init)
    bool MigrateIfNeeded();

private:
    ConfigurationData global_;
    KeyModeData banks_[BANK_AMT];
    ControlChangeData cc_[BANK_AMT];

    bool dirty_ = false;
    unsigned long lastChangeTime_ = 0;
    static constexpr unsigned long IDLE_FLUSH_MS = 2000;
    static constexpr const char* CONFIG_FILE = "/configuration_data.json";
    static constexpr uint8_t CURRENT_VERSION = 201;

    void LoadFromFlash();
    void SaveToFlash();
    bool MigrateV103ToV200(JsonDocument& doc);
    bool MigrateV200ToV201(JsonDocument& doc);
    void PopulateStructsFromDoc(const JsonDocument& doc);
    void PopulateDocFromStructs(JsonDocument& doc);
};
