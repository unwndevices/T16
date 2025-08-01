# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

T16 is a MIDI controller firmware project built for ESP32-S3 microcontrollers, featuring a 4x4 pressure-sensitive key matrix with comprehensive MIDI capabilities. The project consists of two main components:

1. **Firmware** (`src/`): C++ Arduino framework code for ESP32-S3 hardware
2. **Web Configurator** (`editor-tx/`): React/Vite web application for device configuration

## Architecture

### Firmware Structure (`src/`)

- `main.cpp` - Main application loop, mode handling, and MIDI processing
- `Configuration.hpp/cpp` - Device settings management and JSON persistence
- `pinout.h` - Hardware pin definitions for different board revisions (REV_A/REV_B)
- `Libs/` - Core system libraries:
  - `Adc.hpp/cpp` - Analog input processing with calibration and filtering
  - `Keyboard.hpp` - Key state management and pressure sensing
  - `MidiProvider.hpp/cpp` - MIDI communication (USB/TRS/BLE)
  - `TouchSlider.hpp/cpp` - Capacitive touch slider handling
  - `DataManager.hpp` - JSON configuration storage on LittleFS
  - `Leds/` - LED control system with visual feedback patterns

### Key Hardware Components

- 16-key (4x4) pressure-sensitive matrix using analog multiplexer (74HC4051)
- 7-segment capacitive touch slider
- 2 physical buttons (mode/touch)
- WS2812B LED matrix (16 LEDs)
- ESP32-S3 with USB/BLE/TRS MIDI connectivity

### Operating Modes

- **Keyboard Mode**: Note-based MIDI input with velocity/aftertouch
- **XY Pad Mode**: Continuous controller mode using key matrix
- **Strips Mode**: Individual column control for expressive playing
- **Strum Mode**: Guitar-like strumming simulation
- **Quick Settings**: Configuration adjustment interface

## Development Commands

### Firmware (PlatformIO)

```bash
# Build firmware for ESP32-S3 (default environment)
pio run

# Build for specific environment
pio run -e esp32s3        # Debug build
pio run -e release        # Release build

# Upload firmware to device
pio run -t upload

# Serial monitor for debugging
pio device monitor

# Clean build files
pio run -t clean
```

### Web Configurator

```bash
cd editor-tx

# Install dependencies
npm install

# Development server with hot reload
npm run dev

# Build for production
npm run build

# Lint code
npm run lint

# Preview production build
npm run preview
```

## Configuration System

The device uses JSON-based configuration stored in LittleFS:

- `calibration_data.json` - ADC calibration values for pressure sensing
- `configuration_data.json` - User settings, MIDI mappings, and device parameters

Configuration is managed through:

- Web interface via WebMIDI API
- SysEx MIDI commands for parameter updates
- Serial interface for development/debugging

## Hardware Variants

The codebase supports multiple board revisions controlled by build flags:

- `REV_A` - Original hardware layout
- `REV_B` - Updated board revision (current default)

Key matrix mapping changes between revisions are handled in `main.cpp:38-41`.

## Important Development Notes

- Pressure sensing uses ADC with 74HC4051 multiplexer requiring careful timing
- MIDI timing is critical - avoid blocking operations in main loop
- LED patterns run on separate timing to prevent MIDI jitter
- Calibration routine must be run after hardware changes
- Configuration changes require proper JSON validation before storage
- USB MIDI, TRS MIDI, and BLE MIDI can run simultaneously

## Testing Hardware

Physical device testing requires:

1. ESP32-S3 with T16 hardware
2. MIDI monitoring software (MIDI-OX, MIDI Monitor, etc.)
3. Web browser with WebMIDI support (Chrome/Edge)
4. Proper power supply (USB or external 5V)

Test the pressure sensing calibration by running the calibration routine through the web interface or Quick Settings mode on the device.

## Development Workflow and Quality Standards

### Branch and PR Strategy

- **ALWAYS** create feature branches for any changes, never work directly on main
- Use descriptive branch names: `feature/hardware-abstraction`, `enhancement/velocity-detection`, `refactor/configuration-system`
- Create Pull Requests for ALL changes, even small ones
- Include comprehensive descriptions in PRs explaining what changed and why
- Link PRs to any related issues or planning documents
- Get code review before merging any changes

### Code Quality Rules

- **NEVER** cut corners or implement quick hacks
- **NEVER** make up functionality that doesn't exist - always verify against actual codebase
- Follow existing code patterns and conventions in the project
- Test all changes thoroughly before creating PRs
- Ensure backwards compatibility unless explicitly breaking changes are required
- Document any new APIs or significant changes

### Implementation Standards

- Build and test ALL variants (T16/T32/T64) when making hardware-related changes
- Run firmware compilation tests: `pio run -e t16_release`, `pio run -e t32_release`, `pio run -e t64_release`
- Test web configurator changes: `cd editor-tx && npm run build && npm run lint`
- Validate configuration migration paths work correctly
- Verify calibration routines function on actual hardware when possible

### Safety and Reliability

- NEVER commit changes that could brick devices or corrupt user configurations
- Always implement proper error handling and recovery mechanisms
- Use the validation systems described in data-validation-integrity.md
- Test configuration migration thoroughly before releasing
- Preserve user calibration data and settings across firmware updates

### Planning and Architecture

- Follow the architectural plans in claude/ directory - these represent carefully considered designs
- Implement changes in logical phases as outlined in the planning documents
- Maintain the modular architecture and abstraction layers
- Keep the codebase scalable for future hardware variants
- keep the phases in the claude/ directory in sync with the codebase at phase completion
