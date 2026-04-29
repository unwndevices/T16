#include "AppEngine.hpp"
#include "Libs/Leds/Palettes.hpp"
#include "Libs/DataManager.hpp"
#include "Scales.hpp"
#include "services/serial_commands/DiagnosticCommands.hpp"
#include <FastLED.h>

namespace t16
{

// File-scope trampoline for C-style callbacks
static AppEngine* sEngine = nullptr;

static void sysExTrampoline(byte* data, unsigned length)
{
    if (sEngine)
    {
        sEngine->handleSysEx(data, length);
    }
}

static void buttonTrampoline(int idx, Button::State state)
{
    if (sEngine)
    {
        sEngine->handleButtonEvent(idx, state);
    }
}

AppEngine::AppEngine()
    : sysExHandler_(configManager_, midiProvider_)
    , inputProcessor_(configManager_, midiProvider_, ledManager_, keyboard_, params_, palettes_)
    , sliderProcessor_(modeManager_, configManager_, midiProvider_, ledManager_, slider_, keyboard_, params_, inputProcessor_)
    , buttonHandler_(modeManager_, configManager_, midiProvider_, inputProcessor_, sliderProcessor_, params_, touchBtn_, modeBtn_)
    , calibrationService_(adc_, ledManager_)
{
    sEngine = this;
}

void AppEngine::init()
{
    Serial.begin(115200);

    // Initialize palettes
    palettes_[0] = unwn_gp;
    palettes_[1] = topo_gp;
    palettes_[2] = alt_gp;
    palettes_[3] = acid_gp;

    // MIDI initialization
    midiProvider_.Init(CurrentPinout::RX, CurrentPinout::TX, CurrentPinout::TX2);
    delay(200);
    midiProvider_.SetHandleSystemExclusive(sysExTrampoline);

    // Button initialization
    touchBtn_.Init(CurrentPinout::TOUCH);
    modeBtn_.Init(CurrentPinout::MODE);
    touchBtn_.onStateChanged.Connect(buttonTrampoline);
    modeBtn_.onStateChanged.Connect(buttonTrampoline);

    // LED initialization
    ledManager_.Init();

    // Slider initialization
    if (!slider_.Init(sliderSensors_))
    {
        ledManager_.TestAll(HUE_RED);
        delay(3000);
        ledManager_.OffAll();
        ESP.restart();
    }

    // ADC initialization
    AdcChannelConfig adc_config;
    adc_config.InitMux(CurrentPinout::COM, CurrentPinout::S0, CurrentPinout::S1, CurrentPinout::S2, CurrentPinout::S3);
    adc_.Init(&adc_config, 16);

    // Calibration data load
    DataManager calibration("/calibration_data.json");
    calibration.Init();
    if (!calibration.LoadArray(calibrationData_.minVal, "minVal", 16))
    {
        calibrationService_.runHardwareTest(keys_, 16);
        Serial.println("Calibration data not found, starting calibration routine");
        calibrationService_.runCalibration(keys_, 16, touchBtn_, modeBtn_, keyboard_, slider_);
        // runCalibration restarts the ESP -- won't reach here
    }
    calibration.LoadArray(calibrationData_.maxVal, "maxVal", 16);

    // Check if touch button held during boot -- reset config
    touchBtn_.Update();

    // ConfigManager initialization (replaces old DataManager config approach)
    configManager_.Init();
    if (touchBtn_.GetRaw())
    {
        log_d("Resetting configuration");
        configManager_.Save(true);
    }

    // Apply loaded configuration
    params_.bank = 0;
    inputProcessor_.applyConfiguration();
    log_d("Configuration initialized");

    // Set up ADC calibration and start
    adc_.SetCalibration(calibrationData_.minVal, calibrationData_.maxVal, 16);
    adc_.Start();

    // Keyboard initialization
    KeyboardConfig keyboard_config;
    keyboard_config.Init(keys_, 16);
    keyboard_.Init(&keyboard_config, &adc_);

    KeyModeData& kb = configManager_.Bank(params_.bank);
    keyboard_.SetVelocityLut((Keyboard::Lut)kb.velocity_curve);
    keyboard_.SetAftertouchLut((Keyboard::Lut)kb.aftertouch_curve);

    // Set initial mode and wire keyboard callbacks
    modeManager_.setMode(KEYBOARD);
    keyboard_.SetOnStateChanged([this](int idx, Key::State state) {
        inputProcessor_.processKey(idx, state);
    });

    // Set initial slider mode visual
    sliderProcessor_.onSliderModeChanged();

    // Register diagnostic commands
    registerDiagnosticCommands(serialCommands_, configManager_, modeManager_,
                               midiProvider_, keyboard_, slider_);

    log_d("AppEngine initialized");
}

void AppEngine::update()
{
    midiProvider_.Read();

    touchBtn_.Update();
    modeBtn_.Update();
    slider_.Update();

    keyboard_.Update();
    fill_solid(matrixleds, 16, CRGB::Black);

    renderModeVisuals();

    sliderProcessor_.update();
    ledManager_.RunPattern();
    FastLED.show();

    configManager_.CheckIdleFlush(millis());
    serialCommands_.update();
}

void AppEngine::handleButtonEvent(int idx, Button::State state)
{
    buttonHandler_.handleButton(idx, state);
}

void AppEngine::handleSysEx(byte* data, unsigned length)
{
    sysExHandler_.ProcessSysEx(data, length);
}

void AppEngine::renderModeVisuals()
{
    Mode mode = modeManager_.getMode();

    if (mode == XY_PAD)
    {
        Vec2 xy;
        xy.x = keyboard_.GetX();
        xy.y = keyboard_.GetY();

        ControlChangeData& cc = configManager_.CC(params_.bank);

        if (keyboard_.XChanged())
        {
            midiProvider_.SendControlChange(
                (int)cc.id[0], (uint8_t)(xy.x * 0.33333f * 127.0f), cc.channel[0]);
        }

        if (keyboard_.YChanged())
        {
            midiProvider_.SendControlChange(
                (int)cc.id[1], (uint8_t)(xy.y * 0.33333f * 127.0f), cc.channel[1]);
        }
        ledManager_.SetPosition(xy.x, xy.y);

        float pressure = keyboard_.GetPressure();
        if (pressure >= 0.00f)
        {
            if (!params_.midiLearn)
            {
                midiProvider_.SendControlChange(
                    cc.id[2], (uint8_t)(pressure * 127.0f), cc.channel[2]);
            }
            ledManager_.SetAmount(1.0f - pressure);
            ledManager_.SetColor((uint8_t)(pressure * 255.0f));
            ledManager_.SetState(true);
        }
    }
    else if (mode == KEYBOARD)
    {
        ledManager_.DrawMarkers();
    }
    else if (mode == STRIPS)
    {
        ControlChangeData& cc = configManager_.CC(params_.bank);

        for (int i = 0; i < 4; i++)
        {
            ledManager_.SetStrip(i, keyboard_.GetStrip(i));
            if (keyboard_.StripChanged(i))
            {
                midiProvider_.SendControlChange(
                    cc.id[i + 4],
                    (uint8_t)(127.0f - keyboard_.GetStrip(i) * 0.33333f * 127.0f),
                    cc.channel[i + 3]);
            }
        }
    }
    // Note: FWBUG-03 -- duplicate XY_PAD branch removed (was dead code in old main.cpp)
    // STRUM and QUICK_SETTINGS modes have no per-frame rendering
}

} // namespace t16
