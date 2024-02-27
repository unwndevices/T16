#include "pinout.h"
#include <Arduino.h>

#define configTICK_RATE_HZ 4000

// GLOBAL DEFINES
#define CV_AMT 8
#define BANK_AMT 4
#define CV_TOTAL (CV_AMT * BANK_AMT)

#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

#include "DataManager.hpp"
DataManager calibration("/calibration_data.json");
DataManager config("/configuration_data.json");

#include <FastLED.h>

DEFINE_GRADIENT_PALETTE(unwn_gp){0, 176, 65, 251, 127, 230, 2, 2, 255, 255, 111, 156};
DEFINE_GRADIENT_PALETTE(topo_gp){0, 255, 205, 71, 255, 34, 193, 195};
DEFINE_GRADIENT_PALETTE(alt_gp){0, 189, 70, 70, 255, 78, 75, 232};
DEFINE_GRADIENT_PALETTE(acid_gp){0, 255, 217, 105, 255, 170, 255, 110};

#include "led/Led.hpp"
LedManager led_manager;

#include "adc.hpp"
Adc adc;

#include "Keyboard.hpp"
Key keys[] = {6, 7, 15, 11, 5, 2, 14, 9, 4, 1, 13, 8, 3, 0, 12, 10};
Keyboard keyboard;

#include "TouchSlider.hpp"
uint8_t slider_sensor[] = {PIN_T1, PIN_T2, PIN_T3, PIN_T4, PIN_T5, PIN_T6, PIN_T7};
TouchSlider slider;

uint8_t marker = 0;

#include "Button.hpp"
Button t_btn(PIN_EXT1);
Button m_btn(PIN_EXT5);

enum Mode
{
    KEYBOARD,
    XY_PAD,
    STRIPS,
    MODE_AMOUNT
};

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

struct ConfigurationData
{
    uint8_t version = 1;
    uint8_t mode = Mode::KEYBOARD;
    uint8_t brightness = 254;
    uint8_t palette = 0;
    uint8_t channel = 1;
} cfg;

struct KeyModeData
{
    uint8_t base_octave = 0;
    uint8_t base_note = 24;
    uint8_t velocity_curve = 0;
    bool flip_x = false;
    bool flip_y = false;
} kb_cfg;

struct ControlChangeData
{
    uint8_t channel[CV_TOTAL];
    uint8_t id[CV_TOTAL];
} cc_cfg;

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
    config.SaveVar(cfg.channel, "channel");
    config.SaveVar(kb_cfg.base_octave, "octave");
    config.SaveVar(kb_cfg.base_note, "note");
    config.SaveVar(kb_cfg.velocity_curve, "velocity");

    for (int i = 0; i < CV_TOTAL; i++)
    {
        cc_cfg.channel[i] = 1;
        cc_cfg.id[i] = i + 13;
    }

    config.SaveArray(cc_cfg.channel, "channels", CV_TOTAL);
    config.SaveArray(cc_cfg.id, "cc_ids", CV_TOTAL);
}

void ProcessKey(int idx, Key::State state)
{
    if (state == Key::State::PRESSED)
    {
        uint8_t velocity = keyboard.GetVelocity(idx);
        MIDI.sendNoteOn(kb_cfg.base_note + (kb_cfg.base_octave * 12) + 15 - idx, velocity, cfg.channel);
        led_manager.SetPosition((uint8_t)(idx % 4), (uint8_t)(idx / 4));
        led_manager.SetColor(255 - velocity * 2);
        led_manager.SetSpeed((float)velocity / 127.0f);
    }

    else if (state == Key::State::RELEASED)
    {
        MIDI.sendNoteOff(kb_cfg.base_note + (kb_cfg.base_octave * 12) + 15 - idx, 0, cfg.channel);
    }

    else if (state == Key::State::AFTERTOUCH)
    {
        uint8_t pressure = keyboard.GetAftertouch(idx) * 127;
        MIDI.sendAfterTouch(kb_cfg.base_note + (kb_cfg.base_octave * 12) + 15 - idx, pressure, cfg.channel);
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
                MIDI.sendPitchBend(parameters.bend, cfg.channel);
                parameters.isBending = false;
            }
        }
        else if (slider.IsTouched())
        {
            // convert the float value to a 14 bit integer, with 0 being the float value 0.5
            value = (slider.GetPosition() - 0.5f) * 2.0f;
            parameters.bend = (value * 8192.0f);
            MIDI.sendPitchBend((int)parameters.bend, cfg.channel);
            led_manager.SetSlider(slider.GetPosition(), false);
            parameters.isBending = true;
        }
        break;
    case SliderMode::OCTAVE:
        kb_cfg.base_octave = (uint8_t)(slider.GetPosition() * 7);
        led_manager.SetSlider(slider.GetPosition(), false);
        break;
    case SliderMode::MOD:
        parameters.mod = slider.GetPosition();
        MIDI.sendControlChange(1, (uint8_t)(parameters.mod * 127.0f), cfg.channel);
        led_manager.SetSlider(slider.GetPosition());
        break;
    case SliderMode::SLEW:
        parameters.slew = slider.GetPosition();
        led_manager.SetSlider(parameters.slew);
        keyboard.SetSlew(parameters.slew);
        break;
    case SliderMode::BANK:
        parameters.bank = slider.GetQuantizedPosition(4);
        log_d("Bank: %d", parameters.bank);
        led_manager.SetSlider(slider.GetPosition(), false);
        keyboard.SetBank(parameters.bank);
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
        slider.SetPosition((float)kb_cfg.base_octave);
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
        led_manager.TransitionToPattern(new NoBlur());
        slider_mode = SliderMode::BEND;
        ProcessSliderButton();
        break;
    case Mode::XY_PAD:
        log_d("Mode: XY");
        keyboard.RemoveOnStateChanged();
        led_manager.TransitionToPattern(new TouchBlur());
        slider_mode = SliderMode::SLEW;
        ProcessSliderButton();
        break;
    case Mode::STRIPS:
        log_d("Mode: Strips");
        keyboard.RemoveOnStateChanged();
        led_manager.TransitionToPattern(new Strips());
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
            log_d("MIDI Learn mode");
            parameters.midiLearn = !parameters.midiLearn;
            keyboard.PlotLuts();
        }
        if (idx == PIN_EXT5)
        {
            log_d("Mode button long pressed");
            log_d("MIDI panic!");
            for (uint8_t i = 0; i < 78; i++)
            {
                MIDI.sendNoteOff(i, 0, cfg.channel);
            }
        }
    }
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
        MIDI.sendSysEx(sizeof(message), message);
    }

    if (data[2] == 127 && data[3] == 7 && data[4] == 3)
    {
        log_d("SysEx configuration dump request");
        char buffer[512];
        buffer[0] = 127;
        buffer[1] = 7;
        buffer[2] = 4;
        size_t size = config.SerializeToBuffer(buffer + 3);

        MIDI.sendSysEx(size + 3, reinterpret_cast<byte *>(buffer));
    }

    if (data[2] == 127 && data[3] == 7 && data[4] == 5) // TODO proper message
    {
        log_d("SysEx configuration load request");
        JsonDocument config;
        deserializeJson(config, data + 5);
        serializeJsonPretty(config, Serial);

        // config.DeserializeFromBuffer(reinterpret_cast<char *>(data + 5));
    }
}

void setup()
{
    // MIDI initialization
    log_d("Free heap: %d", ESP.getFreeHeap());
    MIDI.begin();

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);

    MIDI.setHandleSystemExclusive(ProcessSysEx);
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

    config.Init();
    if (!config.LoadVar(cfg.version, "version"))
    {
        log_d("first time configuration");
        InitConfiguration();
    }
    else
    {
        log_d("Configuration found");
        log_d("Loading configuration");
        config.LoadVar(cfg.mode, "mode");
        config.LoadVar(cfg.brightness, "brightness");
        config.LoadVar(cfg.channel, "channel");
        config.LoadVar(kb_cfg.base_octave, "octave");
        config.LoadVar(kb_cfg.velocity_curve, "velocity");
        config.LoadVar(kb_cfg.base_note, "note");
        config.LoadArray(cc_cfg.channel, "channels", CV_TOTAL);
        config.LoadArray(cc_cfg.id, "cc_ids", CV_TOTAL);
    }

    config.Print();

    log_d("Configuration initialized");
    log_d("Free heap: %d", ESP.getFreeHeap());

    adc.SetCalibration(calibration_data.minVal, calibration_data.maxVal, 16);
    adc.Start();
    // keyboard initialization
    KeyboardConfig keyboard_config;

    keyboard_config.Init(keys, 16);
    keyboard.Init(&keyboard_config, &adc);
    //keyboard.SetVelocityLut((Keyboard::VelocityLut)kb_cfg.velocity_curve);
    keyboard.SetVelocityLut(Keyboard::VelocityLut::EXPONENTIAL);
    keyboard.SetOnStateChanged(&ProcessKey);
    log_d("Free heap: %d", ESP.getFreeHeap());
}

void loop()
{
    MIDI.read();

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
            log_d("cc: %d, value: %d", cc_cfg.id[0], (uint8_t)(xy.x * 0.33333f * 127.0f));
            MIDI.sendControlChange((int)cc_cfg.id[0], (uint8_t)(xy.x * 0.33333f * 127.0f), cc_cfg.channel[0]);
            led_manager.SetPosition(xy.x, xy.y);
        }

        if (keyboard.YChanged())
        {
            MIDI.sendControlChange((int)cc_cfg.id[1], (uint8_t)(xy.y * 0.33333f * 127.0f), cc_cfg.channel[1]);
            led_manager.SetPosition(xy.x, xy.y);
        }

        float pressure = keyboard.GetPressure();
        if (pressure > 0.00f)
        {

            if (!parameters.midiLearn)
            {
                MIDI.sendControlChange(cc_cfg.id[2], (uint8_t)(pressure * 127.0f), cc_cfg.channel[2]);
            }
            led_manager.SetAmount(1.0f - pressure);
            led_manager.SetColor((uint8_t)(pressure * 255.0f));
            log_d("Pressure: %f", pressure);
            led_manager.SetState(true);
        }
    }

    else if (cfg.mode == Mode::KEYBOARD)
    {

        led_manager.SetMarker(15 - marker - 12);
        led_manager.SetMarker(15 - marker);

        //
    }

    else if (cfg.mode == Mode::STRIPS)
    {
        for (int i = 0; i < 4; i++)
        {
            if (keyboard.StripChanged(i))
            {
                MIDI.sendControlChange(cc_cfg.id[i + 3], (uint8_t)(keyboard.GetStrip(i) * 0.33333f * 127.0f), cc_cfg.channel[i + 3]);
                led_manager.SetStrip(i, keyboard.GetStrip(i));
            }
        }
    }

    ProcessSlider();
    led_manager.RunPattern();
    FastLED.show();
}
