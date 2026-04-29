# Phase 11 — Plan 02 Summary: LED Layer Migration

## What Shipped

- `LedManager.hpp` — replaced four `#define` macros (`kMatrixWidth`, `kMatrixHeight`, `sliderLength`, `NUM_LEDS`) with `inline constexpr uint8_t` aliased from `variant::CurrentVariant::kConfig.*`. Added `#include "../../variant.hpp"`.
- `LedManager.cpp` — replaced literal `16` in `matrixleds(leds_set(N, 16))` and `patternleds[16]` with `kMatrixWidth * kMatrixHeight` (both REV_A and REV_B branches).
- `Pattern.hpp` — array bound on `extern CRGB patternleds[]` updated to `variant::CurrentVariant::kConfig.MATRIX_WIDTH * MATRIX_HEIGHT` so T32 (8×4=32) compiles. Added include of `variant.hpp`.
- `Strum.hpp`, `TouchBlur.hpp` — `fill_solid(patternleds, 16, ...)` → `fill_solid(patternleds, kMatrixWidth * kMatrixHeight, ...)`.
- `NoBlur.hpp` — `blur2d(patternleds, 4, 4, 40)` → `blur2d(patternleds, kMatrixWidth, kMatrixHeight, 40)`.

## Verification

- `pio run -e t16_release` → SUCCESS (26s).
- `pio run -e t32_release` → SUCCESS (26s) after fixing a Pattern.hpp/LedManager.hpp size-mismatch (was hardcoded `[16]`, now uses variant matrix size).

## Deviation Note

Plan 02 acceptance text only listed Pattern.hpp implicitly. We extended Plan 02 to update `extern CRGB patternleds[]` array bounds in both `Pattern.hpp` and `LedManager.hpp` to `kMatrixWidth * kMatrixHeight` — required for T32 (matrix is 8×4=32, not 16). Strictly within Plan 02 scope (LED layer migration); no behavior change on T16.

## Requirements Touched

- HAL-02 (LED slice).
