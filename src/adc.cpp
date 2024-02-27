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
    analogSetAttenuation(ADC_0db);
    _pin = pin;
    pinMode(_pin, INPUT);
}

void AdcChannelConfig::InitMux(uint8_t pin, uint8_t mux_pin_0, uint8_t mux_pin_1, uint8_t mux_pin_2, uint8_t mux_pin_3)
{
    analogSetAttenuation(ADC_0db);
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
        _channels[i].minVal = min[i];
        _channels[i].maxVal = max[i];
    }
}

void Adc::CalibrationRoutine()
{
    log_d("Please send a 'm' to calibrate max values");
    SetMuxChannel(0);
    while (Serial.read() != 'm')
    {
        log_d("%d", analogRead(_config._pin));
        delay(300);
    }
    CalibrateMax();
    log_d("Please send a 'n' to calibrate min values");
    SetMuxChannel(0);
    while (Serial.read() != 'n')
    {
        log_d("%d", analogRead(_config._pin));
        delay(300);
    }
    CalibrateMin();
}

void Adc::CalibrateMin()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        SetMuxChannel(i);
        uint16_t i_v = 0;
        for (uint8_t j = 0; j < 16; j++)
        {
            i_v += analogRead(_config._pin);
            delay(10);
        }
        i_v /= 16;

        //_channels[i].minVal = constrain(i_v, 0, 4095);
        _channels[i].minVal = constrain(0, 0, 4095);
    }
}

void Adc::CalibrateMax()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        SetMuxChannel(i);
        uint16_t i_v = 0;
        for (uint8_t j = 0; j < 16; j++)
        {
            i_v += analogRead(_config._pin);
            delay(10);
        }
        i_v /= 16;

        _channels[i].maxVal = constrain(i_v, 0, 4095);
    }
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

ulong Adc::microseconds = 0;
ulong Adc::previousMicroseconds = 0;
void Adc::Update(void *parameter)
{
    Adc *adcInstance = static_cast<Adc *>(parameter);

    while (1)
    {
        adcInstance->ReadValues();
    }
}

void Adc::ReadValues()
{
    uint8_t value_index = 0;
    SetMuxChannel(iterator);
    uint16_t i_v = analogRead(_config._pin);
    i_v = constrain(map(i_v, _channels[iterator].minVal, _channels[iterator].maxVal, 4095, 0), 0, 4095);

    _channels[iterator].buffer[avg_iterator] = i_v;
    i_v = AverageValue(iterator);
    _channels[iterator].value = (float)i_v / 4095.0f;

    // fonepole(_channels[value_index].value, v, 0.5f);
    iterator++;
    if (iterator == 16)
    {
        avg_iterator++;
        if (avg_iterator == 4)
        {
            avg_iterator = 0;
        }
        iterator = 0;
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