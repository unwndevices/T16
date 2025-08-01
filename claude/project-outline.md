# T16 Key Matrix MIDI Controller - Project Overhaul Outline

## Current Codebase Analysis

### Hardware Architecture
- **Current Model**: 16-key (4x4) matrix with ESP32-S3
- **Key Detection**: Analog multiplexer (74HC4051) with pressure-sensitive keys
- **Interfaces**: 
  - Touch slider (7 capacitive sensors)
  - 2 physical buttons (mode, touch)
  - WS2812B LED matrix (16 LEDs)
  - MIDI TRS/BLE/USB connectivity

### Existing Firmware Structure (`src/`)
```
main.cpp                 - Main application loop and mode handling
Configuration.hpp/cpp    - Settings management and persistence
pinout.h                - Hardware pin definitions (REV_A/REV_B)
Libs/
├── Adc.hpp/cpp         - Analog input with calibration and filtering
├── Keyboard.hpp        - Key state management and input processing
├── MidiProvider.hpp/cpp - MIDI communication (TRS/BLE/USB)
├── TouchSlider.hpp/cpp - Capacitive slider handling
├── Button.hpp          - Physical button debouncing
├── DataManager.hpp     - JSON-based configuration storage
├── Leds/
│   ├── LedManager.hpp  - LED control and patterns
│   └── patterns/       - Visual feedback patterns
└── Types.hpp           - Common data structures
```

### Current Capabilities
- **5 Operating Modes**: Keyboard, XY Pad, Strips, Strum, Quick Settings
- **Pressure Sensing**: Velocity (attack time) + aftertouch (sustained pressure)
- **Configuration**: 4 banks with scale/MIDI mapping, web-based editor
- **LED Feedback**: Real-time visual response to touch input
- **Calibration**: Automatic pressure calibration routine

## Proposed Overhaul Architecture

### 1. Hardware Variants Support

#### Target Models
- **T16**: 4x4 matrix (current)
- **T32**: 8x4 matrix (32 keys)  
- **T64**: 16x4 matrix (64 keys)

#### Hardware Abstraction Layer
```cpp
// Hardware configuration template
template<uint8_t COLS, uint8_t ROWS>
class HardwareConfig {
    static constexpr uint8_t KEY_COUNT = COLS * ROWS;
    static constexpr uint8_t MUX_COUNT = (KEY_COUNT + 15) / 16;
    // Variant-specific pin mappings and configurations
};

// Build-time variant selection
#ifdef T16_BUILD
    using Hardware = HardwareConfig<4, 4>;
#elif defined(T32_BUILD)
    using Hardware = HardwareConfig<8, 4>;
#elif defined(T64_BUILD)
    using Hardware = HardwareConfig<16, 4>;
#endif
```

### 2. Enhanced Pressure Sensitivity & Velocity

#### Current Issues
- Velocity based on attack time only (main.cpp:55)
- Fixed aftertouch threshold (Keyboard.hpp:137)
- Limited velocity curve options

#### Proposed Improvements
```cpp
class PressureSensor {
public:
    struct Config {
        float velocityThreshold = 0.18f;
        float aftertouchThreshold = 0.78f;
        float fullRangeAftertouch = false;  // New mode
        VelocityCurve velocityMode = HYBRID;
        AftertouchCurve aftertouchMode = LINEAR;
    };
    
    enum VelocityCurve {
        TIME_BASED,     // Current method
        PRESSURE_BASED, // Peak pressure velocity
        HYBRID,         // Combination of both
        DUAL_STAGE     // Separate attack/sustain
    };
    
    // Enhanced velocity calculation
    uint8_t CalculateVelocity(float peakPressure, uint32_t attackTime);
    uint8_t CalculateAftertouch(float currentPressure, bool fullRange);
};
```

#### Full-Height Aftertouch Mode
- **Traditional Mode**: Velocity from attack, aftertouch from sustained pressure
- **Full-Height Mode**: Fixed velocity (configurable), entire key height = aftertouch range
- **Dual Sensor Mode**: Split key sensing for attack/sustain zones

### 3. Modular Firmware Architecture

#### Core System Redesign
```
Core/
├── HardwareAbstraction.hpp  - Template-based hardware support
├── KeyMatrix.hpp           - Generic key matrix handling
├── PressureSensor.hpp       - Enhanced pressure/velocity processing
├── ConfigurationManager.hpp - Type-safe configuration system
└── SystemManager.hpp        - Mode coordination and state management

Modules/
├── MIDI/
│   ├── MidiEngine.hpp      - Protocol-agnostic MIDI processing
│   ├── USBMidi.hpp         - USB MIDI handling
│   ├── TRSMidi.hpp         - Hardware MIDI I/O
│   └── BLEMidi.hpp         - Bluetooth MIDI
├── Input/
│   ├── KeyboardMode.hpp    - Note-based input
│   ├── ControllerMode.hpp  - CC/XY pad modes
│   ├── StrumMode.hpp       - Guitar simulation
│   └── CustomMode.hpp      - User-definable modes
├── Visual/
│   ├── LEDEngine.hpp       - Hardware-agnostic LED control
│   ├── PatternEngine.hpp   - Visual feedback patterns
│   └── Themes.hpp          - Color schemes and styles
└── Communication/
    ├── WebInterface.hpp    - HTTP server for configuration
    ├── SerialInterface.hpp - Debug and development interface
    └── ProtocolHandler.hpp - Configuration protocol
```

### 4. Build System for Multi-Variant Support

#### PlatformIO Configuration
```ini
[platformio]
default_envs = t16_release

[env]
platform = espressif32
framework = arduino
lib_deps = 
    adafruit/Adafruit TinyUSB Library @ 3.3.0
    h2zero/NimBLE-Arduino@^1.4.0
    lathoub/BLE-MIDI @ ^2.2.0
    fastled/FastLED @ ^3.6.0
    bblanchon/ArduinoJson@^7.0.3

# T16 (4x4) Build Configurations
[env:t16_debug]
board = unwn_s3
build_flags = 
    -DT16_BUILD
    -DKEY_COLS=4
    -DKEY_ROWS=4
    -DDEBUG_LEVEL=5

[env:t16_release]
board = unwn_s3
build_flags = 
    -DT16_BUILD
    -DKEY_COLS=4 
    -DKEY_ROWS=4
    -DDEBUG_LEVEL=0
    -Os

# T32 (8x4) Build Configurations  
[env:t32_debug]
board = unwn_s3
build_flags = 
    -DT32_BUILD
    -DKEY_COLS=8
    -DKEY_ROWS=4
    -DDEBUG_LEVEL=5

[env:t32_release]
board = unwn_s3
build_flags = 
    -DT32_BUILD
    -DKEY_COLS=8
    -DKEY_ROWS=4
    -DDEBUG_LEVEL=0
    -Os

# T64 (16x4) Build Configurations
[env:t64_debug] 
board = unwn_s3
build_flags = 
    -DT64_BUILD
    -DKEY_COLS=16
    -DKEY_ROWS=4
    -DDEBUG_LEVEL=5

[env:t64_release]
board = unwn_s3
build_flags = 
    -DT64_BUILD
    -DKEY_COLS=16
    -DKEY_ROWS=4
    -DDEBUG_LEVEL=0
    -Os
```

#### Automated Build Pipeline
```bash
#!/bin/bash
# build-all-variants.sh
variants=("t16" "t32" "t64")
configs=("debug" "release")

for variant in "${variants[@]}"; do
    for config in "${configs[@]}"; do
        echo "Building ${variant}_${config}..."
        pio run -e "${variant}_${config}"
        
        # Copy firmware to release directory
        cp ".pio/build/${variant}_${config}/firmware.bin" \
           "releases/${variant}_v${VERSION}_${config}.bin"
    done
done
```

### 5. Enhanced Configuration Protocol

#### Current Protocol Issues
- SysEx-based but limited functionality (main.cpp:688-724)
- JSON serialization in MIDI stream
- No versioning or compatibility checks
- Limited real-time parameter updates

#### New Protocol Design
```cpp
namespace ConfigProtocol {
    enum MessageType : uint8_t {
        DEVICE_INFO_REQUEST = 0x01,
        DEVICE_INFO_RESPONSE = 0x02,
        CONFIG_DUMP_REQUEST = 0x03,
        CONFIG_DUMP_RESPONSE = 0x04,
        CONFIG_UPDATE = 0x05,
        CONFIG_UPDATE_ACK = 0x06,
        PARAMETER_UPDATE = 0x07,
        PARAMETER_UPDATE_ACK = 0x08,
        CALIBRATION_START = 0x09,
        CALIBRATION_DATA = 0x0A,
        FACTORY_RESET = 0x0B,
        FIRMWARE_UPDATE = 0x0C
    };
    
    struct Header {
        uint8_t manufacturer[3] = {0x7E, 0x7F, 0x00}; // Non-commercial
        uint8_t deviceId = 0x16; // T16 family ID
        MessageType messageType;
        uint16_t payloadLength;
        uint8_t version = 0x01;
    };
    
    // Real-time parameter updates
    struct ParameterUpdate {
        uint8_t bank;
        uint8_t parameterId;
        uint32_t value;
        uint32_t checksum;
    };
}
```

#### Protocol Features
- **Device Discovery**: Automatic device identification
- **Version Compatibility**: Firmware/software compatibility checking
- **Real-time Updates**: Live parameter tweaking without full config reload
- **Checksums**: Data integrity verification
- **Chunked Transfer**: Large configuration handling
- **Backup/Restore**: Full device state management

### 6. Web Configurator Overhaul

#### Current Web App Issues
- React-based but limited responsive design
- SysEx over WebMIDI only
- No offline configuration editing
- Limited visualization of key layouts

#### New Web Architecture
```
web-configurator/
├── src/
│   ├── core/
│   │   ├── DeviceManager.ts      - Device communication abstraction
│   │   ├── ConfigurationStore.ts - State management (Zustand/Redux)
│   │   ├── ProtocolHandler.ts    - Enhanced protocol implementation
│   │   └── PresetManager.ts      - Configuration presets/templates
│   ├── components/
│   │   ├── DeviceLayout/         - Visual key matrix representation
│   │   ├── ParameterControls/    - Real-time parameter adjustment
│   │   ├── PresetBrowser/        - Preset management interface
│   │   ├── CalibrationWizard/    - Guided calibration process
│   │   └── FirmwareUpdater/      - OTA update interface
│   ├── views/
│   │   ├── Dashboard.vue         - Device overview and quick settings
│   │   ├── KeyMapping.vue        - Visual key assignment
│   │   ├── MidiConfiguration.vue - MIDI routing and channels
│   │   ├── LightingDesign.vue    - LED patterns and themes
│   │   └── AdvancedSettings.vue  - Power user configuration
│   └── utils/
│       ├── MidiUtils.ts          - MIDI protocol utilities
│       ├── ConfigValidation.ts   - Configuration validation
│       └── DeviceProfiles.ts     - Hardware variant definitions
```

#### Enhanced Features
- **Multi-Device Support**: Manage multiple controllers simultaneously
- **Offline Editing**: Edit configurations without device connection
- **Visual Key Mapping**: Drag-and-drop note/CC assignment
- **Real-time Preview**: Live parameter adjustment with immediate feedback
- **Preset Library**: Community-shared configurations
- **Responsive Design**: Mobile-friendly interface
- **Progressive Web App**: Installable, offline-capable

#### Communication Interfaces
```typescript
interface DeviceCommunication {
    webMidi: WebMIDIInterface;      // Current method
    webSerial: WebSerialInterface;  // Direct serial for development
    webUsb: WebUSBInterface;        // Future USB interface
    tcp: TCPInterface;              // Network configuration
}

class DeviceManager {
    async detectDevices(): Promise<Device[]>;
    async connectToDevice(device: Device): Promise<Connection>;
    async syncConfiguration(config: Configuration): Promise<void>;
    async updateParameter(param: Parameter, value: any): Promise<void>;
}
```

### 7. Full-Height Aftertouch Implementation

#### Technical Approach
```cpp
class AftertouchEngine {
public:
    enum Mode {
        TRADITIONAL,    // Current: velocity + aftertouch zones
        FULL_HEIGHT,    // New: entire key height as aftertouch
        DUAL_ZONE,      // Split: attack zone + aftertouch zone
        CONTINUOUS      // Experimental: continuous pressure mapping
    };
    
    struct Config {
        Mode mode = TRADITIONAL;
        uint8_t fixedVelocity = 64;  // For full-height mode
        float deadZone = 0.05f;      // Noise rejection
        float maxPressure = 0.95f;   // Maximum useful pressure
        bool useNonLinearCurve = true;
    };
    
private:
    // Enhanced pressure processing
    float processRawPressure(float raw, uint8_t keyIndex);
    uint8_t calculateAftertouchValue(float pressure, Mode mode);
    void updatePressureHistory(uint8_t keyIndex, float pressure);
};
```

#### Calibration for Full-Height Mode
- **Baseline Calibration**: Establish unpressed state per key
- **Maximum Pressure**: Determine full-press values
- **Curve Optimization**: Non-linear response curves for musical expression
- **Temperature Compensation**: Adjust for sensor drift

### 8. Implementation Phases

#### Phase 1: Core Architecture (Weeks 1-3)
- [ ] Implement hardware abstraction layer
- [ ] Create template-based key matrix system
- [ ] Develop enhanced pressure sensing algorithms
- [ ] Basic multi-variant build system

#### Phase 2: Enhanced Features (Weeks 4-6)
- [ ] Full-height aftertouch mode implementation
- [ ] New configuration protocol
- [ ] Improved MIDI engine with better timing
- [ ] Enhanced LED visualization system

#### Phase 3: Web Interface (Weeks 7-9)
- [ ] New web configurator architecture
- [ ] Real-time parameter adjustment
- [ ] Visual key mapping interface
- [ ] Preset management system

#### Phase 4: Multi-Variant Support (Weeks 10-12)
- [ ] T32 hardware support and testing
- [ ] T64 hardware support and testing
- [ ] Cross-variant configuration compatibility
- [ ] Production build automation

#### Phase 5: Polish & Testing (Weeks 13-14)
- [ ] Comprehensive testing across all variants
- [ ] Performance optimization
- [ ] Documentation and user guides
- [ ] Community beta testing program

### 9. File Organization (Proposed)

```
firmware/
├── platformio.ini              - Multi-variant build configuration
├── src/
│   ├── main.cpp               - Application entry point
│   ├── config/
│   │   ├── hardware_t16.h     - T16-specific hardware definitions
│   │   ├── hardware_t32.h     - T32-specific hardware definitions
│   │   ├── hardware_t64.h     - T64-specific hardware definitions
│   │   └── build_config.h     - Build-time configuration selection
│   ├── core/
│   │   ├── HardwareAbstraction.hpp
│   │   ├── KeyMatrix.hpp
│   │   ├── PressureSensor.hpp
│   │   ├── ConfigurationManager.hpp
│   │   └── SystemManager.hpp
│   ├── modules/               - Feature modules
│   └── legacy/                - Current implementation (for migration)
├── tests/                     - Unit and integration tests
├── tools/
│   ├── build-all-variants.sh - Automated build script
│   ├── flash-firmware.sh     - Device programming script
│   └── test-runner.sh         - Automated testing
└── releases/                  - Built firmware binaries

web-configurator/
├── package.json
├── vite.config.js
├── src/
│   ├── main.ts               - Application entry point
│   ├── core/                 - Core functionality
│   ├── components/           - Reusable UI components
│   ├── views/                - Application pages
│   ├── stores/               - State management
│   ├── assets/               - Static resources
│   └── utils/                - Utility functions
├── public/                   - Static assets
├── tests/                    - Frontend tests
└── dist/                     - Built web application

documentation/
├── hardware/                 - Hardware specifications
├── firmware/                 - Firmware documentation  
├── web-app/                  - Web configurator docs
├── protocol/                 - Communication protocol spec
└── user-guides/              - End-user documentation
```

### 10. Key Benefits of Overhaul

#### For Users
- **Enhanced Expression**: Better pressure sensitivity and full-height aftertouch
- **More Models**: Choice of 16, 32, or 64 key configurations
- **Better Software**: More intuitive and powerful configuration interface
- **Improved Reliability**: More robust firmware with better error handling

#### For Development
- **Maintainable Code**: Clean architecture with clear separation of concerns
- **Scalable Design**: Easy to add new hardware variants and features
- **Better Testing**: Comprehensive test coverage and automated validation
- **Documentation**: Clear documentation for contributors and users

#### Technical Improvements
- **Performance**: Optimized algorithms for better response time and accuracy
- **Memory Efficiency**: Better resource utilization for complex configurations
- **Protocol Robustness**: Reliable communication with error recovery
- **Cross-Platform**: Web interface works on desktop, tablet, and mobile devices

This overhaul transforms the T16 from a single-variant device into a scalable platform that can support multiple hardware configurations while providing enhanced musical expression and a superior user experience.