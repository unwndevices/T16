#include "Adc.hpp"

#define ADC_BUFFER 512
#define ADC_NUM_BYTES 64 // 256 samples of 16 bits

// Set to 1 to log calibration table at boot and idle ADC values at ~2 Hz.
// Used to compare host-dependent key sensitivity (laptop vs Raspberry Pi).
#define DEBUG_KEY_PRESS 1

// Build with `pio run -e t32_debug --project-option="build_flags=-DSCAN_TIMING_LOG"`
// (or add SCAN_TIMING_LOG to a custom env's build_flags) to enable per-pass
// timing logs over Serial. Operationalizes T32-01 success gate (CONTEXT D12.4).
// Not enabled in default builds.
#ifdef SCAN_TIMING_LOG
  static unsigned long g_pass_start_us = 0;
  static unsigned long g_pass_count = 0;
#endif

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

void Adc::InitMuxes(const MultiplexerConfig *muxes, uint8_t mux_count)
{
    if (mux_count == 0) return;
    if (mux_count > kMaxMuxes) mux_count = kMaxMuxes;
    _mux_count = mux_count;

    // Store mux configs for ReadValues() to iterate.
    for (uint8_t m = 0; m < mux_count; ++m)
    {
        _muxes[m] = muxes[m];
    }

    analogSetAttenuation(ADC_6db);

    // Configure select pins ONCE — first mux drives them. Any subsequent mux
    // with useSharedSelect=true reuses the same lines.
    for (uint8_t i = 0; i < 4; ++i)
    {
        uint8_t pin = static_cast<uint8_t>(muxes[0].selectPins[i]);
        _mux_pin[i] = pin;
        if (pin != 0) pinMode(pin, OUTPUT);
    }

    // Configure each mux's commonPin (and any non-shared select pins for muxes
    // that explicitly do NOT share — useSharedSelect=false on a non-first mux).
    for (uint8_t m = 0; m < mux_count; ++m)
    {
        uint8_t com = static_cast<uint8_t>(muxes[m].commonPin);
        if (com != 0 && static_cast<int8_t>(muxes[m].commonPin) != -1)
        {
            pinMode(com, INPUT);
        }
        if (m > 0 && !muxes[m].useSharedSelect)
        {
            // Non-shared select pins for a secondary mux: configure them.
            for (uint8_t i = 0; i < 4; ++i)
            {
                uint8_t pin = static_cast<uint8_t>(muxes[m].selectPins[i]);
                if (pin != 0) pinMode(pin, OUTPUT);
            }
        }
        // m == 0 OR useSharedSelect=true → skip (already done above OR inherited).
    }

    // Mirror first mux pin into legacy _config._pin for Init()-era callers.
    _config._pin = static_cast<uint8_t>(muxes[0].commonPin);
    for (uint8_t i = 0; i < 4; ++i) _config._mux_pin[i] = _mux_pin[i];

    // Total channels = sum of keyMapping sizes (= TOTAL_KEYS for the variant).
    _total_channels = 0;
    for (uint8_t m = 0; m < mux_count; ++m)
    {
        _total_channels += static_cast<uint8_t>(muxes[m].keyMapping.size());
    }
    _channels.clear();
    for (uint8_t i = 0; i < _total_channels; ++i)
    {
        _channels.push_back(AdcChannel());
    }
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
#if DEBUG_KEY_PRESS
    Serial.printf("--- CAL DUMP (channels=%u) ---\n", channels);
    for (uint8_t i = 0; i < channels; i++)
    {
        Serial.printf("CAL ch=%2u min=%4u max=%4u span=%4u\n",
                      i, _channels[i].minVal, _channels[i].maxVal,
                      (uint16_t)(_channels[i].maxVal - _channels[i].minVal));
    }
    Serial.printf("--- END CAL DUMP ---\n");
#endif
}

uint16_t Adc::CalibrateMin(uint8_t chn)
{
    // Legacy single-mux wrapper: handle channel select internally and sample mux 0.
    SetMuxChannel(chn);
    return CalibrateMin(chn, 0);
}

uint16_t Adc::CalibrateMax(uint8_t chn)
{
    // Legacy single-mux wrapper: handle channel select internally and sample mux 0.
    SetMuxChannel(chn);
    return CalibrateMax(chn, 0);
}

uint16_t Adc::CalibrateMin(uint8_t logical_key, uint8_t mux_id)
{
    // NOTE: caller must already have called SetMuxChannel(channel_within_mux).
    uint8_t com_pin = (_mux_count > 0 && mux_id < _mux_count)
        ? static_cast<uint8_t>(_muxes[mux_id].commonPin)
        : _config._pin;
    uint i_v = 0;
    for (uint8_t j = 0; j < 16; j++)
    {
        i_v += analogRead(com_pin);
        delay(5);
    }
    i_v /= 16;

    _channels[logical_key].minVal = constrain(i_v, 0, 4095);
    return _channels[logical_key].minVal;
}

uint16_t Adc::CalibrateMax(uint8_t logical_key, uint8_t mux_id)
{
    // NOTE: caller must already have called SetMuxChannel(channel_within_mux).
    uint8_t com_pin = (_mux_count > 0 && mux_id < _mux_count)
        ? static_cast<uint8_t>(_muxes[mux_id].commonPin)
        : _config._pin;
    uint i_v = 0;
    for (uint8_t j = 0; j < 16; j++)
    {
        i_v += analogRead(com_pin);
        delay(5);
    }
    i_v /= 16;

    _channels[logical_key].maxVal = max((uint16_t)constrain(i_v, 0, 4095), _channels[logical_key].maxVal);
    return _channels[logical_key].maxVal;
}

uint16_t Adc::GetRawForMux(uint8_t mux_id) const
{
    if (_mux_count > 0 && mux_id < _mux_count)
    {
        return analogRead(static_cast<uint8_t>(_muxes[mux_id].commonPin));
    }
    return analogRead(_config._pin);
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
        // Yield each tick so the same-core, same-priority Arduino loopTask
        // (board JSON sets ARDUINO_RUNNING_CORE=1, matching this task's pin)
        // gets CPU. Without this the LED render loop runs at ~1 Hz on T32,
        // where _mux_count=2 doubles per-call work vs T16.
        vTaskDelay(1);
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

// Phase 12.03: Multi-mux scan + per-channel keyMapping translation.
// For each electrical channel `iterator` (0..15), set the shared S0..S3 lines
// once, then sample EVERY mux's commonPin before incrementing the iterator.
// _channels[] is indexed by *logical key index* via _muxes[m].keyMapping[ch],
// so callers see one slot per logical key regardless of physical wiring.
//
// T16 (one mux, identity keyMapping) collapses to the original behavior:
// logical_key == iterator, so no semantics change.
void Adc::ReadValues()
{
    // Set select pins once for this channel (shared across all muxes that
    // declare useSharedSelect=true).
    SetMuxChannel(iterator);

#ifdef SCAN_TIMING_LOG
    if (iterator == 0) g_pass_start_us = micros();
#endif

    // Read every mux's commonPin BEFORE incrementing the channel iterator.
    for (uint8_t m = 0; m < _mux_count; ++m)
    {
        uint8_t logical_key = _muxes[m].keyMapping[iterator];
        uint8_t com_pin = static_cast<uint8_t>(_muxes[m].commonPin);
        uint16_t rawValue = analogRead(com_pin);
        uint16_t filteredValue = ApplyFilter(rawValue, logical_key);

        _channels[logical_key].raw = rawValue;
        _channels[logical_key].filtered = filteredValue;
        _channels[logical_key].value = max(
            min(map(filteredValue,
                    _channels[logical_key].minVal,
                    _channels[logical_key].maxVal,
                    0, 4095) / 4095.0f, 1.0f), 0.0f);
    }

    iterator++;
    if (iterator >= 16) iterator = 0;   // 16 = channels per mux, NOT total

#if DEBUG_KEY_PRESS
    // Throttled idle dump — once every ~500 ms, after a full pass completes.
    if (iterator == 0)
    {
        static unsigned long last_dump_ms = 0;
        unsigned long now = millis();
        if (now - last_dump_ms >= 500)
        {
            last_dump_ms = now;
            for (uint8_t i = 0; i < _total_channels; ++i)
            {
                Serial.printf("IDLE ch=%2u raw=%4u filt=%4u min=%4u max=%4u norm=%.3f\n",
                              i,
                              _channels[i].raw,
                              _channels[i].filtered,
                              _channels[i].minVal,
                              _channels[i].maxVal,
                              _channels[i].value);
            }
        }
    }
#endif

#ifdef SCAN_TIMING_LOG
    if (iterator == 0) {
        unsigned long elapsed = micros() - g_pass_start_us;
        ++g_pass_count;
        if ((g_pass_count % 32) == 0) {
            Serial.printf("ADC pass #%lu: %lu us\n", g_pass_count, elapsed);
        }
    }
#endif
}