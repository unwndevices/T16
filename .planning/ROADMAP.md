# Roadmap: T16 Refactor

## Milestones

- ✅ **v1.0 T16 Refactor** — Phases 1-9 (shipped 2026-04-04)
- 🚧 **v1.1 Variant Support** — Phases 10-14 (in progress)

## Phases

<details>
<summary>✅ v1.0 T16 Refactor (Phases 1-9) — SHIPPED 2026-04-04</summary>

- [x] Phase 1: Protocol & Data Foundation (5/5 plans) — completed 2026-04-03
- [x] Phase 2: Firmware Service Extraction (6/6 plans) — completed 2026-04-03
- [x] Phase 3: Web Rewrite (6/6 plans) — completed 2026-04-03
- [x] Phase 4: Integration & CI (4/4 plans) — completed 2026-04-04
- [x] Phase 5: Feature Polish (3/3 plans) — completed 2026-04-04
- [x] Phase 6: CC Sync & Schema Fix (2/2 plans) — completed 2026-04-04
- [x] Phase 7: Firmware Bug & Tech Debt (2/2 plans) — completed 2026-04-04
- [x] Phase 8: BLE MIDI Bridging (2/2 plans) — completed 2026-04-04
- [x] Phase 9: UI Wiring Gap Closure (2/2 plans) — completed 2026-04-04

Full details: `.planning/milestones/v1.0-ROADMAP.md`

</details>

### 🚧 v1.1 Variant Support (Phases 10-14)

- [⚠] **Phase 10: Build System & Variant Selection** — Per-variant pinout headers and PlatformIO build envs select T16 vs T32 at compile time (CODE COMPLETE — hardware smoke test deferred, see VERIFICATION.md)
- [ ] **Phase 11: Hardware Abstraction Layer** — `HardwareVariantConfig` constexpr carrier replaces fixed macros; firmware classes consume variant constants
- [ ] **Phase 12: T32 Hardware Bring-Up** — Dual-mux ADC scan and validated key permutation boot the physical T32 with calibration persisted
- [ ] **Phase 13: Config Schema & Migration** — Schema gains `variant` discriminator with single migration rule from v200; ajv validator enforces per-variant array sizes
- [ ] **Phase 14: Editor-tx Variant Awareness** — `ConfigContext` carries variant state; conditional UI renders 4×4 vs 4×8 layouts and the flasher selects the correct `.bin`

## Phase Details

### Phase 10: Build System & Variant Selection
**Goal**: Compile-time variant selection picks T16 or T32 binaries via PlatformIO build flags.
**Depends on**: v1.0 (Phase 9 complete)
**Requirements**: BUILD-01, BUILD-02, BUILD-03
**Success Criteria** (what must be TRUE):
  1. `pio run -e t16_release` and `pio run -e t32_release` both produce valid `.bin` artifacts on a clean checkout
  2. `pio run` with no env flag still builds T16 release (default target preserved)
  3. `#include "pinout.h"` resolves `CurrentPinout` to the T16 or T32 pinout namespace based on `-DT16` / `-DT32`
  4. Existing T16 firmware behavior is identical after the rename (smoke test on hardware)
**Plans**: 4 plans
  - [x] 10-01-PLAN.md — Add four PlatformIO envs (t16/t32 × debug/release) and set default_envs = t16_release — completed 2026-04-29
  - [x] 10-02-PLAN.md — Create namespaced pinout headers (pinout_t16.h, pinout_t32.h) and rewrite pinout.h as variant selector with CurrentPinout alias — completed 2026-04-29
  - [x] 10-03-PLAN.md — Migrate all firmware call sites from PIN_* macros to CurrentPinout::* (AppEngine, LedManager, ButtonHandler) and verify both variants compile — completed 2026-04-29 (per-TU compile clean; firmware.elf link blocked by pre-existing third-party lib conflict)
  - [x] 10-04-PLAN.md — Update CI to matrix over four envs, split native tests into sibling job, and run manual T16 hardware smoke test — Task 1 completed 2026-04-29; Task 2 (hardware smoke test) deferred to human verification

### Phase 10.1: Lib_deps Conflict Hotfix (INSERTED)

**Goal:** [Urgent work - to be planned]
**Requirements**: TBD
**Depends on:** Phase 10
**Plans:** 0 plans

Plans:
- [ ] TBD (run /gsd-plan-phase 10.1 to break down)

### Phase 11: Hardware Abstraction Layer
**Goal**: Firmware classes consume `HardwareVariantConfig` constexpr constants instead of fixed macros, with no behavior change on T16.
**Depends on**: Phase 10
**Requirements**: HAL-01, HAL-02, HAL-03, HAL-04
**Success Criteria** (what must be TRUE):
  1. `HardwareVariantConfig::TOTAL_KEYS`, `MUX_COUNT`, `LED_COUNT` are compile-time constants and resolve to 16/1/16 (T16) or 32/2/32 (T32)
  2. `Adc`, `Keyboard`, `LedManager`, `DataManager` no longer reference `BANK_AMT` or `NUM_LEDS` macros — they read from the variant config
  3. `MultiplexerConfig` describes each mux (commonPin, enablePin, shared selectPins, keyMapping, useSharedSelect) and `Adc::InitMux` consumes it
  4. T16 hardware still passes calibration and produces identical MIDI output to pre-refactor (regression check on physical T16)
  5. `T32` build compiles cleanly even though hardware bring-up is deferred to Phase 12
**Plans**: TBD

### Phase 12: T32 Hardware Bring-Up
**Goal**: A physical T32 unit boots, scans both muxes, applies the validated key permutation, and persists calibration.
**Depends on**: Phase 11
**Requirements**: T32-01, T32-02, T32-03, T32-04
**Success Criteria** (what must be TRUE):
  1. Multi-mux ADC scan sets `S0..S3` once per channel and reads each `commonPin` before incrementing — verified by oscilloscope or scan-time measurement
  2. T32 dual-mux init matches `origin/3dot0` reference (shared select pins, separate common pins) and all 32 keys produce distinct readings
  3. The T32 key permutation from `origin/3dot0` decomposes into two 16-entry `keyMapping[]` arrays and physical key positions match logical indices when pressed
  4. Calibration completes successfully on physical T32 hardware and `/calibration_data.json` survives a power cycle with correct per-key thresholds
  5. T16 hardware remains functional (regression check after dual-mux changes)
**Plans**: TBD

### Phase 13: Config Schema & Migration
**Goal**: Config schema carries a `variant` discriminator and migrates v200 configs forward without data loss.
**Depends on**: Phase 12
**Requirements**: SCHEMA-01, SCHEMA-02, SCHEMA-03, SCHEMA-04
**Success Criteria** (what must be TRUE):
  1. Loading a v200 (no-variant) config on a T16 device default-injects `"variant": "T16"` and saves at the new schema version
  2. Per-key arrays are sized by `TOTAL_KEYS` and the cross-variant load path (extend / truncate / warn — chosen in discuss) is exercised by a native unit test
  3. ajv validator (`editor-tx/src/services/configValidator.ts`) rejects configs with missing or invalid `variant` and with wrong-sized per-variant arrays
  4. ArduinoJson migration is a single hand-written transform — no `UniversalConfiguration` machinery introduced
  5. Both T16 and T32 firmware load and persist a config round-trip without losing fields
**Plans**: TBD

### Phase 14: Editor-tx Variant Awareness
**Goal**: The web configurator detects the connected variant, renders the matching keyboard/calibration UI, and flashes the correct `.bin`.
**Depends on**: Phase 13
**Requirements**: EDITOR-01, EDITOR-02, EDITOR-03, EDITOR-04
**Success Criteria** (what must be TRUE):
  1. `ConfigContext` exposes `variant` derived from device SysEx handshake or loaded config file, and components consume it via the typed hook
  2. The keyboard editor renders 4×4 when connected to a T16 and 4×8 when connected to a T32 — verified by manual hardware test on both units
  3. Per-key array editors (note maps, scales, CC assignments) render exactly `TOTAL_KEYS` rows for the active variant
  4. The calibration view branches on variant and shows the correct number of pads
  5. `pages/Upload.tsx` selects the matching `.bin` for the target variant (auto-detect or user-pick — decided in discuss) and flashes it successfully via esptool-js
**Plans**: TBD
**UI hint**: yes

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Protocol & Data Foundation | v1.0 | 5/5 | Complete | 2026-04-03 |
| 2. Firmware Service Extraction | v1.0 | 6/6 | Complete | 2026-04-03 |
| 3. Web Rewrite | v1.0 | 6/6 | Complete | 2026-04-03 |
| 4. Integration & CI | v1.0 | 4/4 | Complete | 2026-04-04 |
| 5. Feature Polish | v1.0 | 3/3 | Complete | 2026-04-04 |
| 6. CC Sync & Schema Fix | v1.0 | 2/2 | Complete | 2026-04-04 |
| 7. Firmware Bug & Tech Debt | v1.0 | 2/2 | Complete | 2026-04-04 |
| 8. BLE MIDI Bridging | v1.0 | 2/2 | Complete | 2026-04-04 |
| 9. UI Wiring Gap Closure | v1.0 | 2/2 | Complete | 2026-04-04 |
| 10. Build System & Variant Selection | v1.1 | 4/4 | Code complete (smoke test deferred) | 2026-04-29 |
| 11. Hardware Abstraction Layer | v1.1 | 0/0 | Not started | - |
| 12. T32 Hardware Bring-Up | v1.1 | 0/0 | Not started | - |
| 13. Config Schema & Migration | v1.1 | 0/0 | Not started | - |
| 14. Editor-tx Variant Awareness | v1.1 | 0/0 | Not started | - |

---
*Roadmap last updated: 2026-04-29 — Phase 10 code complete (4/4 plans); manual T16 smoke test deferred to human verification due to pre-existing third-party lib link conflict*
