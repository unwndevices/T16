# Plan 13-03 — Firmware Migration + Variant Gating + Native Tests Summary

**Status:** complete
**Commits:** 3 (`f5fdcda`, `b2f296c`, `71dd91d`)
**Wave:** 2

## What changed

| File | Change |
|------|--------|
| `src/ConfigManager.hpp` | `CURRENT_VERSION` 200 → 201; declared `MigrateV200ToV201(JsonDocument&)` in private |
| `src/ConfigManager.cpp` | Added `MigrateV200ToV201` impl; rewrote `MigrateIfNeeded` dispatch (v201 + variant check, v200 → v201, v1xx → v200 → v201, fallthrough → defaults); `PopulateDocFromStructs` writes `variant`; `DeserializeFromBuffer` does defensive variant rejection; `MigrateV103ToV200` now stamps version=200 (not CURRENT_VERSION) for clean intermediate state |
| `test/test_migration/test_main.cpp` | Renamed test_version_migrated_to_200 → test_version_migrated_to_201; deleted test_v200_passes_through (broken fixture); added 5 new tests; added V200_JSON_ROOT_VERSION fixture and write_config helper |
| `platformio.ini` | Added `-DREV_B` to `[env:native_test]` build_flags (necessary fixup — see Deviations) |

## Version dispatch table (as implemented)

| `version` field | Variant check | Path | Outcome |
|-----------------|---------------|------|---------|
| `== 201` | matches `kConfig.NAME` | passthrough | structs populated from doc; no save |
| `== 201` | mismatches | reset + save | structs reset to defaults; v201 with current variant written |
| `== 200` | n/a (no variant in v200) | `MigrateV200ToV201` → save → `PopulateStructsFromDoc` | v201 written with `variant=kConfig.NAME` |
| `>= 100 && < 200` | n/a | `MigrateV103ToV200` → `MigrateV200ToV201` → save → `PopulateStructsFromDoc` | v201 written with `variant=kConfig.NAME` |
| anything else (incl. `> 201`, `0`, `< 100`) | n/a | reset + save | structs reset to defaults; v201 with current variant written |

SysEx (`DeserializeFromBuffer`) variant check: rejects payload (`return false`, no on-disk write) when `variant` is present and mismatched. Missing `variant` is *tolerated* so older editors sending v200-shaped payloads still work — the migration path catches them when version is too low.

## Test count delta

| Block | Before | After | Delta |
|-------|--------|-------|-------|
| v103 → v201 chain (renamed/preserved) | 11 | 11 | 0 (one renamed: `_to_200` → `_to_201`) |
| v200 → v201 migration | 0 | 2 | +2 |
| v201 round-trip + variant gating | 0 | 3 | +3 |
| **deleted** test_v200_passes_through | 1 | 0 | -1 (broken fixture) |
| **Total** | **12** | **16** | **+4** (net) |

All 16 tests pass under `pio test -e native_test -f test_migration` (2.75s).

## Verification

| Check | Result |
|-------|--------|
| `grep -c "MigrateV200ToV201" src/ConfigManager.cpp` | 4 (definition + 3 dispatch sites) |
| `grep -c 'doc\["variant"\] = variant::CurrentVariant::kConfig.NAME' src/ConfigManager.cpp` | 2 (`MigrateV200ToV201` + `PopulateDocFromStructs`) |
| `grep -c "Variant mismatch on load" src/ConfigManager.cpp` | 1 |
| `grep -c "SysEx config variant mismatch" src/ConfigManager.cpp` | 1 |
| `grep -c "Unknown or future config version" src/ConfigManager.cpp` | 1 |
| `pio run -e t16_release` | SUCCESS (75.7s) |
| `pio run -e t16_debug` | SUCCESS (74.5s) |
| `pio run -e t32_release` | SUCCESS (76.5s) |
| `pio run -e t32_debug` | SUCCESS (78.3s) |
| `pio test -e native_test -f test_migration` | 16 / 16 PASSED |

## Deviations

### Deviation 1: added `-DREV_B` to `[env:native_test]`
The plan said `-DT16` would be sufficient, with the assumption that the migration tests construct the "other" variant string at runtime to exercise both branches. That assumption held for the *runtime* variant gating, but it missed a *compile-time* dependency: `ConfigManager.hpp` includes `Configuration.hpp` → `variant.hpp` → `variant_t16.hpp` → `pinout_t16.h`, and `pinout_t16.h` requires `REV_A` or `REV_B` to choose pin numbers (no default is safe to pick — picking the wrong one would mask hardware bugs).

The native env had been compile-broken since Phase 12 (`pinout_t16.h` was introduced after the test env was created in Phase 01). Adding `-DREV_B` brings the test env back to compiling. This is the same revision the firmware envs use (`-DREV_B` appears in t16_debug, t16_release, t32_debug, t32_release).

**Follow-up risk:** if a future phase introduces compile-time branching on `REV_B` vs `REV_A` inside ConfigManager itself, the native_test env would lock the test to whichever revision is set here. That seems unlikely (pinout is hardware-only) but worth flagging.

### Deviation 2: `MigrateV103ToV200` now stamps `version = 200` (not CURRENT_VERSION)
The original implementation stamped `CURRENT_VERSION` at the end of `MigrateV103ToV200`. With CURRENT_VERSION = 201, that meant v103 → "v201" without ever calling `MigrateV200ToV201` (so no `variant` field). I changed it to stamp `200` — its proper output — and let the chain caller invoke `MigrateV200ToV201` next to land on 201 with variant. End state is identical to before for the v1xx chain, but the intermediate doc now matches its function name.

### Deviation 3: deleted `test_v200_passes_through` instead of fixing it
Per the plan's stated preference ("Choose deletion to keep the suite clean — comment the deletion in the test summary"). Documented in the test file's `V200_JSON_ROOT_VERSION` comment.

## `-DT16` sufficient on native_test? — Yes (with `-DREV_B` added)

`-DT16` is sufficient for the *variant identity* the migration tests need — they construct "T32" at runtime via `(strcmp(...,"T16")==0) ? "T32" : "T16"` to exercise mismatch. The pinout dependency is orthogonal and required `-DREV_B` regardless. No T32 native env is needed for SCHEMA-03 coverage.

## Files committed

```
src/ConfigManager.hpp
src/ConfigManager.cpp
test/test_migration/test_main.cpp
platformio.ini  (added -DREV_B to native_test)
```
