# Architecture

**Analysis Date:** 2026-04-03

## Pattern Overview

**Overall:** Monolithic single-file firmware + SPA web configurator, connected via MIDI SysEx

**Key Characteristics:**
- Firmware: All orchestration logic lives in `src/main.cpp` (~887 lines) with library classes in `src/Libs/`
- Web editor: React SPA with a single god-context (`MidiProvider`) holding all state, config, and communication
- Communication: WebMIDI SysEx for config transfer; JSON serialized over SysEx payloads
- No structured command protocol -- raw byte matching with magic numbers (e.g., `data[2] == 127 && data[3] == 7`)

## Layers

**Hardware Abstraction (`src/Libs/`):**
- Purpose: Encapsulate hardware peripherals (ADC, capacitive touch, buttons, LEDs, MIDI)
- Location: `src/Libs/`
- Contains: `Adc.hpp/cpp`, `Keyboard.hpp`, `TouchSlider.hpp/cpp`, `Button.hpp`, `MidiProvider.hpp/cpp`
- Depends on: Arduino, FastLED, MIDI libraries, FreeRTOS
- Used by: `src/main.cpp` directly

**LED Patterns (`src/Libs/Leds/`):**
- Purpose: Visual feedback patterns for the 4x4 LED matrix + 7-LED slider strip
- Location: `src/Libs/Leds/`
- Contains: `LedManager.hpp`, `patterns/Pattern.hpp` (base class), 8 pattern implementations
- Depends on: FastLED, `src/pinout.h`
- Used by: `src/main.cpp` via `LedManager` instance
- Pattern: Strategy pattern -- `Pattern` is abstract base, `LedManager` holds `currentPattern` and `nextPattern` pointers

**Configuration (`src/Configuration.hpp/cpp`):**
- Purpose: Define data structures and serialize/deserialize device configuration
- Location: `src/Configuration.hpp`, `src/Configuration.cpp`
- Contains: Global structs (`ConfigurationData`, `KeyModeData`, `ControlChangeData`, `Parameters`, `QuickSettingsData`), save/load functions
- Depends on: `DataManager`, ArduinoJson
- Used by: `src/main.cpp`, web editor (mirrors config structure in JS)

**Persistence (`src/Libs/DataManager.hpp`):**
- Purpose: JSON-based key-value storage on LittleFS
- Location: `src/Libs/DataManager.hpp`
- Contains: Template methods for saving/loading variables and arrays
- Depends on: ArduinoJson, LittleFS, StreamUtils
- Used by: `src/Configuration.cpp`

**Application Logic (`src/main.cpp`):**
- Purpose: ALL application orchestration -- mode switching, input processing, MIDI routing, slider behavior
- Location: `src/main.cpp`
- Contains: `setup()`, `loop()`, all `Process*()` callbacks, `ApplyConfiguration()`, `CalibrationRoutine()`
- Depends on: Everything
- Used by: Arduino runtime

**Web Configurator (`editor-tx/`):**
- Purpose: Browser-based configuration editor for the T16
- Location: `editor-tx/src/`
- Contains: React SPA with Chakra UI, WebMIDI integration, firmware upload
- Depends on: React 18, Chakra UI v2, WebMidi.js, react-router-dom, esptool-js
- Used by: End users via hosted web app

## Data Flow

**Configuration Sync (Device <-> Editor):**

1. User connects via WebMIDI in `editor-tx/src/components/MidiProvider.jsx`
2. Editor sends SysEx identity request `[0x7e, 0x7f, 0x06, 0x01]` and config dump request `[0x7e, 0x7f, 0x07, 0x03]`
3. Firmware `ProcessSysEx()` in `src/main.cpp` receives request, serializes config JSON via `DataManager::SerializeToBuffer()`
4. Editor deserializes JSON from SysEx bytes, populates `config` state in MidiContext
5. User edits config in UI, then sends back via SysEx `[0x7e, 127, 7, 5, ...jsonBytes]`
6. Firmware `ProcessSysEx()` deserializes, calls `LoadConfiguration()` then `ApplyConfiguration()`

**Input Processing (Keyboard mode):**

1. `loop()` calls `keyboard.Update()` which polls all 16 keys via ADC mux
2. Each `Key::Update()` reads FSR value, runs state machine (IDLE -> STARTED -> PRESSED -> AFTERTOUCH -> RELEASED)
3. State change emits via `Signal<int, Key::State>` to registered callback (e.g., `ProcessKey()`)
4. `ProcessKey()` maps key index to MIDI note via `note_map[]`, sends via `midi_provider.SendNoteOn()`
5. `MidiProvider` fans out to USB MIDI, BLE MIDI, and/or TRS MIDI based on config flags

**State Management:**
- Firmware: Global structs (`cfg`, `kb_cfg[]`, `cc_cfg[]`, `parameters`) -- no encapsulation
- Web: Single React Context (`MidiContext`) holds everything: connection state, config, sync status, MIDI I/O
- Config mirroring: The web editor maintains `config` (desired) and `currentConfig` (on-device) to track sync status

## Key Abstractions

**Signal (Event System):**
- Purpose: Qt-style signal/slot for decoupled event handling
- Location: `src/Libs/Signal.hpp`
- Pattern: Template class `Signal<Args...>` with `Connect()`, `Disconnect()`, `Emit()`
- Used by: `Key::onStateChanged`, `Button::onStateChanged`, `TouchSlider::onSensorTouched`

**Pattern (LED Animations):**
- Purpose: Abstract base for LED rendering strategies
- Location: `src/Libs/Leds/patterns/Pattern.hpp`
- Pattern: Strategy pattern with virtual `RunPattern()`, `SetPosition()`, etc.
- Implementations: `TouchBlur`, `NoBlur`, `Strips`, `Strum`, `QuickSettings`, `WaveTransition`, `Sea`, `Sea2`, `Droplet`

**MidiProvider (Multi-transport MIDI):**
- Purpose: Unified MIDI output across USB, BLE, and TRS serial
- Location: `src/Libs/MidiProvider.hpp/cpp`
- Pattern: Facade over three MIDI interfaces, with boolean flags to enable/disable each transport
- Note pools: `note_pool[16]`, `chord_pool[5]`, `strum_pool[7]` track active notes for proper NoteOff

**DataManager (Persistence):**
- Purpose: JSON file-based storage on LittleFS flash filesystem
- Location: `src/Libs/DataManager.hpp`
- Pattern: Template methods wrapping ArduinoJson serialization
- Files: `/calibration_data.json`, `/configuration_data.json`

## Entry Points

**Firmware (`src/main.cpp`):**
- Location: `src/main.cpp`
- `setup()` (line 726): Hardware init, calibration check, config load, input binding
- `loop()` (line 811): Main polling loop -- MIDI read, button/slider/keyboard update, mode-specific processing, LED render
- Triggers: Arduino runtime calls `setup()` once, `loop()` continuously

**Web Editor (`editor-tx/src/main.jsx`):**
- Location: `editor-tx/src/main.jsx`
- Triggers: Browser load
- Responsibilities: Mount React app with ChakraProvider theme and MidiProvider context

**Web Router (`editor-tx/src/App.jsx`):**
- Location: `editor-tx/src/App.jsx`
- Routes: `/` (Dashboard), `/upload` (firmware update), `/manual` (QuickStart guide)

## Error Handling

**Strategy:** Minimal -- mostly `log_d()` debug prints on firmware, `console.error()` + toast notifications on web

**Firmware Patterns:**
- Slider init failure: Show red LEDs, delay 3 seconds, restart ESP32 (`src/main.cpp` line 748)
- Missing calibration data: Run calibration routine, restart (`src/main.cpp` line 763)
- Config version mismatch: Overwrite with defaults (`src/main.cpp` line 786)
- No structured error propagation -- errors handled locally where they occur

**Web Patterns:**
- MIDI connection failures: Toast notification via Chakra UI (`editor-tx/src/components/MidiProvider.jsx` line 248)
- SysEx parse failures: Toast + set `isConnected = false` (`MidiProvider.jsx` line 330)
- No error boundaries or retry logic

## Cross-Cutting Concerns

**Logging:** Arduino `log_d()` macros (debug level controlled by `CORE_DEBUG_LEVEL` build flag). Web uses `console.log/error`.

**Validation:** None on firmware side for incoming SysEx config data -- directly deserialized and applied. Web side has no input validation beyond Chakra UI component constraints.

**Authentication:** None -- open MIDI connection, no pairing or auth for config changes.

## Comparison with Reference Repos

**vs. eisei (`/home/unwn/git/unwn/eisei/daisy/`):**
- eisei has structured command handling split into separate files: `serial_commands/DiagnosticCommands.cpp`, `ParameterCommands.cpp`, `PresetCommands.cpp`, `SlotCommands.cpp`
- eisei separates concerns: `CommService`, `CalibrationManager`, `PresetManager`, `FirmwareUpdateService`, `EiseiEngine` as distinct service classes
- eisei uses a proper HAL class (`DaisyEisei`) wrapping the Daisy seed hardware
- T16 has all command processing inline in `ProcessSysEx()` with magic byte matching
- T16 has no service abstraction -- all logic in `main.cpp` free functions and globals

**vs. DROP (`/home/unwn/git/DROP/src/`):**
- DROP uses TypeScript with clear separation: `services/`, `hooks/`, `contexts/`, `components/`, `tools/`, `types/`, `utils/`
- DROP has a `design-system/` with reusable UI primitives (Button, Card, Modal, etc.)
- DROP separates contexts from components (dedicated `contexts/` directory)
- DROP organizes components by feature domain (DeviceBridge, Editor, PixelArt, Visualizer) with barrel exports (`index.ts`)
- T16 web editor has flat `components/` directory mixing providers, UI cards, and domain components
- T16 has no hooks directory, no services layer, no type definitions

**Key Gaps for Refactoring:**
1. Firmware needs service extraction from `main.cpp` (following eisei's pattern)
2. Firmware needs structured command protocol (following eisei's `serial_commands/` pattern)
3. Web editor needs `contexts/`, `hooks/`, `services/` directories (following DROP's pattern)
4. Web editor needs component organization by feature domain (following DROP's pattern)
5. MidiProvider context is a 500-line god-object that should be split into connection, config, and sync concerns

---

*Architecture analysis: 2026-04-03*
