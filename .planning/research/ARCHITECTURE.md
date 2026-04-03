# Architecture Research

**Domain:** ESP32 MIDI controller firmware + web configurator
**Researched:** 2026-04-03
**Confidence:** HIGH (based on working eisei and DROP reference implementations in the same ecosystem)

## System Overview

The T16 is a two-part system: firmware running on ESP32-S3 and a browser-based configurator. They communicate over MIDI SysEx. The refactored architecture extracts services from the monolithic firmware and restructures the web app into separated concerns.

```
                            MIDI SysEx over USB/BLE
 ┌────────────────────────┐          ║          ┌────────────────────────┐
 │      ESP32-S3 MCU      │ <═══════╬═════════> │   Web Configurator    │
 │                        │          ║          │                        │
 │  ┌──────────────────┐  │          ║          │  ┌──────────────────┐  │
 │  │   CommandRouter  │──│──────────╝          │  │   SysExService   │──│──┐
 │  └────────┬─────────┘  │                     │  └────────┬─────────┘  │  │
 │           │             │                     │           │            │  │
 │  ┌────────┴─────────┐  │                     │  ┌────────┴─────────┐  │  │
 │  │  ConfigManager   │  │                     │  │   ConfigContext  │  │  │
 │  └────────┬─────────┘  │                     │  └────────┬─────────┘  │  │
 │           │             │                     │           │            │  │
 │  ┌────────┴─────────┐  │                     │  ┌────────┴─────────┐  │  │
 │  │   ModeManager    │  │                     │  │  ConnectionCtx   │  │  │
 │  └────────┬─────────┘  │                     │  └──────────────────┘  │  │
 │           │             │                     │                        │  │
 │  ┌────────┴─────────┐  │                     │  ┌──────────────────┐  │  │
 │  │  InputProcessor  │  │                     │  │  Feature Pages   │  │  │
 │  └────────┬─────────┘  │                     │  │  (Keyboard, CC,  │  │  │
 │           │             │                     │  │   Scales, etc.)  │  │  │
 │  ┌────────┴─────────┐  │                     │  └──────────────────┘  │  │
 │  │   HAL Layer      │  │                     │                        │  │
 │  │  (Keyboard, ADC, │  │                     │  ┌──────────────────┐  │  │
 │  │  Slider, LEDs,   │  │                     │  │  Design System   │──│──┘
 │  │  MidiProvider)   │  │                     │  │  (Tokens, UI)    │  │
 │  └──────────────────┘  │                     │  └──────────────────┘  │
 └────────────────────────┘                     └────────────────────────┘
```

## Firmware Architecture

### Component Boundaries

| Component | Responsibility | Communicates With | Location |
|-----------|---------------|-------------------|----------|
| `main.cpp` | Init + loop (slim orchestrator) | All services | `src/main.cpp` |
| `CommandRouter` | Parse SysEx, dispatch to handlers | ConfigManager, ModeManager | `src/Services/CommandRouter.hpp/cpp` |
| `ConfigManager` | Load/save/apply config, per-param updates | DataManager, HAL | `src/Services/ConfigManager.hpp/cpp` |
| `ModeManager` | Mode switching, slider mode state | InputProcessor, LedManager | `src/Services/ModeManager.hpp/cpp` |
| `InputProcessor` | Key/slider/button callback routing | MidiProvider, LedManager, ModeManager | `src/Services/InputProcessor.hpp/cpp` |
| `MidiProvider` | Multi-transport MIDI I/O (USB, BLE, TRS) | External MIDI devices | `src/Libs/MidiProvider.hpp/cpp` |
| `Keyboard` | 16-key FSR state machine, velocity, aftertouch | ADC, Signal system | `src/Libs/Keyboard.hpp` |
| `TouchSlider` | 7-sensor capacitive slider | Pinout | `src/Libs/TouchSlider.hpp/cpp` |
| `LedManager` | LED matrix + slider strip rendering | Pattern instances, FastLED | `src/Libs/Leds/LedManager.hpp` |
| `DataManager` | LittleFS JSON persistence | ArduinoJson, LittleFS | `src/Libs/DataManager.hpp` |
| `Configuration` | Data structures (ConfigurationData, KeyModeData, etc.) | Used by ConfigManager | `src/Configuration.hpp/cpp` |

### Recommended Firmware Structure

```
src/
├── main.cpp                    # Slim: init services, wire signals, run loop
├── pinout.h                    # Hardware pin definitions (REV_A/REV_B)
├── Configuration.hpp/cpp       # Data structures only (no save/load logic)
├── Scales.hpp                  # Scale definitions + note mapping
├── Services/                   # Extracted from main.cpp
│   ├── CommandRouter.hpp/cpp   # SysEx protocol: parse commands, dispatch handlers
│   ├── ConfigManager.hpp/cpp   # Config load/save/apply, per-parameter updates
│   ├── ModeManager.hpp/cpp     # Mode switching, slider mode state machine
│   └── InputProcessor.hpp/cpp  # Key/slider/button callbacks, MIDI output routing
├── Libs/                       # Hardware abstraction (minimal changes)
│   ├── Adc.hpp/cpp
│   ├── Button.hpp
│   ├── DataManager.hpp
│   ├── Keyboard.hpp
│   ├── MidiProvider.hpp/cpp
│   ├── Signal.hpp
│   ├── Timer.hpp
│   ├── TouchSlider.hpp/cpp
│   └── Types.hpp
└── Libs/Leds/                  # LED subsystem (keep as-is)
    ├── LedManager.hpp
    └── patterns/
```

### Structure Rationale

- **Services/**: Following eisei's pattern of separating orchestration logic from hardware. In eisei, `CommService`, `PresetManager`, `CalibrationManager` are each responsible for one domain. T16 needs fewer services because the product is simpler, but the same principle applies.
- **Libs/**: Stays as-is. These are hardware abstractions that are already well-bounded. The Signal pattern, ADC, Keyboard, TouchSlider classes are clean and testable.
- **Configuration.hpp**: Becomes data-only. Save/load/apply logic moves into ConfigManager. This mirrors how eisei separates SharedData.hpp (structures) from PresetManager (persistence logic).

## Web Configurator Architecture

### Component Boundaries

| Component | Responsibility | Communicates With | Location |
|-----------|---------------|-------------------|----------|
| `SysExService` | Encode/decode SysEx messages, per-param protocol | ConnectionContext | `src/services/sysex.ts` |
| `ConnectionContext` | WebMIDI connection lifecycle, device discovery | SysExService | `src/contexts/ConnectionContext.tsx` |
| `ConfigContext` | Device config state, dirty tracking, sync status | SysExService, ConnectionContext | `src/contexts/ConfigContext.tsx` |
| `useConfig` | Hook for components to read/write config values | ConfigContext | `src/hooks/useConfig.ts` |
| `useConnection` | Hook for connection state and actions | ConnectionContext | `src/hooks/useConnection.ts` |
| Feature pages | Keyboard, CC, Scales, Settings tabs | useConfig, useConnection | `src/pages/` |
| Design system | Tokens, Card, Select, Slider, Toggle primitives | None (leaf components) | `src/design-system/` |
| Layout components | NavBar, Footer, RootLayout | Router | `src/components/Layout/` |

### Recommended Web Structure

```
editor-tx/src/
├── main.tsx                    # React mount + providers
├── App.tsx                     # Router definition
├── contexts/                   # React contexts (state containers)
│   ├── ConnectionContext.tsx    # WebMIDI lifecycle, input/output refs
│   └── ConfigContext.tsx       # Device config, sync status, dirty tracking
├── hooks/                      # Custom hooks (state access + logic)
│   ├── useConnection.ts        # Connect, disconnect, connection status
│   ├── useConfig.ts            # Read/write config, sync status per field
│   └── useMidiMonitor.ts       # CC message capture for monitor view
├── services/                   # Non-React logic (pure functions/classes)
│   └── sysex.ts                # SysEx encode/decode, command IDs, protocol
├── types/                      # TypeScript type definitions
│   ├── config.ts               # ConfigurationData, KeyModeData, etc.
│   ├── midi.ts                 # MIDI-related types
│   └── sysex.ts                # SysEx command types, protocol constants
├── design-system/              # Reusable UI primitives
│   ├── tokens/                 # Design tokens (colors, spacing, radii)
│   │   └── index.ts
│   ├── components/             # Primitive components
│   │   ├── Card.tsx
│   │   ├── Select.tsx
│   │   ├── Slider.tsx
│   │   ├── Toggle.tsx
│   │   ├── NumberInput.tsx
│   │   └── index.ts            # Barrel export
│   └── index.ts
├── components/                 # Feature-specific components
│   ├── MidiMonitor/
│   │   └── MidiMonitor.tsx
│   ├── NoteGrid/               # 4x4 scale visualizer
│   │   └── NoteGrid.tsx
│   ├── BankSelector/
│   │   └── BankSelector.tsx
│   └── DeviceVisualizer/
│       └── TopoT16Svg.tsx
├── pages/                      # Route-level components
│   ├── Dashboard.tsx           # Tab container
│   ├── Keyboard.tsx            # Bank keyboard settings
│   ├── Scales.tsx              # Custom scale editor
│   ├── ControlChange.tsx       # CC mapping
│   ├── Settings.tsx            # Global device settings
│   ├── Upload.tsx              # Firmware flashing
│   └── QuickStart.tsx          # Manual
├── layouts/
│   └── RootLayout.tsx
├── utils/                      # Shared utilities
│   └── format.ts
└── assets/
    └── firmwares/
```

### Structure Rationale

- **contexts/**: Following DROP's pattern. ConnectionContext owns WebMIDI lifecycle (currently tangled into MidiProvider). ConfigContext owns device state and sync tracking. These are the two independent concerns currently crammed into the 500-line MidiProvider god-context.
- **hooks/**: Thin wrappers over contexts. Components call `useConfig()` to get `{ config, updateConfig, syncStatus }` rather than reaching into raw context. Following DROP's pattern where hooks encapsulate context consumption.
- **services/sysex.ts**: Pure functions for encoding/decoding SysEx messages. No React dependency. This is the protocol layer -- equivalent to how DROP's DeviceBridge/DeviceService.ts handles device communication as a plain class.
- **types/**: TypeScript definitions mirroring firmware Configuration.hpp structs. Single source of truth for the web side. Replaces the inline default config object in MidiProvider.jsx.
- **design-system/**: Following DROP exactly. Tokens for colors/spacing, primitive Card/Select/Slider/Toggle components. Replaces Chakra UI v2 (which is EOL).

## Architectural Patterns

### Pattern 1: Service Extraction (Firmware)

**What:** Extract domain logic from main.cpp into service classes that own their state and expose a clear interface. main.cpp becomes an orchestrator that wires services together and runs the loop.

**When to use:** When main.cpp has grown beyond ~200 lines and mixes concerns.

**Trade-offs:** Slightly more files to navigate, but each file is self-contained and testable. The eisei codebase proves this works well for unwn firmware.

**Example (following eisei):**

```cpp
// src/Services/CommandRouter.hpp
#pragma once
#include "Configuration.hpp"
#include "Libs/MidiProvider.hpp"

class CommandRouter {
public:
    void Init(MidiProvider& midi, ConfigManager& config);
    void ProcessSysEx(byte* data, unsigned length);

private:
    MidiProvider* midi_ = nullptr;
    ConfigManager* config_ = nullptr;

    void HandleVersionRequest();
    void HandleConfigDump();
    void HandleConfigLoad(byte* data, unsigned length);
    void HandleParameterUpdate(byte* data, unsigned length);  // NEW: per-param
    void HandleCalibrationReset();
};
```

```cpp
// src/main.cpp (slim)
#include "Services/CommandRouter.hpp"
#include "Services/ConfigManager.hpp"
#include "Services/ModeManager.hpp"
#include "Services/InputProcessor.hpp"

MidiProvider midi_provider;
CommandRouter command_router;
ConfigManager config_manager;
ModeManager mode_manager;
InputProcessor input_processor;

void setup() {
    midi_provider.Init(PIN_RX, PIN_TX, PIN_TX2);
    config_manager.Init();
    command_router.Init(midi_provider, config_manager);
    mode_manager.Init(led_manager, midi_provider);
    input_processor.Init(keyboard, slider, midi_provider, mode_manager, led_manager);

    midi_provider.SetHandleSystemExclusive(
        [](byte* data, unsigned len) { command_router.ProcessSysEx(data, len); }
    );

    // ... hardware init ...
}

void loop() {
    midi_provider.Read();
    t_btn.Update();
    m_btn.Update();
    slider.Update();
    keyboard.Update();
    input_processor.Process();
    led_manager.RunPattern();
    FastLED.show();
}
```

### Pattern 2: Per-Parameter SysEx Protocol (Firmware + Web)

**What:** Structured SysEx commands with a command byte and parameter ID, replacing magic byte matching. Supports both per-parameter updates (fast, <100ms) and full config dump/load (backup/restore).

**When to use:** Always for the config sync protocol. The current approach sends the entire JSON config on every change, causing ~3 second round-trips with 11 filesystem read/write cycles.

**Trade-offs:** Requires defining a parameter registry and keeping firmware + web in sync on parameter IDs. But the performance gain (from ~3sec to <100ms) justifies this.

**Example:**

```cpp
// SysEx command structure:
// [0xF0, MFR_ID, CMD_ID, ...payload, 0xF7]
//
// CMD_IDENTITY_REQUEST  = 0x01  (editor -> device)
// CMD_IDENTITY_RESPONSE = 0x02  (device -> editor)
// CMD_CONFIG_DUMP_REQ   = 0x10  (editor -> device)
// CMD_CONFIG_DUMP_RESP  = 0x11  (device -> editor)
// CMD_CONFIG_LOAD       = 0x12  (editor -> device, full config)
// CMD_PARAM_SET         = 0x20  (editor -> device, single param)
// CMD_PARAM_ACK         = 0x21  (device -> editor, confirm)
// CMD_CALIBRATE         = 0x30  (editor -> device)
//
// Per-parameter payload: [PARAM_GROUP, PARAM_ID, BANK_INDEX?, VALUE_BYTES...]
// PARAM_GROUP: 0x01=global, 0x02=bank_keyboard, 0x03=bank_cc, 0x04=scale
```

```typescript
// services/sysex.ts (web side)
export enum SysExCommand {
  IdentityRequest  = 0x01,
  IdentityResponse = 0x02,
  ConfigDumpReq    = 0x10,
  ConfigDumpResp   = 0x11,
  ConfigLoad       = 0x12,
  ParamSet         = 0x20,
  ParamAck         = 0x21,
  Calibrate        = 0x30,
}

export function encodeParamSet(
  group: ParamGroup, paramId: number, value: number, bankIndex?: number
): Uint8Array {
  // Encode per-parameter SysEx message
}

export function decodeConfigDump(data: Uint8Array): DeviceConfig {
  // Decode full config from SysEx response
}
```

### Pattern 3: Context Splitting (Web)

**What:** Split the MidiProvider god-context into ConnectionContext (WebMIDI lifecycle) and ConfigContext (device state + sync). Contexts are composed via provider nesting in main.tsx.

**When to use:** When a single context exceeds ~150 lines or mixes unrelated concerns.

**Trade-offs:** Requires coordinating between contexts (ConfigContext depends on ConnectionContext for send/receive). This is the standard React pattern -- DROP uses the same approach.

**Example:**

```typescript
// contexts/ConnectionContext.tsx
interface ConnectionState {
  isConnected: boolean;
  input: MIDIInput | null;
  output: MIDIOutput | null;
  connect: () => Promise<void>;
  disconnect: () => void;
  sendSysEx: (data: Uint8Array) => void;
  onSysEx: (handler: (data: Uint8Array) => void) => () => void;
}

// contexts/ConfigContext.tsx -- consumes ConnectionContext
interface ConfigState {
  config: DeviceConfig;
  syncStatus: SyncStatus;
  updateParam: (group: ParamGroup, paramId: number, value: number, bank?: number) => void;
  requestDump: () => void;
  loadFullConfig: (config: DeviceConfig) => void;
}
```

```tsx
// main.tsx
<ConnectionProvider>
  <ConfigProvider>
    <App />
  </ConfigProvider>
</ConnectionProvider>
```

## Data Flow

### Config Edit Flow (Per-Parameter -- Target)

```
User changes value in UI
    |
    v
useConfig().updateParam(group, paramId, value, bank?)
    |
    v
ConfigContext updates local state immediately (optimistic)
    |
    v
ConfigContext marks field as "pending"
    |
    v
SysExService.encodeParamSet() -> ConnectionContext.sendSysEx()
    |
    v
[MIDI SysEx over USB/BLE] -----> ESP32 CommandRouter.ProcessSysEx()
                                      |
                                      v
                                  CommandRouter dispatches to ConfigManager
                                      |
                                      v
                                  ConfigManager updates struct field + saves to flash
                                      |
                                      v
                                  CommandRouter sends CMD_PARAM_ACK
                                      |
[MIDI SysEx ACK] <-----------------  |
    |
    v
ConfigContext receives ACK, marks field as "synced"
```

**Round-trip target: <100ms.** No JSON serialization, no full config dump, no 11 filesystem writes. Single field update, single file write on firmware side.

### Config Dump Flow (Backup/Restore)

```
User clicks "Backup" or connects device
    |
    v
ConfigContext sends CMD_CONFIG_DUMP_REQ via SysExService
    |
    v
[SysEx] -----> CommandRouter -> ConfigManager serializes full config
                                      |
                                      v
[SysEx with JSON payload] <---------  |
    |
    v
ConfigContext receives, SysExService.decodeConfigDump() parses
    |
    v
ConfigContext updates all local state + marks all as "synced"
```

### Input Processing Flow (Firmware)

```
loop()
  |
  ├── keyboard.Update()  -->  Key state machine  -->  Signal::Emit
  |                                                        |
  |                                                        v
  |                                               InputProcessor.ProcessKey()
  |                                                        |
  |                                                        v
  |                                               MidiProvider.SendNoteOn()
  |                                                        |
  |                                               ┌────────┴────────┐
  |                                               v        v        v
  |                                             USB      BLE      TRS
  |
  ├── slider.Update()  -->  InputProcessor.ProcessSlider()
  |                                |
  |                                v
  |                         ModeManager.GetSliderMode()
  |                                |
  |                         ┌──────┴──────┐
  |                         v             v
  |                    PitchBend     CC / Octave / Bank
  |
  ├── t_btn.Update()  -->  Signal::Emit  -->  InputProcessor.ProcessButton()
  |
  └── led_manager.RunPattern() + FastLED.show()
```

## Build Order (Dependencies)

The refactoring must proceed in dependency order. Components lower in this list depend on components higher up.

### Firmware Build Order

```
Phase A: Foundation (no dependencies)
  1. Configuration.hpp    -- data structures only (remove save/load)
  2. Signal.hpp           -- already done, no changes needed

Phase B: Services (depend on Configuration + Libs)
  3. ConfigManager        -- depends on Configuration, DataManager
  4. CommandRouter        -- depends on ConfigManager, MidiProvider
  5. ModeManager          -- depends on LedManager patterns
  6. InputProcessor       -- depends on ModeManager, MidiProvider, Keyboard

Phase C: Integration
  7. main.cpp slim-down   -- wire services, remove extracted code
  8. SysEx protocol       -- implement per-param commands in CommandRouter
```

### Web Build Order

```
Phase A: Foundation (no dependencies)
  1. types/               -- TypeScript types mirroring firmware structs
  2. services/sysex.ts    -- SysEx encode/decode (pure functions, testable)
  3. design-system/       -- tokens + primitive components

Phase B: State Layer (depends on types + services)
  4. ConnectionContext     -- extract from MidiProvider, WebMIDI lifecycle
  5. ConfigContext         -- extract from MidiProvider, config state + sync

Phase C: Feature Layer (depends on contexts)
  6. hooks/               -- useConnection, useConfig
  7. components/          -- feature components using hooks
  8. pages/               -- migrate from JSX to TSX, use new contexts + hooks
```

### Cross-Cutting Build Order

The firmware SysEx protocol and web SysEx service must be designed together. Define the protocol contract first (command IDs, parameter groups, encoding), then implement both sides.

```
1. Define protocol spec (command IDs, parameter groups, payload formats)
2. Implement firmware CommandRouter with per-param handling
3. Implement web services/sysex.ts encoding/decoding
4. Wire ConfigContext to use per-param commands instead of full dump
5. Verify <100ms round-trip
```

## Anti-Patterns

### Anti-Pattern 1: God Context

**What people do:** Put all state (connection, config, sync, UI state) in a single React context.
**Why it's wrong:** Every state change re-renders all consumers. Connection changes cause config UI to re-render. Config changes cause connection UI to re-render. Performance degrades, code becomes untestable.
**Do this instead:** Split into ConnectionContext (stable, changes rarely) and ConfigContext (changes on every edit). Components subscribe only to what they need.

### Anti-Pattern 2: Full Config Dump on Every Change

**What people do:** Serialize entire config to JSON, send over SysEx, firmware writes all 11 fields to flash.
**Why it's wrong:** ~3 second round-trip. Flash wear. User perceives lag between editing and hearing the change on device. Flash write cycles have a finite lifetime.
**Do this instead:** Per-parameter SysEx commands. Only send the changed field. Firmware updates only the changed struct field and writes only the affected JSON key. Full dump is reserved for backup/restore.

### Anti-Pattern 3: Inline SysEx Magic Numbers

**What people do:** Match raw bytes in `ProcessSysEx()` with hardcoded constants (`data[2] == 127 && data[3] == 7`).
**Why it's wrong:** No documentation of protocol. Adding a new command requires studying existing byte patterns to avoid collisions. No validation, no error response.
**Do this instead:** Define an enum of command IDs. CommandRouter parses the command byte, validates payload length, dispatches to named handler methods. Each handler receives only its payload, not the raw SysEx buffer.

### Anti-Pattern 4: Global State in Firmware

**What people do:** Declare config structs as global variables accessed from anywhere.
**Why it's wrong:** Any function can modify config state without going through proper save/apply logic. Makes testing impossible -- can't inject different configs. Race conditions if FreeRTOS tasks access globals.
**Do this instead:** ConfigManager owns the config structs. Other services receive const references or copies. Mutations go through ConfigManager methods that handle save + apply.

## Integration Points

### WebMIDI <-> ESP32 SysEx

| Aspect | Current | Target |
|--------|---------|--------|
| Protocol | Magic byte sequences | Structured command IDs |
| Config sync | Full JSON dump (~3s) | Per-parameter (<100ms) + full dump for backup |
| Validation | None | Command router validates payload length |
| Error handling | Silent failure | ACK/NACK responses |
| Encoding | JSON over SysEx bytes | Binary for per-param, JSON for full dump |

### Browser Firmware Update (esptool-js)

| Aspect | Notes |
|--------|-------|
| Transport | Web Serial API (separate from MIDI) |
| Binary storage | Bundled in `assets/firmwares/` (committed to repo) |
| Flow | User selects firmware version, esptool-js flashes via USB |
| Independence | Completely separate from config sync -- different transport |

## Sources

- eisei firmware: `/home/unwn/git/unwn/eisei/daisy/` -- CommService, SerialCommandManager, PresetManager, CalibrationManager patterns
- DROP web app: `/home/unwn/git/DROP/src/` -- contexts/, services/DeviceBridge/, hooks/, design-system/, component organization
- T16 current codebase: `/home/unwn/git/T16/src/main.cpp`, `editor-tx/src/components/MidiProvider.jsx`

---
*Architecture research for: T16 MIDI controller firmware + web configurator refactor*
*Researched: 2026-04-03*
