#include "pinout.h"
#include <Arduino.h>

#define configTICK_RATE_HZ 4000

// GLOBAL DEFINES
#define CC_AMT 8
#define BANK_AMT 4

#include "Libs/MidiProvider.hpp"
MidiProvider midi_provider;

#include "Libs/DataManager.hpp"
DataManager calibration("/calibration_data.json");
DataManager config("/configuration_data.json");

#include <FastLED.h>

DEFINE_GRADIENT_PALETTE(unwn_gp){0, 176, 65, 251, 127, 230, 2, 2, 255, 255, 111, 156};
DEFINE_GRADIENT_PALETTE(topo_gp){0, 0, 107, 189, 128, 255, 147, 0, 255, 0, 255, 121};
DEFINE_GRADIENT_PALETTE(alt_gp){0, 189, 70, 70, 255, 78, 75, 232};
DEFINE_GRADIENT_PALETTE(acid_gp){0, 126, 255, 36, 255, 130, 0, 255};
CRGBPalette16 palette[] = {unwn_gp, topo_gp, alt_gp, acid_gp};

#include "Libs/Leds/LedManager.hpp"
LedManager led_manager;

TouchBlur touch_blur;
NoBlur no_blur;
Strips strips;

#include "Libs/Adc.hpp"
Adc adc;

#include "Libs/Keyboard.hpp"
Key keys[] = {6, 7, 15, 11, 5, 2, 14, 9, 4, 1, 13, 8, 3, 0, 12, 10};
Keyboard keyboard;

#include "Libs/TouchSlider.hpp"
uint8_t slider_sensor[] = {PIN_T1, PIN_T2, PIN_T3, PIN_T4, PIN_T5, PIN_T6, PIN_T7};
TouchSlider slider;

uint8_t marker = 0;

#include "Libs/Button.hpp"
Button t_btn(PIN_EXT1);
Button m_btn(PIN_EXT5);

#include "Scales.hpp"

enum SliderMode
{
    BEND,
    OCTAVE,
    MOD,
    BANK,
    SLEW,
    SLIDER_MODE_AMOUNT
};

/////////////////////////
// Data

SliderMode slider_mode = SliderMode::BEND;

struct CalibrationData
{
    uint16_t minVal[16] = {0};
    uint16_t maxVal[16] = {0};
} calibration_data;

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
    uint8_t mode = Mode::KEYBOARD;
    uint8_t brightness = 254;
    uint8_t palette = 0;
    uint8_t midi_trs = 0;
    uint8_t trs_type = 0;
    uint8_t passthrough = 0;

    int8_t custom_scale1[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int8_t custom_scale2[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool hasChanged = false;

} cfg;

KeyModeData kb_cfg[BANK_AMT];
ControlChangeData cc_cfg[CC_AMT];
struct Parameters
{
    float slew = 0.0f;
    uint8_t bank = 0;
    uint8_t mod = 0;
    float bend = 0.0f;
    bool isBending = false;
    bool midiLearn = false;
} parameters;

void InitConfiguration()
{
    config.SaveVar(cfg.version, "version");
    config.SaveVar(cfg.mode, "mode");
    config.SaveVar(cfg.brightness, "brightness");
    config.SaveVar(cfg.midi_trs, "midi_trs");
    config.SaveVar(cfg.trs_type, "trs_type");
    config.SaveVar(cfg.passthrough, "passthrough");

    JsonDocument doc;
    JsonArray banksArray = doc["banks"].to<JsonArray>(); // Initialize configurations for each bank
    for (uint8_t bank = 0; bank < 4; ++bank)
    {
        JsonObject bankObject = banksArray.add<JsonObject>(); // Create a bank object
        bankObject["palette"] = bank;
        bankObject["channel"] = 1;
        bankObject["scale"] = 0;
        bankObject["octave"] = 0;
        bankObject["note"] = 24;
        bankObject["velocity"] = 0;
        bankObject["aftertouch"] = 0;
        bankObject["flip_x"] = 0;
        bankObject["flip_y"] = 0;

        config.SaveArray(cfg.custom_scale1, "custom_scale1", 16);
        config.SaveArray(cfg.custom_scale2, "custom_scale2", 16);

        JsonArray channelArray = bankObject["channels"].to<JsonArray>();
        JsonArray idArray = bankObject["ids"].to<JsonArray>();
        for (int i = 0; i < CC_AMT; i++)
        {
            channelArray[i] = 1;
            idArray[i] = i + 13;
        }
    }

    config.SaveBanksArray(banksArray);
}

void SetMarkerCallback(uint8_t index, bool isRootNote)
{
    led_manager.SetMarker(index, isRootNote);
}

void OnBankChange()
{
    led_manager.SetPalette(palette[kb_cfg[parameters.bank].palette]);
    uint8_t base_note = kb_cfg[parameters.bank].base_note + (kb_cfg[parameters.bank].base_octave * 12);
    SetNoteMap(kb_cfg[parameters.bank].scale, base_note, kb_cfg[parameters.bank].flip_x, kb_cfg[parameters.bank].flip_y, SetMarkerCallback);
    keyboard.SetBank(parameters.bank);
    keyboard.SetVelocityLut((Keyboard::Lut)kb_cfg[parameters.bank].velocity_curve);
    keyboard.SetAftertouchLut((Keyboard::Lut)kb_cfg[parameters.bank].aftertouch_curve);
    led_manager.UpdateTransition();
}

void ProcessKey(int idx, Key::State state)
{
    uint8_t note = note_map[idx] + (kb_cfg[parameters.bank].base_octave * 12);

    if (state == Key::State::PRESSED)
    {
        uint8_t velocity = keyboard.GetVelocity(idx);
        midi_provider.SendNoteOn(note, velocity, kb_cfg[parameters.bank].channel);
        led_manager.SetPosition((uint8_t)(idx % 4), (uint8_t)(idx / 4));
        led_manager.SetColor(255 - velocity * 2);
        led_manager.SetSpeed((127 - velocity) / 2);
    }
    else if (state == Key::State::RELEASED)
    {
        midi_provider.SendNoteOff(note, 0, kb_cfg[parameters.bank].channel);
    }
    else if (state == Key::State::AFTERTOUCH)
    {
        uint8_t pressure = keyboard.GetAftertouch(idx);
        midi_provider.SendAfterTouch(note, (midi::DataByte)pressure, kb_cfg[parameters.bank].channel);
        log_d("Key %d, Aftertouch: %d", idx, pressure);
    }
}

void ProcessSlider()
{
    float value = 0.0f;
    switch (slider_mode)
    {
    case SliderMode::BEND:
        if (!slider.IsTouched())
        {
            led_manager.SetSlider(0.5f, false);
            if (parameters.isBending)
            {
                parameters.bend = 0;
                midi_provider.SendPitchBend(parameters.bend, kb_cfg[parameters.bank].channel);
                parameters.isBending = false;
            }
        }
        else if (slider.IsTouched())
        {
            // convert the float value to a 14 bit integer, with 0 being the float value 0.5
            value = (slider.GetPosition() - 0.5f) * 2.0f;
            parameters.bend = (value * 8192.0f);
            midi_provider.SendPitchBend((int)parameters.bend, kb_cfg[parameters.bank].channel);
            led_manager.SetSlider(slider.GetPosition(), false);
            parameters.isBending = true;
        }
        break;
    case SliderMode::OCTAVE:
        kb_cfg[parameters.bank].base_octave = (uint8_t)(slider.GetPosition() * 7);
        led_manager.SetSlider(slider.GetPosition(), false);
        break;
    case SliderMode::MOD:
        parameters.mod = slider.GetPosition();
        midi_provider.SendControlChange(1, (uint8_t)(parameters.mod * 127.0f), kb_cfg[parameters.bank].channel);
        led_manager.SetSlider(slider.GetPosition());
        break;
    case SliderMode::SLEW:
        parameters.slew = slider.GetPosition();
        led_manager.SetSlider(parameters.slew);
        keyboard.SetSlew(parameters.slew);
        break;
    case SliderMode::BANK:
        uint8_t bank = slider.GetQuantizedPosition(4);
        if (parameters.bank != bank)
        {
            parameters.bank = bank;
            OnBankChange();
        }
        led_manager.SetSlider((float)parameters.bank * 0.334f, false);
        break;
    }
}

void ProcessSliderButton()
{
    switch (slider_mode)
    {
    case SliderMode::BEND:
        log_d("Slider mode: Bend");
        led_manager.SetSliderHue(HSVHue::HUE_PINK + 10);
        break;
    case SliderMode::OCTAVE:
        log_d("Slider mode: Octave");
        slider.SetPosition((float)kb_cfg[parameters.bank].base_octave / 7.0f);
        led_manager.SetSliderHue(HSVHue::HUE_BLUE - 10);
        break;
    case SliderMode::MOD:
        log_d("Slider mode: Mod");
        slider.SetPosition(parameters.mod);
        led_manager.SetSliderHue(HSVHue::HUE_GREEN + 15);
        break;
    case SliderMode::BANK:
        log_d("Slider mode: Bank");
        slider.SetPosition((float)parameters.bank / 3.0f);
        led_manager.SetSliderHue(HSVHue::HUE_PURPLE);
        break;
    case SliderMode::SLEW:
        log_d("Slider mode: Slew");
        slider.SetPosition(parameters.slew);
        led_manager.SetSliderHue(HSVHue::HUE_ORANGE);
        break;
    }
}

void ProcessModeButton()
{
    switch (cfg.mode)
    {
    case Mode::KEYBOARD:
        log_d("Mode: Keyboard");
        keyboard.SetOnStateChanged(&ProcessKey);
        keyboard.SetMode(Mode::KEYBOARD);
        led_manager.TransitionToPattern(&no_blur);
        slider_mode = SliderMode::BEND;
        ProcessSliderButton();
        break;
    case Mode::XY_PAD:
        log_d("Mode: XY");
        keyboard.RemoveOnStateChanged();
        keyboard.SetMode(Mode::XY_PAD);
        led_manager.TransitionToPattern(&touch_blur);
        slider_mode = SliderMode::SLEW;
        ProcessSliderButton();
        break;
    case Mode::STRIPS:
        log_d("Mode: Strips");
        keyboard.RemoveOnStateChanged();
        keyboard.SetMode(Mode::STRIPS);
        led_manager.TransitionToPattern(&strips);
        slider_mode = SliderMode::SLEW;
        ProcessSliderButton();
        break;
    }
}

void ProcessButton(int idx, Button::State state)
{
    if (state == Button::State::CLICKED)
    {
        if (idx == PIN_EXT1)
        {
            log_d("Touch button clicked");
            if (cfg.mode != Mode::XY_PAD && cfg.mode != Mode::STRIPS)
            {
                slider_mode = (SliderMode)((slider_mode + 1) % (SLIDER_MODE_AMOUNT - 1));
            }
            else
            {
                slider_mode = (SliderMode)((slider_mode + 1) % SLIDER_MODE_AMOUNT);
                if (slider_mode < SliderMode::MOD)
                {
                    slider_mode = SliderMode::MOD;
                }
            }
            ProcessSliderButton();
        }
        if (idx == PIN_EXT5)
        {
            log_d("Mode button clicked");
            cfg.mode = (Mode)((cfg.mode + 1) % MODE_AMOUNT);
            ProcessModeButton();
        }
    }

    if (state == Button::State::LONG_PRESSED)
    {
        if (idx == PIN_EXT1)
        {
            log_d("Touch button long pressed");
            log_d("Midi Learn mode");
            parameters.midiLearn = true;
        }
        if (idx == PIN_EXT5)
        {
            log_d("Mode button long pressed");
            log_d("MIDI panic!");
            for (uint8_t i = 0; i < 127; i++)
            {
                midi_provider.SendNoteOff(i, 0, kb_cfg[parameters.bank].channel);
            }
        }
    }

    if (state == Button::State::LONG_RELEASED)
    {
        if (idx == PIN_EXT1)
        {
            log_d("Touch button long released");
            parameters.midiLearn = false;
        }
    }
}

void LoadConfiguration()
{
    log_d("Configuration found");
    log_d("Loading configuration");
    config.LoadVar(cfg.mode, "mode");
    config.LoadVar(cfg.brightness, "brightness");
    config.LoadVar(cfg.midi_trs, "midi_trs");
    config.LoadVar(cfg.trs_type, "trs_type");
    config.LoadVar(cfg.passthrough, "passthrough");

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
            kb_cfg[i].palette = bankObject["palette"];
            kb_cfg[i].channel = bankObject["channel"];
            kb_cfg[i].scale = bankObject["scale"];
            kb_cfg[i].base_octave = bankObject["octave"];
            kb_cfg[i].base_note = bankObject["note"];
            kb_cfg[i].velocity_curve = bankObject["velocity"];
            kb_cfg[i].aftertouch_curve = bankObject["aftertouch"];
            kb_cfg[i].flip_x = bankObject["flip_x"];
            kb_cfg[i].flip_y = bankObject["flip_y"];

            JsonArray channelsArray = bankObject["channels"].as<JsonArray>(); // Convert to JsonArray
            JsonArray idArray = bankObject["ids"].as<JsonArray>();            // Convert to JsonArray
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

void SetCustomScale(int8_t *scale, int8_t *custom_scale, int size)
{
    for (int i = 0; i < size; i++)
    {
        scale[i] = custom_scale[i];
    }
}

// calls functions to apply the configuration
void ApplyConfiguration()
{
    led_manager.SetBrightness(cfg.brightness);

    SetCustomScale(scales[CUSTOM1], cfg.custom_scale1, 16);
    SetCustomScale(scales[CUSTOM2], cfg.custom_scale2, 16);

    log_d("current bank: %d", parameters.bank);

    uint8_t base_note = kb_cfg[parameters.bank].base_note + (kb_cfg[parameters.bank].base_octave * 12);
    SetNoteMap(kb_cfg[parameters.bank].scale, base_note, kb_cfg[parameters.bank].flip_x, kb_cfg[parameters.bank].flip_y, SetMarkerCallback);
    keyboard.SetVelocityLut((Keyboard::Lut)kb_cfg[parameters.bank].velocity_curve);
    keyboard.SetAftertouchLut((Keyboard::Lut)kb_cfg[parameters.bank].aftertouch_curve);
    led_manager.UpdateTransition();
}

void ProcessSysEx(byte *data, unsigned length)
{
    log_d("SysEx received");
    // read the data and use a switch case to handle different types of SysEx messages
    log_d("%d %d %d %d %d", data[0], data[1], data[2], data[3], data[4]);
    if (data[2] == 127 && data[3] == 6 && data[4] == 1)
    {
        log_d("SysEx version request");
        byte message[] = {0x7e, 0x7f, 0x06, 0x02, cfg.version};
        midi_provider.SendSysEx(sizeof(message), message);
    }

    if (data[2] == 127 && data[3] == 7 && data[4] == 3)
    {
        log_d("SysEx configuration dump request");
        char buffer[1024];
        buffer[0] = 127;
        buffer[1] = 7;
        buffer[2] = 4;
        size_t size = config.SerializeToBuffer(buffer + 3, 1024);

        midi_provider.SendSysEx(size + 3, reinterpret_cast<byte *>(buffer));
    }

    if (data[2] == 127 && data[3] == 7 && data[4] == 5) // TODO proper message
    {
        log_d("SysEx configuration load request");
        config.DeserializeFromBuffer(reinterpret_cast<char *>(data + 5));
        LoadConfiguration();
        ApplyConfiguration();
    }
}

void setup()
{
    midi_provider.Init(PIN_RX, PIN_TX);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);

    midi_provider.SetHandleSystemExclusive(ProcessSysEx);
    // Button initialization
    t_btn.Init(PIN_EXT1);
    m_btn.Init(PIN_EXT5);
    t_btn.onStateChanged.Connect(&ProcessButton);
    m_btn.onStateChanged.Connect(&ProcessButton);

    led_manager.Init();
    ProcessModeButton();

    // slider initialization
    slider.Init(slider_sensor);

    // ADC initialization
    AdcChannelConfig adc_config;
    adc_config.InitMux(PIN_COM, PIN_S0, PIN_S1, PIN_S2, PIN_S3);
    adc.Init(&adc_config, 16);

    calibration.Init();
    if (!calibration.LoadArray(calibration_data.minVal, "minVal", 16))
    {
        log_d("Calibration data not found, starting calibration routine");
        adc.CalibrationRoutine();
        adc.GetCalibration(calibration_data.minVal, calibration_data.maxVal, 16);
        calibration.SaveArray(calibration_data.minVal, "minVal", 16);
        calibration.SaveArray(calibration_data.maxVal, "maxVal", 16);
        ESP.restart();
    }
    calibration.LoadArray(calibration_data.maxVal, "maxVal", 16);
    calibration.Print();

    t_btn.Update();
    config.Init();
    if (t_btn.GetRaw())
    {
        log_d("Resetting configuration");
        InitConfiguration();
    }
    if (!config.LoadVar(cfg.version, "version"))
    {
        log_d("first time configuration");
        InitConfiguration();
    }
    else
    {
        LoadConfiguration();
    }

    config.Print();

    parameters.bank = 0;

    uint8_t base_note = kb_cfg[parameters.bank].base_note + (kb_cfg[parameters.bank].base_octave * 12);
    SetNoteMap(kb_cfg[parameters.bank].scale, base_note, kb_cfg[parameters.bank].flip_x, kb_cfg[parameters.bank].flip_y, SetMarkerCallback);

    log_d("Configuration initialized");

    adc.SetCalibration(calibration_data.minVal, calibration_data.maxVal, 16);
    adc.Start();
    // keyboard initialization
    KeyboardConfig keyboard_config;

    keyboard_config.Init(keys, 16);
    keyboard.Init(&keyboard_config, &adc);
    keyboard.SetVelocityLut((Keyboard::Lut)kb_cfg[parameters.bank].velocity_curve);
    keyboard.SetAftertouchLut((Keyboard::Lut)kb_cfg[parameters.bank].aftertouch_curve);
    keyboard.SetOnStateChanged(&ProcessKey);
}

void loop()
{

    midi_provider.Read();

    t_btn.Update();
    m_btn.Update();
    keyboard.Update();
    slider.Update();

    fill_solid(matrixleds, 16, CRGB::Black);

    if (cfg.mode == Mode::XY_PAD)
    {
        Vec2 xy;
        xy.x = keyboard.GetX();
        xy.y = keyboard.GetY();

        if (keyboard.XChanged())
        {
            midi_provider.SendControlChange((int)cc_cfg[parameters.bank].id[0], (uint8_t)(xy.x * 0.33333f * 127.0f), cc_cfg[parameters.bank].channel[0]);
        }

        if (keyboard.YChanged())
        {
            midi_provider.SendControlChange((int)cc_cfg[parameters.bank].id[1], (uint8_t)(xy.y * 0.33333f * 127.0f), cc_cfg[parameters.bank].channel[1]);
        }
        led_manager.SetPosition(xy.x, xy.y);

        float pressure = keyboard.GetPressure();
        if (pressure > 0.00f)
        {

            if (!parameters.midiLearn)
            {
                midi_provider.SendControlChange(cc_cfg[parameters.bank].id[2], (uint8_t)(pressure * 127.0f), cc_cfg[parameters.bank].channel[2]);
            }
            led_manager.SetAmount(1.0f - pressure);
            led_manager.SetColor((uint8_t)(pressure * 255.0f));
            led_manager.SetState(true);
        }
    }

    else if (cfg.mode == Mode::KEYBOARD)
    {
        led_manager.DrawMarkers();
    }

    else if (cfg.mode == Mode::STRIPS)
    {
        for (int i = 0; i < 4; i++)
        {
            led_manager.SetStrip(i, keyboard.GetStrip(i));
            if (keyboard.StripChanged(i))
            {
                midi_provider.SendControlChange(cc_cfg[parameters.bank].id[i + 3], (uint8_t)(127.0f - keyboard.GetStrip(i) * 0.33333f * 127.0f), cc_cfg[parameters.bank].channel[i + 3]);
            }
        }
    }

    ProcessSlider();
    led_manager.RunPattern();
    FastLED.show();
}
