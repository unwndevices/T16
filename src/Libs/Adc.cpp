#include "adc.hpp"

#define ADC_BUFFER 512
#define ADC_NUM_BYTES 64 // 256 samples of 16 bits

AdcChannelConfig::AdcChannelConfig()
{
    _pin = 0;
    for (uint8_t i = 0; i < MUX_SEL_LAST; i++)
    {
        _mux_pin[i] = 0;
    }
}

void AdcChannelConfig::InitSingle(uint8_t pin)
{
    analogSetAttenuation(ADC_6db);
    _pin = pin;
    pinMode(_pin, INPUT);
}

void AdcChannelConfig::InitMux(uint8_t pin, uint8_t mux_pin_0, uint8_t mux_pin_1, uint8_t mux_pin_2, uint8_t mux_pin_3)
{
    analogSetAttenuation(ADC_6db);
    _pin = pin;
    pinMode(_pin, INPUT);
    _mux_pin[MUX_SEL_0] = mux_pin_0;
    _mux_pin[MUX_SEL_1] = mux_pin_1;
    _mux_pin[MUX_SEL_2] = mux_pin_2;
    _mux_pin[MUX_SEL_3] = mux_pin_3;
    for (uint8_t i = 0; i < MUX_SEL_LAST; i++)
    {
        if (_mux_pin[i] != 0)
        {
            pinMode(_mux_pin[i], OUTPUT);
        }
    }
}

//////////////////////////////////

Adc::Adc()
{
}

Adc::~Adc()
{
}

void Adc::Init(AdcChannelConfig *cfg, uint8_t channels)
{
    _config = *cfg;

    if (_config._mux_pin[0] == 0)
    {
        _channels.push_back(AdcChannel());
    }
    else
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            _mux_pin[i] = _config._mux_pin[i];
        }
        for (uint8_t i = 0; i < 16; i++)
        {
            _channels.push_back(AdcChannel());
        }
    }
}

void Adc::SetCalibration(uint16_t *min, uint16_t *max, uint8_t channels)
{
    for (uint8_t i = 0; i < channels; i++)
    {
        _channels[i].maxVal = max[i];
        _channels[i].minVal = min[i];
    }
}

uint16_t Adc::CalibrateMin(uint8_t chn)
{
    SetMuxChannel(chn);
    uint i_v = 0;
    for (uint8_t j = 0; j < 16; j++)
    {
        i_v += analogRead(_config._pin);
        delay(5);
    }
    i_v /= 16;

    _channels[chn].minVal = constrain(i_v, 0, 4095);
    return _channels[chn].minVal;
}

uint16_t Adc::CalibrateMax(uint8_t chn)
{
    SetMuxChannel(chn);
    uint i_v = 0;
    for (uint8_t j = 0; j < 16; j++)
    {
        i_v += analogRead(_config._pin);
        delay(5);
    }
    i_v /= 16;

    _channels[chn].maxVal = max((uint16_t)constrain(i_v, 0, 4095), _channels[chn].maxVal);
    return _channels[chn].maxVal;
}

void Adc::GetCalibration(uint16_t *min, uint16_t *max, uint8_t channels)
{
    for (uint8_t i = 0; i < channels; i++)
    {
        min[i] = _channels[i].minVal;
        max[i] = _channels[i].maxVal;
    }
}

void Adc::Start()
{
    xTaskCreatePinnedToCore(Update, "adc", 4048, this, 1, &_task, 1);
}

void Adc::Stop()
{
    vTaskDelete(_task);
}

ulong Adc::microseconds = 0;
ulong Adc::previousMicroseconds = 0;
void Adc::Update(void *parameter)
{
    Adc *adcInstance = static_cast<Adc *>(parameter);

    while (1)
    {
        adcInstance->ReadValues();
        // vTaskDelay(1);
    }
}

void Adc::SetMuxChannel(uint8_t chn) const
{
    if (chn < 16)
    {
        digitalWrite(_mux_pin[0], (chn & 0x01) ? HIGH : LOW);
        digitalWrite(_mux_pin[1], (chn & 0x02) ? HIGH : LOW);
        digitalWrite(_mux_pin[2], (chn & 0x04) ? HIGH : LOW);
        digitalWrite(_mux_pin[3], (chn & 0x08) ? HIGH : LOW);
    }
}

float Adc::Get(uint8_t chn) const
{
    return _channels[chn].value;
}

uint16_t Adc::GetRaw() const
{
    return analogRead(_config._pin);
}

uint16_t Adc::GetRaw(uint8_t chn) const
{
    return _channels[chn].raw;
}

uint16_t Adc::GetFiltered(uint8_t chn) const
{
    return _channels[chn].filtered;
}

float Adc::GetMux(uint8_t chn, uint8_t index) const
{
    return _channels[chn + index].value;
}

uint16_t Adc::AverageValue(uint8_t chn)
{
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        sum += _channels[chn].buffer[i];
    }
    return sum / 4u;
}

void Adc::SetFilterWindowSize(uint8_t size)
{
    _windowSize = size;
    if (_windowSize > 16)
        _windowSize = 16; // Limit to buffer size
}

uint16_t Adc::ApplyFilter(uint16_t newValue, uint8_t channel)
{
    AdcChannel &ch = _channels[channel];
    ch.buffer[ch.bufferIndex] = newValue;
    ch.bufferIndex = (ch.bufferIndex + 1) % _windowSize;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < _windowSize; ++i)
    {
        sum += ch.buffer[i];
    }
    return sum / _windowSize;
}

// Modify the existing ReadValues method to use the filter
void Adc::ReadValues()
{
    SetMuxChannel(iterator);
    uint16_t rawValue = analogRead(_config._pin);
    uint16_t filteredValue = ApplyFilter(rawValue, iterator);

    _channels[iterator].raw = rawValue;
    _channels[iterator].filtered = filteredValue;
    _channels[iterator].value = max(min(map(filteredValue, _channels[iterator].minVal, _channels[iterator].maxVal, 0, 4095) / 4095.0f, 1.0f), 0.0f);

    iterator++;
    if (iterator >= _channels.size())
    {
        iterator = 0;
    }
}