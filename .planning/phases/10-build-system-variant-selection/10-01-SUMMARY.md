---
phase: 10-build-system-variant-selection
plan: 01
subsystem: build
tags: [platformio, ci, build-flags, variant]

requires:
  - phase: v1.1 milestone setup
    provides: roadmap + requirements for variant support
provides:
  - Four explicit build envs (t16_debug, t16_release, t32_debug, t32_release)
  - default_envs = t16_release so bare `pio run` keeps building T16 release
  - -DT16 / -DT32 build-flag-driven variant selection (paired with Plan 02 pinout selector)
affects: [phase-10-02, phase-10-03, phase-10-04, phase-11, phase-12]

tech-stack:
  added: []
  patterns:
    - "Variant selection via -DT16 / -DT32 build flags rather than -DVARIANT=... token (D10.1)"
    - "Per-variant + per-debug-level env matrix in platformio.ini"

key-files:
  created: []
  modified:
    - platformio.ini

key-decisions:
  - "Bare -DT16 / -DT32 macros (D10.1) — easier to consume from C++ #if defined(T16) than tokenized -DVARIANT=t16"
  - "default_envs = t16_release — preserves T16 muscle memory; bare `pio run` still builds the production T16 binary"
  - "[env_common] preserved verbatim — shared lib_deps, framework, USB-MIDI flag, monitor settings are variant-agnostic"
  - "[env:native] left untouched — Unity unit-test env is independent of firmware variant"
  - "-DREV_B kept on every firmware env — hardware revision is orthogonal to variant per D10.3"

patterns-established:
  - "Build env matrix as authoritative variant×debug-level grid"

requirements-completed: [BUILD-02, BUILD-03]

duration: 5min
completed: 2026-04-29
---

# Phase 10 Plan 01: Build env split (T16/T32 × debug/release) Summary

**Replaced single-variant `[env:esp32s3]`/`[env:release]` with a four-env matrix (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`) keyed off `-DT16`/`-DT32`; `default_envs = t16_release` keeps bare `pio run` on the existing T16 release path.**

## Performance

- **Duration:** ~5 min
- **Completed:** 2026-04-29
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Authored four firmware envs with explicit `-DT16`/`-DT32` flags (no `-DVARIANT=...` token).
- Set `default_envs = t16_release` so existing T16 release workflows keep working with no extra arguments.
- Preserved `[env_common]` (shared lib_deps + monitor settings) and `[env:native]` (Unity test env) unchanged.

## Task Commits

1. **Task 1: Update platformio.ini with four variant envs and new default** — `7e54a5a` (feat)

**Plan metadata:** to be committed alongside SUMMARY by execute-plan flow.

## Files Created/Modified
- `platformio.ini` — replaced two firmware envs with four; updated `default_envs`.

## Acceptance Criteria

| Check | Expected | Actual |
|-------|----------|--------|
| `default_envs = t16_release` | 1 | 1 |
| Four new env headers present | 4 | 4 |
| Old `[env:esp32s3]` / `[env:release]` removed | 0 | 0 |
| `[env:native]` preserved | 1 | 1 |
| `[env_common]` preserved | 1 | 1 |
| `-DT16` occurrences | 2 | 2 |
| `-DT32` occurrences | 2 | 2 |
| `-DREV_B` occurrences | 4 | 4 |

All 12 acceptance criteria pass.

## Deviations from Plan

None — plan executed exactly as written. The action's TAB indentation in `build_flags` was preserved (matched existing file style).

## Issues Encountered

None. Build verification deferred to Plan 03 / Plan 04 by design (Plan 02 must land for `-DT16`/`-DT32` envs to compile).

## Next Phase Readiness

Plan 02 (pinout headers) is unblocked and is the same-wave companion. After Plan 02 lands, `pio run -e t16_release` and `pio run -e t32_release` should compile (Plan 03 adds call-site migration; Plan 04 wires the CI matrix and runs the manual smoke test).

## Self-Check: PASSED
- All acceptance-criteria grep counts verified.
- Single atomic commit produced (`7e54a5a`).
- No unrelated files staged or modified.
