---
phase: 10-build-system-variant-selection
plan: 04
subsystem: ci
tags: [github-actions, ci, matrix, build, smoke-test]

requires:
  - phase: 10-01
    provides: four firmware envs (t16_debug, t16_release, t32_debug, t32_release)
  - phase: 10-03
    provides: code-complete CurrentPinout migration so all envs at least compile
provides:
  - "GitHub Actions firmware-build job runs as a 4-leg matrix over all variant×debug envs"
  - "fail-fast: false ensures T32 regressions don't cancel T16 jobs"
  - "Per-env cache key (pio-<hash>-<env>) so each matrix leg keeps an independent .pio/build cache"
  - "Sibling firmware-tests job for `pio test -e native` (no matrix fan-out for variant-agnostic Unity tests)"
affects: [phase-11, phase-12, phase-13, phase-14]

tech-stack:
  added: []
  patterns:
    - "GitHub Actions matrix strategy keyed off PlatformIO env names"
    - "Per-matrix-leg cache key to prevent cross-leg cache pollution"

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml

key-decisions:
  - "Matrix over all 4 envs with fail-fast: false (D10.4) — variant regressions are independent failure modes that should each be visible"
  - "Native tests split into sibling firmware-tests job rather than per-leg — Unity suite is variant-agnostic, so 4-way fan-out would only waste CI minutes"
  - "Per-leg cache key includes ${{ matrix.env }} — without this, the second leg in any run would clobber the first leg's .pio/build with its own object files (PlatformIO uses build flags as part of its build hash)"
  - "Hardware smoke test (Task 2) routed to VERIFICATION.md as human_needed — the final firmware.elf cannot link due to a pre-existing third-party library conflict (see Plan 03 Blocker)"

patterns-established:
  - "Variant-aware CI matrix that scales to additional variants by adding entries to the env list"

requirements-completed: [BUILD-02, BUILD-03]

duration: 7min
completed: 2026-04-29
---

# Phase 10 Plan 04: CI matrix over four envs + hardware smoke checkpoint Summary

**Replaced single-env GitHub Actions `firmware-build` job with a 4-leg matrix (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`) keyed off Plan 01's platformio.ini envs; split `pio test -e native` into an independent `firmware-tests` sibling job. The manual T16 hardware smoke test (Task 2) is recorded as `human_needed` in VERIFICATION.md because the firmware.elf cannot currently link due to a pre-existing third-party library conflict that this phase did not introduce (Plan 03 Blocker).**

## Performance

- **Duration:** ~7 min (Task 1 only; Task 2 is a deferred human checkpoint)
- **Completed:** 2026-04-29
- **Tasks:** 1 executed (Task 2 deferred to human verification)
- **Files modified:** 1

## Accomplishments
- Authored matrix-strategy `firmware-build` job covering 4 envs with `fail-fast: false` and per-leg cache keys.
- Added new `firmware-tests` job for the variant-agnostic Unity suite.
- Preserved `firmware-format` and `web-build` jobs verbatim.
- Confirmed YAML parses cleanly via `python3 -c "import yaml; yaml.safe_load(...)"`.

## Task Commits

1. **Task 1: Matrix CI + native test split** — `e31641a` (ci)
2. **Task 2: T16 hardware smoke test** — DEFERRED (`checkpoint:human-verify` blocked by upstream linker issue; see "Issues Encountered" and `VERIFICATION.md`)

**Plan metadata:** committed alongside SUMMARY.

## Files Created/Modified
- `.github/workflows/ci.yml` — firmware-build now matrix over 4 envs; new `firmware-tests` sibling job; `firmware-format` + `web-build` preserved.

## Acceptance Criteria

| Check | Expected | Actual |
|-------|----------|--------|
| YAML parses (yaml.safe_load) | yes | yes |
| `matrix:` keyword present | ≥1 | 1 |
| `env: [t16_debug, t16_release, t32_debug, t32_release]` | 1 | 1 |
| `fail-fast: false` | 1 | 1 |
| `pio run -e ${{ matrix.env }}` | 1 | 1 |
| `firmware-tests:` job present | 1 | 1 |
| `pio test -e native` (single occurrence) | 1 | 1 |
| Old `pio run -e esp32s3` removed | 0 | 0 |
| `firmware-format:` job preserved | 1 | 1 |
| `web-build:` job preserved | 1 | 1 |

All 10 Task 1 acceptance criteria pass.

## Deviations from Plan

### [Rule 4 - Architectural change → recorded as human_needed instead of stopping]
- **Found during:** Plan 03 Task 3 build verification (carried into Plan 04 since Task 2's checkpoint depends on a working binary).
- **Issue:** Task 2 is a `checkpoint:human-verify` requiring the developer to flash a `t16_release` binary to physical T16 hardware and confirm identical behavior to v1.0 firmware. However, Plan 03 documented that the firmware.elf currently fails to link because of pre-existing library multi-definition errors (TinyUSB host-controller conflict for T16; BLE-MIDI ODR for T32). These are environment/dependency issues unrelated to Phase 10's variant-selection scope.
- **Action taken:** Per the orchestrator's invocation instructions ("if the checkpoint requires interactive validation, treat it as `human_needed` in VERIFICATION.md and let the main thread route the prompt") and per the GSD execute-phase workflow's standard human-needed routing: the manual smoke test is recorded in VERIFICATION.md so the user can decide next steps (run smoke test against last-known-good binary, schedule a hotfix phase to resolve the library conflict, or defer until Phase 11).
- **Rationale:** The plan's CI YAML changes (Task 1) are independent of binary linking and were committed normally. Task 2 cannot be re-attempted by the orchestrator because (a) it requires physical hardware in the user's possession, (b) the pre-link failures are not Phase 10 work to fix.

**Total deviations:** 1 — Task 2 deferred to human verification (Rule 4 architectural / blocked-by-pre-existing-issue, routed to VERIFICATION.md).
**Impact:** Plan 04's code change (Task 1) is complete and verified. The hardware smoke test (Task 2) is the only outstanding success criterion (#4 from the ROADMAP) and is now in the user's hands.

## Issues Encountered

- The pre-existing third-party library linker conflict (Plan 03 Blocker) blocks the hardware smoke test. Documented in detail in `10-03-SUMMARY.md` and surfaced in `VERIFICATION.md`.
- Likely follow-up: a Phase 10.1 hotfix or pre-Phase-11 dependency-resolution task to pin compatible `framework-arduinoespressif32` + `Adafruit TinyUSB Library` + `BLE-MIDI` versions.

## Next Phase Readiness

- Phase 10 code is in. CI matrix is wired. The next push to GitHub will fan firmware-build into 4 parallel matrix legs (which will all currently FAIL at link until the third-party library conflict is resolved — but the YAML is correct and ready).
- Phase 11 (Hardware Abstraction Layer) can begin once the linker conflict is triaged. The HAL refactor depends only on the CurrentPinout namespace being available, which Plan 03 delivered.
- ROADMAP success criteria status:
  1. ✓ `pio run -e t16_release` and `pio run -e t32_release` envs exist (Plans 01 + 02 + 03 — code-complete; binary blocked by upstream lib conflict).
  2. ✓ Bare `pio run` builds T16 release (Plan 01 default_envs).
  3. ✓ `CurrentPinout` resolves correctly per variant (Plan 02 + Plan 03 verified by per-TU compile success).
  4. ⚠ T16 hardware behavior identical — DEFERRED, recorded as human_needed in VERIFICATION.md.

## Self-Check: PARTIAL
- ✓ All 10 Task 1 acceptance criteria pass.
- ✓ YAML is syntactically valid.
- ✓ One atomic commit produced (`e31641a`).
- ⚠ Task 2 (human verify) deferred to VERIFICATION.md per orchestrator instructions.
