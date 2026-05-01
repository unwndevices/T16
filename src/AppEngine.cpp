#include "AppEngine.hpp"
#include "variant.hpp"
#include "Libs/Leds/Palettes.hpp"
#include "Libs/DataManager.hpp"
#include "Scales.hpp"
#include "services/serial_commands/DiagnosticCommands.hpp"
#include <FastLED.h>

namespace t16
{

// File-scope trampoline for C-style callbacks
static AppEngine* sEngine = nullptr;

namespace {
// Phase 12.01: Migrate pre-v1.1 /calibration_data.json to /calibration_t16.json on
// first boot under the new variant-aware naming scheme. T32 hardware is brand-new,
// so no legacy file ever exists there — skip the rename for T32.
void MigrateLegacyCalibrationFile()
{
    const char* current = variant::CalibrationFilePath();
    if (variant::CurrentVariant::kConfig.NAME[1] == '3') return;     // T32 — nothing to migrate
    if (LittleFS.exists(current)) return;                            // already migrated
    if (!LittleFS.exists(variant::kLegacyCalibrationPath)) return;   // nothing to migrate
    bool ok = LittleFS.rename(variant::kLegacyCalibrationPath, current);
    if (ok) {
        Serial.printf("Migrated %s -> %s\n", variant::kLegacyCalibrationPath, current);
    } else {
        Serial.printf("WARN: rename %s -> %s failed\n", variant::kLegacyCalibrationPath, current);
    }
}
} // anonymous namespace

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

// BOOT_TRACE: temporary boot-instrumentation. Remove once boot crash is diagnosed.
#define BOOT_TRACE(msg) do { Serial.print("[BOOT] "); Serial.println(msg); Serial.flush(); } while (0)

void AppEngine::init()
{
    Serial.begin(115200);
    // Wait up to 3s for USB CDC host enumeration so early prints aren't lost.
    unsigned long _t0 = millis();
    while (!Serial && millis() - _t0 < 3000) { delay(10); }
    delay(200);
    BOOT_TRACE("00 serial-up");

    // Initialize palettes
    palettes_[0] = unwn_gp;
    palettes_[1] = topo_gp;
    palettes_[2] = alt_gp;
    palettes_[3] = acid_gp;
    BOOT_TRACE("01 palettes");

    // MIDI initialization
    midiProvider_.Init(CurrentPinout::RX, CurrentPinout::TX, CurrentPinout::TX2_PIN);
    delay(200);
    midiProvider_.SetHandleSystemExclusive(sysExTrampoline);
    BOOT_TRACE("02 midi");

    // Button initialization
    touchBtn_.Init(CurrentPinout::TOUCH);
    modeBtn_.Init(CurrentPinout::MODE);
    touchBtn_.onStateChanged.Connect(buttonTrampoline);
    modeBtn_.onStateChanged.Connect(buttonTrampoline);
    BOOT_TRACE("03 buttons");

    // LED initialization
    ledManager_.Init();
    BOOT_TRACE("04 leds");

    // Slider initialization
    if (!slider_.Init(sliderSensors_))
    {
        BOOT_TRACE("FAIL slider-init");
        ledManager_.TestAll(HUE_RED);
        delay(3000);
        ledManager_.OffAll();
        ESP.restart();
    }
    BOOT_TRACE("05 slider");

    // ADC initialization
    adc_.InitMuxes(variant::CurrentVariant::kMuxes,
                   sizeof(variant::CurrentVariant::kMuxes) /
                   sizeof(variant::CurrentVariant::kMuxes[0]));
    BOOT_TRACE("06 adc-mux");

    // Backfill Key::mux_id / Key::mux_channel from the variant's mux configs.
    PopulateKeyMuxMapping(keys_, variant::CurrentVariant::kConfig.TOTAL_KEYS,
                          variant::CurrentVariant::kMuxes,
                          sizeof(variant::CurrentVariant::kMuxes) /
                          sizeof(variant::CurrentVariant::kMuxes[0]));
    BOOT_TRACE("07 key-mux-map");

    // Calibration data load
    MigrateLegacyCalibrationFile();
    BOOT_TRACE("08 cal-migrate");
    DataManager calibration(variant::CalibrationFilePath());
    calibration.Init();
    BOOT_TRACE("09 cal-init");
    if (!calibration.LoadArray(calibrationData_.minVal, "minVal", CalibrationData::kSize))
    {
        BOOT_TRACE("10a cal-load-fail -> hardwareTest+runCal");
        calibrationService_.runHardwareTest(keys_, CalibrationData::kSize);
        Serial.println("Calibration data not found, starting calibration routine");
        calibrationService_.runCalibration(keys_, CalibrationData::kSize, touchBtn_, modeBtn_, keyboard_, slider_);
        // runCalibration restarts the ESP -- won't reach here
    }
    BOOT_TRACE("10 cal-min-loaded");
    calibration.LoadArray(calibrationData_.maxVal, "maxVal", CalibrationData::kSize);
    BOOT_TRACE("11 cal-max-loaded");

    // Check if touch button held during boot -- reset config
    touchBtn_.Update();
    BOOT_TRACE("12 touch-update");

    // ConfigManager initialization
    configManager_.Init();
    BOOT_TRACE("13 cfgmgr-init");
    if (touchBtn_.GetRaw())
    {
        BOOT_TRACE("13a cfg-reset");
        log_d("Resetting configuration");
        configManager_.Save(true);
    }

    // Apply loaded configuration
    params_.bank = 0;
    inputProcessor_.applyConfiguration();
    BOOT_TRACE("14 applyConfiguration");
    log_d("Configuration initialized");

    // Set up ADC calibration and start
    adc_.SetCalibration(calibrationData_.minVal, calibrationData_.maxVal, CalibrationData::kSize);
    BOOT_TRACE("15 adc-setcal");
    adc_.Start();
    BOOT_TRACE("16 adc-start");

    // Keyboard initialization
    KeyboardConfig keyboard_config;
    keyboard_config.Init(keys_, variant::CurrentVariant::kConfig.TOTAL_KEYS);
    BOOT_TRACE("17 kbcfg-init");
    keyboard_.Init(&keyboard_config, &adc_);
    BOOT_TRACE("18 kb-init");

    KeyModeData& kb = configManager_.Bank(params_.bank);
    keyboard_.SetVelocityLut((Keyboard::Lut)kb.velocity_curve);
    keyboard_.SetAftertouchLut((Keyboard::Lut)kb.aftertouch_curve);
    BOOT_TRACE("19 kb-luts");

    // Set initial mode and wire keyboard callbacks
    modeManager_.setMode(KEYBOARD);
    keyboard_.SetOnStateChanged([this](int idx, Key::State state) {
        inputProcessor_.processKey(idx, state);
    });
    BOOT_TRACE("20 mode+cb");

    // Set initial slider mode visual
    sliderProcessor_.onSliderModeChanged();
    BOOT_TRACE("21 slider-visual");

    // Register diagnostic commands
    registerDiagnosticCommands(serialCommands_, configManager_, modeManager_,
                               midiProvider_, keyboard_, slider_);
    BOOT_TRACE("22 diag-cmds");

    log_d("AppEngine initialized");
    BOOT_TRACE("99 init-done");
}

void AppEngine::update()
{
    static uint32_t _frame = 0;
    if (_frame < 5) { Serial.print("[FRAME ") ; Serial.print(_frame); Serial.println(" enter]"); Serial.flush(); }

    midiProvider_.Read();
    if (_frame < 5) { BOOT_TRACE("  midi.read"); }

    touchBtn_.Update();
    modeBtn_.Update();
    slider_.Update();
    if (_frame < 5) { BOOT_TRACE("  buttons+slider"); }

    keyboard_.Update();
    if (_frame < 5) { BOOT_TRACE("  kb.update"); }
    fill_solid(matrixleds, kMatrixSize, CRGB::Black);
    if (_frame < 5) { BOOT_TRACE("  fill_black"); }

    renderModeVisuals();
    if (_frame < 5) { BOOT_TRACE("  renderMode"); }

    sliderProcessor_.update();
    if (_frame < 5) { BOOT_TRACE("  sliderProc"); }
    ledManager_.RunPattern();
    if (_frame < 5) { BOOT_TRACE("  runPattern"); }
    FastLED.show();
    if (_frame < 5) { BOOT_TRACE("  fastled.show"); }

    configManager_.CheckIdleFlush(millis());
    serialCommands_.update();
    if (_frame < 5) { Serial.print("[FRAME "); Serial.print(_frame); Serial.println(" exit]"); Serial.flush(); }
    _frame++;
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
