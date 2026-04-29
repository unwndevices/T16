# Phase 11 — Plan 05 Summary: Capabilities SysEx + Final Audit + Hardware Checkpoint

## What Shipped

- `src/SysExProtocol.hpp` — added `constexpr uint8_t CMD_CAPABILITIES = 0x07;` (next free byte; `0x06` was already taken by `CMD_FACTORY_RESET`).
- `src/SysExHandler.hpp` — declared `void HandleCapabilitiesRequest();`.
- `src/SysExHandler.cpp` — added `#include "variant.hpp"`, dispatch case for `CMD_CAPABILITIES`, and handler that emits JSON `{"variant":"...","capabilities":{"touchSlider":...,"koalaMode":...}}`.

## Verification

### Task 5.2 — Grep guard

Saved to `.planning/phases/11-hardware-abstraction-layer/11-05-grep-guard.txt`:
- No legacy `#define` of `BANK_AMT|NUM_LEDS|kMatrixWidth|kMatrixHeight|sliderLength` outside `variant_*` / `pinout_*`. Both checks return `NONE`.

### Task 5.3 — Four-env build matrix

Saved to `.planning/phases/11-hardware-abstraction-layer/11-05-build-status.txt`:

| env          | status | firmware                          |
|--------------|--------|-----------------------------------|
| t16_debug    | 0      | `.pio/build/t16_debug/firmware.bin`   |
| t16_release  | 0      | `.pio/build/t16_release/firmware.bin` |
| t32_debug    | 0      | `.pio/build/t32_debug/firmware.bin`   |
| t32_release  | 0      | `.pio/build/t32_release/firmware.bin` |

All four PlatformIO envs link cleanly.

### Task 5.4 — T16 hardware regression checkpoint

`checkpoint:human-verify` — deferred to end-of-milestone v1.1 batch hardware verification per orchestrator dispatch. Status: `validation_deferred`. The checkpoint stays staged so it must be cleared (PASS recorded in `11-05-hardware-checkpoint.md`) before Phase 12 starts dual-mux work.

## Pre-approved Deviations (from RESEARCH §4)

- HAL-04 implemented as a new `CMD_CAPABILITIES` SysEx command (not a modification to the existing version handshake) — preserves the wire format of the existing 5-byte version response.
- `BANK_AMT` modeled as a `kConfig.BANK_AMT = 4` invariant field on both variants.

## Requirements Touched

- HAL-04 (firmware-side capabilities response).
- ROADMAP success criteria #2 (no surviving variant macros), #5 (T32 builds clean) — both green.
- ROADMAP success criterion #4 (T16 hardware regression) — staged for human verification.
