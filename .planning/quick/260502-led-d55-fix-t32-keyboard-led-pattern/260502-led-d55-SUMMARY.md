---
quick_id: 260502-led-d55
slug: fix-t32-keyboard-led-pattern
date: 2026-05-02
status: complete
---

# Summary — Fix T32 keyboard-mode LED pattern

## What changed

**`src/services/InputProcessor.cpp:54`** — replaced hard-coded `idx % 4` / `idx / 4` with `idx % kMatrixWidth` / `idx / kMatrixWidth`. On T32 (8-wide), key indices 16..31 no longer produce out-of-range `y` (4..7), which previously made `XY()` return `0xFFFF` and triggered OOB writes into `patternleds[0xFFFF]`.

**`src/Libs/Leds/patterns/NoBlur.hpp`** — generalized the 4×4-only pattern:
- `lumaleds[16]` → `lumaleds[kMatrixSize]`
- `luma[10][16]` → `luma[10][kMatrixSize]`
- `blur2d(..., 4, 4, ...)` → `blur2d(..., kMatrixWidth, kMatrixHeight, ...)`
- Render loop: `x:0..3, y:0..3` → `x:0..kMatrixWidth-1, y:0..kMatrixHeight-1`
- LUT index stride: `*4` → `*kMatrixWidth`
- Casts `(uint8_t)pos_x/pos_y` → `(int)pos_x/pos_y` to keep the abs(distance) computation in signed space (defensive — no behavioural change on T16).

## Build verification

- `pio run -e t16_debug` — SUCCESS (RAM 15.6%, Flash 60.6%)
- `pio run -e t32_debug` — SUCCESS (RAM 15.9%, Flash 60.6%)

## What's NOT fixed yet

- **WaveTransition slowness on T32** — held back deliberately. Fixing the OOB write may resolve it through eliminating memory corruption (hypothesis H2 in the analysis). User will test on device first.

## Next steps for the user

1. Flash `t32_debug` build to device.
2. Verify keyboard-mode key presses now produce LED halos across the full 8×4 grid.
3. Re-check mode-transition speed. If still slow, follow up with loop-period instrumentation (hypothesis H1) and check for repeated `UpdateTransition` calls (H3).
