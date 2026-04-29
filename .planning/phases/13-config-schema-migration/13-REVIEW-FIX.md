---
phase: 13-config-schema-migration
fixed_at: 2026-04-29T00:00:00Z
fix_scope: critical_warning
findings_in_scope: 8
fixed: 7
skipped: 1
deferred: 1
iteration: 1
status: partial
---

# Phase 13: Code Review Fix Report

**Fixed at:** 2026-04-29
**Fix scope:** critical + warning (8 findings)
**Result:** 7 fixed, 1 deferred (out-of-scope follow-up)

## Summary

Applied fixes for all critical and warning findings in `13-REVIEW.md` except
WR-03 (param-ACK FIFO misattribution), which was deferred per the review
finding's own recommendation to handle it as a SysEx-protocol-level follow-up.
Three commits landed on `main`. Info findings (IN-01..IN-04) are out of the
default scope and were not addressed; re-run with `--all` to include them.

| ID    | Severity | Status   | Commit       |
|-------|----------|----------|--------------|
| CR-01 | Critical | FIXED    | `6b9ae2c`    |
| WR-01 | Warning  | FIXED    | `6b9ae2c`    |
| WR-02 | Warning  | FIXED    | `6b9ae2c`    |
| WR-03 | Warning  | DEFERRED | (none)       |
| WR-04 | Warning  | FIXED    | `70b1235`    |
| WR-05 | Warning  | FIXED    | `70b1235`    |
| WR-06 | Warning  | FIXED    | `70b1235`    |
| WR-07 | Warning  | FIXED    | `7b641b1`    |

## Fixes Applied

### CR-01 — Version-aware SysEx import (`src/ConfigManager.cpp`)

**Commit:** `6b9ae2c fix(13): version-aware SysEx import + missing-variant repair + uint16 version parse`

`DeserializeFromBuffer` now dispatches on `version` like `MigrateIfNeeded`:
- v1xx payloads chain through `MigrateV103ToV200` + `MigrateV200ToV201`.
- v200 payloads chain through `MigrateV200ToV201`.
- v201 payloads pass through.
- Forward versions (`> CURRENT_VERSION`) are rejected explicitly (D13.2 parity).
- Missing-version (`== 0`) is preserved as a tolerated case for older editors
  that omit the key on v200/v201-shaped payloads — the existing variant gate
  already accepts a missing variant for that case.

This restores the invariant "all version transitions go through `MigrateV*`
helpers" on the SysEx import hot path.

### WR-01 — Missing-variant repair on flash load (`src/ConfigManager.cpp`)

**Commit:** `6b9ae2c`

`MigrateIfNeeded` now distinguishes missing-variant from wrong-variant on v201
files. A missing variant key triggers an in-place repair (variant injected,
file rewritten, structs populated) instead of wiping all user config to
defaults. The wipe path is reserved for true T16/T32 mismatches.

### WR-02 — Widen `version` parse to `uint16_t` (`src/ConfigManager.cpp`)

**Commit:** `6b9ae2c`

`version` is parsed as `uint16_t` at both `DeserializeFromBuffer` and
`PopulateStructsFromDoc`. Payloads `>= 256` no longer silently truncate to
their low byte (which would have collided with v0/v1 in the dispatch table).
`global_.version` remains `uint8_t` with a defensive clamped narrowing — the
struct field can be widened later when the schema actually crosses the byte
boundary.

### WR-04 — Reject non-integer versions in `prepareImport`

**Commit:** `70b1235 fix(13): editor migration polish — float-version reject + explicit predicates + bank validation`

`prepareImport` now explicitly rejects non-integer versions (e.g., 200.5)
with a typed `version`-keyed error before dispatch, instead of falling
through every branch into the misleading "Unhandled version" guard.

### WR-05 — Make `migrateV103` predicate intent explicit

**Commit:** `70b1235`

The `hasGlobalKeys` short-circuit `key in data || key === 'custom_scale1' || key === 'custom_scale2'`
was unconditionally true for the two scale keys (which worked but masked the
real contract). Replaced with a `REQUIRED_GLOBAL_KEYS_V103` constant
(`GLOBAL_KEYS` minus the two scale keys) and a clean `every(key in data)`
predicate. The "scales are optional and default-filled below" rule is now
self-evident at the predicate site.

### WR-06 — Validate bank fields before type cast in `migrateV103`

**Commit:** `70b1235`

`migrateV103` now validates each bank carries the required scalar fields
(`ch`, `scale`, `oct`, `note`, `vel`, `at`, `flip_x`, `flip_y`, `koala_mode`,
`chs`, `ids`) before the `as T16Configuration['banks']` cast. Previously a
v103 file with banks missing required fields would slip past the type
assertion and only fail at downstream ajv validation; now migration returns
`null` up front with a `version`-keyed error.

### WR-07 — Remove broken `firmware-format` CI job

**Commit:** `7b641b1 ci(13): remove firmware-format job — would fail without a .clang-format`

The `firmware-format` job ran `clang-format --dry-run --Werror` against every
`src/*.{hpp,cpp,h}` file, but the repo has no `.clang-format` config. With no
config, clang-format applied LLVM defaults that conflict with the codebase's
actual style (Allman braces, 4-space indent, mixed naming — per CLAUDE.md
"no firmware linting/formatting tools"). Every file would have failed on first
run.

The job was removed and replaced with a NOTE comment in the workflow
explaining the rationale. Re-enable only after a `.clang-format` matching
project conventions is committed.

## Tests Added

### Firmware (`test/test_migration/test_main.cpp`)

6 new cases (16 → 22, all passing) covering CR-01 + WR-01 + WR-02:

- `test_sysex_import_v103_migrates_to_v201` — v103 → v201 via SysEx import path
- `test_sysex_import_v200_migrates_to_v201` — v200 → v201 via SysEx import path
- `test_sysex_import_rejects_forward_version` — v202 SysEx payload rejected
- `test_sysex_import_rejects_overflow_version` — version 300 (>255) rejected (no truncation)
- `test_sysex_import_v201_with_missing_variant_accepted` — missing-variant tolerance preserved
- `test_v201_missing_variant_is_repaired_not_wiped` — flash-load repair preserves user config

### Editor (`editor-tx/src/services/configValidator.test.ts`)

2 new cases (26 → 28, all passing) covering WR-04 + WR-06:

- `rejects a non-integer version (WR-04)` — version 200.5 returns typed error
- `rejects a v103 file with banks missing required scalar fields (WR-06)` —
  malformed banks fail at migration, not downstream ajv

Full editor suite: 164 tests pass. Firmware test_migration: 22 tests pass.

## Verification

- `pio test -e native_test -f test_migration` — 22/22 passed
- `cd editor-tx && npm run test -- --run` — 164/164 passed
- `pio run -e t16_debug` — SUCCESS (firmware builds clean, 13.4% RAM, 68.3% flash)
- Editor `npm run lint` — baseline 22 errors unchanged (no new lint introduced)

Pre-existing failures in `test_config_manager` (`test_init_default_version`
expects 200, `test_serialize_contains_version_200` expects "200" in buffer)
are stale Phase 01 fixtures that were never updated when `CURRENT_VERSION`
moved to 201. Out of scope for this fix pass — flag for a separate cleanup.

## Deferred

### WR-03 — Param-ACK FIFO misattribution (`editor-tx/src/contexts/ConfigContext.tsx:213-225`)

**Status:** DEFERRED — out-of-scope follow-up.

**Reason:** The review finding itself flagged this as "follow-up — sysex
protocol-level fix — ACK should echo enough of the original domain/bank/field
tuple to disambiguate". The proper fix requires firmware ACK protocol
changes (echo the param identity in the response payload), which is a Phase
13.x or Phase 15 protocol bump, not a code-review-fix surface.

The alternative short-term option in the finding ("drop the round-trip
telemetry entirely") only addresses the misleading log line, not the
underlying retry-path bug — `pendingParamTimestamps.has(key)` inside the
500ms retry timer can return false because a FIFO-misattributed ACK has
already deleted a different param's key, so retries silently won't fire for
the actually-pending param. Fixing the retry path correctly requires the
ACK protocol change, so a partial telemetry-only patch would be misleading.

**Recommended next step:** Open a Phase 13.x or Phase 15 plan for "Param-ACK
identity echo" — firmware change to include `(domain, bank, field)` (or a
sequence number) in the ACK payload, plus editor ack-matching by that
identity. Phase 14 has not touched this code path since the review.

## Skipped (Info-Level)

`IN-01`..`IN-04` were not addressed — Info findings are out of the default
fix scope (Critical + Warning only). To address them, re-run:

```
gsd-code-review-fix 13 --all
```

These are quality-of-life refactors:
- IN-01: optional `MigrateV103ToCurrent` wrapper for the chain
- IN-02: comment on `PopulateDocFromStructs` ignoring `global_.version` on write
- IN-03: bank-coverage assertion in `test_v201_matching_variant_passes_through`
- IN-04: optional `overwroteVariant: boolean` flag from `migrateV200ToV201`

---

_Fixed: 2026-04-29_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
