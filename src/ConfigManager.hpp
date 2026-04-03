#pragma once

#include <cstdint>
#include <cstddef>
#include "Configuration.hpp"
#include "SysExProtocol.hpp"

#ifndef NATIVE_TEST
#include "Libs/DataManager.hpp"
#endif

class ConfigManager
{
public:
    void Init();
    void Save(bool force = false);
    bool IsDirty() const;
    void MarkDirty();
    void CheckIdleFlush(unsigned long now);

    ConfigurationData& Global();
    KeyModeData& Bank(uint8_t index);
    ControlChangeData& CC(uint8_t index);

    void SetGlobalParam(uint8_t fieldId, uint8_t value);
    void SetBankParam(uint8_t bank, uint8_t fieldId, uint8_t value);
    void SetCCParam(uint8_t bank, uint8_t ccIndex, uint8_t channel, uint8_t id);

    size_t SerializeToBuffer(char* buffer, size_t maxSize);
    bool DeserializeFromBuffer(const char* buffer, size_t length);
    bool MigrateIfNeeded();

private:
    static constexpr unsigned long IDLE_FLUSH_MS = 2000;

    ConfigurationData global_;
    KeyModeData banks_[BANK_AMT];
    ControlChangeData ccs_[BANK_AMT];

    bool dirty_ = false;
    unsigned long lastDirtyTime_ = 0;
    bool saved_ = false;
};
