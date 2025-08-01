# Hardware Abstraction for T16 Family Variants

## Overview

The T16 family uses modular hardware design with 4x4 key/LED matrices per multiplexer. Each variant scales by adding additional multiplexers while maintaining the same core architecture. This document outlines the hardware abstraction system that handles multiple variants seamlessly.

## Hardware Architecture Analysis

### Current T16 Implementation
```cpp
// Current single-mux implementation (src/pinout.h, main.cpp:38-41)
#ifdef REV_B
Key keys[] = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6};
#else
Key keys[] = {6, 7, 15, 11, 5, 2, 14, 9, 4, 1, 13, 8, 3, 0, 12, 10};
#endif

// Single ADC multiplexer configuration
AdcChannelConfig adc_config;
adc_config.InitMux(PIN_COM, PIN_S0, PIN_S1, PIN_S2, PIN_S3);
```

### Multi-Variant Hardware Scaling
```
T16 (4x4):   1 mux  × 16 keys  = 16 keys  total
T32 (8x4):   2 mux  × 16 keys  = 32 keys  total  
T64 (16x4):  4 mux  × 16 keys  = 64 keys  total
```

## Variant-Specific Configuration Files

### 1. **Hardware Definition Templates**

```cpp
// hardware_config.hpp - Template-based hardware abstraction
template<uint8_t VARIANT_COLS, uint8_t VARIANT_ROWS>
struct HardwareVariantConfig {
    static constexpr uint8_t COLS = VARIANT_COLS;
    static constexpr uint8_t ROWS = VARIANT_ROWS;
    static constexpr uint8_t KEYS_PER_MUX = 16;
    static constexpr uint8_t TOTAL_KEYS = COLS * ROWS;
    static constexpr uint8_t MUX_COUNT = (TOTAL_KEYS + KEYS_PER_MUX - 1) / KEYS_PER_MUX;
    static constexpr uint8_t MATRICES_COUNT = MUX_COUNT; // One 4x4 matrix per mux
    
    // Hardware capabilities
    static constexpr bool HAS_TOUCH_SLIDER = true;
    static constexpr bool HAS_PHYSICAL_BUTTONS = true;
    static constexpr bool SUPPORTS_AFTERTOUCH = true;
    static constexpr bool SUPPORTS_KOALA_MODE = (VARIANT_COLS == 4 && VARIANT_ROWS == 4);
    
    // Memory requirements
    static constexpr size_t ADC_BUFFER_SIZE = MUX_COUNT * KEYS_PER_MUX * sizeof(uint16_t);
    static constexpr size_t LED_BUFFER_SIZE = TOTAL_KEYS * 3; // RGB per key
    static constexpr size_t CALIBRATION_SIZE = TOTAL_KEYS * 2 * sizeof(uint16_t); // min/max per key
};

// Specific variant definitions
using T16Config = HardwareVariantConfig<4, 4>;   // 16 keys, 1 mux
using T32Config = HardwareVariantConfig<8, 4>;   // 32 keys, 2 mux  
using T64Config = HardwareVariantConfig<16, 4>;  // 64 keys, 4 mux

// Build-time variant selection
#ifdef T16_BUILD
    using CurrentHardware = T16Config;
    #define HARDWARE_VARIANT_NAME "T16"
#elif defined(T32_BUILD)
    using CurrentHardware = T32Config;
    #define HARDWARE_VARIANT_NAME "T32"
#elif defined(T64_BUILD)
    using CurrentHardware = T64Config;
    #define HARDWARE_VARIANT_NAME "T64"
#else
    #error "No hardware variant defined"
#endif
```

### 2. **Variant-Specific Pinout Files**

```cpp
// pinout_t16.h - T16 specific pin definitions
#ifndef PINOUT_T16_H
#define PINOUT_T16_H

namespace T16Pinout {
    // Single multiplexer configuration (no shared select pins needed)
    static constexpr uint8_t MUX_COUNT = 1;
    static constexpr bool USE_SHARED_SELECT_PINS = false;
    
    struct MultiplexerConfig {
        uint8_t commonPin;                    // ADC input pin
        uint8_t enablePin;                    // Enable/CS pin (255 = always enabled)
        uint8_t selectPins[4];               // Individual select pins (or shared)
        uint8_t keyMapping[16];              // Physical mux channel → logical key
        bool useSharedSelect;                // Use shared select pins
    };
    
    static constexpr MultiplexerConfig MUX_CONFIGS[MUX_COUNT] = {
        {
            .commonPin = 8,
            .enablePin = 255,                // Always enabled (no enable pin)
            .selectPins = {4, 5, 6, 7},      // Individual select pins
            .keyMapping = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6},
            .useSharedSelect = false
        }
    };
    
    // LED matrix configuration (single WS2812B strip)
    static constexpr uint8_t LED_PIN = 1;
    static constexpr uint8_t LED_COUNT = 16;
    
    // LED index mapping (physical LED to logical key)
    static constexpr uint8_t LED_MAPPING[16] = {
        0, 1, 2, 3,      // Row 0: Keys 0-3
        4, 5, 6, 7,      // Row 1: Keys 4-7  
        8, 9, 10, 11,    // Row 2: Keys 8-11
        12, 13, 14, 15   // Row 3: Keys 12-15
    };
    
    // Touch slider configuration
    static constexpr uint8_t SLIDER_PINS[7] = {3, 9, 10, 11, 12, 13, 14};
    
    // Physical buttons
    static constexpr uint8_t PIN_TOUCH = 21;
    static constexpr uint8_t PIN_MODE = 0;
    
    // MIDI interface pins
    static constexpr uint8_t PIN_TX = 43;
    static constexpr uint8_t PIN_RX = 44;
    static constexpr uint8_t PIN_TX2 = 42;
}

#endif // PINOUT_T16_H
```

```cpp
// pinout_t32.h - T32 specific pin definitions  
#ifndef PINOUT_T32_H
#define PINOUT_T32_H

namespace T32Pinout {
    // Two multiplexer configuration with shared select pins
    static constexpr uint8_t MUX_COUNT = 2;
    
    // Shared select pins for all multiplexers
    static constexpr uint8_t SHARED_SELECT_PINS[4] = {4, 5, 6, 7}; // S0, S1, S2, S3
    static constexpr bool USE_SHARED_SELECT_PINS = true;
    
    struct MultiplexerConfig {
        uint8_t commonPin;                    // ADC input pin
        uint8_t enablePin;                    // Enable/CS pin (255 = always enabled)
        uint8_t selectPins[4];               // Individual select pins (or shared)
        uint8_t keyMapping[16];              // Physical mux channel → logical key
        bool useSharedSelect;                // Use shared select pins
    };
    
    static constexpr MultiplexerConfig MUX_CONFIGS[MUX_COUNT] = {
        // First 4x4 matrix (keys 0-15)
        {
            .commonPin = 8,
            .enablePin = 15,                 // Enable pin for mux 1
            .selectPins = {4, 5, 6, 7},      // Shared select pins
            .keyMapping = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6},
            .useSharedSelect = true
        },
        // Second 4x4 matrix (keys 16-31)  
        {
            .commonPin = 18,
            .enablePin = 16,                 // Enable pin for mux 2
            .selectPins = {4, 5, 6, 7},      // Same shared select pins
            .keyMapping = {30, 31, 29, 28, 26, 27, 24, 25, 17, 16, 19, 18, 21, 20, 23, 22},
            .useSharedSelect = true
        }
    };
    
    // LED matrix configuration (two WS2812B strips or matrix)
    static constexpr uint8_t LED_PIN = 1;
    static constexpr uint8_t LED_COUNT = 32;
    
    // LED mapping for 8x4 layout
    static constexpr uint8_t LED_MAPPING[32] = {
        // First 4x4 matrix
        0, 1, 2, 3,      // Row 0: Keys 0-3
        4, 5, 6, 7,      // Row 1: Keys 4-7
        8, 9, 10, 11,    // Row 2: Keys 8-11  
        12, 13, 14, 15,  // Row 3: Keys 12-15
        // Second 4x4 matrix
        16, 17, 18, 19,  // Row 0: Keys 16-19
        20, 21, 22, 23,  // Row 1: Keys 20-23
        24, 25, 26, 27,  // Row 2: Keys 24-27
        28, 29, 30, 31   // Row 3: Keys 28-31
    };
    
    // Touch slider (extended for larger surface)
    static constexpr uint8_t SLIDER_PINS[7] = {3, 9, 10, 11, 12, 13, 14};
    
    // Physical buttons (same as T16)
    static constexpr uint8_t PIN_TOUCH = 21;
    static constexpr uint8_t PIN_MODE = 0;
    
    // MIDI interface pins (same as T16)
    static constexpr uint8_t PIN_TX = 43;
    static constexpr uint8_t PIN_RX = 44;
    static constexpr uint8_t PIN_TX2 = 42;
}

#endif // PINOUT_T32_H
```

```cpp
// pinout_t64.h - T64 specific pin definitions
#ifndef PINOUT_T64_H  
#define PINOUT_T64_H

namespace T64Pinout {
    // Four multiplexer configuration with shared select pins
    static constexpr uint8_t MUX_COUNT = 4;
    
    // Shared select pins for all multiplexers  
    static constexpr uint8_t SHARED_SELECT_PINS[4] = {4, 5, 6, 7}; // S0, S1, S2, S3
    static constexpr bool USE_SHARED_SELECT_PINS = true;
    
    struct MultiplexerConfig {
        uint8_t commonPin;                    // ADC input pin
        uint8_t enablePin;                    // Enable/CS pin (255 = always enabled)
        uint8_t selectPins[4];               // Individual select pins (or shared)
        uint8_t keyMapping[16];              // Physical mux channel → logical key
        bool useSharedSelect;                // Use shared select pins
    };
    
    static constexpr MultiplexerConfig MUX_CONFIGS[MUX_COUNT] = {
        // First 4x4 matrix (keys 0-15)
        {
            .commonPin = 8,
            .enablePin = 15,                 // Enable pin for mux 1
            .selectPins = {4, 5, 6, 7},      // Shared select pins
            .keyMapping = {14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6},
            .useSharedSelect = true
        },
        // Second 4x4 matrix (keys 16-31)
        {
            .commonPin = 18,
            .enablePin = 16,                 // Enable pin for mux 2  
            .selectPins = {4, 5, 6, 7},      // Same shared select pins
            .keyMapping = {30, 31, 29, 28, 26, 27, 24, 25, 17, 16, 19, 18, 21, 20, 23, 22},
            .useSharedSelect = true
        },
        // Third 4x4 matrix (keys 32-47)
        {
            .commonPin = 38,
            .enablePin = 17,                 // Enable pin for mux 3
            .selectPins = {4, 5, 6, 7},      // Same shared select pins
            .keyMapping = {46, 47, 45, 44, 42, 43, 40, 41, 33, 32, 35, 34, 37, 36, 39, 38},
            .useSharedSelect = true
        },
        // Fourth 4x4 matrix (keys 48-63)
        {
            .commonPin = 48,
            .enablePin = 19,                 // Enable pin for mux 4
            .selectPins = {4, 5, 6, 7},      // Same shared select pins
            .keyMapping = {62, 63, 61, 60, 58, 59, 56, 57, 49, 48, 51, 50, 53, 52, 55, 54},
            .useSharedSelect = true
        }
    };
    
    // LED matrix configuration (likely multiple strips or matrix driver)
    static constexpr uint8_t LED_PIN = 2; // Different pin due to pin constraints
    static constexpr uint8_t LED_COUNT = 64;
    
    // LED mapping for 16x4 layout
    static constexpr uint8_t LED_MAPPING[64] = {
        // Row 0: Keys 0-15
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        // Row 1: Keys 16-31  
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        // Row 2: Keys 32-47
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        // Row 3: Keys 48-63
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
    };
    
    // Touch slider (same as smaller variants)
    static constexpr uint8_t SLIDER_PINS[7] = {3, 9, 10, 11, 12, 13, 14};
    
    // Physical buttons (same pins)
    static constexpr uint8_t PIN_TOUCH = 21;
    static constexpr uint8_t PIN_MODE = 0;
    
    // MIDI interface pins (same)
    static constexpr uint8_t PIN_TX = 43;
    static constexpr uint8_t PIN_RX = 44;  
    static constexpr uint8_t PIN_TX2 = 42;
}

#endif // PINOUT_T64_H
```

### 3. **Build-Time Pinout Selection**

```cpp
// pinout.h - Unified pinout header with build-time selection
#ifndef PINOUT_H
#define PINOUT_H

// Include variant-specific pinouts
#include "pinout_t16.h"
#include "pinout_t32.h"
#include "pinout_t64.h"

// Select pinout based on build configuration
#ifdef T16_BUILD
    namespace CurrentPinout = T16Pinout;
    using PinoutConfig = T16Pinout::MultiplexerConfig;
#elif defined(T32_BUILD)
    namespace CurrentPinout = T32Pinout;
    using PinoutConfig = T32Pinout::MultiplexerConfig;
#elif defined(T64_BUILD)  
    namespace CurrentPinout = T64Pinout;
    using PinoutConfig = T64Pinout::MultiplexerConfig;
#else
    #error "No hardware variant defined"
#endif

// Compile-time constants available to all code
static constexpr uint8_t MUX_COUNT = CurrentPinout::MUX_COUNT;
static constexpr uint8_t TOTAL_KEYS = CurrentHardware::TOTAL_KEYS;
static constexpr uint8_t LED_COUNT = CurrentPinout::LED_COUNT;

// Access to variant-specific configurations
static constexpr auto MUX_CONFIGS = CurrentPinout::MUX_CONFIGS;
static constexpr auto LED_MAPPING = CurrentPinout::LED_MAPPING;
static constexpr auto SLIDER_PINS = CurrentPinout::SLIDER_PINS;

#endif // PINOUT_H
```

## Multi-Multiplexer ADC System

### 1. **Enhanced ADC Manager**

```cpp
// multi_adc.hpp - Multi-multiplexer ADC management with shared select pins
template<uint8_t MUX_COUNT>
class MultiMuxAdcManager {
private:
    struct MuxState {
        uint8_t commonPin;
        uint8_t enablePin;                    // Enable/CS pin (255 = always enabled)
        uint8_t selectPins[4];               // Individual or shared select pins
        uint8_t currentChannel;
        AdcChannel channels[16];
        bool initialized;
        bool useSharedSelect;
        bool isEnabled;
    };
    
    MuxState multiplexers[MUX_COUNT];
    uint8_t currentMux = 0;
    uint8_t currentSharedChannel = 255;      // Track shared select pin state
    uint8_t scanSequence[TOTAL_KEYS];
    bool useSharedSelectPins = false;
    
public:
    bool initialize() {
        // Detect if using shared select pins
        if constexpr (requires { CurrentPinout::USE_SHARED_SELECT_PINS; }) {
            useSharedSelectPins = CurrentPinout::USE_SHARED_SELECT_PINS;
        }
        
        // Initialize shared select pins if used
        if (useSharedSelectPins) {
            initializeSharedSelectPins();
        }
        
        // Initialize each multiplexer
        for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
            const auto& config = CurrentPinout::MUX_CONFIGS[mux];
            
            if (!initializeMux(mux, config)) {
                log_e("Failed to initialize multiplexer %d", mux);
                return false;
            }
        }
        
        // Create optimized scan sequence
        generateScanSequence();
        
        // Start ADC scanning task
        startScanningTask();
        
        return true;
    }
    
    float getKeyValue(uint8_t keyIndex) const {
        if (keyIndex >= TOTAL_KEYS) return 0.0f;
        
        uint8_t muxIndex = keyIndex / 16;
        uint8_t channelIndex = keyIndex % 16;
        
        return multiplexers[muxIndex].channels[channelIndex].value;
    }
    
    void setCalibration(const uint16_t* minValues, const uint16_t* maxValues) {
        for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
            for (uint8_t ch = 0; ch < 16; ch++) {
                uint8_t keyIndex = mux * 16 + ch;
                if (keyIndex < TOTAL_KEYS) {
                    multiplexers[mux].channels[ch].minVal = minValues[keyIndex];
                    multiplexers[mux].channels[ch].maxVal = maxValues[keyIndex];
                }
            }
        }
    }
    
private:
    void initializeSharedSelectPins() {
        // Initialize shared select pins once
        for (uint8_t i = 0; i < 4; i++) {
            pinMode(CurrentPinout::SHARED_SELECT_PINS[i], OUTPUT);
            digitalWrite(CurrentPinout::SHARED_SELECT_PINS[i], LOW);
        }
        currentSharedChannel = 0;
        log_i("Initialized shared select pins: [%d,%d,%d,%d]", 
              CurrentPinout::SHARED_SELECT_PINS[0], CurrentPinout::SHARED_SELECT_PINS[1],
              CurrentPinout::SHARED_SELECT_PINS[2], CurrentPinout::SHARED_SELECT_PINS[3]);
    }
    
    bool initializeMux(uint8_t muxIndex, const PinoutConfig& config) {
        auto& mux = multiplexers[muxIndex];
        
        // Configure pins
        mux.commonPin = config.commonPin;
        mux.enablePin = config.enablePin;
        mux.useSharedSelect = config.useSharedSelect;
        memcpy(mux.selectPins, config.selectPins, sizeof(mux.selectPins));
        
        // Initialize pins
        pinMode(mux.commonPin, INPUT);
        
        // Initialize enable pin if used
        if (mux.enablePin != 255) {
            pinMode(mux.enablePin, OUTPUT);
            digitalWrite(mux.enablePin, HIGH); // Disable initially (active low)
            mux.isEnabled = false;
        } else {
            mux.isEnabled = true; // Always enabled
        }
        
        // Initialize individual select pins if not using shared ones
        if (!mux.useSharedSelect) {
            for (uint8_t i = 0; i < 4; i++) {
                pinMode(mux.selectPins[i], OUTPUT);
                digitalWrite(mux.selectPins[i], LOW);
            }
        }
        
        // Initialize channels
        for (uint8_t ch = 0; ch < 16; ch++) {
            mux.channels[ch] = AdcChannel{};
        }
        
        mux.currentChannel = 0;
        mux.initialized = true;
        
        log_i("Initialized mux %d: common=%d, enable=%d, shared_select=%s", 
              muxIndex, mux.commonPin, mux.enablePin, 
              mux.useSharedSelect ? "yes" : "no");
        
        return true;
    }
    
    void generateScanSequence() {
        // Interleave channels across multiplexers for better response time
        uint8_t seqIndex = 0;
        
        for (uint8_t ch = 0; ch < 16; ch++) {
            for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
                uint8_t keyIndex = mux * 16 + ch;
                if (keyIndex < TOTAL_KEYS) {
                    scanSequence[seqIndex++] = keyIndex;
                }
            }
        }
        
        log_i("Generated scan sequence for %d keys across %d multiplexers", TOTAL_KEYS, MUX_COUNT);
    }
    
    static void scanningTask(void* parameter) {
        auto* manager = static_cast<MultiMuxAdcManager*>(parameter);
        manager->runScanningLoop();
    }
    
    void runScanningLoop() {
        uint32_t sequenceIndex = 0;
        
        while (true) {
            uint8_t keyIndex = scanSequence[sequenceIndex];
            uint8_t muxIndex = keyIndex / 16;
            uint8_t channelIndex = keyIndex % 16;
            
            // Select multiplexer channel
            setMuxChannel(muxIndex, channelIndex);
            
            // Small delay for mux settling
            delayMicroseconds(10);
            
            // Read ADC value
            uint16_t rawValue = analogRead(multiplexers[muxIndex].commonPin);
            
            // Apply filtering and calibration
            processAdcReading(muxIndex, channelIndex, rawValue);
            
            // Move to next key in sequence
            sequenceIndex = (sequenceIndex + 1) % TOTAL_KEYS;
            
            // Yield to other tasks periodically
            if (sequenceIndex == 0) {
                vTaskDelay(1);
            }
        }
    }
    
    void setMuxChannel(uint8_t muxIndex, uint8_t channel) {
        auto& mux = multiplexers[muxIndex];
        
        // Enable the specific multiplexer
        enableMux(muxIndex);
        
        // Set channel selection
        if (mux.useSharedSelect) {
            // Use shared select pins
            setSharedSelectChannel(channel);
        } else {
            // Use individual select pins
            if (mux.currentChannel == channel) return; // Already selected
            
            digitalWrite(mux.selectPins[0], (channel & 0x01) ? HIGH : LOW);
            digitalWrite(mux.selectPins[1], (channel & 0x02) ? HIGH : LOW);
            digitalWrite(mux.selectPins[2], (channel & 0x04) ? HIGH : LOW);
            digitalWrite(mux.selectPins[3], (channel & 0x08) ? HIGH : LOW);
        }
        
        mux.currentChannel = channel;
    }
    
    void enableMux(uint8_t muxIndex) {
        // Disable all other multiplexers first (if using enable pins)
        for (uint8_t i = 0; i < MUX_COUNT; i++) {
            auto& otherMux = multiplexers[i];
            if (i != muxIndex && otherMux.enablePin != 255) {
                digitalWrite(otherMux.enablePin, HIGH); // Disable (active low)
                otherMux.isEnabled = false;
            }
        }
        
        // Enable the target multiplexer
        auto& mux = multiplexers[muxIndex];
        if (mux.enablePin != 255) {
            digitalWrite(mux.enablePin, LOW); // Enable (active low)
            mux.isEnabled = true;
        }
        
        currentMux = muxIndex;
    }
    
    void setSharedSelectChannel(uint8_t channel) {
        if (currentSharedChannel == channel) return; // Already selected
        
        digitalWrite(CurrentPinout::SHARED_SELECT_PINS[0], (channel & 0x01) ? HIGH : LOW);
        digitalWrite(CurrentPinout::SHARED_SELECT_PINS[1], (channel & 0x02) ? HIGH : LOW);
        digitalWrite(CurrentPinout::SHARED_SELECT_PINS[2], (channel & 0x04) ? HIGH : LOW);
        digitalWrite(CurrentPinout::SHARED_SELECT_PINS[3], (channel & 0x08) ? HIGH : LOW);
        
        currentSharedChannel = channel;
    }
    
    void processAdcReading(uint8_t muxIndex, uint8_t channelIndex, uint16_t rawValue) {
        auto& channel = multiplexers[muxIndex].channels[channelIndex];
        
        // Apply filtering
        uint16_t filteredValue = applyFilter(channel, rawValue);
        
        // Apply calibration
        float normalizedValue = (float)(filteredValue - channel.minVal) / 
                               (float)(channel.maxVal - channel.minVal);
        
        // Clamp to valid range
        channel.value = constrain(normalizedValue, 0.0f, 1.0f);
        channel.raw = rawValue;
        channel.filtered = filteredValue;
    }
    
    void startScanningTask() {
        xTaskCreatePinnedToCore(
            scanningTask,
            "MultiMuxADC",
            4096,
            this,
            2, // High priority for low latency
            nullptr,
            1  // Core 1
        );
    }
};

// Type alias for current hardware
using AdcManager = MultiMuxAdcManager<CurrentPinout::MUX_COUNT>;
```

### 2. **Key Matrix Abstraction**

```cpp
// key_matrix.hpp - Hardware-agnostic key matrix
template<uint8_t TOTAL_KEYS>
class KeyMatrix {
private:
    Key keys[TOTAL_KEYS];
    AdcManager adcManager;
    
public:
    bool initialize() {
        // Initialize ADC system
        if (!adcManager.initialize()) {
            log_e("Failed to initialize ADC manager");
            return false;
        }
        
        // Initialize keys with proper mux mapping
        for (uint8_t i = 0; i < TOTAL_KEYS; i++) {
            uint8_t muxIndex = i / 16;
            uint8_t channelIndex = i % 16;
            
            // Get logical key index from physical mapping
            uint8_t logicalIndex = CurrentPinout::MUX_CONFIGS[muxIndex].keyMapping[channelIndex];
            
            keys[i].Init(i, logicalIndex);
        }
        
        log_i("Initialized %d-key matrix with %d multiplexers", TOTAL_KEYS, CurrentPinout::MUX_COUNT);
        return true;
    }
    
    void update() {
        for (uint8_t i = 0; i < TOTAL_KEYS; i++) {
            float keyValue = adcManager.getKeyValue(i);
            keys[i].updateValue(keyValue);
        }
    }
    
    const Key& getKey(uint8_t index) const {
        return keys[constrain(index, 0, TOTAL_KEYS - 1)];
    }
    
    uint8_t getKeyCount() const {
        return TOTAL_KEYS;
    }
    
    // Get key by row/column (useful for layout-based operations)
    const Key& getKey(uint8_t col, uint8_t row) const {
        uint8_t index = row * CurrentHardware::COLS + col;
        return getKey(index);
    }
    
    // Matrix operations
    void setCalibration(const CalibrationData& calibration) {
        adcManager.setCalibration(calibration.minVal, calibration.maxVal);
    }
    
    bool performCalibration() {
        CalibrationRoutine routine;
        return routine.calibrateMatrix(*this);
    }
};

// Current hardware key matrix
using CurrentKeyMatrix = KeyMatrix<CurrentHardware::TOTAL_KEYS>;
```

## LED Matrix System

### 1. **Multi-Matrix LED Manager**

```cpp
// led_matrix.hpp - LED matrix abstraction for multiple variants
template<uint8_t LED_COUNT>
class LedMatrix {
private:
    CRGB leds[LED_COUNT];
    uint8_t brightness = 128;
    
public:
    bool initialize() {
        // Initialize LED strip/matrix based on hardware variant
        FastLED.addLeds<WS2812B, CurrentPinout::LED_PIN, GRB>(leds, LED_COUNT);
        FastLED.setBrightness(brightness);
        FastLED.clear();
        FastLED.show();
        
        log_i("Initialized LED matrix: %d LEDs on pin %d", LED_COUNT, CurrentPinout::LED_PIN);
        return true;
    }
    
    void setKeyColor(uint8_t keyIndex, CRGB color) {
        if (keyIndex >= LED_COUNT) return;
        
        // Map logical key index to physical LED index
        uint8_t ledIndex = CurrentPinout::LED_MAPPING[keyIndex];
        leds[ledIndex] = color;
    }
    
    void setKeyColor(uint8_t col, uint8_t row, CRGB color) {
        uint8_t keyIndex = row * CurrentHardware::COLS + col;
        setKeyColor(keyIndex, color);
    }
    
    void setBrightness(uint8_t newBrightness) {
        brightness = newBrightness;
        FastLED.setBrightness(brightness);
    }
    
    void clear() {
        FastLED.clear();
    }
    
    void show() {
        FastLED.show();
    }
    
    // Matrix-aware operations
    void setRow(uint8_t row, CRGB color) {
        for (uint8_t col = 0; col < CurrentHardware::COLS; col++) {
            setKeyColor(col, row, color);
        }
    }
    
    void setColumn(uint8_t col, CRGB color) {
        for (uint8_t row = 0; row < CurrentHardware::ROWS; row++) {
            setKeyColor(col, row, color);
        }
    }
    
    // Pattern support for different matrix sizes
    void setPattern(const PatternConfig& pattern) {
        switch (CurrentHardware::COLS) {
            case 4:  applyPattern4x4(pattern); break;
            case 8:  applyPattern8x4(pattern); break;
            case 16: applyPattern16x4(pattern); break;
        }
    }
    
private:
    void applyPattern4x4(const PatternConfig& pattern) {
        // T16-specific pattern application
        for (uint8_t row = 0; row < 4; row++) {
            for (uint8_t col = 0; col < 4; col++) {
                CRGB color = pattern.getColorForPosition(col / 4.0f, row / 4.0f);
                setKeyColor(col, row, color);
            }
        }
    }
    
    void applyPattern8x4(const PatternConfig& pattern) {
        // T32-specific pattern application
        for (uint8_t row = 0; row < 4; row++) {
            for (uint8_t col = 0; col < 8; col++) {
                CRGB color = pattern.getColorForPosition(col / 8.0f, row / 4.0f);
                setKeyColor(col, row, color);
            }
        }
    }
    
    void applyPattern16x4(const PatternConfig& pattern) {
        // T64-specific pattern application  
        for (uint8_t row = 0; row < 4; row++) {
            for (uint8_t col = 0; col < 16; col++) {
                CRGB color = pattern.getColorForPosition(col / 16.0f, row / 4.0f);
                setKeyColor(col, row, color);
            }
        }
    }
};

// Current hardware LED matrix
using CurrentLedMatrix = LedMatrix<CurrentPinout::LED_COUNT>;
```

### 2. **Calibration System for Multiple Matrices**

```cpp
// calibration_multi.hpp - Multi-matrix calibration
template<uint8_t TOTAL_KEYS, uint8_t MUX_COUNT>
class MultiMatrixCalibration {
private:
    struct MatrixCalibrationData {
        uint16_t minValues[16];
        uint16_t maxValues[16];
        bool isCalibrated;
        uint32_t calibrationTimestamp;
    };
    
    MatrixCalibrationData matrices[MUX_COUNT];
    CurrentLedMatrix* ledMatrix;
    CurrentKeyMatrix* keyMatrix;
    
public:
    struct CalibrationProgress {
        uint8_t currentMatrix;
        uint8_t currentKey;
        uint8_t totalProgress; // 0-100
        const char* statusMessage;
    };
    
    bool performFullCalibration(std::function<void(const CalibrationProgress&)> progressCallback) {
        CalibrationProgress progress = {};
        
        log_i("Starting calibration for %d matrices (%d keys total)", MUX_COUNT, TOTAL_KEYS);
        
        // Calibrate each 4x4 matrix individually
        for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
            progress.currentMatrix = mux;
            progress.statusMessage = "Calibrating matrix";
            progressCallback(progress);
            
            if (!calibrateMatrix(mux, [&](uint8_t key, const char* message) {
                progress.currentKey = key;
                progress.totalProgress = ((mux * 16 + key) * 100) / TOTAL_KEYS;
                progress.statusMessage = message;
                progressCallback(progress);
            })) {
                log_e("Failed to calibrate matrix %d", mux);
                return false;
            }
            
            matrices[mux].isCalibrated = true;
            matrices[mux].calibrationTimestamp = millis();
        }
        
        // Save calibration data
        saveCalibrationData();
        
        progress.totalProgress = 100;
        progress.statusMessage = "Calibration complete";
        progressCallback(progress);
        
        log_i("Multi-matrix calibration completed successfully");
        return true;
    }
    
private:
    bool calibrateMatrix(uint8_t matrixIndex, std::function<void(uint8_t, const char*)> keyProgressCallback) {
        const uint32_t CALIBRATION_DELAY = 5;
        const uint8_t PRESSES_REQUIRED = 4;
        const uint16_t PRESS_THRESHOLD_OFFSET = 1520;
        
        auto& matrix = matrices[matrixIndex];
        
        // Show which matrix is being calibrated
        showMatrixCalibrationIndicator(matrixIndex);
        
        // Calibrate minimum values for this matrix
        for (uint8_t key = 0; key < 16; key++) {
            uint8_t globalKeyIndex = matrixIndex * 16 + key;
            if (globalKeyIndex >= TOTAL_KEYS) break;
            
            keyProgressCallback(key, "Calibrating minimum values");
            matrix.minValues[key] = calibrateMinValue(matrixIndex, key);
        }
        
        // Calibrate maximum values for this matrix
        for (uint8_t key = 0; key < 16; key++) {
            uint8_t globalKeyIndex = matrixIndex * 16 + key;
            if (globalKeyIndex >= TOTAL_KEYS) break;
            
            keyProgressCallback(key, "Press key firmly multiple times");
            
            // Light up the specific key being calibrated
            showKeyCalibrationIndicator(globalKeyIndex);
            
            uint16_t pressThreshold = matrix.minValues[key] + PRESS_THRESHOLD_OFFSET;
            uint8_t pressCount = 0;
            bool wasPressed = false;
            
            while (pressCount < PRESSES_REQUIRED) {
                uint16_t keyValue = readKeyValue(matrixIndex, key);
                
                if (keyValue > pressThreshold) {
                    wasPressed = true;
                    matrix.maxValues[key] = max(matrix.maxValues[key], keyValue);
                    
                    // Show progress on slider LEDs
                    showCalibrationProgress(pressCount, PRESSES_REQUIRED);
                    
                } else if (keyValue <= pressThreshold - 300 && wasPressed) {
                    wasPressed = false;
                    pressCount++;
                }
                
                delay(CALIBRATION_DELAY);
                yield();
            }
            
            log_d("Matrix %d Key %d calibrated: min=%d, max=%d", 
                  matrixIndex, key, matrix.minValues[key], matrix.maxValues[key]);
        }
        
        return true;
    }
    
    void showMatrixCalibrationIndicator(uint8_t matrixIndex) {
        ledMatrix->clear();
        
        // Highlight the entire matrix being calibrated
        CRGB matrixColor = CRGB::Yellow;
        
        for (uint8_t key = 0; key < 16; key++) {
            uint8_t globalKeyIndex = matrixIndex * 16 + key;
            if (globalKeyIndex < TOTAL_KEYS) {
                ledMatrix->setKeyColor(globalKeyIndex, matrixColor);
            }
        }
        
        ledMatrix->show();
    }
    
    void showKeyCalibrationIndicator(uint8_t globalKeyIndex) {
        ledMatrix->clear();
        
        // Show current matrix in dim yellow
        uint8_t matrixIndex = globalKeyIndex / 16;
        for (uint8_t key = 0; key < 16; key++) {
            uint8_t idx = matrixIndex * 16 + key;
            if (idx < TOTAL_KEYS) {
                ledMatrix->setKeyColor(idx, CRGB(64, 64, 0)); // Dim yellow
            }
        }
        
        // Highlight current key in bright green
        ledMatrix->setKeyColor(globalKeyIndex, CRGB::Green);
        ledMatrix->show();
    }
    
    uint16_t readKeyValue(uint8_t matrixIndex, uint8_t keyIndex) {
        uint8_t globalKeyIndex = matrixIndex * 16 + keyIndex;
        return keyMatrix->getKey(globalKeyIndex).getRawValue();
    }
    
    void saveCalibrationData() {
        // Flatten matrix data into single arrays
        uint16_t minValues[TOTAL_KEYS];
        uint16_t maxValues[TOTAL_KEYS];
        
        for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
            for (uint8_t key = 0; key < 16; key++) {
                uint8_t globalIndex = mux * 16 + key;
                if (globalIndex < TOTAL_KEYS) {
                    minValues[globalIndex] = matrices[mux].minValues[key];
                    maxValues[globalIndex] = matrices[mux].maxValues[key];
                }
            }
        }
        
        // Save to persistent storage
        DataManager calibrationData("/calibration_data.json");
        calibrationData.SaveArray(minValues, "minVal", TOTAL_KEYS);
        calibrationData.SaveArray(maxValues, "maxVal", TOTAL_KEYS);
        
        log_i("Saved calibration data for %d keys across %d matrices", TOTAL_KEYS, MUX_COUNT);
    }
};

// Current hardware calibration system
using CurrentCalibration = MultiMatrixCalibration<CurrentHardware::TOTAL_KEYS, CurrentPinout::MUX_COUNT>;
```

## Integration with Main Application

### 1. **Updated Main.cpp**

```cpp
// main.cpp - Updated for multi-variant support
#include "pinout.h"
#include "hardware_config.hpp"

// Hardware-agnostic includes
#include "key_matrix.hpp"
#include "led_matrix.hpp"
#include "calibration_multi.hpp"

// Global hardware objects
CurrentKeyMatrix keyMatrix;
CurrentLedMatrix ledMatrix;  
CurrentCalibration calibration;

// Other existing objects (unchanged)
MidiProvider midi_provider;
DataManager config("/configuration_data.json");
TouchSlider slider;
Button t_btn(CurrentPinout::PIN_TOUCH);
Button m_btn(CurrentPinout::PIN_MODE);

void setup() {
    log_i("Initializing %s with %d keys", HARDWARE_VARIANT_NAME, CurrentHardware::TOTAL_KEYS);
    
    // Initialize MIDI first
    midi_provider.Init(CurrentPinout::PIN_RX, CurrentPinout::PIN_TX, CurrentPinout::PIN_TX2);
    
    // Initialize LED matrix
    if (!ledMatrix.initialize()) {
        log_e("Failed to initialize LED matrix");
        ESP.restart();
    }
    
    // Initialize key matrix
    if (!keyMatrix.initialize()) {
        log_e("Failed to initialize key matrix");
        ESP.restart();
    }
    
    // Initialize touch slider
    if (!slider.Init(CurrentPinout::SLIDER_PINS)) {
        ledMatrix.clear();
        ledMatrix.setRow(0, CRGB::Red); // Show error
        ledMatrix.show();
        delay(3000);
        ESP.restart();
    }
    
    // Initialize buttons
    t_btn.Init(CurrentPinout::PIN_TOUCH);
    m_btn.Init(CurrentPinout::PIN_MODE);
    
    // Load or perform calibration
    if (!loadCalibrationData()) {
        log_i("No calibration data found, starting calibration routine");
        performHardwareTest();
        performCalibrationRoutine();
    }
    
    // Load configuration
    loadConfiguration();
    
    log_i("%s initialization complete", HARDWARE_VARIANT_NAME);
}

void loop() {
    // Update all hardware
    keyMatrix.update();
    slider.Update();
    t_btn.Update();
    m_btn.Update();
    
    // Process MIDI
    midi_provider.Read();
    
    // Update LEDs and show
    updateLEDPatterns();
    ledMatrix.show();
}

bool loadCalibrationData() {
    DataManager calibrationData("/calibration_data.json");
    if (!calibrationData.Init()) return false;
    
    uint16_t minValues[CurrentHardware::TOTAL_KEYS];
    uint16_t maxValues[CurrentHardware::TOTAL_KEYS];
    
    if (!calibrationData.LoadArray(minValues, "minVal", CurrentHardware::TOTAL_KEYS) ||
        !calibrationData.LoadArray(maxValues, "maxVal", CurrentHardware::TOTAL_KEYS)) {
        return false;
    }
    
    CalibrationData calData;
    memcpy(calData.minVal, minValues, sizeof(minValues));
    memcpy(calData.maxVal, maxValues, sizeof(maxValues));
    
    keyMatrix.setCalibration(calData);
    
    log_i("Loaded calibration data for %d keys", CurrentHardware::TOTAL_KEYS);
    return true;
}

void performCalibrationRoutine() {
    calibration.performFullCalibration([](const CurrentCalibration::CalibrationProgress& progress) {
        log_i("Calibration progress: Matrix %d, Key %d, %d%% - %s",
              progress.currentMatrix, progress.currentKey, 
              progress.totalProgress, progress.statusMessage);
    });
}
```

### 2. **Build System Integration**

```ini
# platformio.ini - Updated for multi-variant builds
[platformio]
default_envs = t16_release

[env]
platform = espressif32
framework = arduino
lib_deps = 
    adafruit/Adafruit TinyUSB Library @ 3.3.0
    fastled/FastLED @ ^3.6.0
    # ... other common dependencies

[env:t16_debug]
board = unwn_s3
build_flags = 
    -DT16_BUILD
    -DDEBUG_LEVEL=5
    -DHARDWARE_VARIANT_T16
build_src_filter = 
    +<*>
    +<pinout_t16.h>

[env:t16_release]
board = unwn_s3  
build_flags =
    -DT16_BUILD
    -DDEBUG_LEVEL=0
    -DHARDWARE_VARIANT_T16
    -Os
build_src_filter =
    +<*>
    +<pinout_t16.h>

[env:t32_debug]
board = unwn_s3
build_flags =
    -DT32_BUILD
    -DDEBUG_LEVEL=5
    -DHARDWARE_VARIANT_T32
build_src_filter =
    +<*>
    +<pinout_t32.h>

[env:t32_release]
board = unwn_s3
build_flags =
    -DT32_BUILD
    -DDEBUG_LEVEL=0
    -DHARDWARE_VARIANT_T32
    -Os
build_src_filter =
    +<*>
    +<pinout_t32.h>

[env:t64_debug]
board = unwn_s3
build_flags =
    -DT64_BUILD
    -DDEBUG_LEVEL=5
    -DHARDWARE_VARIANT_T64
build_src_filter =
    +<*>
    +<pinout_t64.h>

[env:t64_release]
board = unwn_s3
build_flags =
    -DT64_BUILD  
    -DDEBUG_LEVEL=0
    -DHARDWARE_VARIANT_T64
    -Os
build_src_filter =
    +<*>
    +<pinout_t64.h>
```

## Shared Select Pin Optimizations

### 1. **Optimized Scanning Strategy**

```cpp
// Optimized scanning for shared select pins
template<uint8_t MUX_COUNT>
void MultiMuxAdcManager<MUX_COUNT>::runScanningLoop() {
    uint32_t sequenceIndex = 0;
    uint8_t lastChannel = 255;
    
    while (true) {
        uint8_t keyIndex = scanSequence[sequenceIndex];
        uint8_t muxIndex = keyIndex / 16;
        uint8_t channelIndex = keyIndex % 16;
        
        // Optimize for shared select pins: batch same channels
        if (useSharedSelectPins && channelIndex != lastChannel) {
            // Process all multiplexers for this channel at once
            processSameChannelAcrossAllMux(channelIndex);
            lastChannel = channelIndex;
        } else {
            // Standard single mux/channel processing
            enableMux(muxIndex);
            setMuxChannel(muxIndex, channelIndex);
            
            // Small delay for mux settling
            delayMicroseconds(10);
            
            // Read ADC value
            uint16_t rawValue = analogRead(multiplexers[muxIndex].commonPin);
            processAdcReading(muxIndex, channelIndex, rawValue);
        }
        
        // Move to next key in sequence
        sequenceIndex = (sequenceIndex + 1) % TOTAL_KEYS;
        
        // Yield to other tasks periodically
        if (sequenceIndex == 0) {
            vTaskDelay(1);
        }
    }
}

// Process all muxes for the same channel (more efficient with shared select)
void processSameChannelAcrossAllMux(uint8_t channel) {
    // Set shared select pins once for this channel
    setSharedSelectChannel(channel);
    
    // Read from all multiplexers with this channel
    for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
        // Enable this mux
        enableMux(mux);
        
        // Small delay for mux switching
        delayMicroseconds(5);
        
        // Read ADC value
        uint16_t rawValue = analogRead(multiplexers[mux].commonPin);
        processAdcReading(mux, channel, rawValue);
    }
}
```

### 2. **Advanced Scan Sequence for Shared Select**

```cpp
void generateOptimizedScanSequence() {
    if (!useSharedSelectPins) {
        // Use standard interleaved sequence
        generateStandardScanSequence();
        return;
    }
    
    // For shared select pins, group by channel for efficiency
    uint8_t seqIndex = 0;
    
    // Process all channels 0-15, reading from all muxes per channel
    for (uint8_t ch = 0; ch < 16; ch++) {
        for (uint8_t mux = 0; mux < MUX_COUNT; mux++) {
            uint8_t keyIndex = mux * 16 + ch;
            if (keyIndex < TOTAL_KEYS) {
                scanSequence[seqIndex++] = keyIndex;
            }
        }
    }
    
    log_i("Generated optimized scan sequence for shared select pins");
}
```

### 3. **Pin Usage Comparison**

```cpp
// Pin usage analysis for different configurations

namespace PinUsageAnalysis {
    struct PinUsage {
        uint8_t selectPins;
        uint8_t enablePins;
        uint8_t commonPins;
        uint8_t totalPins;
        uint8_t savedPins;
    };
    
    static constexpr PinUsage CONFIGURATIONS[] = {
        // T16 - Single mux, individual select
        {
            .selectPins = 4,     // S0, S1, S2, S3
            .enablePins = 0,     // No enable needed
            .commonPins = 1,     // Single ADC input
            .totalPins = 5,
            .savedPins = 0       // Baseline
        },
        
        // T32 - Two mux, individual select pins
        {
            .selectPins = 8,     // 4 pins × 2 mux
            .enablePins = 0,     // No enables (if feasible)
            .commonPins = 2,     // 2 ADC inputs
            .totalPins = 10,
            .savedPins = 0       // Standard approach
        },
        
        // T32 - Two mux, shared select + enable pins
        {
            .selectPins = 4,     // Shared S0, S1, S2, S3
            .enablePins = 2,     // Enable pins for each mux
            .commonPins = 2,     // 2 ADC inputs
            .totalPins = 8,
            .savedPins = 2       // Saved 2 pins vs individual
        },
        
        // T64 - Four mux, individual select pins
        {
            .selectPins = 16,    // 4 pins × 4 mux
            .enablePins = 0,     // No enables
            .commonPins = 4,     // 4 ADC inputs  
            .totalPins = 20,
            .savedPins = 0       // Standard approach
        },
        
        // T64 - Four mux, shared select + enable pins
        {
            .selectPins = 4,     // Shared S0, S1, S2, S3
            .enablePins = 4,     // Enable pins for each mux
            .commonPins = 4,     // 4 ADC inputs
            .totalPins = 12,
            .savedPins = 8       // Saved 8 pins vs individual!
        }
    };
    
    static constexpr uint8_t getMaxPinSavings(uint8_t variant) {
        switch (variant) {
            case 16: return CONFIGURATIONS[0].savedPins;  // 0 pins saved
            case 32: return CONFIGURATIONS[2].savedPins;  // 2 pins saved  
            case 64: return CONFIGURATIONS[4].savedPins;  // 8 pins saved
            default: return 0;
        }
    }
}
```

### 4. **Alternative Multiplexer Architectures**

```cpp
// Support for different mux topologies
namespace MuxTopology {
    enum Type {
        INDIVIDUAL_SELECT,    // Each mux has own select pins
        SHARED_SELECT,        // All mux share select pins + enable pins
        CASCADED,            // Muxes cascaded (2-level addressing)
        MATRIX_ADDRESSED     // Matrix addressing with row/col select
    };
    
    template<Type TOPOLOGY>
    struct MuxManager;
    
    // Specialization for cascaded topology (saves even more pins)
    template<>
    struct MuxManager<CASCADED> {
        // Use one mux to select which mux, then channel select
        // T64 could use: 1 mux-select + 4 channel-select = 5 pins total
        // vs 12 pins with shared select + enable
        
        static constexpr uint8_t MUX_SELECT_PINS = 2;  // Select which mux (0-3)  
        static constexpr uint8_t CHANNEL_SELECT_PINS = 4; // Select channel (0-15)
        static constexpr uint8_t TOTAL_PINS = 6 + MUX_COUNT; // + common pins
        
        void setMuxAndChannel(uint8_t muxIndex, uint8_t channel) {
            // Set mux selection pins
            digitalWrite(muxSelectPins[0], (muxIndex & 0x01) ? HIGH : LOW);
            digitalWrite(muxSelectPins[1], (muxIndex & 0x02) ? HIGH : LOW);
            
            // Set channel selection pins  
            digitalWrite(channelSelectPins[0], (channel & 0x01) ? HIGH : LOW);
            digitalWrite(channelSelectPins[1], (channel & 0x02) ? HIGH : LOW);
            digitalWrite(channelSelectPins[2], (channel & 0x04) ? HIGH : LOW);
            digitalWrite(channelSelectPins[3], (channel & 0x08) ? HIGH : LOW);
            
            // Small delay for cascaded mux settling
            delayMicroseconds(15);
        }
    };
}
```

### 5. **Build-Time Topology Selection**

```cpp
// Automatic topology selection based on variant
#ifdef T16_BUILD
    static constexpr MuxTopology::Type SELECTED_TOPOLOGY = MuxTopology::INDIVIDUAL_SELECT;
#elif defined(T32_BUILD)
    static constexpr MuxTopology::Type SELECTED_TOPOLOGY = MuxTopology::SHARED_SELECT;
#elif defined(T64_BUILD)
    // Choose most pin-efficient topology for T64
    #ifdef MINIMIZE_PIN_USAGE
        static constexpr MuxTopology::Type SELECTED_TOPOLOGY = MuxTopology::CASCADED;
    #else
        static constexpr MuxTopology::Type SELECTED_TOPOLOGY = MuxTopology::SHARED_SELECT;
    #endif
#endif

// Use appropriate manager for selected topology
using AdcManager = MuxTopology::MuxManager<SELECTED_TOPOLOGY>;
```

## Benefits of Hardware Abstraction

### **Unified Codebase**
- **Single main.cpp** works across all variants
- **Template-based scaling** automatically handles different sizes
- **Compile-time optimization** removes unused code paths
- **Type safety** prevents invalid key/LED access

### **Maintainability**
- **Variant-specific configs** in separate files
- **Clear hardware mapping** documentation
- **Easy addition** of new variants
- **Consistent API** across all hardware

### **Performance**
- **Optimized scanning** sequence across multiple muxes
- **Minimal overhead** from abstraction (compile-time resolved)
- **Efficient memory usage** scaled to actual hardware
- **Real-time performance** maintained across variants

### **Testing and Validation**
- **Matrix-aware calibration** with visual feedback
- **Hardware-specific test routines**
- **Consistent behavior** across all variants
- **Easy debugging** with variant identification

The hardware abstraction system provides a scalable foundation that maintains code simplicity while supporting the full range of T16 family variants, from 16 keys to 64 keys, with automatic adaptation to the specific hardware configuration.