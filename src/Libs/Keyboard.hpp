#pragma once

#include <stdint.h>
#include "Adc.hpp"
#include "Signal.hpp"
#include "../Types.hpp"

float fmap(float x, float in_min, float in_max, float out_min, float out_max);

class Key
{
public:
    enum State
    {
        IDLE,
        STARTED,
        PRESSED,
        RELEASED,
        AFTERTOUCH
    };

    Key(uint8_t index);

    void Update(Adc *adc);
    State GetState() const;
    float GetAftertouch();
    float GetPressure();

    static void SetPressThreshold(float threshold);
    static void SetATThreshold(float threshold);

    uint8_t idx;
    uint8_t mux_idx;          // logical key index — used by Update via adc->GetMux(0, mux_idx)
    uint8_t mux_id = 0;       // which mux this key belongs to (0..MUX_COUNT-1). Default 0 for T16.
    uint8_t mux_channel = 0;  // channel-within-mux (0..15) for SetMuxChannel during calibration.
    State state = IDLE;
    float value = 0.0f;
    float velocity = 0.0f;

    Signal<int, Key::State> onStateChanged;

private:
    ulong pressStartTime = 0;
    uint8_t debounceTime = 10;
    float pressure = 0.0f;

    static float press_threshold;
    static float rel_threshold;
    static float at_threshold;
    static uint8_t instances;
};

// key_amount is supplied by the caller (see AppEngine), which reads
// variant::CurrentVariant::kConfig.TOTAL_KEYS — no internal literal.
class KeyboardConfig
{
public:
    KeyboardConfig() {};

    void Init(Key *keys, uint8_t key_amount);

    uint8_t _key_amount;
    Key *_keys;
};

// Backfill `mux_id` and `mux_channel` on each Key after Adc::InitMuxes has run.
// For each Key, scans the variant's mux configs for an entry where
// keyMapping[ch] == keys[i].mux_idx (the logical key this Key reads), then
// stores the owning mux index and the 0..15 channel-within-mux. On T16 (single
// mux, identity mapping) this collapses to mux_id=0, mux_channel=mux_idx.
void PopulateKeyMuxMapping(Key *keys, uint8_t key_amount,
                           const MultiplexerConfig *muxes, uint8_t mux_count);

typedef struct Vec2
{
    float x;
    float y;
} Vec2;

class Keyboard
{
public:
    enum Lut
    {
        LINEAR,
        EXPONENTIAL,
        LOGARITHMIC,
        QUADRATIC,
        LUT_AMOUNT
    };

    Keyboard() {};
    ~Keyboard() {};

    void Init(KeyboardConfig *cfg, Adc *adc);
    void SetOnStateChanged(std::function<void(int, Key::State)> handler);
    void RemoveOnStateChanged();
    void Update();

    float GetKey(uint8_t chn);
    Key::State GetKeyState(uint8_t chn);
    uint8_t GetVelocity(uint8_t chn);
    float GetAftertouch(uint8_t chn);

    float GetX();
    float GetY();
    float GetPressure();
    uint8_t GetPressure(uint8_t chn);

    bool XChanged();
    bool YChanged();

    void SetSlew(float slew_lim);

    float GetStrip(uint8_t chn);
    bool StripChanged(uint8_t chn);

    void SetBank(uint8_t bank);
    void SetVelocityLut(Lut lut);
    void SetAftertouchLut(Lut lut);
    void SetPressThreshold(float threshold);
    void SetATThreshold(float threshold);
    void PlotLuts();
    void SetMode(Mode mode);
    void SetATFullRange(bool full_at);

private:
    KeyboardConfig _config;
    Adc *_adc;
    Vec2 position[4];
    Vec2 last_position[4];
    float max_pressure = 0.0f;
    float threshold = 0.3f;
    float touch_threshold = 0.15f;
    float slew = 0.4f;

    Mode mode = KEYBOARD;

    // STRIP MODE
    float strip_position[16] = {3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f};
    float strip_last_position[16] = {0.0f};
    uint8_t _bank = 0;
    bool bank_changed = false;

    unsigned long deltaTime = 0;
    unsigned long previousTime = 0;

    Lut velocityLut = LINEAR;
    Lut aftertouchLut = LINEAR;

    uint8_t output_lut[Lut::LUT_AMOUNT][128] = {0};

    void GenerateLUTs();
    void CalcXY();
    void CalcStrip();
};
