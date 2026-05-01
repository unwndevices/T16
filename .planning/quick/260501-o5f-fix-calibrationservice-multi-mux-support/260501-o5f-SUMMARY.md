---
quick_id: 260501-o5f
type: execute
status: complete
completed: 2026-05-01
commits:
  - b6fa786  # Task 1: Key::mux_id + PopulateKeyMuxMapping
  - 4955030  # Task 2: multi-mux Adc API
  - b9a4789  # Task 3: CalibrationService wiring
---

# 260501-o5f — Fix CalibrationService for Multi-Mux Hardware

## One-liner

CalibrationService now samples the correct mux's commonPin per key, so T32 (2 muxes, 32 keys) calibrates and self-tests keys 16–31 against mux 1 (GPIO17) instead of mirroring mux 0; T16 path is byte-equivalent.

## Files Changed

| File | Rationale |
|------|-----------|
| `src/Libs/Keyboard.hpp` | Add `Key::mux_id` and `Key::mux_channel` fields + declare `PopulateKeyMuxMapping` free function. |
| `src/Libs/Keyboard.cpp` | Implement `PopulateKeyMuxMapping`: scan variant `MultiplexerConfig::keyMapping` for each Key's logical index, store owning mux + 0..15 channel-within-mux. |
| `src/AppEngine.cpp` | Call `PopulateKeyMuxMapping` immediately after `Adc::InitMuxes`, before any calibration path runs. |
| `src/Libs/Adc.hpp` | Declare two-arg `CalibrateMin(logical_key, mux_id)` / `CalibrateMax(...)` and `GetRawForMux(mux_id)`; keep single-arg overloads as wrappers. |
| `src/Libs/Adc.cpp` | New two-arg overloads sample `_muxes[mux_id].commonPin` and write `_channels[logical_key]`. Single-arg versions become thin wrappers (`SetMuxChannel(chn)` + delegate to `mux_id=0`). `GetRaw()` no-arg untouched. |
| `src/services/CalibrationService.cpp` | `runHardwareTest` and both loops in `runCalibration` now use `SetMuxChannel(keys[i].mux_channel)` + `GetRawForMux(keys[i].mux_id)` + 2-arg `CalibrateMin/Max(i, keys[i].mux_id)`. `minVal[]`/`maxVal[]` now indexed by logical key `i` (matches `SetCalibration`/`GetCalibration` iteration). Indexing convention documented inline. |

## Build Verification

- `pio run -e t16_debug` → SUCCESS (RAM 13.4%, Flash 68.3%)
- `pio run -e t32_debug` → SUCCESS (RAM 13.7%, Flash 68.3%)

Both ran clean after each task; final builds confirm no regressions from cumulative changes.

## T16 Equivalence Check

T16 has a single mux with identity `keyMapping = {0..15}`. For every Key:

- `mux_id = 0` (only mux that exists)
- `mux_channel = mux_idx` (because `keyMapping[mux_idx] == mux_idx`)

So `SetMuxChannel(keys[i].mux_channel)` ≡ old `SetMuxChannel(keys[i].mux_idx)` and `GetRawForMux(0)` ≡ old `GetRaw()` (both do `analogRead(_config._pin)` since `_config._pin` is mirrored to mux 0's commonPin in `InitMuxes`). Behavior is byte-equivalent on T16.

## Manual Follow-up

T32 hardware calibration ritual must be exercised on real hardware to confirm:

- Keys 16–31 produce sensible idle/pressed deltas during the calibration ritual (idle ≈ 1500–1800, pressed ≈ 3500–4000) instead of the mux-0 mirrored values.
- Hardware self-test (`runHardwareTest`) does not flag false fails on the upper-mux keys.

This is a hardware-only verification — no automated test exists. Batch it with the milestone v1.1 hardware UAT.

## Pre-existing Issue Surfaced (Out of Scope — Flagged by Planner)

There is a latent positional-vs-logical `_channels[]` indexing inconsistency on T16 REV_B that this fix preserves intentionally:

- `keys_[]` on T16 REV_B is `{14, 15, 13, 12, 10, 11, 8, 9, 1, 0, 3, 2, 5, 4, 7, 6}` — each Key's `mux_idx` is the *logical channel into `_channels[]`* it should read at runtime.
- `Key::Update` reads `adc->GetMux(0, mux_idx)` → `_channels[mux_idx].value` (e.g., position 0 reads `_channels[14]`).
- But `SetCalibration(min, max, n)` and `GetCalibration(...)` iterate `_channels[i] = min[i]` by *positional* index, and this fix continues that convention: `CalibrateMin(i, mux_id)` writes to `_channels[i]`.
- Result on T16 REV_B: position 0's calibration ends up in `_channels[0]`, but position 0's runtime read pulls from `_channels[14]` — they target different slots.

This mismatch existed before today's change and is not introduced by it. Fixing it requires deciding which side is canonical (per-position calibration vs per-logical-channel storage) and aligning `keys_[]` semantics across `Update`, `SetCalibration`, and `GetCalibration` consistently. Track as a separate task if it ever surfaces as a user-visible miscalibration on T16 REV_B; today's change keeps the existing convention so current LittleFS calibration files remain valid.

## Success Criteria Confirmation

- [x] Both T16 and T32 PlatformIO debug builds compile without errors or new warnings.
- [x] Calibration code path uses the multi-mux Adc API for `GetRaw` and `CalibrateMin/Max`.
- [x] T16 build path is byte-equivalent in behavior (single mux, wrappers delegate to `mux_id=0`).
- [x] `Key::mux_id` and `Key::mux_channel` are populated by `PopulateKeyMuxMapping` after `Adc::InitMuxes`.
- [x] Indexing convention in CalibrationService is consistent: first arg / array index = logical key `i`, mux selection via `keys[i].mux_id`, channel select via `keys[i].mux_channel`.

## Self-Check: PASSED

- All 6 modified files exist and contain the documented changes.
- Commits b6fa786, 4955030, b9a4789 all present in `git log`.
- Both build environments succeeded on the final pass.
