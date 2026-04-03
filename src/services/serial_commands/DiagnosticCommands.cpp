#include "DiagnosticCommands.hpp"
#include "../SerialCommandManager.hpp"
#include "../ModeManager.hpp"
#include "../../ConfigManager.hpp"
#include "../../Configuration.hpp"
#include "../../Libs/MidiProvider.hpp"
#include "../../Libs/Keyboard.hpp"
#include "../../Libs/TouchSlider.hpp"

namespace t16
{

// File-scope static pointers for function-pointer-based command handlers
static ConfigManager* sConfig = nullptr;
static ModeManager* sMode = nullptr;
static MidiProvider* sMidi = nullptr;
static Keyboard* sKeyboard = nullptr;
static TouchSlider* sSlider = nullptr;

static void cmdVersion(const char* args)
{
    Serial.printf("Firmware version: %d\n", sConfig->Global().version);
    Serial.printf("Config format: v%d\n", sConfig->Global().version);
}

static void cmdConfig(const char* args)
{
    const ConfigurationData& g = sConfig->Global();
    Serial.println("--- Configuration ---");
    Serial.printf("  Mode:        %d\n", sMode->getMode());
    Serial.printf("  Sensitivity: %d\n", g.sensitivity);
    Serial.printf("  Brightness:  %d\n", g.brightness);
    Serial.printf("  MIDI BLE:    %s\n", g.midi_ble ? "on" : "off");
    Serial.printf("  MIDI TRS:    %s\n", g.midi_trs ? "on" : "off");
    Serial.printf("  TRS Type:    %s\n", g.trs_type ? "B" : "A");
    Serial.printf("  Passthrough: %s\n", g.passthrough ? "on" : "off");

    for (uint8_t b = 0; b < BANK_AMT; b++)
    {
        const KeyModeData& kb = sConfig->Bank(b);
        Serial.printf("  Bank %d: ch=%d scale=%d oct=%d note=%d vel=%d at=%d\n",
                      b, kb.channel, kb.scale, kb.base_octave, kb.base_note,
                      kb.velocity_curve, kb.aftertouch_curve);
    }
    Serial.println("---------------------");
}

static void cmdKeys(const char* args)
{
    Serial.println("--- Key States ---");
    for (uint8_t i = 0; i < 16; i++)
    {
        Key::State s = sKeyboard->GetKeyState(i);
        const char* stateStr;
        switch (s)
        {
        case Key::State::IDLE:       stateStr = "IDLE"; break;
        case Key::State::STARTED:    stateStr = "STARTED"; break;
        case Key::State::PRESSED:    stateStr = "PRESSED"; break;
        case Key::State::RELEASED:   stateStr = "RELEASED"; break;
        case Key::State::AFTERTOUCH: stateStr = "AFTERTOUCH"; break;
        default:                     stateStr = "UNKNOWN"; break;
        }
        float value = sKeyboard->GetKey(i);
        Serial.printf("  Key %2d: %-10s  value=%.3f\n", i, stateStr, value);
    }
    Serial.println("------------------");
}

static void cmdSlider(const char* args)
{
    Serial.println("--- Slider ---");
    Serial.printf("  Touched:  %s\n", sSlider->IsTouched() ? "yes" : "no");
    Serial.printf("  Position: %.3f\n", sSlider->GetPosition());
    Serial.printf("  Speed:    %.3f\n", sSlider->GetSpeed());
    for (uint8_t i = 0; i < 7; i++)
    {
        Serial.printf("  Sensor %d: %d  touched=%s\n", i, sSlider->sensorValues[i],
                      sSlider->IsTouched(i) ? "yes" : "no");
    }
    Serial.println("--------------");
}

static void cmdMode(const char* args)
{
    Mode m = sMode->getMode();
    SliderMode sm = sMode->getSliderMode();

    const char* modeStr;
    switch (m)
    {
    case KEYBOARD:       modeStr = "KEYBOARD"; break;
    case STRUM:          modeStr = "STRUM"; break;
    case XY_PAD:         modeStr = "XY_PAD"; break;
    case STRIPS:         modeStr = "STRIPS"; break;
    case QUICK_SETTINGS: modeStr = "QUICK_SETTINGS"; break;
    default:             modeStr = "UNKNOWN"; break;
    }

    const char* sliderStr;
    switch (sm)
    {
    case BEND:      sliderStr = "BEND"; break;
    case OCTAVE:    sliderStr = "OCTAVE"; break;
    case MOD:       sliderStr = "MOD"; break;
    case BANK:      sliderStr = "BANK"; break;
    case SLEW:      sliderStr = "SLEW"; break;
    case STRUMMING: sliderStr = "STRUMMING"; break;
    case QUICK:     sliderStr = "QUICK"; break;
    default:        sliderStr = "UNKNOWN"; break;
    }

    Serial.printf("Mode: %s\n", modeStr);
    Serial.printf("Slider mode: %s\n", sliderStr);
}

static void cmdPanic(const char* args)
{
    Serial.println("MIDI Panic -- sending NoteOff for all channels...");
    for (uint8_t ch = 1; ch <= 16; ch++)
    {
        for (uint8_t note = 0; note < 128; note++)
        {
            sMidi->SendNoteOff(note, ch);
        }
    }
    Serial.println("Done.");
}

void registerDiagnosticCommands(
    SerialCommandManager& mgr,
    ConfigManager& config,
    ModeManager& mode,
    MidiProvider& midi,
    Keyboard& keyboard,
    TouchSlider& slider)
{
    sConfig = &config;
    sMode = &mode;
    sMidi = &midi;
    sKeyboard = &keyboard;
    sSlider = &slider;

    mgr.registerCommand("version", "Show firmware version", cmdVersion);
    mgr.registerCommand("config", "Dump current configuration", cmdConfig);
    mgr.registerCommand("keys", "Show key states and ADC values", cmdKeys);
    mgr.registerCommand("slider", "Show slider state and position", cmdSlider);
    mgr.registerCommand("mode", "Show current mode and slider mode", cmdMode);
    mgr.registerCommand("panic", "Send NoteOff on all channels (MIDI panic)", cmdPanic);
}

} // namespace t16
