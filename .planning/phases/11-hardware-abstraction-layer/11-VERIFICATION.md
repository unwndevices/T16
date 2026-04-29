---
phase: 11
status: validation_deferred
last_updated: 2026-04-29
---

# Phase 11 Verification — Hardware Abstraction Layer

## Status: validation_deferred

All five plans (11-01 through 11-05) are code-complete. Automated build + grep guards all pass. Hardware regression has been deferred to end-of-milestone v1.1 batch verification per the orchestrator dispatch instruction.

## Automated Gates — PASSED

- `pio run -e t16_debug` → exit 0
- `pio run -e t16_release` → exit 0
- `pio run -e t32_debug` → exit 0
- `pio run -e t32_release` → exit 0
- Grep guard: no surviving `#define` of `BANK_AMT|NUM_LEDS|kMatrixWidth|kMatrixHeight|sliderLength` outside `variant_*` / `pinout_*` headers (`11-05-grep-guard.txt`).
- Grep guard: no free `const uint8_t BANK_AMT` definitions (`11-05-grep-guard.txt`).
- Four-env build matrix recorded in `11-05-build-status.txt` (all `status=0`).

## Deferred Gates — Human Verification (end of v1.1)

- ROADMAP success criterion #4 — T16 hardware passes calibration and produces identical MIDI output to the pre-Phase-11 baseline. Procedure recorded in `11-05-PLAN.md` Task 5.4. Outcome to be written to `11-05-hardware-checkpoint.md` with `STATUS: PASS` or `STATUS: FAIL`.

This gate must clear before Phase 12 (T32 Hardware Bring-Up) starts touching the dual-mux scan path.

## Plans

| Plan | Title                                                                  | Status   |
|------|------------------------------------------------------------------------|----------|
| 11-01 | Variant header scaffolding                                            | complete |
| 11-02 | LED layer migration                                                   | complete |
| 11-03 | Adc multiplexer migration (`InitMuxes`)                               | complete |
| 11-04 | Configuration / BANK_AMT / Keyboard / DataManager migration           | complete |
| 11-05 | Capabilities SysEx + final audit + hardware checkpoint (deferred)     | complete (hardware deferred) |

## Requirements

- HAL-01 ✓ (`HardwareVariantConfig` per variant)
- HAL-02 ✓ (LedManager / Adc / Configuration / Keyboard / DataManager consume variant config or are explicitly variant-agnostic)
- HAL-03 ✓ (`MultiplexerConfig` consumed by `Adc::InitMuxes`)
- HAL-04 ✓ (`CMD_CAPABILITIES` SysEx command emits variant + capability JSON)
