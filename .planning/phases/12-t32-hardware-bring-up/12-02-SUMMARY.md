---
plan_id: 12.02
phase: 12
status: complete
completed: 2026-04-29
hardware_verification: deferred (milestone v1.1 batch)
---

# Plan 12.02 — Summary

## What Was Built

- Replaced placeholder identity `keyMapping[]` arrays in `src/variant_t32.hpp` with the validated 32-key permutation from `origin/3dot0:src/main.cpp:10`.
- Mux 0 keyMapping = `{17, 16, 19, 18, 25, 24, 27, 26, 10, 11, 8, 9, 3, 2, 0, 1}`
- Mux 1 keyMapping = `{21, 20, 23, 22, 29, 28, 31, 30, 14, 15, 12, 13, 7, 6, 4, 5}`
- Added compile-time validation in `namespace variant::t32::detail`: stores the verbatim `k3dot0Keys[32]` array and a constexpr `InvertedKeyMapping(channel)` function. Two `static_assert` lambdas verify both `kMuxes[0].keyMapping` and `kMuxes[1].keyMapping` match the inversion.
- Updated comment block — removed "Phase 12 placeholder" and "Phase 11 placeholder" notices; replaced with source-of-truth citation.

## Verification

Software gates (binding):
- `grep -n '17, 16, 19, 18' src/variant_t32.hpp` returns 1 match (mux 0).
- `grep -n '21, 20, 23, 22' src/variant_t32.hpp` returns 1 match (mux 1).
- `grep -n 'origin/3dot0:src/main.cpp:10' src/variant_t32.hpp` returns 1 match (citation).
- `grep -c 'static_assert' src/variant_t32.hpp` returns 2.
- All four PlatformIO envs build clean (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`). The static_assert fires at compile time for any inversion mistake — it is the canonical test for T32-03.

Hardware (deferred):
- Task 12.02.3 — recorded in `12-VERIFICATION.md` "Validation Deferred → Milestone v1.1 Batch".

## Key Files Changed

- `src/variant_t32.hpp` — keyMapping arrays, source citation, `detail::k3dot0Keys`, `detail::InvertedKeyMapping`, two `static_assert` blocks.

## Notes / Deviations

**Caught typo in plan**: The plan listed `..., 2, 3, 0, 1` for mux 0 channels 12-15 and `..., 6, 7, 4, 5` for mux 1. The static_assert (correctly NOT relaxed per user direction) flagged the divergence. Re-deriving from the 3dot0 array:
- mux 0 c=12: `keys[3]=12` → `pos=3` (plan said 2)
- mux 0 c=13: `keys[2]=13` → `pos=2` (plan said 3)
- mux 1 c=28: `keys[7]=28` → `pos=7` (plan said 6)
- mux 1 c=29: `keys[6]=29` → `pos=6` (plan said 7)

The two pairs were transposed in the plan's hand-derivation table. The committed values match the static_assert-verified inversion.

This is exactly the failure mode the static_assert was designed to catch — one machine-checked source of truth (`k3dot0Keys[]`) caught a manual transcription error in the planner's expected values.

## Self-Check: PASSED
