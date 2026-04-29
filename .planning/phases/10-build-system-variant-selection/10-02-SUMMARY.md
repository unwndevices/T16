---
phase: 10-build-system-variant-selection
plan: 02
subsystem: pinout
tags: [pinout, namespace, c++17, variant, refactor]

requires:
  - phase: 10-01
    provides: -DT16 / -DT32 build flags wired through platformio envs
provides:
  - "namespace pinout::t16 with REV_A and REV_B constexpr constants"
  - "namespace pinout::t32 placeholder constants (Phase 12 will replace)"
  - "pinout.h variant selector emitting `using CurrentPinout = pinout::tXX;`"
  - "#error guard for missing variant flag (no silent fallback)"
affects: [phase-10-03, phase-10-04, phase-11, phase-12]

tech-stack:
  added: []
  patterns:
    - "C++17 nested namespace syntax (namespace pinout::t16 {}) — eisei target style"
    - "#pragma once on all new headers (replacing #ifndef/#define guards)"
    - "constexpr uint8_t pin constants instead of bare #define macros (D10.3)"
    - "Single global `using CurrentPinout = pinout::tXX;` alias at file scope"

key-files:
  created:
    - src/pinout_t16.h
    - src/pinout_t32.h
  modified:
    - src/pinout.h

key-decisions:
  - "namespace pinout::t16 / pinout::t32 — paired sibling namespaces under a shared parent (D10.3)"
  - "REV_A / REV_B branching stays inside pinout_t16.h — the revisions are T16-specific (T32 has no REV branches per CONTEXT.md scope)"
  - "T32 placeholder mirrors T16 REV_B values verbatim — only goal is compile success; banner comment forbids hardware flashing"
  - "T32 check comes BEFORE T16 in pinout.h selector — T16 is the default-envs fallback so it sits in #elif"
  - "`using CurrentPinout = ...;` lives at file scope (not in an anonymous namespace) per RESEARCH.md Pitfall 2 (ODR safety)"

patterns-established:
  - "Variant selector header: thin shim that #includes the active variant header and aliases its namespace"
  - "Per-variant pinout headers expose unprefixed names (S0, LED_PIN, TOUCH) — the namespace provides the disambiguation"

requirements-completed: [BUILD-01]

duration: 8min
completed: 2026-04-29
---

# Phase 10 Plan 02: Namespaced pinout headers + variant selector Summary

**Replaced 65 lines of bare `#define PIN_*` macros with three small C++17 headers: `pinout_t16.h` (REV_A/REV_B constexpr constants under `namespace pinout::t16`), `pinout_t32.h` (Phase-12 placeholder mirroring T16 REV_B), and a 17-line `pinout.h` selector that aliases the active variant as `using CurrentPinout = pinout::tXX;`.**

## Performance

- **Duration:** ~8 min
- **Completed:** 2026-04-29
- **Tasks:** 3
- **Files modified/created:** 3

## Accomplishments
- Authored `src/pinout_t16.h` with all 21 pin constants × 2 REV branches preserved verbatim from the prior `pinout.h`.
- Authored `src/pinout_t32.h` with a clear `PHASE 10 PLACEHOLDER` banner so reviewers cannot mistake the values for real T32 hardware.
- Rewrote `src/pinout.h` from 65 lines of `#define`s into a 17-line variant selector emitting a single `CurrentPinout` alias.
- Removed every `#define PIN_*` macro from the pinout headers; bare PIN_* remain only in call-site files (Plan 03 migrates those).

## Task Commits

1. **Task 1: Create src/pinout_t16.h** — `a216a40` (feat)
2. **Task 2: Create src/pinout_t32.h placeholder** — `df73a08` (feat)
3. **Task 3: Rewrite src/pinout.h as variant selector** — `901c1a3` (feat)

**Plan metadata:** committed alongside SUMMARY.

## Files Created/Modified
- `src/pinout_t16.h` (NEW, 75 lines) — namespace pinout::t16 with REV_A/REV_B branches
- `src/pinout_t32.h` (NEW, 45 lines) — namespace pinout::t32 placeholder (Phase 12 replaces)
- `src/pinout.h` (REWRITE, 65 → 17 lines) — variant selector with `using CurrentPinout = pinout::tXX;`

## Acceptance Criteria

All three tasks' acceptance grep checks pass:
- `pinout_t16.h`: namespace + cstdint + pragma once present; both REV branches present; LED_PIN=1, S0 in {4,5}, TOUCH in {2,21}; no leftover `#define PIN_`.
- `pinout_t32.h`: namespace + banner + S0 + LED_PIN + TOUCH constants present; no `#define PIN_`.
- `pinout.h`: zero `#define PIN_`, both `using CurrentPinout = pinout::tXX;` lines, `#error` fallback, `#pragma once`, file under 30 lines (17 actual).

## Pin-Value Verification

Every constant in `pinout_t16.h` was cross-referenced with the original `pinout.h` macro values before writing. No remap; verbatim copy.

| Constant | REV_A | REV_B | Source line in old pinout.h |
|----------|-------|-------|----------------------------|
| S0..S3   | 5,4,7,6 | 4,5,6,7 | 6-9 / 36-39 |
| COM      | 8     | 8     | 11 / 41 |
| LED_PIN  | 1     | 1     | 13 / 43 |
| T1..T7   | 3,9,10,11,12,13,14 | identical | 15-21 / 45-51 |
| TX/RX/TX2 | 43/44/42 | identical | 23-25 / 53-55 |
| TOUCH    | 2     | 21    | 27 / 57 |
| EXT3/EXT4 | 41/40 | identical | 28-29 / 58-59 |
| MODE     | 0     | 0     | 30 / 60 |

The only REV-dependent values are S0..S3 (swap order) and TOUCH (2 vs 21) — exactly matching the audit in 10-RESEARCH.md.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None. As expected, existing call sites in `AppEngine.{hpp,cpp}`, `LedManager.cpp`, and `ButtonHandler.cpp` still reference the deleted `PIN_*` macros; they will not compile until Plan 03 lands. This is the documented interlock between Plans 02 and 03.

## Next Phase Readiness

Plan 03 (call-site migration) is unblocked. Plan 03 will run a full T16 + T32 build verification at the end of its Task 3, which will be the first end-to-end compile check after all three plans 01/02/03 land.

## Self-Check: PASSED
- All acceptance-criteria grep counts verified by automated check.
- Three atomic commits produced (`a216a40`, `df73a08`, `901c1a3`).
- No unrelated files staged or modified.
