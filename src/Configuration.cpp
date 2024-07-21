#include "Configuration.hpp"

CalibrationData calibration_data;
ConfigurationData cfg;
KeyModeData kb_cfg[BANK_AMT];
ControlChangeData cc_cfg[BANK_AMT];
Parameters parameters;
QuickSettingsData qs;

void SaveConfiguration(DataManager &config, bool overwrite)
{
    config.SaveVar(cfg.version, "version");
    config.SaveVar(cfg.mode, "mode");
    config.SaveVar(cfg.sensitivity, "sensitivity");
    config.SaveVar(cfg.brightness, "brightness");
    config.SaveVar(cfg.midi_trs, "midi_trs");
    config.SaveVar(cfg.trs_type, "trs_type");
    config.SaveVar(cfg.passthrough, "passthrough");
    config.SaveVar(cfg.midi_ble, "midi_ble");

    JsonDocument doc;
    JsonArray banksArray = doc["banks"].to<JsonArray>(); // Initialize configurations for each bank
    for (uint8_t bank = 0; bank < BANK_AMT; ++bank)
    {
        JsonObject bankObject = banksArray.add<JsonObject>(); // Create a bank object
        bankObject["pal"] = bank;
        bankObject["ch"] = kb_cfg[bank].channel;
        bankObject["scale"] = kb_cfg[bank].scale;
        bankObject["oct"] = kb_cfg[bank].base_octave;
        bankObject["note"] = kb_cfg[bank].base_note;
        bankObject["vel"] = kb_cfg[bank].velocity_curve;
        bankObject["at"] = kb_cfg[bank].aftertouch_curve;
        bankObject["flip_x"] = kb_cfg[bank].flip_x;
        bankObject["flip_y"] = kb_cfg[bank].flip_y;
        JsonArray channelArray = bankObject["chs"].to<JsonArray>();
        JsonArray idArray = bankObject["ids"].to<JsonArray>();
        for (int i = 0; i < CC_AMT; i++)
        {
            channelArray[i] = cc_cfg[bank].channel[i];
            idArray[i] = cc_cfg[bank].id[i];
        }

        config.SaveArray(cfg.custom_scale1, "custom_scale1", 16);
        config.SaveArray(cfg.custom_scale2, "custom_scale2", 16);
    }

    config.SaveBanksArray(banksArray);
}

void LoadQuickSettings(uint8_t bank)
{
    qs.settings[0].value = cfg.brightness;
    qs.settings[1].value = cfg.midi_trs;
    qs.settings[2].value = cfg.trs_type;
    qs.settings[3].value = cfg.midi_ble;
    qs.settings[4].value = kb_cfg[bank].channel;
    qs.settings[5].value = kb_cfg[bank].scale;
    qs.settings[6].value = kb_cfg[bank].base_octave;
    qs.settings[7].value = kb_cfg[bank].base_note;
    qs.settings[8].value = kb_cfg[bank].velocity_curve;
    qs.settings[9].value = kb_cfg[bank].aftertouch_curve;
    qs.settings[10].value = kb_cfg[bank].flip_x;
    qs.settings[11].value = kb_cfg[bank].flip_y;
}

void SaveQuickSettings(uint8_t bank)
{
    cfg.brightness = qs.settings[0].value;
    cfg.midi_trs = qs.settings[1].value;
    cfg.trs_type = qs.settings[2].value;
    cfg.midi_ble = qs.settings[3].value;
    kb_cfg[bank].channel = qs.settings[4].value;
    kb_cfg[bank].scale = qs.settings[5].value;
    kb_cfg[bank].base_octave = qs.settings[6].value;
    kb_cfg[bank].base_note = qs.settings[7].value;
    kb_cfg[bank].velocity_curve = qs.settings[8].value;
    kb_cfg[bank].aftertouch_curve = qs.settings[9].value;
    kb_cfg[bank].flip_x = qs.settings[10].value;
    kb_cfg[bank].flip_y = qs.settings[11].value;
}

void LoadConfiguration(DataManager &config, bool partial)
{
    log_d("Configuration found");
    log_d("Loading configuration");
    config.LoadVar(cfg.mode, "mode");
    config.LoadVar(cfg.sensitivity, "sensitivity");
    config.LoadVar(cfg.brightness, "brightness");
    config.LoadVar(cfg.midi_trs, "midi_trs");
    config.LoadVar(cfg.trs_type, "trs_type");
    config.LoadVar(cfg.passthrough, "passthrough");
    config.LoadVar(cfg.midi_ble, "midi_ble");

    config.LoadArray(cfg.custom_scale1, "custom_scale1", 16);
    config.LoadArray(cfg.custom_scale2, "custom_scale2", 16);

    JsonArray banksArray;
    JsonDocument doc = config.LoadJsonDocument();
    banksArray = doc["banks"].as<JsonArray>();
    {
        log_d("banks array loaded");
        for (int i = 0; i < BANK_AMT; ++i)
        {
            JsonObject bankObject = banksArray[i].as<JsonObject>(); // Convert to JsonObject
            kb_cfg[i].palette = bankObject["pal"];
            kb_cfg[i].channel = bankObject["ch"];
            kb_cfg[i].scale = bankObject["scale"];
            kb_cfg[i].base_octave = bankObject["oct"];
            kb_cfg[i].base_note = bankObject["note"];
            kb_cfg[i].velocity_curve = bankObject["vel"];
            kb_cfg[i].aftertouch_curve = bankObject["at"];
            kb_cfg[i].flip_x = bankObject["flip_x"];
            kb_cfg[i].flip_y = bankObject["flip_y"];

            JsonArray channelsArray = bankObject["chs"].as<JsonArray>(); // Convert to JsonArray
            JsonArray idArray = bankObject["ids"].as<JsonArray>();       // Convert to JsonArray
            for (int j = 0; j < CC_AMT; j++)
            {
                cc_cfg[i].channel[j] = channelsArray[j];
                cc_cfg[i].id[j] = idArray[j];
            }
        }
        for (int i = 0; i < 4; i++)
        {
            log_d("bank[%d] scale: %d", i, kb_cfg[i].scale);
        }
    }
}