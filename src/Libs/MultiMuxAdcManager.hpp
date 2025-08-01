#ifndef MULTIMUX_ADC_MANAGER_HPP
#define MULTIMUX_ADC_MANAGER_HPP

#include <Arduino.h>
#include <vector>
#include <array>
#include "../hardware_config.hpp"
#include "../pinout.h"

template<HardwareVariant variant>
class MultiMuxAdcManager {
private:
    static constexpr uint8_t NUM_MULTIPLEXERS = HardwareConfig<variant>::NUM_MULTIPLEXERS;
    static constexpr uint8_t MUX_CHANNELS = HardwareConfig<variant>::MUX_CHANNELS;
    static constexpr uint8_t TOTAL_CHANNELS = NUM_MULTIPLEXERS * MUX_CHANNELS;
    static constexpr bool SHARED_SELECT_PINS = MultiplexerConfig<variant>::SHARED_SELECT_PINS;
    
    using PinoutType = HardwarePinout<variant>;
    
public:
    struct AdcChannel {
        float value = 0.0f;
        uint16_t raw = 0;
        uint16_t filtered = 0;
        uint16_t minVal = 2000;
        uint16_t maxVal = 1000;
        uint16_t buffer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t bufferIndex = 0;
    };

    MultiMuxAdcManager() : _channels(TOTAL_CHANNELS), _windowSize(16) {
        static_assert(NUM_MULTIPLEXERS >= 1 && NUM_MULTIPLEXERS <= 4, 
                     "Number of multiplexers must be between 1 and 4");
    }

    void Init() {
        _setupSelectPins();
        _setupComPins();
        
        analogReadResolution(12);
        analogSetPinAttenuation(A0, ADC_11db);
        
        for (auto& channel : _channels) {
            channel = AdcChannel{};
        }
    }

    void SetCalibration(const uint16_t* min, const uint16_t* max) {
        for (uint8_t i = 0; i < TOTAL_CHANNELS && i < HardwareConfig<variant>::NUM_KEYS; i++) {
            _channels[i].minVal = min[i];
            _channels[i].maxVal = max[i];
        }
    }

    void GetCalibration(uint16_t* min, uint16_t* max) const {
        for (uint8_t i = 0; i < TOTAL_CHANNELS && i < HardwareConfig<variant>::NUM_KEYS; i++) {
            min[i] = _channels[i].minVal;
            max[i] = _channels[i].maxVal;
        }
    }

    void ReadAllChannels() {
        if constexpr (SHARED_SELECT_PINS) {
            _readAllChannelsSharedSelect();
        } else {
            _readAllChannelsIndividualSelect();
        }
    }

    float Get(uint8_t channelIndex) const {
        if (channelIndex >= TOTAL_CHANNELS) return 0.0f;
        return _channels[channelIndex].value;
    }

    uint16_t GetRaw(uint8_t channelIndex) const {
        if (channelIndex >= TOTAL_CHANNELS) return 0;
        return _channels[channelIndex].raw;
    }

    uint16_t GetFiltered(uint8_t channelIndex) const {
        if (channelIndex >= TOTAL_CHANNELS) return 0;
        return _channels[channelIndex].filtered;
    }

    uint16_t CalibrateMin(uint8_t channelIndex) {
        if (channelIndex >= TOTAL_CHANNELS) return 0;
        
        uint16_t reading = _readSingleChannel(channelIndex);
        if (reading < _channels[channelIndex].minVal) {
            _channels[channelIndex].minVal = reading;
        }
        return _channels[channelIndex].minVal;
    }

    uint16_t CalibrateMax(uint8_t channelIndex) {
        if (channelIndex >= TOTAL_CHANNELS) return 0;
        
        uint16_t reading = _readSingleChannel(channelIndex);
        if (reading > _channels[channelIndex].maxVal) {
            _channels[channelIndex].maxVal = reading;
        }
        return _channels[channelIndex].maxVal;
    }

    void SetFilterWindowSize(uint8_t size) {
        if (size > 16) size = 16;
        if (size < 1) size = 1;
        _windowSize = size;
        
        for (auto& channel : _channels) {
            memset(channel.buffer, 0, sizeof(channel.buffer));
            channel.bufferIndex = 0;
        }
    }

    static constexpr uint8_t GetTotalChannels() {
        return TOTAL_CHANNELS;
    }

    static constexpr uint8_t GetNumMultiplexers() {
        return NUM_MULTIPLEXERS;
    }

private:
    std::vector<AdcChannel> _channels;
    uint8_t _windowSize;

    void _setupSelectPins() {
        const auto& selectPins = PinoutType::Mux::select_pins;
        for (uint8_t i = 0; i < 3; i++) {
            pinMode(selectPins[i], OUTPUT);
            digitalWrite(selectPins[i], LOW);
        }
    }

    void _setupComPins() {
        const auto& comPins = PinoutType::Mux::com_pins;
        for (uint8_t i = 0; i < NUM_MULTIPLEXERS; i++) {
            pinMode(comPins[i], INPUT);
        }
    }

    void _setMuxChannel(uint8_t channel) {
        if (channel >= MUX_CHANNELS) return;
        
        const auto& selectPins = PinoutType::Mux::select_pins;
        digitalWrite(selectPins[0], (channel & 0x01) ? HIGH : LOW);
        digitalWrite(selectPins[1], (channel & 0x02) ? HIGH : LOW);
        digitalWrite(selectPins[2], (channel & 0x04) ? HIGH : LOW);
        
        delayMicroseconds(10);
    }

    void _readAllChannelsSharedSelect() {
        static_assert(SHARED_SELECT_PINS, "This method should only be called for shared select pins");
        
        for (uint8_t muxChannel = 0; muxChannel < MUX_CHANNELS; muxChannel++) {
            _setMuxChannel(muxChannel);
            
            const auto& comPins = PinoutType::Mux::com_pins;
            for (uint8_t muxIndex = 0; muxIndex < NUM_MULTIPLEXERS; muxIndex++) {
                uint8_t channelIndex = muxIndex * MUX_CHANNELS + muxChannel;
                if (channelIndex < TOTAL_CHANNELS) {
                    uint16_t reading = analogRead(comPins[muxIndex]);
                    _updateChannel(channelIndex, reading);
                }
            }
        }
    }

    void _readAllChannelsIndividualSelect() {
        static_assert(!SHARED_SELECT_PINS, "This method should only be called for individual select pins");
        
        for (uint8_t channelIndex = 0; channelIndex < TOTAL_CHANNELS; channelIndex++) {
            uint16_t reading = _readSingleChannel(channelIndex);
            _updateChannel(channelIndex, reading);
        }
    }

    uint16_t _readSingleChannel(uint8_t channelIndex) {
        if (channelIndex >= TOTAL_CHANNELS) return 0;
        
        uint8_t muxIndex = channelIndex / MUX_CHANNELS;
        uint8_t muxChannel = channelIndex % MUX_CHANNELS;
        
        _setMuxChannel(muxChannel);
        
        const auto& comPins = PinoutType::Mux::com_pins;
        return analogRead(comPins[muxIndex]);
    }

    void _updateChannel(uint8_t channelIndex, uint16_t rawReading) {
        if (channelIndex >= TOTAL_CHANNELS) return;
        
        auto& channel = _channels[channelIndex];
        channel.raw = rawReading;
        channel.filtered = _applyFilter(rawReading, channelIndex);
        
        uint16_t range = (channel.maxVal > channel.minVal) ? 
                        (channel.maxVal - channel.minVal) : 1;
        
        if (channel.filtered <= channel.minVal) {
            channel.value = 0.0f;
        } else if (channel.filtered >= channel.maxVal) {
            channel.value = 1.0f;
        } else {
            channel.value = float(channel.filtered - channel.minVal) / float(range);
        }
    }

    uint16_t _applyFilter(uint16_t newValue, uint8_t channelIndex) {
        if (channelIndex >= TOTAL_CHANNELS) return newValue;
        
        auto& channel = _channels[channelIndex];
        
        channel.buffer[channel.bufferIndex] = newValue;
        channel.bufferIndex = (channel.bufferIndex + 1) % _windowSize;
        
        uint32_t sum = 0;
        for (uint8_t i = 0; i < _windowSize; i++) {
            sum += channel.buffer[i];
        }
        
        return sum / _windowSize;
    }
};

#endif // MULTIMUX_ADC_MANAGER_HPP