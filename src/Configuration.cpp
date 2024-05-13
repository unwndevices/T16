#include "Configuration.hpp"

CalibrationData calibration_data;
ConfigurationData cfg;
KeyModeData kb_cfg[BANK_AMT];
ControlChangeData cc_cfg[CC_AMT];
Parameters parameters;

void InitConfiguration(DataManager &config, bool overwrite)
{
    config.SaveVar(cfg.version, "version");
    config.SaveVar(cfg.mode, "mode");
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
        bankObject["ch"] = 1;
        bankObject["scale"] = 0;
        bankObject["oct"] = 2;
        bankObject["note"] = 0;
        bankObject["vel"] = 1;
        bankObject["at"] = 1;
        bankObject["flip_x"] = 0;
        bankObject["flip_y"] = 0;
        JsonArray channelArray = bankObject["chs"].to<JsonArray>();
        JsonArray idArray = bankObject["ids"].to<JsonArray>();
        for (int i = 0; i < CC_AMT; i++)
        {
            channelArray[i] = 1;
            idArray[i] = i + 13;
        }

        config.SaveArray(cfg.custom_scale1, "custom_scale1", 16);
        config.SaveArray(cfg.custom_scale2, "custom_scale2", 16);
    }

    config.SaveBanksArray(banksArray);
}

void LoadConfiguration(DataManager &config)
{
    log_d("Configuration found");
    log_d("Loading configuration");
    config.LoadVar(cfg.mode, "mode");
    config.LoadVar(cfg.brightness, "brightness");
    config.LoadVar(cfg.midi_trs, "midi_trs");
    config.LoadVar(cfg.trs_type, "trs_type");
    config.LoadVar(cfg.passthrough, "passthrough");
    config.LoadVar(cfg.midi_ble, "midi_ble");

    config.LoadArray(cfg.custom_scale1, "custom_scale1", 16);
    config.LoadArray(cfg.custom_scale2, "custom_scale2", 16);

    log_d("base cfg loaded");

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
            for (int j = 0; j < channelsArray.size(); j++)
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