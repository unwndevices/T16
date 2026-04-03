#include "ConfigManager.hpp"
#include <LittleFS.h>

void ConfigManager::Init()
{
    if (!LittleFS.begin())
    {
        log_d("Failed to mount LittleFS");
        return;
    }

    LoadFromFlash();
    MigrateIfNeeded();
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void ConfigManager::Save(bool force)
{
    if (force || dirty_)
    {
        SaveToFlash();
        dirty_ = false;
    }
}

bool ConfigManager::IsDirty() const
{
    return dirty_;
}

void ConfigManager::MarkDirty()
{
    dirty_ = true;
    lastChangeTime_ = millis();
}

void ConfigManager::CheckIdleFlush(unsigned long now)
{
    if (dirty_ && (now - lastChangeTime_ >= IDLE_FLUSH_MS))
    {
        Save(true);
        log_d("Config flushed to flash (idle)");
    }
}

// ---------------------------------------------------------------------------
// Direct struct access
// ---------------------------------------------------------------------------

ConfigurationData& ConfigManager::Global()
{
    return global_;
}

const ConfigurationData& ConfigManager::Global() const
{
    return global_;
}

KeyModeData& ConfigManager::Bank(uint8_t index)
{
    return banks_[index];
}

const KeyModeData& ConfigManager::Bank(uint8_t index) const
{
    return banks_[index];
}

ControlChangeData& ConfigManager::CC(uint8_t index)
{
    return cc_[index];
}

const ControlChangeData& ConfigManager::CC(uint8_t index) const
{
    return cc_[index];
}

// ---------------------------------------------------------------------------
// Per-parameter mutation
// ---------------------------------------------------------------------------

void ConfigManager::SetGlobalParam(uint8_t fieldId, uint8_t value)
{
    switch (fieldId)
    {
        case 0: global_.mode = value; break;
        case 1: global_.sensitivity = value; break;
        case 2: global_.brightness = value; break;
        case 3: global_.midi_trs = value; break;
        case 4: global_.trs_type = value; break;
        case 5: global_.passthrough = value; break;
        case 6: global_.midi_ble = value; break;
        default:
            log_d("Unknown global field ID: %d", fieldId);
            return;
    }
    MarkDirty();
}

void ConfigManager::SetBankParam(uint8_t bank, uint8_t fieldId, uint8_t value)
{
    if (bank >= BANK_AMT)
    {
        log_d("Bank index out of range: %d", bank);
        return;
    }

    switch (fieldId)
    {
        case 0: banks_[bank].channel = value; break;
        case 1: banks_[bank].scale = value; break;
        case 2: banks_[bank].base_octave = value; break;
        case 3: banks_[bank].base_note = value; break;
        case 4: banks_[bank].velocity_curve = value; break;
        case 5: banks_[bank].aftertouch_curve = value; break;
        case 6: banks_[bank].flip_x = value; break;
        case 7: banks_[bank].flip_y = value; break;
        case 8: banks_[bank].koala_mode = value; break;
        default:
            log_d("Unknown bank field ID: %d", fieldId);
            return;
    }
    MarkDirty();
}

void ConfigManager::SetCCParam(uint8_t bank, uint8_t ccIndex, uint8_t channel, uint8_t id)
{
    if (bank >= BANK_AMT)
    {
        log_d("Bank index out of range: %d", bank);
        return;
    }
    if (ccIndex >= CC_AMT)
    {
        log_d("CC index out of range: %d", ccIndex);
        return;
    }

    cc_[bank].channel[ccIndex] = channel;
    cc_[bank].id[ccIndex] = id;
    MarkDirty();
}

// ---------------------------------------------------------------------------
// Serialization for SysEx dump/load
// ---------------------------------------------------------------------------

size_t ConfigManager::SerializeToBuffer(char* buffer, size_t maxSize)
{
    JsonDocument doc;
    PopulateDocFromStructs(doc);
    return serializeJson(doc, buffer, maxSize);
}

bool ConfigManager::DeserializeFromBuffer(const char* buffer, size_t length)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buffer, length);
    if (error)
    {
        log_d("DeserializeFromBuffer failed: %s", error.c_str());
        return false;
    }

    PopulateStructsFromDoc(doc);
    MarkDirty();
    return true;
}

// ---------------------------------------------------------------------------
// Migration
// ---------------------------------------------------------------------------

bool ConfigManager::MigrateIfNeeded()
{
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file)
    {
        // No config file -- save defaults
        log_d("No config file found, saving defaults");
        global_.version = CURRENT_VERSION;
        SaveToFlash();
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        log_d("Config parse error during migration check: %s", error.c_str());
        global_.version = CURRENT_VERSION;
        SaveToFlash();
        return false;
    }

    uint8_t version = doc["version"] | 0;

    if (version == CURRENT_VERSION)
    {
        // Already v200, no migration needed
        return false;
    }

    if (version >= 100 && version < 200)
    {
        // v1xx format (102, 103, etc.) -- migrate to v200
        MigrateV103ToV200(doc);

        // Save migrated doc directly to flash
        File outFile = LittleFS.open(CONFIG_FILE, "w");
        if (outFile)
        {
            serializeJson(doc, outFile);
            outFile.close();
        }

        // Reload structs from migrated doc
        PopulateStructsFromDoc(doc);
        log_d("Migration complete");
        return true;
    }

    // Unknown version -- treat as fresh install
    log_d("Unknown config version %d, saving defaults", version);
    global_.version = CURRENT_VERSION;
    SaveToFlash();
    return false;
}

bool ConfigManager::MigrateV103ToV200(JsonDocument& doc)
{
    // v103 format: global fields flat at root level
    // v200 format: global fields nested under "global" key

    JsonObject global = doc["global"].to<JsonObject>();

    // Move scalar global fields from root to global object
    global["mode"] = doc["mode"] | 0;
    global["sensitivity"] = doc["sensitivity"] | 1;
    global["brightness"] = doc["brightness"] | 6;
    global["midi_trs"] = doc["midi_trs"] | 0;
    global["trs_type"] = doc["trs_type"] | 0;
    global["passthrough"] = doc["passthrough"] | 0;
    global["midi_ble"] = doc["midi_ble"] | 0;

    // Move custom scale arrays
    if (doc["custom_scale1"].is<JsonArray>())
    {
        global["custom_scale1"] = doc["custom_scale1"];
    }
    else
    {
        JsonArray cs1 = global["custom_scale1"].to<JsonArray>();
        for (int i = 0; i < 16; i++) cs1.add(i);
    }

    if (doc["custom_scale2"].is<JsonArray>())
    {
        global["custom_scale2"] = doc["custom_scale2"];
    }
    else
    {
        JsonArray cs2 = global["custom_scale2"].to<JsonArray>();
        for (int i = 0; i < 16; i++) cs2.add(i);
    }

    // Remove old root-level keys
    doc.remove("mode");
    doc.remove("sensitivity");
    doc.remove("brightness");
    doc.remove("midi_trs");
    doc.remove("trs_type");
    doc.remove("passthrough");
    doc.remove("midi_ble");
    doc.remove("custom_scale1");
    doc.remove("custom_scale2");

    // Update version
    doc["version"] = CURRENT_VERSION;

    // Banks array stays as-is (same structure in both versions)

    log_d("Migrated config from v103 to v200");
    return true;
}

// ---------------------------------------------------------------------------
// Internal: Flash I/O
// ---------------------------------------------------------------------------

void ConfigManager::LoadFromFlash()
{
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file)
    {
        log_d("No config file found, using defaults");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        log_d("Config parse error: %s", error.c_str());
        return;
    }

    PopulateStructsFromDoc(doc);
}

void ConfigManager::SaveToFlash()
{
    JsonDocument doc;
    PopulateDocFromStructs(doc);

    File file = LittleFS.open(CONFIG_FILE, "w", true);
    if (!file)
    {
        log_d("Failed to open config file for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
    log_d("Config saved to flash");
}

// ---------------------------------------------------------------------------
// Internal: Struct <-> JSON mapping
// ---------------------------------------------------------------------------

void ConfigManager::PopulateStructsFromDoc(const JsonDocument& doc)
{
    uint8_t version = doc["version"] | 0;
    global_.version = version;

    // v200 format: global fields under "global" key
    JsonObjectConst globalObj = doc["global"].as<JsonObjectConst>();
    if (!globalObj.isNull())
    {
        global_.mode = globalObj["mode"] | 0;
        global_.sensitivity = globalObj["sensitivity"] | 1;
        global_.brightness = globalObj["brightness"] | 6;
        global_.midi_trs = globalObj["midi_trs"] | 0;
        global_.trs_type = globalObj["trs_type"] | 0;
        global_.passthrough = globalObj["passthrough"] | 0;
        global_.midi_ble = globalObj["midi_ble"] | 0;

        for (int i = 0; i < 16; i++)
        {
            global_.custom_scale1[i] = globalObj["custom_scale1"][i] | (int8_t)i;
            global_.custom_scale2[i] = globalObj["custom_scale2"][i] | (int8_t)i;
        }
    }
    else
    {
        // Fallback: v103 flat format (pre-migration read)
        global_.mode = doc["mode"] | 0;
        global_.sensitivity = doc["sensitivity"] | 1;
        global_.brightness = doc["brightness"] | 6;
        global_.midi_trs = doc["midi_trs"] | 0;
        global_.trs_type = doc["trs_type"] | 0;
        global_.passthrough = doc["passthrough"] | 0;
        global_.midi_ble = doc["midi_ble"] | 0;

        for (int i = 0; i < 16; i++)
        {
            global_.custom_scale1[i] = doc["custom_scale1"][i] | (int8_t)i;
            global_.custom_scale2[i] = doc["custom_scale2"][i] | (int8_t)i;
        }
    }

    // Banks (same structure in v103 and v200)
    JsonArrayConst banksArray = doc["banks"].as<JsonArrayConst>();
    if (!banksArray.isNull())
    {
        for (uint8_t i = 0; i < BANK_AMT && i < banksArray.size(); i++)
        {
            JsonObjectConst bankObj = banksArray[i].as<JsonObjectConst>();

            banks_[i].palette = bankObj["pal"] | (uint8_t)i;
            banks_[i].channel = bankObj["ch"] | (uint8_t)1;
            banks_[i].scale = bankObj["scale"] | (uint8_t)0;
            banks_[i].base_octave = bankObj["oct"] | (uint8_t)2;
            banks_[i].base_note = bankObj["note"] | (uint8_t)0;
            banks_[i].velocity_curve = bankObj["vel"] | (uint8_t)1;
            banks_[i].aftertouch_curve = bankObj["at"] | (uint8_t)1;
            banks_[i].flip_x = bankObj["flip_x"] | (uint8_t)0;
            banks_[i].flip_y = bankObj["flip_y"] | (uint8_t)0;
            banks_[i].koala_mode = bankObj["koala_mode"] | (uint8_t)0;

            // CC data
            JsonArrayConst chsArray = bankObj["chs"].as<JsonArrayConst>();
            JsonArrayConst idsArray = bankObj["ids"].as<JsonArrayConst>();
            for (uint8_t j = 0; j < CC_AMT; j++)
            {
                if (!chsArray.isNull() && j < chsArray.size())
                    cc_[i].channel[j] = chsArray[j] | (uint8_t)1;
                if (!idsArray.isNull() && j < idsArray.size())
                    cc_[i].id[j] = idsArray[j] | (uint8_t)0;
            }
        }
    }
}

void ConfigManager::PopulateDocFromStructs(JsonDocument& doc)
{
    doc["version"] = CURRENT_VERSION;

    // Global fields under "global" key (v200 format)
    JsonObject globalObj = doc["global"].to<JsonObject>();
    globalObj["mode"] = global_.mode;
    globalObj["sensitivity"] = global_.sensitivity;
    globalObj["brightness"] = global_.brightness;
    globalObj["midi_trs"] = global_.midi_trs;
    globalObj["trs_type"] = global_.trs_type;
    globalObj["passthrough"] = global_.passthrough;
    globalObj["midi_ble"] = global_.midi_ble;

    JsonArray cs1 = globalObj["custom_scale1"].to<JsonArray>();
    JsonArray cs2 = globalObj["custom_scale2"].to<JsonArray>();
    for (int i = 0; i < 16; i++)
    {
        cs1.add(global_.custom_scale1[i]);
        cs2.add(global_.custom_scale2[i]);
    }

    // Banks array
    JsonArray banksArray = doc["banks"].to<JsonArray>();
    for (uint8_t i = 0; i < BANK_AMT; i++)
    {
        JsonObject bankObj = banksArray.add<JsonObject>();
        bankObj["pal"] = banks_[i].palette;
        bankObj["ch"] = banks_[i].channel;
        bankObj["scale"] = banks_[i].scale;
        bankObj["oct"] = banks_[i].base_octave;
        bankObj["note"] = banks_[i].base_note;
        bankObj["vel"] = banks_[i].velocity_curve;
        bankObj["at"] = banks_[i].aftertouch_curve;
        bankObj["flip_x"] = banks_[i].flip_x;
        bankObj["flip_y"] = banks_[i].flip_y;
        bankObj["koala_mode"] = banks_[i].koala_mode;

        JsonArray chsArray = bankObj["chs"].to<JsonArray>();
        JsonArray idsArray = bankObj["ids"].to<JsonArray>();
        for (uint8_t j = 0; j < CC_AMT; j++)
        {
            chsArray.add(cc_[i].channel[j]);
            idsArray.add(cc_[i].id[j]);
        }
    }
}
