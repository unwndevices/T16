# Phase 11 — Plan 01 Summary: Variant Header Scaffolding

## What Shipped

Created the variant configuration scaffolding:

- `src/variant_config.hpp` — `HardwareVariantConfig` and `MultiplexerConfig` structs.
- `src/variant_t16.hpp` — `variant::t16::kConfig` and `kMuxes[1]`.
- `src/variant_t32.hpp` — `variant::t32::kConfig` and `kMuxes[2]` (placeholder posture).
- `src/variant.hpp` — selector that aliases `variant::CurrentVariant` to the active variant via `-DT16` / `-DT32`.

No consumers migrated yet — pure addition. Mirrors the existing `pinout.h` selector pattern.

## Verification

- Both PlatformIO envs build cleanly:
  - `pio run -e t16_release` → SUCCESS, 72.06s, RAM 13.4%, Flash 64.2%.
  - `pio run -e t32_release` → SUCCESS, 72.59s.
- All four header files exist and use `#pragma once`.
- Selector matches `pinout.h` shape (`#if defined(T32) / #elif defined(T16) / #else #error`).

## Requirements Touched

- HAL-01 (HardwareVariantConfig struct exposed) — scaffolded.
- HAL-03 (MultiplexerConfig struct) — scaffolded.

## Notes

- T32 headers are placeholders matching pinout_t32.h posture — Phase 12 will validate dual-mux pin assignments before flashing T32 hardware.
- `BANK_AMT = 4` modeled as an invariant field on both variants per RESEARCH §4 / pre-approved CONTEXT deviation.
