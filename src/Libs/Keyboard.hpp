#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP
#include "Configuration.hpp"
#include <stdint.h>
#include "adc.hpp"
#include "Signal.hpp"

#ifdef T32
#define MODULE_COUNT 2
#else
#define MODULE_COUNT 1
#endif

enum Mode
{
    KEYBOARD,
    STRUM,
    XY_PAD,
    STRIPS,
    QUICK_SETTINGS,
    CALIBRATION,
    MODE_AMOUNT
};

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

    Key(uint8_t index) : mux_idx(index)
    {
        idx = instances++;
    };

    void Update(Adc *adc);

    State GetState() const
    {
        return state;
    };

    float GetAftertouch()
    {
        return fmap(value, at_threshold, 0.95f, 0.0f, 1.0f);
    }

    uint8_t idx;
    uint8_t mux_idx;
    State state = IDLE;
    float value = 0.0f;
    float velocity = 0.0f;

    Signal<int, Key::State> onStateChanged;

    static void SetPressThreshold(float threshold)
    {
        press_threshold = threshold;
        rel_threshold = min(max(threshold - 0.07f, 0.05f), 0.10f);
    }

    static void SetATThreshold(float threshold)
    {
        at_threshold = threshold;
    }

    float GetPressure()
    {
        return pressure;
    }

    uint16_t GetRaw()
    {
        return raw;
    }

private:
    ulong pressStartTime = 0;
    uint8_t debounceTime = 10;
    float pressure = 0.0f;
    uint16_t raw = 0;

    static float press_threshold;
    static float rel_threshold;
    static float at_threshold;
    static uint8_t instances;
};

class KeyboardConfig
{
public:
    KeyboardConfig() {};

    void Init(Key *keys, uint8_t key_amount)
    {
        _keys = keys;
        _key_amount = key_amount;
    }

    uint8_t _key_amount;
    Key *_keys; // Pointer to a key array
};

typedef struct Vec2
{
    float x;
    float y;
} Vec2;

class Strip4
{
public:
    // amount represents how many concurrent sets of 4 faders we want to handle
    Strip4(uint8_t amount)
    {
        fader_sets = amount;
    }

    void Init(KeyboardConfig *cfg)
    {
        _config = cfg;
        for (uint8_t bank = 0; bank < BANK_AMT; bank++)
        {
            for (uint8_t strip = 0; strip < 4; strip++)
            {
                strip_position[bank][strip] = 0.0f;
                strip_last_position[bank][strip] = 0.0f;
            }
        }
    }

    void SetBank(uint8_t bank)
    {
        _bank = bank;
    }

    float GetStrip(uint8_t bank, uint8_t chn)
    {
        return strip_position[bank][chn];
    }

    bool StripChanged(uint8_t bank, uint8_t chn)
    {
        if (strip_position[bank][chn] != strip_last_position[bank][chn])
        {
            strip_last_position[bank][chn] = strip_position[bank][chn];
            return true;
        }
        return false;
    }

    void Update(uint16_t deltaTime, float slew)
    {
        // Update each set of faders
        for (uint8_t set = 0; set < fader_sets; set++)
        {
            // Calculate which bank this set maps to (with wrapping)
            uint8_t bank = (_bank + set) % BANK_AMT;

            // Update each strip in the current set
            for (uint8_t strip = 0; strip < 4; strip++)
            {
                log_d("set %d, strip %d", set, strip);
                float y = 0.0f;
                float total = 0.0f;
                uint8_t pressed_keys = 0;
                float weight = 0.0f;

                // Process the 4 keys for this strip
                for (uint8_t key = 0; key < 4; key++)
                {
                    // Calculate the key index in the 8x4 matrix
                    // strip represents the column (0-3 for first set, 4-7 for second set, etc)
                    // key represents the row (0-3)
                    uint8_t key_index = (strip + (set * 4)) + (key * 8); // 8 columns total

                    log_d("key %d", key_index);
                    float value = _config->_keys[key_index].value;

                    if (value < touch_threshold)
                    {
                        value = 0.0f;
                    }
                    else
                    {
                        pressed_keys++;
                    }
                    y += key * value;
                    total += value;
                }

                if (pressed_keys > 0)
                {
                    weight = total / pressed_keys;
                }
                weight = max(weight * weight, 0.03f);

                if (total > threshold)
                {
                    strip_position[bank][strip] = Adc::slew_limiter(
                        y / total,
                        strip_position[bank][strip],
                        slew * weight,
                        deltaTime);
                }
            }
        }
    }

    float strip_position[BANK_AMT][4];

private:
    KeyboardConfig *_config;
    uint8_t fader_sets; // Number of concurrent 4-fader sets to handle
    uint8_t _bank = 0;

    // Keep positions stored per bank so they persist
    float strip_last_position[BANK_AMT][4];

    static constexpr float threshold = 0.15f;
    static constexpr float touch_threshold = 0.3f;
};

class XYPad
{
public:
    XYPad(uint8_t num_sets) : pad_sets(num_sets)
    {
        // Initialize all values
        for (uint8_t i = 0; i < BANK_AMT; i++)
        {
            position[i] = Vec2{0.0f, 0.0f};
            last_position[i] = Vec2{0.0f, 0.0f};
            max_pressure[i] = 0.0f;
        }
    }

    void Init(KeyboardConfig *cfg)
    {
        _config = cfg;
    }

    void SetBank(uint8_t bank)
    {
        if (_bank == bank || bank > 3)
        {
            return;
        }
        _bank = bank;
    }

    void Update(uint16_t deltaTime, float slew)
    {
        // Update each set of XY pads
        for (uint8_t set = 0; set < pad_sets; set++)
        {
            // Calculate which bank index this set maps to
            uint8_t bank = (_bank + set) % BANK_AMT;

            float x = 0.0f;
            float y = 0.0f;
            float total = 0.0f;
            uint8_t pressed_keys = 0;
            float weight = 0.0f;

            max_pressure[bank] = 0.0f;

            // Process the 4x4 grid for this set
            for (uint8_t row = 0; row < 4; row++)
            {
                for (uint8_t col = 0; col < 4; col++)
                {
                    // Calculate the key index in the 8x4 matrix
                    // For first set: use cols 0-3, for second set: use cols 4-7
                    uint8_t key_index = (col + (set * ROWS)) + (row * COLS);

                    float value = _config->_keys[key_index].value;
                    if (value < touch_threshold)
                    {
                        value = 0.0f;
                    }
                    else
                    {
                        pressed_keys++;
                    }

                    if (value > max_pressure[bank])
                    {
                        max_pressure[bank] = fmap(value, touch_threshold, 1.0f, 0.0f, 1.0f);
                    }

                    x += (col % 4) * value; 
                    y += (row % 4) * value;
                    total += value;
                }
            }

            // Calculate the weight (average of pressed keys)
            if (pressed_keys > 0)
            {
                weight = total / pressed_keys;
            }

            // Ensure weight doesn't go below a certain threshold
            weight = max(weight * weight, 0.03f);

            if (total > threshold)
            {
                position[bank].x = Adc::slew_limiter(x / total, position[bank].x, slew * weight, deltaTime);
                position[bank].y = Adc::slew_limiter(y / total, position[bank].y, slew * weight, deltaTime);
            }
        }
    }

    float GetX(uint8_t set) const
    {
        uint8_t bank = (_bank + set) % pad_sets;
        return position[bank].x;
    }

    float GetY(uint8_t set) const
    {
        uint8_t bank = (_bank + set) % pad_sets;
        return position[bank].y;
    }

    float GetPressure(uint8_t set) const
    {
        uint8_t bank = (_bank + set) % pad_sets;
        return max_pressure[bank];
    }

    bool XChanged(uint8_t set)
    {
        uint8_t bank = (_bank + set) % pad_sets;
        if (position[bank].x != last_position[bank].x)
        {
            last_position[bank].x = position[bank].x;
            return true;
        }
        return false;
    }

    bool YChanged(uint8_t set)
    {
        uint8_t bank = (_bank + set) % pad_sets;
        if (position[bank].y != last_position[bank].y)
        {
            last_position[bank].y = position[bank].y;
            return true;
        }
        return false;
    }

private:
    KeyboardConfig *_config;
    uint8_t _bank = 0;
    uint8_t pad_sets; // Number of XY pad sets to handle

    Vec2 position[BANK_AMT];      // Array of positions for each bank and set
    Vec2 last_position[BANK_AMT]; // Array of last positions for each bank and set
    float max_pressure[BANK_AMT]; // Array of max pressures for each bank and set

    static constexpr float threshold = 0.3f;
    static constexpr float touch_threshold = 0.15f;
};

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

    Keyboard() : strips(MODULE_COUNT), xypad(MODULE_COUNT) {};
    ~Keyboard() {};

    void Init(KeyboardConfig *cfg, Adc *adc);

    void SetOnStateChanged(std::function<void(int, Key::State)> handler)
    {
        for (uint8_t i = 0; i < _config._key_amount; i++)
        {
            _config._keys[i].onStateChanged.Connect(handler);
        }
    };

    void RemoveOnStateChanged()
    {
        for (uint8_t i = 0; i < _config._key_amount; i++)
        {
            _config._keys[i].onStateChanged.DisconnectAll();
        }
    };

    void Update();

    float GetKey(uint8_t chn)
    {
        return _config._keys[chn].value;
    };

    Key::State GetKeyState(uint8_t chn)
    {
        return _config._keys[chn].GetState();
    }

    uint8_t GetVelocity(uint8_t chn)
    {
        uint8_t velocity = (uint8_t)(_config._keys[chn].velocity * 127.0f);
        return output_lut[velocityLut][velocity];
    };

    float GetAftertouch(uint8_t chn)
    {
        uint8_t aftertouch = (uint8_t)(_config._keys[chn].GetAftertouch() * 127.0f);
        return output_lut[aftertouchLut][aftertouch];
    }

    float GetX(uint8_t set = 0)
    {
        return xypad.GetX(set);
    }

    float GetY(uint8_t set = 0)
    {
        return xypad.GetY(set);
    }

    bool XChanged(uint8_t set = 0)
    {
        return xypad.XChanged(set);
    }

    bool YChanged(uint8_t set = 0)
    {
        return xypad.YChanged(set);
    }

    float GetKeyRaw(uint8_t chn)
    {
        return _config._keys[chn].GetRaw();
    }

    float GetXYPressure(uint8_t set = 0)
    {
        return xypad.GetPressure(set);
    }

    uint8_t GetPressure(uint8_t chn)
    {
        uint8_t pressure = (uint8_t)(_config._keys[chn].GetPressure() * 127.0f);
        return output_lut[velocityLut][pressure];
    }

    void SetSlew(float slew_lim)
    {
        float max = 100.0f, min = 0.5f;
        if (slew_lim < 0.3f)
        {
            this->slew = fmap(slew_lim, 0.3f, 0.0f, 8.0f, max);
        }
        else
        {
            this->slew = fmap(slew_lim, 1.0f, 0.3f, min, 8.0f);
        }
    };

    float GetStrip(uint8_t chn)
    {
        uint8_t group = chn / 4;
        uint8_t strip = chn % 4;
        return strips.strip_position[_bank + group][strip];
    };

    bool StripChanged(uint8_t chn);

    void SetBank(uint8_t bank)
    {
        if (_bank == bank || bank > 3)
        {
            return;
        }
        _bank = bank;
        strips.SetBank(_bank);
        bank_changed = true;
    };

    void SetVelocityLut(Lut lut)
    {
        velocityLut = lut;
    };

    void SetAftertouchLut(Lut lut)
    {
        aftertouchLut = lut;
    };

    void SetPressThreshold(float threshold)
    {
        Key::SetPressThreshold(threshold);
    }

    void SetATThreshold(float threshold)
    {
        Key::SetATThreshold(threshold);
    }

    void SetMode(Mode mode)
    {
        this->mode = mode;
    };

    void SetATFullRange(bool full_at)
    {
        if (!full_at)
        {
            for (uint8_t i = 0; i < _config._key_amount; i++)
            {
                _config._keys[i].SetATThreshold(0.9f);
            }
        }
        else
        {
            for (uint8_t i = 0; i < _config._key_amount; i++)
            {
                _config._keys[i].SetATThreshold(0.3f);
            }
        }
    }

    Strip4 strips; // Pointer to array of Strip4 instances
    XYPad xypad;

private:
    KeyboardConfig _config;
    Adc *_adc;

    float slew = 0.4f;

    Mode mode = Mode::KEYBOARD;

    uint8_t _bank = 0;
    bool bank_changed = false;

    unsigned long deltaTime = 0;
    unsigned long previousTime = 0;

    Lut velocityLut = LINEAR;
    Lut aftertouchLut = LINEAR;

    uint8_t output_lut[Lut::LUT_AMOUNT][128] = {0};

    void GenerateLUTs();
};

#endif // KEYBOARD_HPP
