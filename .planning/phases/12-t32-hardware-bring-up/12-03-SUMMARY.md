---
plan_id: 12.03
phase: 12
status: complete
completed: 2026-04-29
hardware_verification: deferred (milestone v1.1 batch)
---

# Plan 12.03 — Summary

## What Was Built

- `src/pinout_t32.h` — Added `COM2 = 17` placeholder (ESP32-S3 ADC2_CH6) for the mux 1 common pin, with explicit `PLACEHOLDER GPIO` callout.
- `src/variant_t32.hpp` — Mux 1's `commonPin` swapped from `COM` to `COM2`; only mux 0 still references `COM`. The `static_assert` validation from plan 12.02 continues to pass.
- `src/Libs/Adc.hpp` — Added private `kMaxMuxes = 4`, `MultiplexerConfig _muxes[kMaxMuxes]`, and `uint8_t _mux_count` to track the active mux array.
- `src/Libs/Adc.cpp::InitMuxes` rewritten — stores ALL mux configs, configures S0..S3 ONCE from mux 0, then iterates each mux to configure `commonPin` and any non-shared select pins (`useSharedSelect=false` on a non-first mux).
- `src/Libs/Adc.cpp::ReadValues` rewritten — sets shared `SetMuxChannel(iterator)` once per channel, then samples EVERY `_muxes[m].commonPin` and writes into `_channels[_muxes[m].keyMapping[iterator]]`. Iterator wraps at 16 (channels per mux), NOT at `_channels.size()`.

## Verification

Software gates (binding):
- `grep -n '_mux_count' src/Libs/Adc.hpp` → 1 match.
- `grep -n '_muxes\[' src/Libs/Adc.cpp` → multiple matches (assignment + usage).
- `grep -n 'useSharedSelect' src/Libs/Adc.cpp` → multiple matches (gate logic).
- `grep -n 'kMaxMuxes' src/Libs/Adc.hpp` → 2 matches (constant + array sizing).
- `grep -n 'keyMapping\[iterator\]' src/Libs/Adc.cpp` → 1 match.
- `grep -n 'iterator >= 16' src/Libs/Adc.cpp` → 1 match.
- `grep -rn 'mux_idx \* 16\|index \* 16' src/Libs/` → 0 matches.
- `grep -n 'pinout::t32::COM2' src/variant_t32.hpp` → 1 match.
- `grep -c 'pinout::t32::COM\b' src/variant_t32.hpp` → 1 (mux 0 only).
- All four PlatformIO envs build clean (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`).

Hardware (deferred):
- Task 12.03.5 (A: schematic GPIO confirmation, B: scope capture, C: scan-time measurement, D: 32-keys-distinct) recorded in `12-VERIFICATION.md` "Validation Deferred → Milestone v1.1 Batch".

## Key Files Changed

- `src/pinout_t32.h` — `COM2` constant added.
- `src/variant_t32.hpp` — mux 1 `commonPin` field updated.
- `src/Libs/Adc.hpp` — `_muxes[]`, `_mux_count`, `kMaxMuxes` added to private section.
- `src/Libs/Adc.cpp` — `InitMuxes` and `ReadValues` rewritten.

## Notes / Deviations

- T16 behavior is preserved: single mux + identity `keyMapping[ch] = ch` means `_channels[logical_key] == _channels[iterator]` and `iterator >= 16 == _channels.size()`. Both `t16_debug` and `t16_release` link.
- `Key::Update()` still reads `adc->GetMux(0, mux_idx)` (= `_channels[mux_idx]`). Under T16's identity keyMapping this is unchanged; under T32 it would mis-index the now-translated `_channels[]`. The T32 keys_ array sizing is Plan 12.04.2's responsibility — that plan picks a sequential `{0..31}` initializer so `Key i` reads logical key i.
- `COM2 = 17` is a working ESP32-S3 ADC2 pin but explicitly flagged as a schematic guess. Hardware verification (12.03.5.A) is deferred.

## Self-Check: PASSED
