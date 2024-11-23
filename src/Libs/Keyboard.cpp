#include "Keyboard.hpp"

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return max(min((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min, out_max), out_min);
};

float Key::press_threshold = 0.18f;
float Key::rel_threshold = 0.06f;
float Key::at_threshold = 0.78f;
uint8_t Key::instances = 0;

void Key::Update(Adc *adc)
{

    value = adc->GetMux(mux_idx);
    raw = adc->GetMuxRaw(mux_idx);

    if (value > rel_threshold && state == IDLE)
    {
        log_d("Key started");
        state = STARTED;
        pressStartTime = millis();
    }
    else if (state == STARTED && value > press_threshold)
    {
        log_d("Key pressed");
        state = PRESSED;
        ulong pressTime = millis() - pressStartTime;
        velocity = fmap((float)pressTime, 55.0f, 4.0f, 0.18f, 1.0f);

        onStateChanged.Emit(idx, state);
    }
    else if (value < rel_threshold && (state == PRESSED || state == AFTERTOUCH))
    {
        log_d("Key released");
        state = RELEASED;
        onStateChanged.Emit(idx, state);
    }
    else if (value < rel_threshold && (state == STARTED || state == RELEASED))
    {
        log_d("Key idle");
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

    if (value > 0.10f)
    {
        pressure = fmap(value, 0.10f, at_threshold, 0.1f, 1.0f);
    }
    else
    {
        pressure = 0.1f;
    }
}

void Keyboard::Init(KeyboardConfig *cfg, Adc *adc)
{
    _config = *cfg;
    _adc = adc;
    GenerateLUTs();

    strips.Init(&_config);
    xypad.Init(&_config);
    log_d("Keyboard initialized");
};

void Keyboard::Update()
{
    // Record the current time before the update loop
    unsigned long currentTime = millis();

    for (uint8_t i = 0; i < _config._key_amount; i++)
    {
        _config._keys[i].Update(_adc);
    }

    // Calculate deltaTime
    deltaTime = currentTime - previousTime;
    // Update previousTime for the next iteration
    previousTime = currentTime;

    if (mode == Mode::XY_PAD)
    {
        xypad.Update(deltaTime, slew);
    }
    else if (mode == Mode::STRIPS)
    {
        strips.Update(deltaTime, slew);
    }
}

bool Keyboard::StripChanged(uint8_t chn)
{
    uint8_t group = chn / 4;
    uint8_t strip = chn % 4;
    return strips.StripChanged(_bank + group, strip);
};

void Keyboard::GenerateLUTs()
{
    for (uint8_t i = 0; i < 128; i++)
    {
        output_lut[LINEAR][i] = i;
        output_lut[EXPONENTIAL][i] = (i * i) >> 7;
        output_lut[LOGARITHMIC][i] = (uint8_t)(128.0f * log2f(1.0f + (float)i / 127.0f));
        output_lut[QUADRATIC][i] = (i * i) >> 8;
    }
}

