#include "Keyboard.hpp"

// --- Free functions ---

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return max(min((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min, out_max), out_min);
}

// --- Key static member definitions ---

float Key::press_threshold = 0.18f;
float Key::rel_threshold = 0.06f;
float Key::at_threshold = 0.78f;
uint8_t Key::instances = 0;

// --- Key implementation ---

Key::Key(uint8_t index) : mux_idx(index)
{
    idx = instances++;
}

void Key::Update(Adc *adc)
{
    value = adc->GetMux(0, mux_idx);

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

Key::State Key::GetState() const
{
    return state;
}

float Key::GetAftertouch()
{
    return fmap(value, at_threshold, 0.95f, 0.0f, 1.0f);
}

float Key::GetPressure()
{
    return pressure;
}

void Key::SetPressThreshold(float threshold)
{
    press_threshold = threshold;
    rel_threshold = min(max(threshold - 0.07f, 0.05f), 0.10f);
}

void Key::SetATThreshold(float threshold)
{
    at_threshold = threshold;
}

// --- KeyboardConfig implementation ---

void KeyboardConfig::Init(Key *keys, uint8_t key_amount)
{
    _keys = keys;
    _key_amount = key_amount;
}

// --- Keyboard implementation ---

void Keyboard::Init(KeyboardConfig *cfg, Adc *adc)
{
    _config = *cfg;
    _adc = adc;
    GenerateLUTs();
    log_d("Keyboard initialized");
}

void Keyboard::SetOnStateChanged(std::function<void(int, Key::State)> handler)
{
    for (uint8_t i = 0; i < _config._key_amount; i++)
    {
        _config._keys[i].onStateChanged.Connect(handler);
    }
}

void Keyboard::RemoveOnStateChanged()
{
    for (uint8_t i = 0; i < _config._key_amount; i++)
    {
        _config._keys[i].onStateChanged.DisconnectAll();
    }
}

void Keyboard::Update()
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

    if (mode == XY_PAD)
    {
        CalcXY();
    }
    else if (mode == STRIPS)
    {
        CalcStrip();
    }
}

float Keyboard::GetKey(uint8_t chn)
{
    return _config._keys[chn].value;
}

Key::State Keyboard::GetKeyState(uint8_t chn)
{
    return _config._keys[chn].GetState();
}

uint8_t Keyboard::GetVelocity(uint8_t chn)
{
    uint8_t velocity = (uint8_t)(_config._keys[chn].velocity * 127.0f);
    return output_lut[velocityLut][velocity];
}

float Keyboard::GetAftertouch(uint8_t chn)
{
    uint8_t aftertouch = (uint8_t)(_config._keys[chn].GetAftertouch() * 127.0f);
    return output_lut[aftertouchLut][aftertouch];
}

float Keyboard::GetX()
{
    return position[_bank].x;
}

float Keyboard::GetY()
{
    return position[_bank].y;
}

float Keyboard::GetPressure()
{
    return max_pressure;
}

uint8_t Keyboard::GetPressure(uint8_t chn)
{
    uint8_t pressure = (uint8_t)(_config._keys[chn].GetPressure() * 127.0f);
    return output_lut[velocityLut][pressure];
}

bool Keyboard::XChanged()
{
    if (position[_bank].x != last_position[_bank].x)
    {
        last_position[_bank].x = position[_bank].x;
        return true;
    }
    return false;
}

bool Keyboard::YChanged()
{
    if (position[_bank].y != last_position[_bank].y)
    {
        last_position[_bank].y = position[_bank].y;
        return true;
    }
    return false;
}

void Keyboard::SetSlew(float slew_lim)
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
}

float Keyboard::GetStrip(uint8_t chn)
{
    return strip_position[chn + (_bank * 4)];
}

bool Keyboard::StripChanged(uint8_t chn)
{
    chn = chn + (_bank * 4);
    if (strip_position[chn] != strip_last_position[chn])
    {
        strip_last_position[chn] = strip_position[chn];
        return true;
    }
    return false;
}

void Keyboard::SetBank(uint8_t bank)
{
    if (_bank == bank || bank > 3)
    {
        return;
    }
    _bank = bank;
    bank_changed = true;
}

void Keyboard::SetVelocityLut(Lut lut)
{
    velocityLut = lut;
}

void Keyboard::SetAftertouchLut(Lut lut)
{
    aftertouchLut = lut;
}

void Keyboard::SetPressThreshold(float threshold)
{
    Key::SetPressThreshold(threshold);
}

void Keyboard::SetATThreshold(float threshold)
{
    Key::SetATThreshold(threshold);
}

void Keyboard::PlotLuts()
{
    for (uint8_t i = 0; i < Lut::LUT_AMOUNT; i++)
    {
        for (uint8_t j = 0; j < 128; j++)
        {
            Serial.println(">" + String(i) + ":" + String(output_lut[i][j]) + ":" + String(j) + "|xy");
        }
    }
}

void Keyboard::SetMode(Mode mode)
{
    this->mode = mode;
}

void Keyboard::SetATFullRange(bool full_at)
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

void Keyboard::CalcXY()
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
        return;
    }
    else
    {
        position[_bank].x = Adc::slew_limiter(x / total, position[_bank].x, slew * weight, deltaTime);
        position[_bank].y = Adc::slew_limiter(y / total, position[_bank].y, slew * weight, deltaTime);
    }
}

void Keyboard::CalcStrip()
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
