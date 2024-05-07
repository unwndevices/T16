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
    uint8_t velocity_curve = 0;
    uint8_t aftertouch_curve = 0;
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
    uint8_t brightness = 254;
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
};

extern CalibrationData calibration_data;
extern ConfigurationData cfg;
extern KeyModeData kb_cfg[BANK_AMT];
extern ControlChangeData cc_cfg[CC_AMT];
extern Parameters parameters;

void InitConfiguration(DataManager &config, bool overwrite = false);
void LoadConfiguration(DataManager &config);

#endif // CONFIGURATION_HPP
