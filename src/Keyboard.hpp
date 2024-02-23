#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <stdint.h>
#include "adc.hpp"
#include "Signal.hpp"

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return max(min((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min, out_max), out_min);
};

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

    void Update(Adc *adc)
    {
        value = adc->GetMux(0, mux_idx);

        if (value > 0.15f && state == IDLE)
        {
            state = STARTED;
            pressStartTime = millis();
        }
        else if (state == STARTED && value > 0.3f)
        {
            state = PRESSED;
            ulong pressTime = millis() - pressStartTime;
            velocity = fmap((float)pressTime, 55.0f, 3.0f, 0.04f, 1.0f);

            onStateChanged.Emit(idx, state);
        }
        else if (value < 0.25f && (state == PRESSED || state == AFTERTOUCH))
        {
            state = RELEASED;
            onStateChanged.Emit(idx, state);
        }
        else if (value < 0.15f && (state == STARTED || state == RELEASED))
        {
            state = IDLE;
        }

        else if (value > at_threshold)
        {
            if (state != AFTERTOUCH)
            {
                state = AFTERTOUCH;
            }
            onStateChanged.Emit(idx, state);
        }
    }

    State GetState() const
    {
        return state;
    };

    float GetAftertouch()
    {
        return fmap(value, at_threshold, 1.0f, 0.0f, 1.0f);
    }

    uint8_t idx;
    uint8_t mux_idx;
    State state = IDLE;
    float value = 0.0f;
    float velocity = 0.0f;

    Signal<int, Key::State> onStateChanged;

private:
    ulong pressStartTime = 0;
    uint8_t debounceTime = 10;

    float at_threshold = 0.85f;

    static uint8_t instances;
};

uint8_t Key::instances = 0;

class KeyboardConfig
{
public:
    KeyboardConfig(){};

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

class Keyboard
{
public:
    Keyboard(){};
    ~Keyboard(){};

    void Init(KeyboardConfig *cfg, Adc *adc)
    {
        _config = *cfg;
        _adc = adc;

        log_d("Keyboard initialized");
    };

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

    void Update()
    {
        // Record the current time before the update loop
        unsigned long currentTime = millis();

        for (uint8_t i = 0; i < _config._key_amount; i++)
        {
            // TODO make it for multiple muxes
            _config._keys[i].Update(_adc);
        }

        // Calculate deltaTime
        deltaTime = currentTime - previousTime;

        // Update previousTime for the next iteration
        previousTime = currentTime;

        // Call functions that require deltaTime as a parameter
        CalcXY();
        CalcStrip();
    }

    float GetKey(uint8_t chn)
    {
        return _config._keys[chn].value;
    };

    uint8_t GetVelocity(uint8_t chn)
    {
        uint8_t velocity = (uint8_t)(_config._keys[chn].velocity * 127.0f);
        return velocity;
    };

    float GetAftertouch(uint8_t chn)
    {
        return _config._keys[chn].GetAftertouch();
    }

    float GetX()
    {
        return position[_bank].x;
    };

    float GetY()
    {
        return position[_bank].y;
    };
    float GetPressure()
    {
        return max_pressure;
    };

    bool XChanged()
    {
        if (position[_bank].x != last_position[_bank].x)
        {
            last_position[_bank].x = position[_bank].x;
            return true;
        }
        return false;
    };

    bool YChanged()
    {
        if (position[_bank].y != last_position[_bank].y)
        {
            last_position[_bank].y = position[_bank].y;
            return true;
        }
        return false;
    };

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
        return strip_position[chn + (_bank * 4)];
    };

    bool StripChanged(uint8_t chn)
    {
        chn = chn + (_bank * 4);
        if (strip_position[chn] != strip_last_position[chn])
        {
            strip_last_position[chn] = strip_position[chn];
            return true;
        }
        return false;
    };

    void SetBank(uint8_t bank)
    {
        if (_bank == bank || bank > 3)
        {
            return;
        }
        _bank = bank;
        uint8_t _min = 4 * _bank;
        uint8_t _max = _min + 4;
        for (uint8_t i = _min; i < _max; i++)
        {
            strip_last_position[i] = -1.0f;
        }
    };

private:
    KeyboardConfig _config;
    Adc *_adc;
    Vec2 position[4];
    Vec2 last_position[4];
    float max_pressure = 0.0f;
    float threshold = 0.3f;
    float touch_threshold = 0.15f;
    float slew = 0.4f;

    // STRIP MODE
    float strip_position[16] = {0.0f};
    float strip_last_position[16] = {0.0f};
    uint8_t _bank = 0;

    unsigned long deltaTime = 0;
    unsigned long previousTime = 0;

    void CalcXY()
    {
        float x = 0.0f;
        float y = 0.0f;
        float total = 0.0f;
        uint8_t pressed_keys = 0;
        float weight = 0.0f;

        max_pressure = 0.0f;
        for (uint8_t i = 0; i < _config._key_amount; i++)
        {
            float value = _config._keys[i].value;
            if (value < touch_threshold)
            {
                value = 0.0f;
            }
            else
            {
                pressed_keys++;
            }
            if (value > max_pressure)
            {
                max_pressure = fmap(value, touch_threshold, 1.0f, 0.0f, 1.0f);
            }
            x += (i % 4) * value;
            y += (i / 4) * value;
            total += value;
        }
        // Calculate the weight (average of pressed keys)
        if (pressed_keys > 0)
        {
            weight = total / pressed_keys;
        }

        // Ensure weight doesn't go below a certain threshold (0.1f min)
        weight = max(weight * weight, 0.03f);

        if (total < threshold)
        {
            max_pressure = 0.0f;
            return;
        }
        else
        {
            position[_bank].x = Adc::slew_limiter(x / total, position[_bank].x, slew * weight, deltaTime);
            position[_bank].y = Adc::slew_limiter(y / total, position[_bank].y, slew * weight, deltaTime);
        }
    };

    void CalcStrip()
    {
        uint8_t _min = 4 * _bank;
        uint8_t _max = _min + 4;
        for (uint8_t i = _min; i < _max; i++)
        {
            float y = 0.0f;
            float total = 0.0f;
            uint8_t pressed_keys = 0;
            float weight = 0.0f;

            // Calculate the total value and count pressed keys

            for (uint8_t j = 0; j < 4; j++)
            {
                float value = _config._keys[i - _min + j * 4].value;
                if (value < touch_threshold)
                {
                    value = 0.0f;
                }
                else
                {
                    pressed_keys++;
                }
                y += j * value;
                total += value;
            }

            // Calculate the weight (average of pressed keys)
            if (pressed_keys > 0)
            {
                weight = total / pressed_keys;
            }

            // Ensure weight doesn't go below a certain threshold (0.1f min)
            weight = max(weight * weight, 0.03f);

            // Apply slew limiting with adjusted speed based on weight
            if (total > threshold)
            {
                strip_position[i] = Adc::slew_limiter(y / total, strip_position[i], slew * weight, deltaTime);
            }
        }
    }
};

#endif // KEYBOARD_HPP
