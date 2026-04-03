# Project Research Summary

**Project:** T16 Firmware Refactor + Web Configurator Rewrite
**Domain:** ESP32-S3 FSR MIDI controller firmware + browser-based configurator
**Researched:** 2026-04-03
**Confidence:** HIGH

## Executive Summary

T16 is a dual-codebase product: ESP32-S3 firmware (PlatformIO/Arduino/C++17) and a browser-based React configurator communicating over MIDI SysEx. The existing codebase has a working feature set but accumulates architectural debt: a monolithic 887-line `main.cpp`, a 500-line god-context `MidiProvider.jsx`, and a config sync protocol that does 11 filesystem write cycles per change (producing ~3 second round-trips). The refactor follows proven patterns from two in-house reference codebases -- eisei (firmware) and DROP (web) -- which are in the same technology ecosystem. The stack is almost entirely constrained; both codebases must match DROP's exact dependency versions for cross-project consistency.

The recommended approach treats the two halves as a single system with a protocol boundary in the middle. The SysEx protocol must be designed before either side is touched, because the firmware extraction and the web rewrite both depend on it. The protocol change carries the highest breakage risk (existing devices become unconfigurable if firmware and editor are mismatched), so it must include a version handshake and a transition period that supports both old and new command formats. Per-parameter sync (<100ms round-trip) is the primary UX goal; everything else is secondary.

The two critical risks are config schema divergence (firmware and web independently drift) and breaking MIDI output during firmware service extraction (stuck notes, dropped notes). Both are preventable with upfront schema definition and incremental, validated extraction. Flash wear from the current 11-writes-per-save pattern must also be eliminated in the DataManager rewrite before per-parameter sync is enabled, or the increased sync frequency will multiply the write rate.

## Key Findings

### Recommended Stack

The stack is highly constrained by existing choices and the DROP reference. PlatformIO + Arduino Core 2.x must stay because Core 3.x is not stable on PlatformIO and FastLED 3.9.x has documented RMT regressions on ESP32-S3 with Core 2.x. NimBLE must stay on 1.4.x for the same reason. The web stack matches DROP exactly: React 19, Vite 7, TypeScript strict, Vitest, ESLint 9 flat config. The design system is a custom Radix-based system (ported from DROP) replacing the EOL Chakra v2 stack. Dependencies to remove: axios (unused), prop-types (replaced by TypeScript), `@emotion/*`, `framer-motion`, `@chakra-ui/*`.

The one meaningful upgrade opportunity is adding a PlatformIO `[env:native]` environment for host-side unit testing of pure logic (config parsing, SysEx protocol encoding, state machines) without hardware. This directly supports safe incremental extraction.

**Core technologies:**
- PlatformIO + Arduino Core 2.x: build system + runtime -- constrained, no migration path
- FastLED 3.6.x: LED control -- pin to this version, 3.9.x has regressions on target hardware
- NimBLE 1.4.x + BLE-MIDI 2.2.x: BLE stack -- tied to Arduino Core 2.x, upgrade as a trio
- React 19 + Vite 7 + TypeScript 5.8: web framework -- match DROP versions exactly
- Custom design system + Radix primitives: UI -- port from DROP, replaces Chakra v2
- webmidi 3.x + esptool-js 0.5.7: device communication -- already in use, bump versions
- Unity (PlatformIO native): firmware testing -- enables host-side tests for logic

### Expected Features

All table stakes features already exist in some form. The refactor must not break any of them. The primary differentiator -- per-parameter SysEx sync -- does not exist yet and is the core value proposition of the entire effort.

**Must have (table stakes -- already exists, must survive refactor):**
- Per-key velocity + aftertouch curve selection
- Multi-transport MIDI (USB, BLE, TRS)
- Bank/preset switching (4 banks)
- Key calibration routine (fix the infinite-loop-on-broken-key bug)
- Config persistence (LittleFS -- fix the 11-write bottleneck)
- Scale/note mapping (17 built-in + 2 custom)
- On-device quick settings
- WebMIDI SysEx connection + config dump/restore
- Config backup/restore to file (.topo, with schema validation on import)
- Firmware update via esptool-js (fix the bootloader UX)

**Should have (differentiators -- does not exist yet):**
- Per-parameter SysEx sync (<100ms) -- the stated core value proposition
- Structured SysEx protocol with manufacturer ID and command enum
- Note grid visualizer with scale overlay (partially exists, broken)
- Config version migration (non-destructive)
- Live MIDI monitor with CC visualization (partially exists)
- Firmware update without bootloader button hold (SysEx-triggered bootloader)
- TypeScript migration with shared config types

**Defer (after initial rewrite ships):**
- PWA support for mobile BLE configuration
- Serial command interface for diagnostics
- Task scheduler (internal improvement, existing tight-loop works)
- Config sharing with metadata / community presets

### Architecture Approach

Both codebases split by the same principle: extract domain logic from monoliths into services with clear boundaries. On the firmware side, `main.cpp` becomes a slim orchestrator; four service classes (`CommandRouter`, `ConfigManager`, `ModeManager`, `InputProcessor`) extract from it. On the web side, the 500-line `MidiProvider` god-context splits into `ConnectionContext` (WebMIDI lifecycle) and `ConfigContext` (device state + sync tracking), with a pure `services/sysex.ts` layer for encoding/decoding. The protocol boundary between sides is a structured SysEx command protocol (command ID enum, parameter groups, binary per-param payloads, JSON for full dump).

**Major components:**
1. `CommandRouter` (firmware) -- parse SysEx, dispatch to named handlers, validate payload length
2. `ConfigManager` (firmware) -- in-memory config with debounced flash writes, per-parameter update methods
3. `ConnectionContext` + `ConfigContext` (web) -- split the god-context, each owns one concern
4. `services/sysex.ts` (web) -- pure encode/decode functions, no React dependency, protocol constants
5. Design system (web) -- tokens + Radix-based primitives ported from DROP

### Critical Pitfalls

1. **Config schema divergence** -- firmware and web independently maintain config field names. Prevent by defining `types/config.ts` as canonical source before writing any protocol code. Add a round-trip test.

2. **Breaking MIDI output during service extraction** -- synchronous signal-to-MIDI-send path must be preserved. Extract one service at a time. Validate with MIDI loopback after each extraction, not at the end. The `note_pool` / `chord_pool` / `strum_pool` arrays must survive intact.

3. **SysEx protocol change bricks existing devices** -- new structured commands are incompatible with the old magic-byte protocol. Implement a version handshake on connect. Support both protocols for one firmware version. The web editor must show a "firmware update recommended" banner rather than silently failing.

4. **LittleFS flash wear** -- current 11-writes-per-save must become 1 write before per-parameter sync is enabled. The DataManager rewrite is a prerequisite, not a follow-up.

5. **Cross-core race conditions** -- ADC runs on core 0, everything else on core 1. Refactoring may introduce new cross-core sharing. Keep all config mutation on a single core. Document core assignment for every service.

## Implications for Roadmap

### Phase 1: Protocol Foundation
**Rationale:** The SysEx protocol is the dependency everything else blocks on. Firmware extraction needs to know what the CommandRouter must handle. Web rewrite needs to know what `services/sysex.ts` must encode. Config types must be defined before either side is written. This is zero-feature work that unlocks all subsequent phases.
**Delivers:** Protocol spec document (command IDs, parameter groups, payload formats), `types/config.ts` canonical schema, round-trip test proving firmware JSON keys match web types, DataManager rewrite (batch writes, in-memory config, dirty-flag debounce), structured SysEx manufacturer ID prefix.
**Addresses:** Config schema divergence, LittleFS flash wear, protocol backward compatibility design.
**Avoids:** Pitfalls 1 (schema divergence), 4 (SysEx bricks devices), 5 (flash wear).

### Phase 2: Firmware Service Extraction
**Rationale:** With the protocol spec in hand, extract firmware services in dependency order: ConfigManager first (owns config state), then CommandRouter (dispatches to ConfigManager), then ModeManager and InputProcessor (depend on the above). Each extraction is validated with MIDI loopback before moving to the next. This is the highest-risk phase -- stuck notes or velocity regressions are the danger.
**Delivers:** Slim `main.cpp`, four service classes (`CommandRouter`, `ConfigManager`, `ModeManager`, `InputProcessor`), per-parameter SysEx handler in CommandRouter, PlatformIO `[env:native]` unit tests for ConfigManager and SysEx parsing.
**Addresses:** Table stakes survival (all existing features), per-parameter firmware support.
**Avoids:** Pitfalls 2 (broken MIDI output), 3 (velocity regression), 7 (cross-core races). Establish core assignment per service in this phase.

### Phase 3: Web Rewrite
**Rationale:** The web rewrite is parallelizable with Phase 2 for experienced hands, but safer in sequence since it depends on the protocol spec from Phase 1. Start with a feature inventory checklist of every function in the old `MidiProvider.jsx` context before writing any new code. Build foundation-up following the architecture build order: types -> sysex.ts -> design system -> contexts -> hooks -> pages.
**Delivers:** Full TypeScript codebase, `ConnectionContext` + `ConfigContext` replacing `MidiProvider.jsx`, `services/sysex.ts` with per-parameter encode/decode, design system ported from DROP (Chakra v2 removed), all pages migrated to TSX, feature parity verified against inventory checklist.
**Addresses:** TypeScript migration, design system modernization, god-context elimination, CC page bug (hardcoded to bank 0).
**Avoids:** Pitfall 6 (rewrite loses features). Feature inventory checklist is the gate condition.

### Phase 4: Per-Parameter Sync Integration
**Rationale:** This is the integration phase -- connecting the per-parameter firmware (Phase 2) to the per-parameter web service (Phase 3). The protocol spec (Phase 1) means both sides are already aligned on command IDs and parameter encoding. This phase is about wiring, validation, and measuring the <100ms round-trip target.
**Delivers:** End-to-end per-parameter config sync under 100ms, optimistic UI updates with per-field sync status, ACK/NACK error handling, version handshake on connect, "firmware update recommended" banner for protocol mismatches.
**Addresses:** Core value proposition -- per-parameter sync is the stated goal.
**Avoids:** Pitfall 4 (protocol incompatibility) via the handshake.

### Phase 5: UX Polish + Differentiators
**Rationale:** With the core loop working (firmware services extracted, web rewritten, per-param sync verified), invest in the features that make the product feel complete rather than functional. These are lower-risk because they sit on top of the stable foundation.
**Delivers:** Note grid visualizer (working, not broken), firmware update without bootloader button hold (SysEx-triggered bootloader mode), config backup schema validation on import, MIDI monitor visual display, config undo stack, actionable connection error messages.
**Addresses:** Differentiator features, UX pitfalls (no visual feedback, broken scale visualizer, hiding errors).

### Phase Ordering Rationale

- Phase 1 must come first because it defines the shared contract. All other phases build on it.
- Phase 2 before Phase 3 is recommended (not required) because the web rewrite is lower risk and can absorb feedback from the firmware extraction.
- Phase 4 cannot start until both Phase 2 and Phase 3 are complete -- it requires both sides.
- Phase 5 is independent of Phase 4 internals, but should come after Phase 3 to build on the new web foundation.
- The DataManager rewrite (batch writes) in Phase 1 is mandatory before Phase 4 enables per-parameter sync -- otherwise increased sync frequency multiplies flash writes.

### Research Flags

Phases with standard patterns (established, skip research-phase):
- **Phase 3 (Web Rewrite):** Well-documented patterns from DROP reference. Context splitting, hook extraction, design system porting are all established.
- **Phase 5 (UX Polish):** Standard UI work, no novel technology.

Phases likely needing deeper research during planning:
- **Phase 2 (Firmware Service Extraction):** The cross-core ADC boundary and FreeRTOS task interactions should be researched before extracting services that touch those boundaries. Specifically: confirm thread-safety of `Signal::Emit` across the ADC/loop core boundary.
- **Phase 4 (Per-Parameter Sync):** BLE MIDI MTU negotiation and chunked SysEx for large payloads need implementation research. The default 20-byte BLE MTU will truncate full config dumps. iOS Safari BLE WebMIDI behavior is a known inconsistency.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All versions verified against DROP repo and official release notes. Constraints (Arduino Core 2.x, FastLED 3.6.x, NimBLE 1.4.x) are backed by specific GitHub issues. |
| Features | HIGH | Feature set is concrete -- existing codebase was walked function-by-function. Competing products (OpenDeck, Morningstar, Sensel) used for table stakes validation. |
| Architecture | HIGH | Based on verified reference implementations (eisei firmware, DROP web). Not theoretical -- these patterns are running in production. |
| Pitfalls | HIGH | Derived from codebase analysis of known bugs, not speculation. Cross-core race condition and flash wear are documented in CONCERNS.md. |

**Overall confidence:** HIGH

### Gaps to Address

- **BLE chunked SysEx implementation:** Full config dumps over BLE will exceed the default 20-byte MTU. MTU negotiation and chunk reassembly need a concrete implementation plan before Phase 4. Validate on iOS Safari and Android Chrome specifically.
- **`Signal::Emit` thread safety:** The ADC task on core 0 writing values that `Keyboard::Update()` on core 1 reads through the Signal system -- confirm this is safe with the current implementation before Phase 2 extraction adds new cross-core interactions.
- **Bootloader SysEx command (Phase 5):** ESP32-S3 has a documented method for entering download mode via software, but the exact sequence for PlatformIO + Arduino Core 2.x needs verification before implementing the firmware-side handler.
- **apexcharts removal decision:** MIDI monitor currently uses apexcharts. Decision to keep (replace with uplot per DROP pattern) or remove entirely should be made during Phase 3 planning, not deferred.

## Sources

### Primary (HIGH confidence)
- DROP reference repo (`/home/unwn/git/DROP/`) -- exact versions and architecture patterns verified
- eisei reference repo (`/home/unwn/git/unwn/eisei/daisy/`) -- service architecture patterns verified
- T16 codebase analysis -- `src/main.cpp`, `src/Configuration.cpp`, `editor-tx/src/components/MidiProvider.jsx`
- `.planning/codebase/CONCERNS.md` -- known bugs and technical debt
- [React Releases](https://react.dev/versions) -- 19.0.4 latest patch
- [Vite Releases](https://vite.dev/releases) -- Vite 7 stable
- [FastLED ESP32-S3 RMT issues](https://github.com/fastled/fastled/issues/2147) -- 3.9.x regressions confirmed
- [NimBLE-Arduino GitHub](https://github.com/h2zero/NimBLE-Arduino) -- v2.x requires Core 3.x confirmed
- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html) -- native env Unity framework

### Secondary (MEDIUM confidence)
- [OpenDeck Wiki](https://github.com/shanteacontrols/OpenDeck/wiki/Configurable-features) -- table stakes feature validation
- [Morningstar Editor User Guide](https://manuals.morningstar.io/mc-midi-controller/Editor-User-Guide-(Version-1.3.6).1035304961.html) -- UX patterns and backup/restore
- [Intech Grid Editor](https://intech.studio/products/editor) -- MIDI monitor and advanced configurator patterns
- [WebMIDI SysEx Editor Framework (MIDI.org)](https://midi.org/innovation-award/webmidi-sysex-editor-framework) -- SysEx protocol patterns

### Tertiary (LOW confidence)
- iOS Safari BLE WebMIDI behavior -- community reports only, no authoritative source. Needs hands-on validation in Phase 4.

---
*Research completed: 2026-04-03*
*Ready for roadmap: yes*
