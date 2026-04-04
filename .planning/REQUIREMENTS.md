# Requirements: T16 Refactor

**Defined:** 2026-04-03
**Core Value:** Every parameter change reaches the device in under 100ms, and any developer can modify one feature without risking another.

## v1 Requirements

Requirements for the full refactor. Each maps to roadmap phases.

### Protocol

- [x] **PROTO-01**: Device uses structured SysEx with unwn manufacturer ID prefix and command byte framing
- [x] **PROTO-02**: Editor can send per-parameter updates via SysEx (single value change, <100ms round-trip)
- [x] **PROTO-03**: Editor can request and receive full config dump for initial sync and backup/restore
- [x] **PROTO-04**: Firmware validates SysEx payload length and structure before deserialization
- [x] **PROTO-05**: Protocol supports version handshake on connect (firmware reports protocol version to editor)

### Firmware Architecture

- [x] **FWARCH-01**: Application logic extracted from main.cpp into service classes (CommandRouter, ConfigManager, ModeManager, InputProcessor)
- [x] **FWARCH-02**: main.cpp is slim — init + loop calling services, under 150 lines
- [x] **FWARCH-03**: All header-only implementations moved to .cpp files (proper compilation units)
- [x] **FWARCH-04**: Global mutable state encapsulated into service classes with explicit ownership
- [x] **FWARCH-05**: MIDI transport abstracted into a list of interfaces (loop over active transports, not if/if/if)
- [x] **FWARCH-06**: DataManager rewritten — load once, modify in-memory, write once (not 11 filesystem round-trips per save)
- [x] **FWARCH-07**: Task scheduler replaces tight loop() for timing-controlled execution of subsystems

### Firmware Bugs

- [ ] **FWBUG-01**: Memory leak in LedManager pattern transitions fixed (proper cleanup of previous pattern)
- [x] **FWBUG-02**: TouchSlider::SetPosition(uint8_t, uint8_t) no-op fixed (assigns to lastPosition)
- [x] **FWBUG-03**: Unreachable XY_PAD branch in loop() removed
- [x] **FWBUG-04**: HardwareTest infinite loop on broken key fixed (timeout + error indication instead)

### Firmware Features

- [x] **FWFEAT-01**: Serial command interface for diagnostics (modular command categories following eisei pattern)
- [x] **FWFEAT-02**: SysEx command triggers ESP32 bootloader mode for firmware update (no physical button hold)
- [x] **FWFEAT-03**: Configuration version migration — non-destructive upgrade from v103 to new format

### Web Architecture

- [x] **WEBARCH-01**: Full rewrite in TypeScript (no JavaScript, no PropTypes)
- [ ] **WEBARCH-02**: Custom design system with Radix primitives and CSS custom properties (matching DROP)
- [x] **WEBARCH-03**: MidiProvider god-context split into ConnectionContext and ConfigContext
- [x] **WEBARCH-04**: Codebase organized by feature domain: contexts/, hooks/, services/, components/, types/
- [x] **WEBARCH-05**: Shared TypeScript types for config schema (single source of truth for firmware JSON keys)
- [x] **WEBARCH-06**: React 19 + Vite 7 + ESLint 9 flat config (matching DROP toolchain)

### Web Features

- [ ] **WEBFEAT-01**: Note grid visualizer showing 4x4 key mapping with scale degree colors for current bank
- [x] **WEBFEAT-02**: PWA support — service worker, manifest, offline capability, mobile BLE configuration
- [x] **WEBFEAT-03**: Firmware update without bootloader button — click "Update" triggers auto-bootloader via SysEx
- [x] **WEBFEAT-04**: Config backup import validates against schema before applying
- [ ] **WEBFEAT-05**: Live MIDI monitor with CC value visualization (real-time message display)
- [x] **WEBFEAT-06**: Consistent, polished UI/UX across all configurator pages (spacing, transitions, responsive)

### Testing & CI

- [x] **TEST-01**: Firmware unit tests via PlatformIO native env (config parsing, SysEx encoding, state machines)
- [x] **TEST-02**: Web component and integration tests via Vitest + Testing Library
- [x] **TEST-03**: CI pipeline via GitHub Actions (build firmware, build web, run tests, lint)
- [x] **TEST-04**: clang-format for firmware code, ESLint 9 + Prettier for web code

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Community

- **COMM-01**: Config sharing with metadata (preset name, description, author in exports)
- **COMM-02**: Community preset library (browse/import shared configurations)

### Advanced Firmware

- **ADVFW-01**: Config version migration with incremental migration functions per version bump
- **ADVFW-02**: Dynamic bank count (user-configurable number of banks)
- **ADVFW-03**: Library upgrades (FastLED, NimBLE, Arduino Core) when PlatformIO ecosystem stabilizes

### Web Polish

- **WEBPOL-01**: Undo/redo stack for config changes
- **WEBPOL-02**: Keyboard shortcuts for common actions
- **WEBPOL-03**: Config diff view (show what changed vs device)

## Out of Scope

| Feature | Reason |
|---------|--------|
| CMake migration | PlatformIO/Arduino constraint — staying on current build system |
| WiFi OTA updates | Security concerns, ESP32-S3 WiFi+BLE instability, esptool-js is simpler |
| Web Serial for config | Would duplicate SysEx protocol, MIDI is the native transport |
| Mobile native app | PWA covers mobile use case without app store friction |
| Lua/scripting engine | Overkill for 16-key controller, fixed modes are sufficient |
| Real-time audio processing | T16 is a MIDI controller, not a synth |
| Cloud sync / accounts | Requires server infrastructure, local file export is sufficient |
| MIDI learn | Confusing UX for positional pad controller (not knob controller) |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PROTO-01 | Phase 1 | Complete |
| PROTO-02 | Phase 1 | Complete |
| PROTO-03 | Phase 1 | Complete |
| PROTO-04 | Phase 1 | Complete |
| PROTO-05 | Phase 1 | Complete |
| FWARCH-01 | Phase 2 | Complete |
| FWARCH-02 | Phase 2 | Complete |
| FWARCH-03 | Phase 2 | Complete |
| FWARCH-04 | Phase 2 | Complete |
| FWARCH-05 | Phase 2 | Complete |
| FWARCH-06 | Phase 1 | Complete |
| FWARCH-07 | Phase 2 | Complete |
| FWBUG-01 | Phase 2 | Pending |
| FWBUG-02 | Phase 2 | Complete |
| FWBUG-03 | Phase 2 | Complete |
| FWBUG-04 | Phase 2 | Complete |
| FWFEAT-01 | Phase 2 | Complete |
| FWFEAT-02 | Phase 4 | Complete |
| FWFEAT-03 | Phase 1 | Complete |
| WEBARCH-01 | Phase 3 | Complete |
| WEBARCH-02 | Phase 3 | Pending |
| WEBARCH-03 | Phase 3 | Complete |
| WEBARCH-04 | Phase 3 | Complete |
| WEBARCH-05 | Phase 1 | Complete |
| WEBARCH-06 | Phase 3 | Complete |
| WEBFEAT-01 | Phase 5 | Pending |
| WEBFEAT-02 | Phase 5 | Complete |
| WEBFEAT-03 | Phase 4 | Complete |
| WEBFEAT-04 | Phase 4 | Complete |
| WEBFEAT-05 | Phase 5 | Pending |
| WEBFEAT-06 | Phase 3 | Complete |
| TEST-01 | Phase 2 | Complete |
| TEST-02 | Phase 3 | Complete |
| TEST-03 | Phase 4 | Complete |
| TEST-04 | Phase 4 | Complete |

**Coverage:**
- v1 requirements: 35 total
- Mapped to phases: 35
- Unmapped: 0

---
*Requirements defined: 2026-04-03*
*Last updated: 2026-04-03 after roadmap creation*
