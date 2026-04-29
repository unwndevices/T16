---
gsd_state_version: 1.0
milestone: v1.1
milestone_name: Variant Support
status: phase_complete_human_needed
stopped_at: Phase 10 code complete (4/4 plans); hardware smoke test deferred to human verification
last_updated: "2026-04-29T00:00:00.000Z"
last_activity: 2026-04-29
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 4
  completed_plans: 4
  percent: 17
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-04-29)

**Core value:** Every parameter change reaches the device in under 100ms, and any developer can modify one feature without risking another.
**Current focus:** Milestone v1.1 — Variant Support (Phase 10 code complete — human verification needed before advancing)

## Current Position

Phase: 10 — Build System & Variant Selection (4/4 plans code-complete; VERIFICATION.md = human_needed)
Plan: —
Status: Code refactor done; firmware.elf link blocked by pre-existing third-party library conflict (TinyUSB/BLE-MIDI), which makes the Plan-04 T16 hardware smoke test impossible until that lib_deps issue is resolved. See `.planning/phases/10-build-system-variant-selection/10-VERIFICATION.md` for next-step options.
Last activity: 2026-04-29 — Phase 10 planned (RESEARCH.md + 4 PLAN.md files)

Progress: [░░░░░░░░░░] 0% (0/5 v1.1 phases)

## Performance Metrics

**Velocity:**

- Total plans completed: 0 (v1.1)
- Average duration: -
- Total execution time: 0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

**Recent Trend:**

- Last 5 plans: -
- Trend: -

*Updated after each plan completion*
| Phase 01 P02 | 2min | 2 tasks | 2 files |
| Phase 01 P01 | 3min | 2 tasks | 7 files |
| Phase 01 P04 | 8min | 2 tasks | 15 files |
| Phase 01 P05 | 1min | 2 tasks | 2 files |
| Phase 02 P02 | 5min | 2 tasks | 4 files |
| Phase 02 P01 | 8min | 2 tasks | 9 files |
| Phase 02 P04 | 10min | 2 tasks | 10 files |
| Phase 02 P05 | 11min | 2 tasks | 14 files |
| Phase 02 P06 | 5min | 2 tasks | 4 files |
| Phase 03 P01 | 3min | 2 tasks | 13 files |
| Phase 03 P03 | 5min | 2 tasks | 10 files |
| Phase 03 P04 | 7min | 3 tasks | 36 files |
| Phase 03 P06 | 8min | 2 tasks | 36 files |
| Phase 04 P03 | 2min | 2 tasks | 5 files |
| Phase 04 P04 | 3min | 3 tasks | 4 files |
| Phase 05 P03 | 5min | 2 tasks | 12 files |
| Phase 06 P02 | 3min | 2 tasks | 7 files |
| Phase 07 P02 | 2min | 2 tasks | 6 files |
| Phase 08 P02 | 4min | 2 tasks | 7 files |
| Phase 09 P02 | 2min | 1 tasks | 2 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [v1.1 Roadmap]: Five phases derived from natural REQ-ID groupings (BUILD, HAL, T32, SCHEMA, EDITOR) — each ends with both T16 and T32 functional
- [v1.1 Roadmap]: Phase 11 must keep T16 working when introducing variant constants; T32 build compiles but hardware bring-up deferred to Phase 12
- [v1.1 Roadmap]: Schema migration is single hand-written transform — `UniversalConfiguration` and reversible migrations explicitly out of scope
- [Roadmap]: Protocol-first approach -- SysEx contract defined before either codebase is touched
- [Roadmap]: DataManager rewrite in Phase 1 (prerequisite for per-param sync, prevents flash wear multiplication)
- [Roadmap]: Phases 2 and 3 both depend on Phase 1, Phase 4 depends on both (diamond dependency)
- [Phase 01]: ConfigManager uses integer field IDs for per-parameter setters, matching SysEx addressing scheme
- [Phase 01]: MigrateIfNeeded handles v100-199 range (not just v103) for forward compatibility with v102 defaults
- [Phase 01]: Used json-schema-to-typescript instead of quicktype (Node 22 compatibility)
- [Phase 01]: Restructured platformio.ini to use [env_common] for ESP32 envs, keeping native test env clean
- [Phase 01]: Native test pattern: include .cpp files directly in test_main.cpp for PlatformIO native linking
- [Phase 01]: test_deserialize_missing_global_key asserts TRUE -- v103 flat-format fallback is correct behavior
- [Phase 02]: Concrete transport classes as nested private classes in MidiProvider.cpp, not in header
- [Phase 02]: SysEx dispatch kept separate from transport loop (USB+BLE only, not serial)
- [Phase 02]: Kept enums unscoped with global using declarations for backward compatibility -- avoids touching 75+ references in main.cpp
- [Phase 02]: Used namespace t16 with using declarations to bridge old and new code gradually
- [Phase 02]: SliderProcessor takes InputProcessor reference for strum state access in STRUMMING mode
- [Phase 02]: ButtonHandler uses ModeManager.cycleSliderMode() instead of inline allowed_modes arrays (D-04)
- [Phase 02]: File-scope trampolines for C-style callbacks, LedManager factory for mode patterns, Palettes split to .hpp/.cpp
- [Phase 02]: Tested InputProcessor note logic via Scales.cpp directly rather than full class instantiation (too many hardware deps for native env)
- [Phase 02]: MockTransport pattern established for testing transport dispatch with recording fields
- [Phase 03]: Pinned ESLint to 9.x -- eslint-plugin-react-hooks does not yet support ESLint 10
- [Phase 03]: Used radix-ui unified package instead of individual @radix-ui/* packages
- [Phase 03]: Used react-router v7 (replaces react-router-dom v6)
- [Phase 03]: Used .Provider JSX pattern with @types/react v18 for context components
- [Phase 03]: Context + typed hook pattern: createContext<T|null>(null) + useX() with throw guard
- [Phase 03]: Used react-router v7 imports instead of react-router-dom (package alignment)
- [Phase 03]: Explicit file paths for new components to avoid old JSX file shadowing during migration
- [Phase 03]: Downgraded ESLint from strictTypeChecked to recommendedTypeChecked to avoid false positives with React patterns
- [Phase 04]: 100ms delay before esp_restart() to allow USB ACK flush
- [Phase 04]: ESP32-S3 VID 0x303A / PID 0x1001 for auto-detecting bootloader serial port
- [Phase 04]: 500ms ACK timeout with single retry for param sync measurement
- [Phase 05]: Used --legacy-peer-deps for vite-plugin-pwa (Vite 8 peer dep gap)
- [Phase 05]: BLE connectBLE() defers full MIDI bridging -- SysEx chunking needed first
- [Phase 06]: CC updates always send channel+id together (atomic) matching firmware SetCCParam
- [Phase 07]: FACTORY_RESET command ID is 0x06, matching firmware SysExProtocol.hpp
- [Phase 08]: SysExSender union type (Output | MidiTransport) for transport-agnostic SysEx functions
- [Phase 08]: BLE transport only set for BLE connections; USB keeps output directly; transport ?? output fallback
- [Phase 09]: Map ValidationError objects to field:message strings for import error display

### Pending Todos

None yet.

### Roadmap Evolution

- Phase 10.1 inserted after Phase 10: Lib_deps Conflict Hotfix (URGENT) — pin compatible third-party lib versions so all 4 envs link; unblocks deferred T16 hardware smoke test from Phase 10.

### Blockers/Concerns

- [v1.1 Discuss]: 8 open questions in research digest — variant field semantics, firmware distribution (auto-detect vs pick), v103→vNext field diff, per-key array extension semantics, calibration file format, Koala mode on T32, namespace style, T64 placeholder. Resolve in `/gsd-discuss-phase`.
- [Research]: BLE chunked SysEx -- full config dumps exceed 20-byte BLE MTU. Needs implementation plan before Phase 4.
- [Research]: Signal::Emit thread safety across ADC/loop core boundary -- verify before Phase 2 extraction.
- [Research]: ESP32-S3 software bootloader entry sequence for PlatformIO + Arduino Core 2.x -- verify before Phase 4.

## Session Continuity

Last session: 2026-04-29T00:00:00.000Z
Stopped at: v1.1 roadmap drafted (Phases 10-14)
Resume file: None
