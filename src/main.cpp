#include "pinout.h"
#include <Arduino.h>

#include "Performance.hpp"

#include "Configuration.hpp"

#include "Libs/MidiProvider.hpp"
MidiProvider midi_provider;

#include "Libs/DataManager.hpp"
DataManager calibration("/calibration_data.json");
DataManager config("/configuration_data.json");

#include <FastLED.h>

DEFINE_GRADIENT_PALETTE(unwn_gp){0, 255, 246, 197, 128, 255, 178, 28, 255, 255, 83, 0};
// DEFINE_GRADIENT_PALETTE(unwn_gp){0, 176, 65, 251, 127, 230, 2, 2, 255, 255, 111, 156};
DEFINE_GRADIENT_PALETTE(topo_gp){0, 0, 107, 189, 128, 255, 147, 0, 255, 0, 255, 121};
DEFINE_GRADIENT_PALETTE(alt_gp){0, 189, 70, 70, 255, 78, 75, 232};
DEFINE_GRADIENT_PALETTE(acid_gp){0, 126, 255, 36, 255, 130, 0, 255};
CRGBPalette16 palette[] = {unwn_gp, topo_gp, alt_gp, acid_gp};

#include "Libs/Leds/LedManager.hpp"
LedManager led_manager;

TouchBlur touch_blur;
NoBlur no_blur;
Strips strips;
Strum strum;
QuickSettings quick;

#include "Libs/Adc.hpp"
Adc adc;

#include "Libs/Keyboard.hpp"
#ifdef REV_B
Key keys[] = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6};
#else
Key keys[] = {6, 7, 15, 11, 5, 2, 14, 9, 4, 1, 13, 8, 3, 0, 12, 10};
#endif
Keyboard keyboard;

#include "Libs/TouchSlider.hpp"
uint8_t slider_sensor[] = {PIN_T1, PIN_T2, PIN_T3, PIN_T4, PIN_T5, PIN_T6, PIN_T7};
TouchSlider slider;

uint8_t marker = 0;

#include "Libs/Button.hpp"
Button t_btn(PIN_TOUCH);
Button m_btn(PIN_MODE);

#include "Scales.hpp"

enum SliderMode
{
    BEND,
    OCTAVE,
    MOD,
    BANK,
    SLEW,
    STRUMMING,
    QUICK,
    SLIDER_MODE_AMOUNT
};

/////////////////////////
// Data

SliderMode slider_mode = SliderMode::BEND;

void SetMarkerCallback(uint8_t index, bool isRootNote)
{
    led_manager.SetMarker(index, isRootNote);
}

void OnBankChange()
{
    led_manager.SetPalette(palette[kb_cfg[parameters.bank].palette]);
    uint8_t base_note = kb_cfg[parameters.bank].base_note + (kb_cfg[parameters.bank].base_octave * 12);
    SetNoteMap(kb_cfg[parameters.bank].scale, base_note, kb_cfg[parameters.bank].flip_x, kb_cfg[parameters.bank].flip_y, SetMarkerCallback);
    SetChordMapping(kb_cfg[parameters.bank].scale);
    keyboard.SetBank(parameters.bank);
    keyboard.SetVelocityLut((Keyboard::Lut)kb_cfg[parameters.bank].velocity_curve);
    keyboard.SetAftertouchLut((Keyboard::Lut)kb_cfg[parameters.bank].aftertouch_curve);
    // Set Chord mode?
    led_manager.UpdateTransition();
}

uint8_t current_chord = 0;
uint8_t current_base_note = 0;
uint8_t current_key_idx = 0;

void ProcessStrum(int idx, Key::State state)
{
    if (state == Key::State::PRESSED)
    {
        if (idx < 12)
        {
            current_key_idx = idx;
            current_base_note = note_map[idx] + (kb_cfg[parameters.bank].base_octave * 12);
            led_manager.SetNote(idx);
            log_d("Pressed: idx=%d, base_note=%d, chord=%d", idx, current_base_note, current_chord);
            current_chord = current_chord_mapping[idx];
            led_manager.SetChord(current_chord);
        }
        else
        {
            current_chord = idx - 12;
            current_chord_mapping[current_base_note] = current_chord;
            led_manager.SetChord(current_chord);
        }
    }
}

void ProcessSliderStrum(uint8_t idx, bool state)
{
    if (cfg.mode == Mode::STRUM)
        if (state)
        {
            int velocity = keyboard.GetPressure(current_key_idx);
            log_d("velocity: %d", velocity);
            midi_provider.SendChordNoteOn(idx, strum_chords[current_chord][idx] + current_base_note, velocity, kb_cfg[parameters.bank].channel);
            led_manager.SetSliderLed(idx, 254);
            FastLED.show();
        }
        else
        {
            midi_provider.SendChordNoteOff(idx, kb_cfg[parameters.bank].channel);
            led_manager.SetSliderLed(idx, 25);
        }
}

void ProcessKey(int idx, Key::State state)
{
    uint8_t note = note_map[idx] + (kb_cfg[parameters.bank].base_octave * 12);

    if (state == Key::State::PRESSED)
    {
        uint8_t velocity = keyboard.GetVelocity(idx);
        midi_provider.SendNoteOn(idx, note, velocity, kb_cfg[parameters.bank].channel);
        led_manager.SetPosition((uint8_t)(idx % 4), (uint8_t)(idx / 4));
        led_manager.SetColor(255 - velocity * 2);
        led_manager.SetSpeed((127 - velocity) / 2);
    }
    else if (state == Key::State::RELEASED)
    {
        midi_provider.SendNoteOff(idx, kb_cfg[parameters.bank].channel);
    }
    else if (state == Key::State::AFTERTOUCH)
    {
        uint8_t pressure = keyboard.GetAftertouch(idx);
        log_d("Aftertouch: %d", pressure);
        midi_provider.SendAfterTouch(idx, (midi::DataByte)pressure, kb_cfg[parameters.bank].channel);
    }
}

uint8_t current_qs_option = 0;
uint8_t current_value_length = 0;
void ProcessQuickSettings(int idx, Key::State state)
{
    if (state == Key::State::PRESSED)
    {
        log_d("Quick Settings: %d", idx);
        if (idx < 4)
        {
            current_qs_option = parameters.quickSettingsPage * 4 + idx;
            led_manager.SetOption(current_qs_option, 4);
            log_d("Current QS Option: %d", current_qs_option);
            uint8_t current_value = qs.settings[current_qs_option].value;
            current_value_length = qs.settings[current_qs_option].numOptions;
            led_manager.SetValue(current_value, current_value_length);
            log_d("Current Value: %d", current_value);
            log_d("Current Value Length: %d", current_value_length);
        }
        else
        {
            led_manager.SetValue(idx - 4, current_value_length);
            qs.settings[current_qs_option].value = idx - 4;
        }
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
                parameters.bend = 0.0f;
                midi_provider.SendPitchBend(parameters.bend, kb_cfg[parameters.bank].channel);
                parameters.isBending = false;
            }
        }
        else if (slider.IsTouched())
        {
            // convert the float value to a 14 bit integer, with 0 being the float value 0.5
            value = (slider.GetPosition() - 0.5f) * 2.0f;
            parameters.bend = (value * 8191.0f);
            log_d("bend: %d", (int)parameters.bend);
            midi_provider.SendPitchBend((int)parameters.bend, kb_cfg[parameters.bank].channel);
            led_manager.SetSlider(slider.GetPosition(), false);
            parameters.isBending = true;
        }
        break;
    case SliderMode::OCTAVE:
        kb_cfg[parameters.bank].base_octave = slider.GetQuantizedPosition(7);
        led_manager.SetSlider((uint8_t)slider.GetQuantizedPosition(7), false);
        break;
    case SliderMode::MOD:
        parameters.mod = slider.GetPosition();
        midi_provider.SendControlChange(cc_cfg[parameters.bank].id[3], (uint8_t)(parameters.mod * 127.0f), cc_cfg[parameters.bank].channel[3]);
        led_manager.SetSlider(slider.GetPosition());
        break;
    case SliderMode::SLEW:
        parameters.slew = slider.GetPosition();
        led_manager.SetSlider(parameters.slew);
        keyboard.SetSlew(parameters.slew);
        break;
    case SliderMode::BANK:
    {
        uint8_t bank = slider.GetQuantizedPosition(4);
        if (parameters.bank != bank)
        {
            parameters.bank = bank;
            OnBankChange();
        }
        led_manager.SetSlider((uint8_t)(slider.GetQuantizedPosition(4) * 2), false, 30);
        break;
    }
    case SliderMode::STRUMMING:
        break;
    case SliderMode::QUICK:
        uint8_t page = slider.GetQuantizedPosition(3);
        if (parameters.quickSettingsPage != page)
        {
            parameters.quickSettingsPage = page;
        }
        led_manager.SetSlider((uint8_t)(page * 3), false, 40);
        break;
    }
}

void ProcessSliderButton()
{
    switch (slider_mode)
    {
    case SliderMode::BEND:
        log_d("Slider mode: Bend");
        led_manager.SetSliderHue(HSVHue::HUE_PINK);
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
    case SliderMode::STRUMMING:
        log_d("Slider mode: Strum");
        led_manager.SetSliderHue(HSVHue::HUE_BLUE + 10);
        break;
    case SliderMode::QUICK:
        log_d("Slider mode: Quick");
        slider.SetPosition(0.0f);
        led_manager.SetSliderHue(HSVHue::HUE_PINK);
        break;
    }
}

void ProcessModeButton()
{
    switch (cfg.mode)
    {
    case Mode::KEYBOARD:
        log_d("Mode: Keyboard");
        keyboard.RemoveOnStateChanged();
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
    case Mode::STRUM:
        log_d("Mode: Strum");
        keyboard.RemoveOnStateChanged();
        keyboard.SetOnStateChanged(&ProcessStrum);
        slider.onSensorTouched.Connect(ProcessSliderStrum);
        keyboard.SetMode(Mode::STRUM);
        led_manager.TransitionToPattern(&strum);
        slider_mode = SliderMode::STRUMMING;
        ProcessSliderButton();
        break;
    case Mode::QUICK_SETTINGS:
        log_d("Mode: Quick Settings");
        slider_mode = SliderMode::QUICK;
        keyboard.RemoveOnStateChanged();
        keyboard.SetOnStateChanged(&ProcessQuickSettings);
        LoadQuickSettings(parameters.bank);
        led_manager.TransitionToPattern(&quick);
        break;
    }
}

void ProcessButton(int idx, Button::State state)
{

    if (state == Button::State::CLICKED)
    {
        if (idx == PIN_TOUCH)
        {
            log_d("Touch button clicked");
            if (cfg.mode == Mode::KEYBOARD)
            {
                SliderMode allowed_modes[] = {SliderMode::BEND, SliderMode::MOD, SliderMode::OCTAVE, SliderMode::BANK};
                int num_modes = sizeof(allowed_modes) / sizeof(allowed_modes[0]);
                int current_index = -1;
                for (int i = 0; i < num_modes; i++)
                {
                    if (slider_mode == allowed_modes[i])
                    {
                        current_index = i;
                        break;
                    }
                }
                current_index = (current_index + 1) % num_modes;
                slider_mode = allowed_modes[current_index];
            }
            else if (cfg.mode == Mode::STRUM)
            {
                SliderMode allowed_modes[] = {SliderMode::STRUMMING, SliderMode::OCTAVE, SliderMode::BANK};
                int num_modes = sizeof(allowed_modes) / sizeof(allowed_modes[0]);
                int current_index = -1;
                for (int i = 0; i < num_modes; i++)
                {
                    if (slider_mode == allowed_modes[i])
                    {
                        current_index = i;
                        break;
                    }
                }
                current_index = (current_index + 1) % num_modes;
                slider_mode = allowed_modes[current_index];
            }
            else if (cfg.mode == Mode::XY_PAD || cfg.mode == Mode::STRIPS)
            {
                SliderMode allowed_modes[] = {SliderMode::SLEW, SliderMode::BANK};
                int num_modes = sizeof(allowed_modes) / sizeof(allowed_modes[0]);
                int current_index = -1;
                for (int i = 0; i < num_modes; i++)
                {
                    if (slider_mode == allowed_modes[i])
                    {
                        current_index = i;
                        break;
                    }
                }
                current_index = (current_index + 1) % num_modes;
                slider_mode = allowed_modes[current_index];
            }
            else if (cfg.mode == Mode::QUICK_SETTINGS)
            {
                slider_mode = SliderMode::QUICK;
            }
            ProcessSliderButton();
        }

        if (idx == PIN_MODE)
        {
            log_d("Mode button clicked");
            cfg.mode = (Mode)((cfg.mode + 1) % (MODE_AMOUNT - 1)); // Exclude Quick Settings mode
            if (cfg.mode == Mode::QUICK_SETTINGS)
            {
                cfg.mode = Mode::KEYBOARD; // Skip Quick Settings mode and go to Keyboard mode
            }
            ProcessModeButton();
        }
    }

    if (state == Button::State::LONG_PRESSED)
    {
        if (idx == PIN_TOUCH)
        {
            if (m_btn.GetState() == Button::State::LONG_PRESSED)
            {
                if (cfg.mode != Mode::QUICK_SETTINGS)
                {
                    log_d("Both buttons long pressed");
                    log_d("Entering Quick Settings mode");
                    cfg.mode = Mode::QUICK_SETTINGS;
                    ProcessModeButton();
                }
                else
                {
                    log_d("Both buttons long pressed");
                    log_d("Exiting Quick Settings mode");
                    SaveQuickSettings(parameters.bank);
                    SaveConfiguration(config);
                    log_d("Saved configuration");
                    cfg.mode = Mode::KEYBOARD;
                    ProcessModeButton();
                }
            }
            else
            {
                log_d("Touch button long pressed");
                log_d("Midi Learn mode");
                parameters.midiLearn = true;
            }
        }
        if (idx == PIN_MODE)
        {
            if (t_btn.GetState() == Button::State::LONG_PRESSED)
            {
                if (cfg.mode != Mode::QUICK_SETTINGS)
                {
                    log_d("Both buttons long pressed");
                    log_d("Entering Quick Settings mode");
                    cfg.mode = Mode::QUICK_SETTINGS;
                    ProcessModeButton();
                }
                else
                {
                    log_d("Both buttons long pressed");
                    log_d("Exiting Quick Settings mode");
                    SaveQuickSettings(parameters.bank);
                    SaveConfiguration(config);
                    log_d("Saved configuration");
                    cfg.mode = Mode::KEYBOARD;
                    ProcessModeButton();
                }
            }
            else
            {
                log_d("Mode button long pressed");
                log_d("MIDI panic!");
                for (uint8_t i = 0; i < 16; i++)
                {
                    midi_provider.SendNoteOff(i, kb_cfg[parameters.bank].channel);
                }
            }
        }
    }

    if (state == Button::State::LONG_RELEASED)
    {
        if (idx == PIN_TOUCH)
        {
            log_d("Touch button long released");
            parameters.midiLearn = false;
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
    midi_provider.SetMidiBle((bool)cfg.midi_ble);
    midi_provider.SetMidiOut((bool)cfg.midi_trs);
    midi_provider.SetMidiTRSType((bool)cfg.trs_type);
    midi_provider.SetMidiThru((bool)cfg.passthrough);
    uint8_t brightness = cfg.brightness * 35 + 10;
    led_manager.SetBrightness(brightness);
    log_d("Brightness: %d", brightness);

    SetCustomScale(scales[CUSTOM1], cfg.custom_scale1, 16);
    SetCustomScale(scales[CUSTOM2], cfg.custom_scale2, 16);

    log_d("current bank: %d", parameters.bank);

    uint8_t base_note = kb_cfg[parameters.bank].base_note + (kb_cfg[parameters.bank].base_octave * 12);
    SetNoteMap(kb_cfg[parameters.bank].scale, base_note, kb_cfg[parameters.bank].flip_x, kb_cfg[parameters.bank].flip_y, SetMarkerCallback);
    SetChordMapping(kb_cfg[parameters.bank].scale);
    keyboard.SetVelocityLut((Keyboard::Lut)kb_cfg[parameters.bank].velocity_curve);
    keyboard.SetAftertouchLut((Keyboard::Lut)kb_cfg[parameters.bank].aftertouch_curve);
    led_manager.UpdateTransition();
    cfg.mode = Mode::KEYBOARD;
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
        char buffer[4098];
        buffer[0] = 127;
        buffer[1] = 7;
        buffer[2] = 4;
        size_t size = config.SerializeToBuffer(buffer + 3, 4098);
        midi_provider.SendSysEx(size + 3, reinterpret_cast<byte *>(buffer));
    }

    if (data[2] == 127 && data[3] == 7 && data[4] == 5) // TODO proper message
    {
        log_d("SysEx configuration load request");
        config.DeserializeFromBuffer(reinterpret_cast<char *>(data + 5));
        LoadConfiguration(config);
        ApplyConfiguration();
    }
}

bool CalibrationRoutine()
{
    for (int i = 0; i < 16; i++)
    {
        adc.CalibrateMax(i);
    }
    Serial.println("Min calibration done");

    for (int i = 0; i < 16; i++)
    {
        led_manager.SetLed(i, true);
        // wait for the mode button to be pressed
        adc.SetMuxChannel(keys[i].mux_idx);
        while (m_btn.GetState() != Button::State::CLICKED)
        {
            Serial.printf("Getting raw value: %d\n", adc.GetRaw());
            FastLED.show();
            m_btn.Update();
        }
        m_btn.Update();
        adc.CalibrateMin(keys[i].mux_idx);
        delay(500);
    }
    Serial.println("Max calibration done");

    return true;
}

void HardwareTest()
{
    led_manager.TestAll();
}

void setup()
{
    midi_provider.Init(PIN_RX, PIN_TX, PIN_TX2);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);

    midi_provider.SetHandleSystemExclusive(ProcessSysEx);
    // Button initialization
    t_btn.Init(PIN_TOUCH);
    m_btn.Init(PIN_MODE);
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
        HardwareTest();
        Serial.println("Calibration data not found, starting calibration routine");
        CalibrationRoutine();
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
        SaveConfiguration(config);
    }
    if (!config.LoadVar(cfg.version, "version"))
    {
        log_d("first time configuration");
        SaveConfiguration(config);
    }
    else
    {
        LoadConfiguration(config);
        ApplyConfiguration();
    }

    cfg.mode = Mode::KEYBOARD;

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
    // Set Chord mode?
    keyboard.SetOnStateChanged(&ProcessKey);
}

void loop()
{

    midi_provider.Read();

    t_btn.Update();
    m_btn.Update();
    slider.Update();

    keyboard.Update();
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
        if (pressure >= 0.00f)
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
                midi_provider.SendControlChange(cc_cfg[parameters.bank].id[i + 4], (uint8_t)(127.0f - keyboard.GetStrip(i) * 0.33333f * 127.0f), cc_cfg[parameters.bank].channel[i + 3]);
            }
        }
    }

    else if (cfg.mode == Mode::XY_PAD)
    {
        led_manager.SetPosition(keyboard.GetX(), keyboard.GetY());
    }

    else if (cfg.mode == Mode::STRUM)
    {
        // TODO
    }

    else if (cfg.mode == Mode::QUICK_SETTINGS)
    {
    }

    ProcessSlider();
    led_manager.RunPattern();
    FastLED.show();
}
