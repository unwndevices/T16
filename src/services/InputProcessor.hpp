#pragma once

#include <stdint.h>
#include "../Types.hpp"
#include "../Configuration.hpp"
#include "../Libs/Keyboard.hpp"
#include <FastLED.h>

// Forward declarations
class ConfigManager;
class MidiProvider;
class LedManager;

namespace t16
{

class InputProcessor
{
public:
    InputProcessor(ConfigManager& config, MidiProvider& midi, LedManager& leds,
                   Keyboard& keyboard, Parameters& params, CRGBPalette16* palettes);

    void processKey(int idx, Key::State state);
    void processStrum(int idx, Key::State state);
    void processQuickSettings(int idx, Key::State state);
    void applyConfiguration();
    void onBankChange();

private:
    ConfigManager& configManager_;
    MidiProvider& midiProvider_;
    LedManager& ledManager_;
    Keyboard& keyboard_;
    Parameters& params_;
    CRGBPalette16* palettes_;

    // Strum state
    uint8_t currentChord_ = 0;
    uint8_t currentBaseNote_ = 0;
    uint8_t currentKeyIdx_ = 0;

    // Quick settings state
    uint8_t currentQsOption_ = 0;
    uint8_t currentValueLength_ = 0;

    static void setCustomScale(int8_t* scale, int8_t* customScale, int size);
    void setMarkerCallback(uint8_t index, bool isRootNote);
};

} // namespace t16
