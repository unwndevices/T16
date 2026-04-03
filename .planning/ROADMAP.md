# Roadmap: T16 Refactor

## Overview

The T16 refactor treats firmware and web configurator as a single system with a protocol boundary in the middle. The SysEx protocol must be defined first because both sides depend on it. From there, firmware services are extracted from the monolithic main.cpp, the web editor is rewritten in TypeScript with a modern design system, both halves are wired together for per-parameter sync, and finally differentiator features are polished on top of the stable foundation.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Protocol & Data Foundation** - Define the shared SysEx contract, rewrite DataManager, establish canonical config types
- [ ] **Phase 2: Firmware Service Extraction** - Extract monolithic main.cpp into service classes, fix known bugs, add firmware tests
- [ ] **Phase 3: Web Rewrite** - Full TypeScript rewrite with custom design system, split god-context, feature-domain organization
- [ ] **Phase 4: Integration & CI** - Wire per-parameter sync end-to-end, firmware update flow, CI pipeline, linting
- [ ] **Phase 5: Feature Polish** - Note grid visualizer, PWA support, MIDI monitor

## Phase Details

### Phase 1: Protocol & Data Foundation
**Goal**: Firmware and web share a verified protocol contract, config persists efficiently, and the canonical config schema exists as a single source of truth
**Depends on**: Nothing (first phase)
**Requirements**: PROTO-01, PROTO-02, PROTO-03, PROTO-04, PROTO-05, FWARCH-06, WEBARCH-05, FWFEAT-03
**Success Criteria** (what must be TRUE):
  1. Firmware accepts structured SysEx commands with manufacturer ID prefix and command byte framing
  2. Firmware handles per-parameter SysEx updates and full config dump requests without filesystem thrashing (single write per save cycle)
  3. Firmware validates incoming SysEx payload length and structure before acting on it
  4. Firmware and web share a TypeScript config type definition that matches the actual firmware JSON keys (round-trip verified)
  5. Existing config (v103) migrates non-destructively to the new format on first boot
**Plans:** 5 plans

Plans:
- [x] 01-01-PLAN.md — JSON Schema, TypeScript type generation, SysEx protocol constants
- [x] 01-02-PLAN.md — ConfigManager with load-once/save-once persistence and v103 migration
- [x] 01-03-PLAN.md — SysEx command handler with validated dispatch
- [x] 01-04-PLAN.md — PlatformIO native test environment and unit tests
- [x] 01-05-PLAN.md — Gap closure: fix test compilation errors and npm dependency

### Phase 2: Firmware Service Extraction
**Goal**: All application logic lives in testable service classes, main.cpp is a slim orchestrator, and all known firmware bugs are fixed
**Depends on**: Phase 1
**Requirements**: FWARCH-01, FWARCH-02, FWARCH-03, FWARCH-04, FWARCH-05, FWARCH-07, FWBUG-01, FWBUG-02, FWBUG-03, FWBUG-04, FWFEAT-01, TEST-01
**Success Criteria** (what must be TRUE):
  1. main.cpp is under 150 lines -- init + loop calling services, no business logic
  2. MIDI output works identically across USB, BLE, and TRS with transport abstraction (no stuck notes, no velocity regressions)
  3. LED pattern transitions do not leak memory, touch slider position updates work, XY_PAD dead branch is gone, and hardware test times out on broken keys instead of hanging
  4. Serial command interface provides diagnostic output following eisei command pattern
  5. Firmware unit tests run on host (PlatformIO native env) covering config parsing, SysEx encoding, and state machine logic
**Plans:** 6 plans

Plans:
- [x] 02-01-PLAN.md — Shared types, ModeManager service, Scales.hpp split
- [x] 02-02-PLAN.md — MidiProvider transport abstraction, TouchSlider bug fix
- [x] 02-03-PLAN.md — Header splits (Keyboard, Button, LedManager, Pattern) + LedManager unique_ptr
- [x] 02-04-PLAN.md — InputProcessor, SliderProcessor, ButtonHandler, CalibrationService extraction
- [x] 02-05-PLAN.md — AppEngine orchestrator, slim main.cpp, SerialCommandManager
- [ ] 02-06-PLAN.md — Unit tests for ModeManager, transport, InputProcessor

### Phase 3: Web Rewrite
**Goal**: The web configurator is a fully typed TypeScript application with a modern design system, clean context separation, and feature-domain organization
**Depends on**: Phase 1
**Requirements**: WEBARCH-01, WEBARCH-02, WEBARCH-03, WEBARCH-04, WEBARCH-06, WEBFEAT-06, TEST-02
**Success Criteria** (what must be TRUE):
  1. Every source file is TypeScript (no .js/.jsx, no PropTypes) with strict mode enabled
  2. Design system uses Radix primitives and CSS custom properties -- Chakra v2 is fully removed
  3. MidiProvider god-context is replaced by ConnectionContext (WebMIDI lifecycle) and ConfigContext (device state + sync)
  4. Codebase is organized by feature domain (contexts/, hooks/, services/, components/, types/)
  5. All configurator pages have consistent spacing, transitions, and responsive layout
**Plans:** 6 plans
**UI hint**: yes

Plans:
- [x] 03-01-PLAN.md — Toolchain scaffold (React 19, Vite, TypeScript strict, ESLint 9, Vitest, directory structure)
- [ ] 03-02-PLAN.md — Design system tokens and primitive components (Radix + CSS Modules)
- [x] 03-03-PLAN.md — Context split (ConnectionContext, ConfigContext, ToastContext) + services + hooks
- [ ] 03-04-PLAN.md — Layout shell (NavBar, Footer) + Dashboard page with 4 config tabs
- [ ] 03-05-PLAN.md — Upload page, Manual page, App router, main entry point
- [ ] 03-06-PLAN.md — Cleanup old JSX files + component/service tests

### Phase 4: Integration & CI
**Goal**: Per-parameter config changes reach the device in under 100ms, firmware updates work without holding the bootloader button, and CI validates every push
**Depends on**: Phase 2, Phase 3
**Requirements**: FWFEAT-02, WEBFEAT-03, WEBFEAT-04, TEST-03, TEST-04
**Success Criteria** (what must be TRUE):
  1. Changing a single parameter in the web editor reflects on the device within 100ms round-trip (measured, not estimated)
  2. Clicking "Update Firmware" in the editor triggers bootloader mode via SysEx -- no physical button hold required
  3. Importing a config backup validates against the schema and rejects malformed files with a clear error message
  4. GitHub Actions CI builds firmware, builds web, runs all tests, and runs linters on every push
**Plans**: TBD
**UI hint**: yes

### Phase 5: Feature Polish
**Goal**: Differentiator features that make the configurator feel complete -- visual note mapping, mobile access, and real-time MIDI feedback
**Depends on**: Phase 3
**Requirements**: WEBFEAT-01, WEBFEAT-02, WEBFEAT-05
**Success Criteria** (what must be TRUE):
  1. Note grid visualizer displays 4x4 key mapping with scale degree colors for the active bank, updating when bank or scale changes
  2. Configurator works as a PWA on mobile -- installable, offline-capable, and can configure the device over BLE
  3. MIDI monitor displays incoming messages in real-time with CC value visualization
**Plans**: TBD
**UI hint**: yes

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5
Note: Phases 2 and 3 both depend on Phase 1. Phase 4 depends on both 2 and 3.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Protocol & Data Foundation | 0/5 | Executing (gap closure) | - |
| 2. Firmware Service Extraction | 0/6 | Planned | - |
| 3. Web Rewrite | 0/6 | Planned | - |
| 4. Integration & CI | 0/0 | Not started | - |
| 5. Feature Polish | 0/0 | Not started | - |
