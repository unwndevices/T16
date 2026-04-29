# Phase 13 — Config Schema & Migration: Verification

**Status:** passed
**Date:** 2026-04-29
**Plans complete:** 4 / 4 (13-01, 13-02, 13-03, 13-04)
**Wave structure:** Wave 1 (13-01, 13-04) → Wave 2 (13-02, 13-03)
**Execution mode:** sequential inline (no parallel subagents — runtime fallback)

## Requirements coverage

| Req ID | Statement | Coverage |
|--------|-----------|----------|
| SCHEMA-01 | Config schema declares variant discriminator | ✅ `schema/t16-config.schema.json` v201 + `variant: enum["T16","T32"]` required; firmware writes `variant` on every save (`PopulateDocFromStructs`) |
| SCHEMA-02 | Cross-variant load adapts (editor) and rejects (firmware) | ✅ `adaptConfigForVariant` exported from configValidator; `MigrateIfNeeded` rejects mismatched on-disk variant; `DeserializeFromBuffer` rejects mismatched SysEx |
| SCHEMA-03 | v200 → v201 migration default-injects variant | ✅ `migrateV200ToV201` in editor-tx (defaults to T16 fallback) and `MigrateV200ToV201` in firmware (sources from `kConfig.NAME`); chained with v103 migration |
| SCHEMA-04 | ajv validates variant field | ✅ ajv driven by updated schema rejects missing-variant and out-of-enum-variant configs (3 dedicated tests pass) |

## Locked decisions audit

| Decision | Implemented as |
|----------|---------------|
| D13.1 — Cross-variant adapt in editor only | ✅ `adaptConfigForVariant` is a pure function; firmware has zero adaptation logic |
| D13.2 — Schema v201 with forward-compat warning | ✅ `CURRENT_VERSION = 201`; firmware logs + defaults on `version > 201`; editor rejects with explicit error |
| D13.3 — Native PlatformIO `test/` env | ✅ `[env:native_test]` in platformio.ini with `-DT16` (and `-DREV_B`, see deviation note); CI `firmware-tests` job runs `pio test -e native_test` |
| D13.4 — Firmware rejects variant-mismatched configs | ✅ `MigrateIfNeeded` resets to defaults + saves on mismatch; `DeserializeFromBuffer` returns false on SysEx mismatch without overwriting disk |

## Build / test gates

| Gate | Result |
|------|--------|
| `pio run -e t16_release` | SUCCESS |
| `pio run -e t16_debug` | SUCCESS |
| `pio run -e t32_release` | SUCCESS |
| `pio run -e t32_debug` | SUCCESS |
| `pio test -e native_test -f test_migration` | 16 / 16 PASSED |
| `cd editor-tx && npm run typecheck` | exit 0 |
| `cd editor-tx && npm test -- --run` | 103 / 103 PASSED (8 suites) |
| `cd editor-tx && npm test -- --run configValidator` | 26 / 26 PASSED |
| `cd editor-tx && npm run build` | SUCCESS (1141 KiB precache) |
| `cd editor-tx && npm run lint` | 21 problems — confirmed pre-existing (unchanged delta from main) |
| `node schema/validate-types.js` | PASS |
| `node -e "..." schema invariant` | `schema OK` |
| `python3 yaml.safe_load .github/workflows/ci.yml` | YAML valid |

## Files modified

```
schema/t16-config.schema.json                          (Plan 13-01)
editor-tx/src/types/config.ts                          (Plan 13-01, regenerated)
editor-tx/src/contexts/ConfigContext.tsx               (Plan 13-01)
editor-tx/src/services/configValidator.ts              (Plan 13-02)
editor-tx/src/services/configValidator.test.ts         (Plan 13-02)
src/ConfigManager.hpp                                  (Plan 13-03)
src/ConfigManager.cpp                                  (Plan 13-03)
test/test_migration/test_main.cpp                      (Plan 13-03)
platformio.ini                                         (Plan 13-04 + 13-03 fixup)
.github/workflows/ci.yml                               (Plan 13-04)
docs/schema-v201.md                                    (Plan 13-04, new)
.planning/phases/13-config-schema-migration/13-{01..04}-SUMMARY.md (this phase)
.planning/phases/13-config-schema-migration/13-VERIFICATION.md     (this file)
```

## Commits (12)

| SHA | Subject |
|-----|---------|
| `172c07f` | feat(13-01): bump schema to v201 with variant discriminator |
| `308a4dd` | feat(13-01): regenerate editor-tx types from v201 schema |
| `d84d092` | feat(13-01): update DEFAULT_CONFIG to v201 with variant 'T16' |
| `cdb9bc1` | docs(13-01): record SUMMARY for schema v201 bump and types regen |
| `785661c` | feat(13-04): rename [env:native] to [env:native_test] with -DT16 |
| `5f2496d` | feat(13-04): update CI firmware-tests job to native_test env |
| `3ea7a86` | docs(13-04): add canonical v201 schema reference |
| `7b372d3` | docs(13-04): record SUMMARY for env rename + CI + canonical doc |
| `8e46033` | feat(13-02): add v200→v201 migration + variant adapter to editor-tx |
| `81aa869` | docs(13-02): record SUMMARY for editor-tx v201 migration + adapter |
| `f5fdcda` | feat(13-03): bump CURRENT_VERSION to 201 and declare MigrateV200ToV201 |
| `b2f296c` | feat(13-03): implement v201 migration + variant gating in ConfigManager |
| `71dd91d` | test(13-03): extend migration suite with v200→v201 + variant gating |
| `c2f127a` | docs(13-03): record SUMMARY for firmware v201 migration + native tests |

(14 commits including SUMMARY commits.)

## Cross-cutting deviations / risks

1. **`json-schema-to-typescript` is not declared in `editor-tx/package.json`** — `bash schema/generate-types.sh` requires it but it had to be `--no-save --legacy-peer-deps` installed transiently. Recommend a follow-up phase to formalize this devDependency.

2. **`-DREV_B` added to `[env:native_test]`** — the native env had been compile-broken since Phase 12 (transitive include of `pinout_t16.h`). Plan 13-03 surfaced and fixed it. Plan 13-04 only saw the env-rename scope; the fix landed in Plan 13-03 with rationale documented.

3. **`MigrateV103ToV200` now stamps `version=200` (was `CURRENT_VERSION`)** — correctness fix tied to the v201 chain. Documented in 13-03-SUMMARY.

4. **Editor-tx lint baseline** — 21 pre-existing lint problems unchanged. Out of scope for Phase 13.

5. **Hardware verification deferred** — All changes are software-only. Variant-rejection paths exercised in native tests but not against real LittleFS on device. Bundle into the milestone v1.1 hardware bring-up batch.

## Outstanding for Phase 14

(per CONTEXT.md "Out of Scope for Phase 13")
- ConfigContext / variant-conditional UI in editor-tx
- `.bin` selection in flasher
- Cross-variant load modal (the modal that consumes `adaptConfigForVariant`)

## Verification disposition

**passed** — all software gates clean, all locked decisions implemented, all requirements covered. Hardware bring-up deferred to milestone v1.1 batch (consistent with Phase 12 disposition).
