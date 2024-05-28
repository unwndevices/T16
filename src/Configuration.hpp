#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Libs/DataManager.hpp"

#define configTICK_RATE_HZ 4000
const uint8_t CC_AMT = 8;
const uint8_t BANK_AMT = 4;

struct CalibrationData
{
    uint16_t minVal[16] = {0};
    uint16_t maxVal[16] = {0};
};

struct KeyModeData
{
    uint8_t palette = 0;
    uint8_t channel = 1;
    uint8_t scale = 0;
    uint8_t base_octave = 0;
    uint8_t base_note = 24;
    uint8_t velocity_curve = 1;
    uint8_t aftertouch_curve = 1;
    uint8_t flip_x = 0;
    uint8_t flip_y = 0;
    bool hasChanged = false;
};

struct ControlChangeData
{
    uint8_t channel[CC_AMT];
    uint8_t id[CC_AMT];
    bool hasChanged = false;
};

struct ConfigurationData
{
    uint8_t version = 1;
    uint8_t mode = 0;
    uint8_t brightness = 6;
    uint8_t palette = 0;
    uint8_t midi_trs = 0;
    uint8_t trs_type = 0;
    uint8_t passthrough = 0;
    uint8_t midi_ble = 0;

    int8_t custom_scale1[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int8_t custom_scale2[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool hasChanged = false;
};

struct Parameters
{
    float slew = 0.0f;
    uint8_t bank = 0;
    uint8_t mod = 0;
    float bend = 0.0f;
    uint8_t current_chord = 0;
    bool isBending = false;
    bool midiLearn = false;
    bool quickSettings = false;
    uint8_t quickSettingsPage = 0;
};

struct SettingPair
{
    uint8_t value;
    uint8_t numOptions;
};

struct QuickSettingsData
{
    static const uint8_t NUM_SETTINGS = 12;

    SettingPair settings[NUM_SETTINGS] = {
        // Page 1
        {8, 8}, // brightness: 0-255 (256 options)
        {0, 2}, // midi_trs: 0-1 (2 options)
        {0, 2}, // trs_type: 0-1 (2 options)
        {0, 2}, // midi_ble: 0-1 (2 options)

        // Page 2
        {1, 12}, // channel: 1-16 (16 options)
        {0, 12}, // scale: 0-N (N+1 options, where N is the number of available scales)
        {0, 8},  // base_octave: 0-10 (11 options, assuming 11 octaves)
        {0, 12}, // base_note: 0-127 (128 options, MIDI note range)

        // Page 3
        {1, 4}, // velocity_curve: 0-M (M+1 options, where M is the number of velocity curves)
        {1, 4}, // aftertouch_curve: 0-P (P+1 options, where P is the number of aftertouch curves)
        {0, 2}, // flip_x: 0-1 (2 options)
        {0, 2}  // flip_y: 0-1 (2 options)
    };
};

extern CalibrationData calibration_data;
extern ConfigurationData cfg;
extern KeyModeData kb_cfg[BANK_AMT];
extern ControlChangeData cc_cfg[CC_AMT];
extern Parameters parameters;
extern QuickSettingsData qs;

void SaveConfiguration(DataManager &config, bool overwrite = false);
void LoadQuickSettings(uint8_t bank);
void SaveQuickSettings(uint8_t bank);
void LoadConfiguration(DataManager &config);

#endif // CONFIGURATION_HPP
