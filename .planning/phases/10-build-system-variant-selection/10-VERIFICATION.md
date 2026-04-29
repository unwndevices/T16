---
phase: 10-build-system-variant-selection
status: human_needed
plans_complete: 4
plans_total: 4
requirements_complete: [BUILD-01, BUILD-02, BUILD-03]
verified: 2026-04-29
---

# Phase 10: Build System & Variant Selection — Verification

## Summary

Phase 10 is **code-complete**: all 4 plans landed, every acceptance criterion in every PLAN.md passes, and the tree-wide PIN_* audit is clean. The phase **cannot be marked `passed`** because ROADMAP success criterion #4 (manual T16 hardware smoke test) requires a working `firmware.elf`, and the binary currently fails to link due to a pre-existing third-party library conflict that is **outside Phase 10's scope** to fix.

| Requirement | Status | Evidence |
|-------------|--------|----------|
| BUILD-01 — per-variant pinout headers | ✓ | `src/pinout.h`, `src/pinout_t16.h`, `src/pinout_t32.h` exist; tree-wide PIN_* audit returns 0 lines |
| BUILD-02 — four-env platformio.ini | ✓ | `platformio.ini` declares t16_debug, t16_release, t32_debug, t32_release; default_envs = t16_release |
| BUILD-03 — CI matrix over all four envs | ✓ | `.github/workflows/ci.yml` `firmware-build` job uses `matrix: env: [t16_debug, t16_release, t32_debug, t32_release]` with `fail-fast: false` |

| ROADMAP Success Criterion | Status | Notes |
|---------------------------|--------|-------|
| #1: `pio run -e t16_release` and `pio run -e t32_release` produce binaries | ⚠ Code compiles cleanly for both envs; firmware.elf link blocked by pre-existing lib conflicts (NOT introduced by Phase 10) |
| #2: Bare `pio run` builds T16 release | ✓ default_envs = t16_release |
| #3: CurrentPinout resolves correctly per variant | ✓ Per-translation-unit compile success under both -DT16 and -DT32 |
| #4: T16 hardware behavior identical after rename | ⚠ **HUMAN VERIFICATION NEEDED** — see below |

## Plans Completed

| Plan | Status | Commits | Notes |
|------|--------|---------|-------|
| 10-01 platformio.ini split | ✓ | `7e54a5a` | All 12 acceptance criteria pass |
| 10-02 namespaced pinout headers | ✓ | `a216a40`, `df73a08`, `901c1a3` | All grep checks pass; deviation 1 (namespace alias syntax) caught and fixed in Plan 03 |
| 10-03 call-site migration | ✓ (code) / ⚠ (binary blocked) | `d5650c8`, `529fc2d`, `ea6a82a` | All 13 grep checks pass; tree-wide audit clean; per-TU compile clean; 2 Rule-1 deviations auto-fixed |
| 10-04 CI matrix YAML | ✓ (Task 1) / human_needed (Task 2) | `e31641a` | All 10 Task 1 acceptance criteria pass; Task 2 routed here |

## Deviations Auto-Fixed

| Rule | Plan | Issue | Fix | Commit |
|------|------|-------|-----|--------|
| 1 | 03 | PLAN.md/RESEARCH.md specified `using CurrentPinout = pinout::tXX;` — invalid C++ (using-aliases declare types, not namespaces) | Changed to `namespace CurrentPinout = pinout::tXX;` in src/pinout.h | `ea6a82a` |
| 1 | 03 | `constexpr uint8_t TX2 = 42;` collides with Arduino-ESP32 `#define TX2 (gpio_num_t)20` macro from HardwareSerial.h | Renamed `TX2 → TX2_PIN` in pinout_t16.h, pinout_t32.h, and the AppEngine.cpp call site (matches LED_PIN _PIN-suffix precedent) | `ea6a82a` |

## Items Requiring Human Verification

### 1. T16 hardware smoke test (Plan 04 Task 2 — checkpoint:human-verify)

**Why deferred:** The smoke test requires flashing a `t16_release` firmware.bin to physical T16 hardware and confirming identical pre-Phase-10 MIDI/LED/button behavior. Currently the binary cannot be linked due to a pre-existing third-party library multi-definition conflict that Phase 10 did not introduce and is out of scope to fix.

**Pre-existing blocker (third-party library link conflict):**
- **t16_release:** `multiple definition` errors between Arduino-ESP32's prebuilt `libarduino_tinyusb.a` (provides `hcd_dwc2_*` USB host controller) and `Adafruit TinyUSB Library @ 3.3.0` from lib_deps (provides `hcd_max3421_*` host controller). Both unconditionally export the same `hcd_*` interface.
- **t32_release:** `multiple definition of bleMidi::BLEMIDI_ESP32_NimBLE::begin(...)` — ODR violation in `BLE-MIDI 2.2.0` lib_dep header (`begin` defined in `.h` without `inline`).

**To unblock the smoke test (recommended next steps, not Phase 10 work):**
- Open a `gsd-debug` session or a focused 10.1 hotfix to pin compatible versions of `framework-arduinoespressif32`, `Adafruit TinyUSB Library`, and `BLE-MIDI` so all four envs link.
- Once binaries link, perform the smoke test described in `10-04-PLAN.md` Task 2:
  1. `pio run -e t16_release` produces `.pio/build/t16_release/firmware.bin`.
  2. Flash to physical T16 (`pio run -e t16_release -t upload` or use the editor-tx web flasher).
  3. Power-cycle and verify startup animation, all 16 keys producing NoteOn/NoteOff, touch slider CC output, MODE button cycling modes, TOUCH button registering, TRS MIDI output (PIN_TX2_PIN / PIN_TX migration in Plan 03).
  4. Confirm no behavioral regression vs. a known-good v1.0 binary.

### 2. CI matrix execution

The next `git push` will trigger the new 4-leg matrix in GitHub Actions. All four legs are expected to FAIL at the link step until the third-party library conflict is resolved — that is consistent with the local results documented in `10-03-SUMMARY.md` and is not a regression introduced by the YAML change.

## Verification Verdict

**Status: human_needed**

Phase 10's code-level objectives (per-variant pinout headers, build-flag-driven variant selection, namespace-based call-site API, CI matrix) are complete and verified. The hardware smoke test cannot be performed by the orchestrator and depends on a binary that is blocked by a separate, pre-existing third-party library issue. The user must decide whether to:

1. Accept Phase 10 as done at the code level and tackle the lib-deps conflict in a focused 10.1 hotfix before Phase 11 begins,
2. Run the smoke test against a v1.0 reference binary (no Phase 10 code) to satisfy criterion #4 retroactively,
3. Defer the smoke test until Phase 11 (HAL) when the lib_deps issue is resolved as part of broader firmware refactor work.
