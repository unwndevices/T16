# Phase 12 — T32 Hardware Bring-Up: Research

**Status:** Manual research notes (orchestrator-produced).
**Source of truth for behavior:** `origin/3dot0` branch.

---

## 1. Canonical `origin/3dot0` references

Confirmed accessible after `git fetch origin 3dot0` (remote: `github.com:unwndevices/T16`).

| Concern | File @ origin/3dot0 | Key lines / symbols |
|---|---|---|
| 32-key permutation array | `src/main.cpp` | Line 9–13: `#ifdef T32` block declaring `Key keys[]` of length 32 |
| Dual-mux ADC scan loop | `src/Libs/Adc.cpp` | `Adc::ReadValues()` — sets `SetMuxChannel(iterator)` once, reads `_pin` then conditionally reads `_pin2` for the second mux on the same channel |
| Dual-pin storage | `src/Libs/Adc.hpp/cpp` | `_pin2` field and `InitMux(... uint8_t pin2 = 0)` overload |
| Iterator semantics | `src/Libs/Adc.cpp` | `iterator++; if (iterator >= _channels.size() / 2) iterator = 0;` — iterates 0..15 only, both muxes read in same iteration |
| Calibration pin selection | `src/Libs/Adc.cpp` | `CalibrateMin/Max`: `uint8_t pin = (chn < 16) ? _config._pin : _config._pin2;` |
| GetMuxRaw indexing | `src/Libs/Adc.cpp` | `GetMuxRaw(index)` returns `_channels[index].raw` for `index ∈ [0, 32)` |
| Pinout (single-mux era) | `src/pinout.h` | Only `PIN_COM` (single common pin) — 3dot0 hard-codes the second pin elsewhere; current T16 codebase has no second pin defined yet → **action: add `pinout::t32::COM2` to `pinout_t32.h` in Plan 03** |

### Validated 32-key permutation (T32-03 ground truth)

From `origin/3dot0:src/main.cpp:10`:

```c++
Key keys[] = {
    14, 15, 13, 12,  30, 31, 29, 28,
    10, 11,  8,  9,  26, 27, 24, 25,
     1,  0,  3,  2,  17, 16, 19, 18,
     5,  4,  7,  6,  21, 20, 23, 22
};
```

**Decomposition into per-mux `keyMapping[16]`** (interleaved column-major: every 4-key column belongs to mux 0 if 3dot0 indices are 0–15, mux 1 if 16–31):

Reading the array as 8 columns × 4 rows, indices 0–15 (mux 0) and 16–31 (mux 1) alternate every 4 entries:

- mux 0 channels 0..15 (read order = scan iterator 0..15) map to physical key indices:
  `14, 15, 13, 12,  10, 11, 8, 9,  1, 0, 3, 2,  5, 4, 7, 6`
- mux 1 channels 0..15 (read order = same iterator, second mux read) map to physical key indices:
  `30, 31, 29, 28,  26, 27, 24, 25,  17, 16, 19, 18,  21, 20, 23, 22`

**Method:** the 3dot0 `Adc::ReadValues()` writes mux-0 sample to `_channels[iterator]` and mux-1 sample to `_channels[iterator + 16]`. The 3dot0 `keys[]` array is consumed in declaration order with `Key(uint8_t index)` storing `mux_idx = index`. So `keys[0]` reads from `_channels[14]` (mux 0, channel 14) — meaning **the value at `keys[0]` is whatever physical key is wired to mux 0 channel 14**. To preserve identical behavior with the new `keyMapping[]` field, mux 0's `keyMapping[ch]` must answer "for scan channel `ch`, which logical key index does this electrical channel produce?". Inverting the 3dot0 array gives that mapping.

Plan 02 contains the inversion table and a pure-host unit test that asserts the inversion against the 3dot0 array.

---

## 2. LittleFS rename support

ESP32 Arduino core (espressif32 v6.x): `LittleFS.rename(const char* pathFrom, const char* pathTo)` is supported (return type `bool`). Confirmed in `<LittleFS.h>` of arduino-esp32. Fallback (read+write+delete) is only required if a future port targets a core that exposes only `FS::rename` on a sub-FS.

**Decision:** use `LittleFS.rename` directly; assert non-empty result before deleting source.

---

## 3. Scan-time budget reference (T32-01 success gate, D12.4)

Per CONTEXT D12.4: "T32 scan loop time stays within the existing T16 budget — no per-mux inflation, both muxes batched per channel."

**Operationalization (no oscilloscope dependency):**
- Add a microsecond log inside `Adc::ReadValues()` once per full 16-channel pass (not per-channel — that would distort timing).
- T16 baseline: capture 5–10 sequential pass times in `LOG_DEBUG` mode and record in the verification artifact.
- T32 acceptance: median pass time within `2× T16 baseline` (we read 2 muxes per channel, but only one extra `analogRead` ≈ 11–14 µs on ESP32-S3, so realistic budget is ~1.05–1.15× — leave 2× as a regression ceiling, not the target).

Concrete number to fill at T16-baseline-capture step (Plan 04 verification): TBD on hardware. **Soft target documented now: T16 ≈ 200–400 µs per full 16-channel pass under `analogRead`. T32 hard ceiling: 2× T16 measured baseline.**

---

## 4. `MultiplexerConfig::keyMapping` sizing

Already locked at `std::array<uint8_t, 16>` in `src/variant_config.hpp:30`. No change needed; this size is correct for 16:1 muxes (CD74HC4067-class). Future variants with different mux widths would change the type — out of scope for Phase 12.

---

## 5. Validation Architecture

Tests live in two places:

| Test | Type | Location |
|---|---|---|
| keyMapping decomposition matches inverted 3dot0 array | Pure host C++ unit test | `test/test_variant_t32_keymap/test_main.cpp` (PlatformIO `test_native` env if it exists, else inline static_assert in variant_t32.hpp) |
| Scan iterator visits each channel once and reads both common pins | Logic test (mock `analogRead`/`digitalWrite`) | Optional — if `test_native` is available; otherwise documented manual verification |
| Per-variant calibration filename | Inline assert + grep verification on `AppEngine.cpp` and `CalibrationService.cpp` | Plan 01 acceptance criteria |
| Migration of legacy `/calibration_data.json` → `/calibration_t16.json` | Behavioral test on hardware (file appears; old name deleted) | Plan 01 + manual T16 hardware verify (deferred to milestone-end batch) |

**Hardware-dependent items deferred to end-of-milestone batch (per orchestrator instruction):**
- Oscilloscope verification of `S0..S3` driven once per channel.
- T32 boot, scan, and calibration on physical hardware.
- T16 regression smoke after dual-mux changes.
- Calibration file migration on a real T16 unit.

These surface as `checkpoint:human-verify` tasks tagged `validation_deferred: true` in their plans, and as a "Validation Deferred" section in `12-VERIFICATION.md`.
