# Milestone v1.1 Requirements — Variant Support

**Goal:** Add compile-time T16/T32 variant support on top of v1.0 firmware and editor-tx, leveraging the validated hardware bring-up from `origin/3dot0`.

**Research:** `.planning/research/v1.1-variant-research.md`

---

## v1.1 Requirements

### Build System

- [ ] **BUILD-01**: Per-variant pinout headers (`pinout_t16.h`, `pinout_t32.h`) with a single `pinout.h` that aliases `CurrentPinout` to the active variant via build flag
- [ ] **BUILD-02**: Four PlatformIO build envs (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`) selecting variant via `-DT16` / `-DT32`
- [ ] **BUILD-03**: Default build target (`default_envs`) builds T16 release so existing `pio run` flows keep working

### Hardware Abstraction

- [ ] **HAL-01**: `HardwareVariantConfig` constexpr struct exposes `TOTAL_KEYS`, `MUX_COUNT`, `LED_COUNT` as compile-time constants per variant
- [ ] **HAL-02**: `Adc`, `Keyboard`, `LedManager`, `DataManager` consume `HardwareVariantConfig` constants instead of macros (`BANK_AMT`, `NUM_LEDS`, etc.)
- [ ] **HAL-03**: `MultiplexerConfig` struct describes each mux (commonPin, enablePin, shared selectPins, keyMapping, useSharedSelect); used by `Adc::InitMux`
- [ ] **HAL-04**: Capability flags (`HAS_TOUCH_SLIDER`, `SUPPORTS_KOALA_MODE`) expose feature availability per variant; editor-tx hides unsupported features in T32 UI

### T32 Hardware Bring-Up

- [ ] **T32-01**: Multi-mux ADC scan batches all muxes per channel — set `S0..S3` once, read every `commonPin` before incrementing channel
- [ ] **T32-02**: T32 dual-mux initialization with shared select pins and separate common pins, matching `origin/3dot0` reference behavior
- [ ] **T32-03**: T32 key permutation (validated 32-key array from `origin/3dot0`) decomposes into two 16-entry `keyMapping[]` per mux
- [ ] **T32-04**: Calibration routine completes successfully on physical T32 hardware and persists per-variant calibration to LittleFS

### Config Schema & Migration

- [ ] **SCHEMA-01**: JSON config gains required top-level `"variant": "T16" | "T32"` discriminator; schema version bumped (v200 → vNext)
- [ ] **SCHEMA-02**: Per-key arrays sized by `TOTAL_KEYS` are variant-bound; cross-variant load semantics decided in discuss-phase (extend-with-defaults, truncate, or warn-and-prompt) and surfaced to user
- [ ] **SCHEMA-03**: Single hand-written migration rule transforms v200 configs → vNext (default-inject `variant`); ArduinoJson + transform function only — no `UniversalConfiguration` machinery
- [ ] **SCHEMA-04**: ajv validator (`editor-tx/src/services/configValidator.ts`) enforces `variant` enum and per-variant array sizes on import

### Editor-tx Variant Awareness

- [x] **EDITOR-01**: `ConfigContext` lifts `variant` to context state, derived from device SysEx handshake response or loaded config file (Phase 14, plans 14-01/14-02)
- [x] **EDITOR-02**: Conditional UI for keyboard layout (4×4 for T16, 4×8 for T32) and calibration view branches on `variant` (Phase 14, plans 14-03/14-04/14-05; calibration view deferred — does not yet exist in editor-tx, will reuse <KeyboardGrid>)
- [x] **EDITOR-03**: Per-key array editors (note maps, scales, CC assignments) render the correct number of keys per variant (Phase 14, plan 14-03)
- [x] **EDITOR-04**: Flasher (`pages/Upload.tsx`) selects the correct `.bin` for the target variant — selection mechanism (user pick vs auto-detect via existing connection) decided in discuss-phase (Phase 14, plan 14-05; T32 binary awaits Phase 12 hardware tag)

---

## Future Requirements

Deferred from v1.1; revisit in a later milestone:

- **Calibration variant tagging on disk** — `/calibration_data.json` carries variant tag and force-recalibrates on mismatch (current v1.1 plan: per-variant filenames or tolerated wipe on first boot of new variant)
- **On-disk header for config robustness** — magic + version + size + checksum prefix on LittleFS JSON
- **T64 variant** — no hardware exists; revisit when prototype is on hand
- **Koala mode / T16-only feature redesign on T32** — disabled on T32 in v1.1; full redesign deferred

---

## Out of Scope

Explicit exclusions for v1.1 (with reasoning):

- **`ParameterRegistry` + `ParameterDescriptor`** — duplicates editor-tx's ajv on-device, burns flash; v1.0 architecture already covers parameterization
- **`FeatureFlagManager` 64-bit bitmask** — speculative future flags (`MIDI_2_0_SUPPORT`, `CLOUD_PRESETS`, etc.) violate YAGNI
- **`UniversalConfiguration` heap-allocated key-value bag** — ArduinoJson already provides extensibility
- **`ExtensibleConfiguration` TLV/extension blocks** — JSON is already extensible
- **Reversible / downgrade migrations** — no user need; downgrades not supported
- **New binary SysEx protocol** — v1.0 standardized on JSON-over-SysEx; do not entangle with variant work
- **Triple-redundancy + CRC32 critical-data protection** — solving a non-observed problem
- **Editor-tx rewrite** — already at quality bar (TypeScript, React 19, Radix design system); variant work fits existing seams
- **Doc's `static constexpr auto MUX_CONFIGS` aliasing in headers** — ODR risk, already a known issue per CLAUDE.md

---

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 10 | Pending |
| BUILD-02 | Phase 10 | Pending |
| BUILD-03 | Phase 10 | Pending |
| HAL-01 | Phase 11 | Pending |
| HAL-02 | Phase 11 | Pending |
| HAL-03 | Phase 11 | Pending |
| HAL-04 | Phase 11 | Pending |
| T32-01 | Phase 12 | Pending |
| T32-02 | Phase 12 | Pending |
| T32-03 | Phase 12 | Pending |
| T32-04 | Phase 12 | Pending |
| SCHEMA-01 | Phase 13 | Pending |
| SCHEMA-02 | Phase 13 | Pending |
| SCHEMA-03 | Phase 13 | Pending |
| SCHEMA-04 | Phase 13 | Pending |
| EDITOR-01 | Phase 14 | Complete (software gates passed; hardware UAT batched to v1.1 close) |
| EDITOR-02 | Phase 14 | Complete (calibration view deferred — pre-existing) |
| EDITOR-03 | Phase 14 | Complete |
| EDITOR-04 | Phase 14 | Complete (T32 binary awaits Phase 12 tag) |

**Coverage:** 19/19 v1.1 requirements mapped (no orphans, no duplicates).
