# Phase 11 — Plan 04 Summary: Configuration / BANK_AMT Migration

## What Shipped

- `src/Configuration.hpp` — replaced `const uint8_t BANK_AMT = 4;` with `inline constexpr uint8_t BANK_AMT = variant::CurrentVariant::kConfig.BANK_AMT;`. Added `#include "variant.hpp"`. All consumers (`Configuration.cpp`, `ConfigManager.hpp/cpp`, `SysExHandler.cpp`, `DiagnosticCommands.cpp`) compile unchanged — same identifier, now sourced from variant config.
- `src/Libs/Keyboard.hpp` — added contract comment documenting that `key_amount` flows from caller (which reads `variant::CurrentVariant::kConfig.TOTAL_KEYS`). No literal `16` related to TOTAL_KEYS exists in Keyboard; the `strip_position[16]` arrays are strip-pattern fixed-width buffers, not variant-dependent.
- `src/Libs/DataManager.hpp` — added contract comment confirming variant-agnostic design. No literal `16`/`BANK_AMT` references exist; sizes always flow from caller's data types.

## Verification

- `pio run -e t16_release` → SUCCESS (28s).
- `pio run -e t32_release` → SUCCESS (28s).
- Audit summary: `Keyboard` is variant-agnostic at the API level (caller-supplied `key_amount`); `DataManager` is variant-agnostic by design (templated JSON serialization).

## Requirements Touched

- HAL-02 (Configuration / Keyboard / DataManager slice).

## Notes

- Per RESEARCH §4 (pre-approved CONTEXT deviation), `BANK_AMT` is modeled as an invariant `kConfig.BANK_AMT = 4` field on both variants. Migration is consistent with HAL-02 even though the value never differs.
