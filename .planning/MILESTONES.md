# Milestones

## v1.0 T16 Refactor (Shipped: 2026-04-04)

**Phases completed:** 5 phases, 24 plans, 47 tasks

**Key accomplishments:**

- JSON Schema (v200) with generated TypeScript types and matching SysEx protocol constants in C++ and TypeScript
- Load-once/save-once ConfigManager replacing per-field DataManager, with dirty-flag idle flush and v103-to-v200 migration
- Structured SysEx command handler replacing magic-byte dispatch with validated, domain-routed command processing for version handshake, config dump/load, and per-parameter updates
- 43 native unit tests across 3 suites (protocol, ConfigManager, migration) plus schema-to-TypeScript validation, all running on host without ESP32 hardware
- Fixed 3 test compilation/assertion errors and verified npm type generation pipeline, bringing Phase 01 to 14/14 verification score
- ModeManager service with mode/slider-mode state machine, shared Types.hpp with namespaced enums, and Scales.hpp split into proper .hpp/.cpp compilation units
- MidiTransport interface with loop-based dispatch replacing if/if/if, plus TouchSlider::SetPosition no-op bug fix
- Split Keyboard/Button/LedManager/Pattern into .hpp/.cpp pairs, fixed LedManager memory leak with unique_ptr pattern ownership
- Four service classes extracted from main.cpp: InputProcessor (key/strum/quick-settings), SliderProcessor (slider modes), ButtonHandler (button dispatch via ModeManager), CalibrationService (hardware test with 3s timeout fix)
- AppEngine orchestrator replaces 873-line main.cpp with 14-line entry point, serial diagnostic commands via USB
- 29 new unit tests covering ModeManager state machine, transport mock dispatch, SetPosition bug fix regression, and InputProcessor note mapping logic
- React 19 + TypeScript strict + Vite 8 + ESLint 9 flat config + Vitest with feature-domain directory scaffold replacing Chakra UI / JS toolchain
- Complete design system with 10 Radix-wrapped and custom components, CSS design tokens, and barrel export for unified consumption
- Split MidiProvider god-context into ConnectionContext (WebMIDI lifecycle), ConfigContext (config + sync via useReducer), and MIDI service layer with typed hooks
- App shell (NavBar, Footer, Layout) and Dashboard with 4 tabbed config sections (Keyboard, Scales, CC, Settings) consuming design system and context hooks
- Upload and Manual pages rewritten in TypeScript with App router and provider-tree entry point completing the application shell
- Deleted 28 old JSX files, fixed TypeScript strict build with esptool-js 0.6 API, added 19 unit tests for MIDI service and context layers
- GitHub Actions CI with firmware build/test/format and web typecheck/lint/format/test/build, plus clang-format and Prettier configs
- Ajv-based config import validation with JSON schema checking, v103 migration, and .topo export with schema version field
- CMD_BOOTLOADER (0x05) added to firmware SysEx protocol with RTC register bootloader entry and web protocol mirror
- One-click firmware update via SysEx bootloader command with Web Serial auto-connect, plus per-parameter sync round-trip measurement logged to console
- 4x4 note grid with HSL scale-degree coloring, firmware-matching computeNoteMap algorithm, and 25 unit tests
- Real-time MIDI monitor page with useMidiMonitor hook, CC/velocity value bars, and bounded 100-message log
- PWA installable app with workbox service worker, BLE MIDI connection service with tested packet parser, and offline indicator banner

---
