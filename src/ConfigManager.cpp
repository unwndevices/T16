#include "ConfigManager.hpp"
#include <ArduinoJson.h>

#ifdef NATIVE_TEST
#include <cstdio>
#include "LittleFS.h"
#else
#include <LittleFS.h>
#endif

void ConfigManager::Init()
{
    // Try to load from LittleFS
    auto file = LittleFS.open("/configuration_data.json", "r");
    if (file)
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (!error && !doc.isNull())
        {
            // Check version for migration
            // v103: root-level "version" key
            // v200: nested "global.version" key
            uint8_t version = doc["version"] | 0;
            if (version == 0)
            {
                version = doc["global"]["version"] | 0;
            }

            if (version >= 100 && version < 200)
            {
                // v103 format: flat root globals
                global_.version = 200;
                global_.mode = doc["mode"] | (uint8_t)0;
                global_.sensitivity = doc["sensitivity"] | (uint8_t)1;
                global_.brightness = doc["brightness"] | (uint8_t)6;
                global_.midi_trs = doc["midi_trs"] | (uint8_t)0;
                global_.trs_type = doc["trs_type"] | (uint8_t)0;
                global_.passthrough = doc["passthrough"] | (uint8_t)0;
                global_.midi_ble = doc["midi_ble"] | (uint8_t)0;

                // Load custom scales
                JsonArray cs1 = doc["custom_scale1"];
                if (!cs1.isNull())
                {
                    for (int i = 0; i < 16; i++)
                        global_.custom_scale1[i] = cs1[i] | (int8_t)i;
                }

                JsonArray cs2 = doc["custom_scale2"];
                if (!cs2.isNull())
                {
                    for (int i = 0; i < 16; i++)
                        global_.custom_scale2[i] = cs2[i] | (int8_t)i;
                }

                // Load banks
                JsonArray banksArr = doc["banks"];
                if (!banksArr.isNull())
                {
                    for (uint8_t i = 0; i < BANK_AMT && i < banksArr.size(); i++)
                    {
                        JsonObject b = banksArr[i];
                        banks_[i].palette = b["pal"] | (uint8_t)0;
                        banks_[i].channel = b["ch"] | (uint8_t)1;
                        banks_[i].scale = b["scale"] | (uint8_t)0;
                        banks_[i].base_octave = b["oct"] | (uint8_t)2;
                        banks_[i].base_note = b["note"] | (uint8_t)0;
                        banks_[i].velocity_curve = b["vel"] | (uint8_t)1;
                        banks_[i].aftertouch_curve = b["at"] | (uint8_t)1;
                        banks_[i].flip_x = b["flip_x"] | (uint8_t)0;
                        banks_[i].flip_y = b["flip_y"] | (uint8_t)0;
                        banks_[i].koala_mode = b["koala_mode"] | (uint8_t)0;

                        JsonArray chs = b["chs"];
                        JsonArray ids = b["ids"];
                        for (int j = 0; j < CC_AMT; j++)
                        {
                            if (!chs.isNull()) ccs_[i].channel[j] = chs[j] | (uint8_t)1;
                            if (!ids.isNull()) ccs_[i].id[j] = ids[j] | (uint8_t)0;
                        }
                    }
                }

                // Save migrated config
                dirty_ = true;
                Save(true);
                return;
            }
            else if (version >= 200)
            {
                // v200 format: nested "global" object
                JsonObject g = doc["global"];
                if (!g.isNull())
                {
                    global_.version = g["version"] | (uint8_t)200;
                    global_.mode = g["mode"] | (uint8_t)0;
                    global_.sensitivity = g["sensitivity"] | (uint8_t)1;
                    global_.brightness = g["brightness"] | (uint8_t)6;
                    global_.midi_trs = g["midi_trs"] | (uint8_t)0;
                    global_.trs_type = g["trs_type"] | (uint8_t)0;
                    global_.passthrough = g["passthrough"] | (uint8_t)0;
                    global_.midi_ble = g["midi_ble"] | (uint8_t)0;

                    JsonArray cs1 = g["custom_scale1"];
                    if (!cs1.isNull())
                    {
                        for (int i = 0; i < 16; i++)
                            global_.custom_scale1[i] = cs1[i] | (int8_t)i;
                    }
                    JsonArray cs2 = g["custom_scale2"];
                    if (!cs2.isNull())
                    {
                        for (int i = 0; i < 16; i++)
                            global_.custom_scale2[i] = cs2[i] | (int8_t)i;
                    }
                }

                // Load banks
                JsonArray banksArr = doc["banks"];
                if (!banksArr.isNull())
                {
                    for (uint8_t i = 0; i < BANK_AMT && i < banksArr.size(); i++)
                    {
                        JsonObject b = banksArr[i];
                        banks_[i].palette = b["pal"] | (uint8_t)0;
                        banks_[i].channel = b["ch"] | (uint8_t)1;
                        banks_[i].scale = b["scale"] | (uint8_t)0;
                        banks_[i].base_octave = b["oct"] | (uint8_t)2;
                        banks_[i].base_note = b["note"] | (uint8_t)0;
                        banks_[i].velocity_curve = b["vel"] | (uint8_t)1;
                        banks_[i].aftertouch_curve = b["at"] | (uint8_t)1;
                        banks_[i].flip_x = b["flip_x"] | (uint8_t)0;
                        banks_[i].flip_y = b["flip_y"] | (uint8_t)0;
                        banks_[i].koala_mode = b["koala_mode"] | (uint8_t)0;

                        JsonArray chs = b["chs"];
                        JsonArray ids = b["ids"];
                        for (int j = 0; j < CC_AMT; j++)
                        {
                            if (!chs.isNull()) ccs_[i].channel[j] = chs[j] | (uint8_t)1;
                            if (!ids.isNull()) ccs_[i].id[j] = ids[j] | (uint8_t)0;
                        }
                    }
                }
                return;
            }
        }
        // If we get here, file was invalid
    }

    // No file or invalid: use defaults
    global_ = ConfigurationData();
    global_.version = 200;
    for (uint8_t i = 0; i < BANK_AMT; i++)
    {
        banks_[i] = KeyModeData();
        ccs_[i] = ControlChangeData();
    }
}

void ConfigManager::Save(bool force)
{
    if (!force && !dirty_) return;

    char buffer[2048];
    size_t len = SerializeToBuffer(buffer, sizeof(buffer));

    auto file = LittleFS.open("/configuration_data.json", "w", true);
    if (file)
    {
        file.write((const uint8_t*)buffer, len);
        file.close();
    }

    dirty_ = false;
    saved_ = true;
}

bool ConfigManager::IsDirty() const
{
    return dirty_;
}

void ConfigManager::MarkDirty()
{
    dirty_ = true;
    lastDirtyTime_ = millis();
}

void ConfigManager::CheckIdleFlush(unsigned long now)
{
    if (!dirty_) return;
    if (now - lastDirtyTime_ < IDLE_FLUSH_MS) return;
    Save(true);
}

ConfigurationData& ConfigManager::Global()
{
    return global_;
}

KeyModeData& ConfigManager::Bank(uint8_t index)
{
    if (index >= BANK_AMT) index = 0;
    return banks_[index];
}

ControlChangeData& ConfigManager::CC(uint8_t index)
{
    if (index >= BANK_AMT) index = 0;
    return ccs_[index];
}

void ConfigManager::SetGlobalParam(uint8_t fieldId, uint8_t value)
{
    switch (fieldId)
    {
        case SysEx::FIELD_GLOBAL_MODE:        global_.mode = value; break;
        case SysEx::FIELD_GLOBAL_SENSITIVITY:  global_.sensitivity = value; break;
        case SysEx::FIELD_GLOBAL_BRIGHTNESS:   global_.brightness = value; break;
        case SysEx::FIELD_GLOBAL_MIDI_TRS:     global_.midi_trs = value; break;
        case SysEx::FIELD_GLOBAL_TRS_TYPE:     global_.trs_type = value; break;
        case SysEx::FIELD_GLOBAL_PASSTHROUGH:  global_.passthrough = value; break;
        case SysEx::FIELD_GLOBAL_MIDI_BLE:     global_.midi_ble = value; break;
        default: return;
    }
    MarkDirty();
}

void ConfigManager::SetBankParam(uint8_t bank, uint8_t fieldId, uint8_t value)
{
    if (bank >= BANK_AMT) return;
    switch (fieldId)
    {
        case SysEx::FIELD_BANK_CHANNEL:    banks_[bank].channel = value; break;
        case SysEx::FIELD_BANK_SCALE:      banks_[bank].scale = value; break;
        case SysEx::FIELD_BANK_OCTAVE:      banks_[bank].base_octave = value; break;
        case SysEx::FIELD_BANK_NOTE:        banks_[bank].base_note = value; break;
        case SysEx::FIELD_BANK_VELOCITY:    banks_[bank].velocity_curve = value; break;
        case SysEx::FIELD_BANK_AFTERTOUCH:  banks_[bank].aftertouch_curve = value; break;
        case SysEx::FIELD_BANK_FLIP_X:      banks_[bank].flip_x = value; break;
        case SysEx::FIELD_BANK_FLIP_Y:      banks_[bank].flip_y = value; break;
        case SysEx::FIELD_BANK_KOALA_MODE:  banks_[bank].koala_mode = value; break;
        default: return;
    }
    MarkDirty();
}

void ConfigManager::SetCCParam(uint8_t bank, uint8_t ccIndex, uint8_t channel, uint8_t id)
{
    if (bank >= BANK_AMT || ccIndex >= CC_AMT) return;
    ccs_[bank].channel[ccIndex] = channel;
    ccs_[bank].id[ccIndex] = id;
    MarkDirty();
}

size_t ConfigManager::SerializeToBuffer(char* buffer, size_t maxSize)
{
    JsonDocument doc;

    // v200 format: nested "global" object
    JsonObject g = doc["global"].to<JsonObject>();
    g["version"] = global_.version;
    g["mode"] = global_.mode;
    g["sensitivity"] = global_.sensitivity;
    g["brightness"] = global_.brightness;
    g["midi_trs"] = global_.midi_trs;
    g["trs_type"] = global_.trs_type;
    g["passthrough"] = global_.passthrough;
    g["midi_ble"] = global_.midi_ble;

    JsonArray cs1 = g["custom_scale1"].to<JsonArray>();
    for (int i = 0; i < 16; i++) cs1.add(global_.custom_scale1[i]);

    JsonArray cs2 = g["custom_scale2"].to<JsonArray>();
    for (int i = 0; i < 16; i++) cs2.add(global_.custom_scale2[i]);

    // Banks
    JsonArray banksArr = doc["banks"].to<JsonArray>();
    for (uint8_t i = 0; i < BANK_AMT; i++)
    {
        JsonObject b = banksArr.add<JsonObject>();
        b["pal"] = banks_[i].palette;
        b["ch"] = banks_[i].channel;
        b["scale"] = banks_[i].scale;
        b["oct"] = banks_[i].base_octave;
        b["note"] = banks_[i].base_note;
        b["vel"] = banks_[i].velocity_curve;
        b["at"] = banks_[i].aftertouch_curve;
        b["flip_x"] = banks_[i].flip_x;
        b["flip_y"] = banks_[i].flip_y;
        b["koala_mode"] = banks_[i].koala_mode;

        JsonArray chs = b["chs"].to<JsonArray>();
        JsonArray ids = b["ids"].to<JsonArray>();
        for (int j = 0; j < CC_AMT; j++)
        {
            chs.add(ccs_[i].channel[j]);
            ids.add(ccs_[i].id[j]);
        }
    }

    return serializeJson(doc, buffer, maxSize);
}

bool ConfigManager::DeserializeFromBuffer(const char* buffer, size_t length)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buffer, length);
    if (error) return false;

    // Check for v200 format
    JsonObject g = doc["global"];
    if (g.isNull()) return false;

    global_.version = g["version"] | (uint8_t)200;
    global_.mode = g["mode"] | (uint8_t)0;
    global_.sensitivity = g["sensitivity"] | (uint8_t)1;
    global_.brightness = g["brightness"] | (uint8_t)6;
    global_.midi_trs = g["midi_trs"] | (uint8_t)0;
    global_.trs_type = g["trs_type"] | (uint8_t)0;
    global_.passthrough = g["passthrough"] | (uint8_t)0;
    global_.midi_ble = g["midi_ble"] | (uint8_t)0;

    JsonArray cs1 = g["custom_scale1"];
    if (!cs1.isNull())
    {
        for (int i = 0; i < 16; i++)
            global_.custom_scale1[i] = cs1[i] | (int8_t)i;
    }
    JsonArray cs2 = g["custom_scale2"];
    if (!cs2.isNull())
    {
        for (int i = 0; i < 16; i++)
            global_.custom_scale2[i] = cs2[i] | (int8_t)i;
    }

    JsonArray banksArr = doc["banks"];
    if (!banksArr.isNull())
    {
        for (uint8_t i = 0; i < BANK_AMT && i < banksArr.size(); i++)
        {
            JsonObject b = banksArr[i];
            banks_[i].palette = b["pal"] | (uint8_t)0;
            banks_[i].channel = b["ch"] | (uint8_t)1;
            banks_[i].scale = b["scale"] | (uint8_t)0;
            banks_[i].base_octave = b["oct"] | (uint8_t)2;
            banks_[i].base_note = b["note"] | (uint8_t)0;
            banks_[i].velocity_curve = b["vel"] | (uint8_t)1;
            banks_[i].aftertouch_curve = b["at"] | (uint8_t)1;
            banks_[i].flip_x = b["flip_x"] | (uint8_t)0;
            banks_[i].flip_y = b["flip_y"] | (uint8_t)0;
            banks_[i].koala_mode = b["koala_mode"] | (uint8_t)0;

            JsonArray chs = b["chs"];
            JsonArray ids = b["ids"];
            for (int j = 0; j < CC_AMT; j++)
            {
                if (!chs.isNull()) ccs_[i].channel[j] = chs[j] | (uint8_t)1;
                if (!ids.isNull()) ccs_[i].id[j] = ids[j] | (uint8_t)0;
            }
        }
    }

    MarkDirty();
    return true;
}

bool ConfigManager::MigrateIfNeeded()
{
    auto file = LittleFS.open("/configuration_data.json", "r");
    if (!file) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) return false;

    uint8_t version = doc["version"] | 0;
    if (version >= 100 && version < 200)
    {
        // Re-init triggers migration
        Init();
        return true;
    }
    return false;
}
