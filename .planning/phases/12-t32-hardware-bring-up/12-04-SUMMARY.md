---
plan_id: 12.04
phase: 12
status: complete
completed: 2026-04-29
hardware_verification: deferred (milestone v1.1 batch)
---

# Plan 12.04 — Summary

## What Was Built

- `src/Configuration.hpp` — `CalibrationData` now sizes `minVal[]` / `maxVal[]` via `static constexpr kSize = variant::CurrentVariant::kConfig.TOTAL_KEYS`. T16 → 16, T32 → 32.
- `src/AppEngine.hpp` — `Key keys_[]` resized via `kConfig.TOTAL_KEYS`. Added T32 branch with sequential `{0..31}` initializer (logical_key = position under keyMapping-driven translation from 12-03). T16 `REV_B` / non-`REV_B` initializers preserved.
- `src/AppEngine.cpp` — Five literal `16`s in calibration call sites swapped to `CalibrationData::kSize`. `keyboard_config.Init` now uses `kConfig.TOTAL_KEYS`.
- `src/services/CalibrationService.cpp` — Local arrays `minVal[]`, `maxVal[]`, `minVals[]`, `maxVals[]` sized via `constexpr kMaxKeys = kConfig.TOTAL_KEYS`. `numKeys` argument is consumed end-to-end.
- `src/Libs/Adc.cpp` — Added `SCAN_TIMING_LOG`-gated per-pass µs timer. Records `micros()` at `iterator==0` start, computes elapsed at the next `iterator==0` (= one full 16-channel pass), prints every 32nd pass over Serial. Zero overhead in default build.

## Verification

Software gates (binding):
- `grep -n 'minVal\[16\]\|maxVal\[16\]' src/Configuration.hpp` → 0 matches.
- `grep -n 'CalibrationData::kSize\|kSize = variant::CurrentVariant' src/Configuration.hpp` → 1 match.
- `grep -c 'CalibrationData::kSize\|kConfig.TOTAL_KEYS' src/AppEngine.{cpp,hpp}` → ≥5 matches.
- `grep -n 'minVals\[16\]\|maxVals\[16\]' src/services/CalibrationService.cpp` → 0 matches.
- `grep -n 'TOTAL_KEYS\|kSize\|CalibrationFilePath' src/services/CalibrationService.cpp` → present.
- `grep -c 'SCAN_TIMING_LOG' src/Libs/Adc.cpp` → 5 matches (definition guard, two `#ifdef` blocks, `#endif`s, comment).
- `grep -n 'g_pass_count' src/Libs/Adc.cpp` → 4 matches.
- All four PlatformIO envs build clean (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`).

Hardware (deferred):
- Task 12.04.5 — recorded in `12-VERIFICATION.md` "Validation Deferred → Milestone v1.1 Batch":
  - **A**. T16 regression smoke (16 keys + slider + persistence)
  - **B**. T32 dual-mux scan + key permutation (32 keys raster, spot-check)
  - **C**. T32-01 binding gate — scan-time measurement T32 ≤ 2× T16 baseline
  - **D**. Calibration persistence cross-check
  - **E**. GPIO-COM2 schematic confirmation (carries over from 12.03.5.A)

## Key Files Changed

- `src/Configuration.hpp` — `CalibrationData::kSize`.
- `src/AppEngine.hpp` — `keys_[]` sized via `kConfig.TOTAL_KEYS`, T32 branch added.
- `src/AppEngine.cpp` — 6 literal-16 sites swapped to `CalibrationData::kSize` / `kConfig.TOTAL_KEYS`.
- `src/services/CalibrationService.cpp` — local arrays sized via `kMaxKeys`.
- `src/Libs/Adc.cpp` — `SCAN_TIMING_LOG` block.

## Notes / Deviations

- T32 `keys_` initializer chosen as sequential `{0..31}` (each Key reads `_channels[mux_idx == position]`). This is the natural match for the keyMapping-driven `_channels[]` translation introduced by Plan 12.03. The plan did not specify this explicitly — the rationale is documented inline in `AppEngine.hpp`.
- T16 `keys_` initializer kept verbatim (`{14, 15, 13, ...}` for REV_B, alt for non-REV_B). T16 keyMapping is identity, so `_channels[]` is still indexed by raw electrical channel — preserving original semantics.
- `SCAN_TIMING_LOG` is opt-in via build flag. The plan suggested a custom env (`t32_scan_timing`); deferred — developers can enable with `pio run -e t32_debug --project-option="build_flags=-DSCAN_TIMING_LOG"` instead. This avoids env churn in `platformio.ini`, which the v1.1 milestone has been treating as settled.
- Plan referenced PlatformIO envs as `esp32s3 / release / t32 / t32_release`; actual env names are `t16_debug / t16_release / t32_debug / t32_release`. Built against the actual names — all four pass.

## Self-Check: PASSED
