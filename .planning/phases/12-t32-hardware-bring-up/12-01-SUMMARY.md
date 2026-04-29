---
plan_id: 12.01
phase: 12
status: complete
completed: 2026-04-29
hardware_verification: deferred (milestone v1.1 batch)
---

# Plan 12.01 — Summary

## What Was Built

- `variant::CalibrationFilePath()` helper in `src/variant.hpp` returns the active variant's calibration filename, derived from `variant::CurrentVariant::kConfig.NAME` at compile time. T16 → `/calibration_t16.json`, T32 → `/calibration_t32.json`.
- `variant::kLegacyCalibrationPath` constant exposes the pre-v1.1 filename `/calibration_data.json` for the migration helper.
- `MigrateLegacyCalibrationFile()` in `src/AppEngine.cpp` (anonymous namespace, file-private) one-shot renames the legacy file to the per-variant filename on first boot when applicable. Skips T32 entirely (no legacy file ever existed there) and skips when the new file already exists. Logs success/failure over Serial.
- All four `LittleFS` literal references to `/calibration_data.json` replaced with `variant::CalibrationFilePath()`:
  - `src/AppEngine.cpp` boot-time load
  - `src/services/CalibrationService.cpp` calibration save
  - `src/SysExHandler.cpp` SysEx calibration reset
  - `src/SysExHandler.cpp` SysEx factory reset

## Verification

Software gates (binding):
- `grep -rn '"/calibration_data.json"' src/` returns 1 match — the `kLegacyCalibrationPath` constant in variant.hpp (acceptable; the literal is no longer used at any LittleFS call site).
- `grep -rn 'CalibrationFilePath' src/` returns 8 matches (definition + 4 use sites + 3 SysEx/Migration references).
- All four PlatformIO envs build clean (t16_debug, t16_release, t32_debug, t32_release).

Hardware (deferred):
- Task 12.01.3 — recorded in `12-VERIFICATION.md` "Validation Deferred → Milestone v1.1 Batch".

## Key Files Changed

- `src/variant.hpp` (created `CalibrationFilePath()`, `kLegacyCalibrationPath`)
- `src/AppEngine.cpp` (added `MigrateLegacyCalibrationFile()`, swapped literal)
- `src/services/CalibrationService.cpp` (swapped literal, added include)
- `src/SysExHandler.cpp` (swapped 2 literals)

## Notes / Deviations

- The literal string `"/calibration_data.json"` still appears once in `src/variant.hpp` as the value of `kLegacyCalibrationPath`. The plan's grep-zero rule applies to use sites; the constant definition is the single intended residue.
- Plan referenced PlatformIO envs as `esp32s3 / release / t32 / t32_release`; the actual env names in `platformio.ini` are `t16_debug / t16_release / t32_debug / t32_release`. Built against the actual names — all four pass.

## Self-Check: PASSED
