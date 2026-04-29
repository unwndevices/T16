---
phase: 13-config-schema-migration
reviewed: 2026-04-29T00:00:00Z
depth: standard
files_reviewed: 11
files_reviewed_list:
  - .github/workflows/ci.yml
  - docs/schema-v201.md
  - editor-tx/src/contexts/ConfigContext.tsx
  - editor-tx/src/services/configValidator.test.ts
  - editor-tx/src/services/configValidator.ts
  - editor-tx/src/types/config.ts
  - platformio.ini
  - schema/t16-config.schema.json
  - src/ConfigManager.cpp
  - src/ConfigManager.hpp
  - test/test_migration/test_main.cpp
findings:
  critical: 1
  warning: 7
  info: 4
  total: 12
status: issues_found
---

# Phase 13: Code Review Report

**Reviewed:** 2026-04-29
**Depth:** standard
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Phase 13 introduces the v201 schema (variant discriminator) with migration paths on
both firmware and editor sides. The implementation is structurally sound: the
dispatch tables on both sides are symmetric, the chained migration `v103 → v200
→ v201` works correctly, and the test coverage (16 firmware + 14 editor) is
thorough.

The review found one BLOCKER on the firmware SysEx path (no version-aware
migration in `DeserializeFromBuffer`), several WARNINGs around defensive parsing
edges (uint8 truncation, silent default-injection on missing variant, ACK
mis-attribution in the editor sync timing path), and a few INFO-level items.

The two test/doc files in scope are correct; no test reliability issues. The
generated `editor-tx/src/types/config.ts` is excluded from active review because
it is regenerated from the schema (`// DO NOT EDIT — generated`).

## Critical Issues

### CR-01: `DeserializeFromBuffer` has no version check or migration path

**File:** `src/ConfigManager.cpp:161-187`
**Issue:** The SysEx config-load handler (`DeserializeFromBuffer`) checks the
`variant` field but never inspects `version`. It then calls
`PopulateStructsFromDoc(doc)`, which itself only branches on whether
`doc["global"]` exists (v200/v201 nested) versus a v103 flat fallback — no
migration is invoked.

This means:

1. A v200-shaped SysEx payload (no `variant` key — explicitly tolerated at
   line 176-177 via the `fileVariant[0] != '\0'` guard) will be loaded directly
   via `PopulateStructsFromDoc`, and `global_.version` will be stamped 200
   (line 405). The next `Save()` will then write v201 to flash via
   `PopulateDocFromStructs` (line 478), silently bumping the on-disk version
   without running any migration logic. For v200 → v201 there is no field
   transform, so the data ends up correct — but the invariant "all version
   transitions go through `MigrateV*` functions" is broken on the SysEx path.

2. A future v202+ payload sent by a newer editor would also bypass the
   forward-version reject path and be loaded with whatever `PopulateStructsFromDoc`
   manages to extract. This contradicts D13.2 (forward-incompatible: "log warning,
   fall back to defaults") that the on-flash path enforces in `MigrateIfNeeded`
   lines 272-282.

3. A genuinely malformed payload that happens to have a `global` object will
   still populate structs and `MarkDirty()`, scheduling a flash write of
   degraded data.

**Severity rationale:** Data-integrity risk on the SysEx hot path, asymmetric
behavior versus the on-flash migration contract, and missing forward-version
defense documented as a design requirement (D13.2). This is the single highest-
exposure surface added in Phase 13 because every editor save round-trips
through it.

**Fix:**
```cpp
bool ConfigManager::DeserializeFromBuffer(const char* buffer, size_t length)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buffer, length);
    if (error)
    {
        log_d("DeserializeFromBuffer failed: %s", error.c_str());
        return false;
    }

    // Reject forward versions explicitly (parity with MigrateIfNeeded / D13.2).
    uint16_t version = doc["version"] | 0;  // wider than uint8 to catch overflow
    if (version > CURRENT_VERSION)
    {
        log_d("SysEx config version %u > %u — rejecting", version, CURRENT_VERSION);
        return false;
    }

    // Variant check (existing logic) ...
    const char* fileVariant = doc["variant"] | "";
    if (fileVariant[0] != '\0' && strcmp(fileVariant, variant::CurrentVariant::kConfig.NAME) != 0)
    {
        log_d("SysEx config variant mismatch (msg=%s, device=%s) — rejecting",
              fileVariant, variant::CurrentVariant::kConfig.NAME);
        return false;
    }

    // Run migrations so older payloads land on v201 before populating structs.
    if (version >= 100 && version < 200) {
        MigrateV103ToV200(doc);
        MigrateV200ToV201(doc);
    } else if (version == 200) {
        MigrateV200ToV201(doc);
    } else if (version != CURRENT_VERSION) {
        log_d("SysEx config version %u not recognized — rejecting", version);
        return false;
    }

    PopulateStructsFromDoc(doc);
    MarkDirty();
    return true;
}
```

## Warnings

### WR-01: v201 file with missing `variant` field is silently destroyed instead of repaired

**File:** `src/ConfigManager.cpp:219-238`
**Issue:** When `version == CURRENT_VERSION` and the on-flash file is malformed
in a way that omits the `variant` key (e.g., user editing the JSON by hand,
partial flash write recovered by LittleFS, downgraded firmware that wrote v201
without variant due to an earlier bug), `doc["variant"] | ""` yields `""`, and
`strcmp("", kConfig.NAME) != 0` is true. The branch then resets all structs to
defaults and overwrites the file — silent **user-config data loss** with only a
`log_d` trace.

The intent of the variant-mismatch branch is to handle a *real* mismatch
(T32 file on T16 device). Treating "no variant at all" identically to a true
mismatch is too aggressive — a missing variant on a v201 file is more likely
file corruption than a wrong-device file, and at minimum should attempt a
v200-style repair (re-inject the current variant) rather than wipe.

**Fix:** Distinguish the two cases:
```cpp
const char* fileVariant = doc["variant"] | "";
if (fileVariant[0] == '\0') {
    // Missing variant on a v201 file — repair by injecting current variant
    // and re-saving (matches v200→v201 semantics).
    log_d("v201 config missing variant — repairing with %s",
          variant::CurrentVariant::kConfig.NAME);
    doc["variant"] = variant::CurrentVariant::kConfig.NAME;
    File outFile = LittleFS.open(CONFIG_FILE, "w");
    if (outFile) { serializeJson(doc, outFile); outFile.close(); }
    PopulateStructsFromDoc(doc);
    return true;
}
if (strcmp(fileVariant, variant::CurrentVariant::kConfig.NAME) != 0) {
    // True mismatch — defaults reset path (existing logic)
    ...
}
```

### WR-02: `version` parsed as `uint8_t` truncates payloads ≥ 256

**File:** `src/ConfigManager.cpp:217` (also `:404` in `PopulateStructsFromDoc`)
**Issue:** `uint8_t version = doc["version"] | 0;` truncates JSON values ≥ 256
to their low byte. A payload with `"version": 256` becomes `0`, falling into
the "unknown or future version" branch (which is the safe path, so no
immediate exploit), but the dispatch table in `MigrateIfNeeded` becomes
non-injective — `version == 0` could mean "JSON missing version", "JSON parse
gave 0", or "JSON had 256". Future schema bumps that exceed 255 would silently
collide with v0/v1.

The schema enum is currently bounded to 201, so this is latent but real once
the schema crosses byte boundaries. ArduinoJson 7's `as<uint16_t>()` resolves
it cleanly.

**Fix:**
```cpp
uint16_t version = doc["version"] | 0;
// keep CURRENT_VERSION as uint8_t for now, just widen the parse target
```
(Apply at `:217` and `:404`. The struct field `global_.version` can stay
`uint8_t` until the schema actually exceeds 255 — but the parse must be
truncation-safe.)

### WR-03: Param-ACK matching attributes round-trip to wrong pending entry

**File:** `editor-tx/src/contexts/ConfigContext.tsx:213-225`
**Issue:** `handleSysexData` takes the *first* entry from
`pendingParamTimestamps` when a param-ACK arrives:
```ts
const entry = timestamps.entries().next()
if (!entry.done) {
  const [key, sendTime] = entry.value
  ...
  timestamps.delete(key)
}
```
There is no field to bind the ACK to a specific param. With multiple
in-flight param updates (which the codebase does support — each `updateParam`
call inserts an entry, and the 500ms retry timer deletes only its own key),
ACKs are dequeued FIFO regardless of which param they actually acknowledge.

Practical consequences:
- The `[T16 Sync] Param ${key} round-trip: ${roundTrip.toFixed(1)}ms` log
  reports incorrect param↔latency mappings.
- A late ACK arriving after a retry timeout has already deleted its key will
  dequeue an *unrelated* still-pending entry, hiding a sync failure for a
  different param (the second-level "sync failed after retry" log will never
  fire for that other param).
- DEFAULT_CONFIG sync invariants in tests would not catch this because the
  test path doesn't exercise concurrent in-flight updates.

This file is in Phase 13 scope (DEFAULT_CONFIG bump to v201), so the issue is
in-scope for the review even though the bug pre-dates Phase 13. Flag for a
follow-up (sysex protocol-level fix — ACK should echo enough of the original
domain/bank/field tuple to disambiguate).

**Fix:** Either include the (domain, bank, field) tuple in the ACK payload
and look up by that, or — short-term — drop the round-trip telemetry entirely
rather than report numbers that are only valid in the single-flight case.

### WR-04: `prepareImport` "unreachable" branch can be reached by float versions

**File:** `editor-tx/src/services/configValidator.ts:127-188`
**Issue:** `version` is taken via `typeof obj?.version === 'number'` and falls
back to `201`. `number` includes floats. The dispatch on lines 132/143/153/168
(`=== 201`, `> 201`, `=== 200`, `< 200`) does NOT cover `200 < version < 201`
(e.g., `200.5`) or `201 < version < 202` (none if integer-only) and similarly
non-integer values between 200 and 201 fall through every branch except the
"unreachable" guard at line 187. The guard does the right thing (rejects),
but the comment is wrong and the dispatch is fragile to a non-integer file.

The schema would reject the v201 case at validation time, but for a v200.5
input the code reaches the "unreachable" branch with the misleading message
"Unhandled version" rather than a typed error.

**Fix:** Either coerce to integer (`Math.trunc`) at the dispatch entry, or
add an explicit `if (!Number.isInteger(version))` guard at line 130-131 with a
schema-aligned error message.

### WR-05: `migrateV103` validation predicate is logically broken (always true for two keys)

**File:** `editor-tx/src/services/configValidator.ts:51-53`
**Issue:** The check
```ts
const hasGlobalKeys = GLOBAL_KEYS.every(
  key => key in data || (key === 'custom_scale1' || key === 'custom_scale2'),
)
```
short-circuits to `true` for the two scale keys regardless of whether they
exist in `data` — `key === 'custom_scale1'` is unconditionally `true` when
iterating that key. So the predicate effectively asks "do all 7 *non-scale*
global keys exist?" and treats the scales as always-present (defaults are
filled in below).

The behavior is *correct* (defaults are handled at lines 68-73) but the
predicate's intent is opaque and a future contributor could "simplify" the
expression and break the default-filling guarantee.

**Fix:** Make the intent explicit:
```ts
const REQUIRED_GLOBAL_KEYS = GLOBAL_KEYS.filter(
  k => k !== 'custom_scale1' && k !== 'custom_scale2',
)
const hasGlobalKeys = REQUIRED_GLOBAL_KEYS.every(key => key in data)
```

### WR-06: `migrateV103` casts unvalidated bank objects to `T16Configuration['banks']`

**File:** `editor-tx/src/services/configValidator.ts:78-82`
**Issue:** `(data.banks as Record<string, unknown>[]).map(...) as T16Configuration['banks']`
trusts each bank object to have all required fields (`ch`, `scale`, `oct`,
`note`, `vel`, `at`, `flip_x`, `flip_y`, `koala_mode`, `chs`, `ids`). The
only validation is `data.banks.length === 4`. A v103 file with 4 banks each
missing any required field would produce an "object" that satisfies the type
cast but fails ajv validation downstream.

This is recovered (the chained `validateConfig` in `prepareImport` rejects
it), so it's a WARNING not a BLOCKER. But the type assertion masks the missing
shape check from TS, weakening static guarantees.

**Fix:** Validate each bank's required scalar fields inside `migrateV103`
before the cast, OR drop the type cast and let ajv handle it (returning
`Record<string, unknown>` from `migrateV103` and adjusting the chain in
`prepareImport`). The latter is cleaner.

### WR-07: `firmware-format` CI job will fail without a `.clang-format`

**File:** `.github/workflows/ci.yml:64-74`
**Issue:** The `firmware-format` job runs
`clang-format --dry-run --Werror` against every `.hpp`/`.cpp`/`.h` file in
`src/`. The repo has no `.clang-format` config (per CLAUDE.md "no firmware
linting/formatting tools"), so clang-format applies the LLVM defaults — which
contradict the codebase's actual style (Allman braces, 4-space indent, mixed
naming). Every file will fail, breaking CI on first run.

This job appears to be pre-existing (not added in Phase 13), but Phase 13's
SUMMARY claims CI passes. Confirm whether the firmware-format job is currently
gating or has been disabled/skipped on the runner.

**Fix:** Either add a `.clang-format` matching the existing style, or remove
the `firmware-format` job from CI until a style config is decided. (CLAUDE.md
notes this is a known gap — Phase 13 didn't introduce it but the CI matrix
re-touched in 13-04 didn't address it either.)

## Info

### IN-01: `MigrateV103ToV200` no longer matches its name's promise after Phase 13

**File:** `src/ConfigManager.cpp:285-341`
**Issue:** Per Plan 13-03 deviation 2, the function now stamps `version = 200`
and is always chained through `MigrateV200ToV201`. The function does what its
name says, but every call site invokes it followed by `MigrateV200ToV201` in
the same block. A small refactor to a `MigrateV103ToCurrent` chain wrapper
would reduce duplication and prevent a future caller from invoking
`MigrateV103ToV200` standalone and ending up with a v200 doc on disk.

**Fix:** Optional refactor — introduce `MigrateV103ToCurrent(JsonDocument&)`
that calls both helpers, and have the dispatch in `MigrateIfNeeded` call only
the wrapper.

### IN-02: `PopulateDocFromStructs` ignores `global_.version`

**File:** `src/ConfigManager.cpp:476-479`
**Issue:** `doc["version"] = CURRENT_VERSION;` always stamps the compile-time
constant. `global_.version` (set by `PopulateStructsFromDoc` at `:405` from
JSON) is never written back to disk. This is intentional (we always save
current-version) but means `global_.version` is effectively a debugging field,
not a data-flow field. Worth a comment.

**Fix:** Add a comment at `:478` noting the field is intentionally ignored on
write so future readers don't try to "preserve" it.

### IN-03: `test_v201_matching_variant_passes_through` uses empty banks array

**File:** `test/test_migration/test_main.cpp:212`
**Issue:** The fixture has `"banks":[]`, which is valid for the loop in
`PopulateStructsFromDoc` (loop bound `i < BANK_AMT && i < banksArray.size()`
just exits early), but this means the test does not actually exercise bank
preservation on the v201-passthrough path. The two scalar assertions
(`mode`, `brightness`) cover the global object only. Add a single bank-field
assertion or a populated banks array to give v201 passthrough true coverage.

**Fix:** Either use a 4-bank fixture in the v201 test, or add a comment
explaining banks are deliberately omitted since the migration tests above
already cover bank preservation.

### IN-04: `migrateV200ToV201` always overwrites a stray `variant` field

**File:** `editor-tx/src/services/configValidator.ts:96-103`
**Issue:** The comment notes that "a v200 input with a stray `variant` field
is treated as malformed — migration overwrites it with the caller's
defaultVariant. This is correct because `variant` only became part of the
schema at v201." This is fine for the firmware-symmetric case, but a user who
manually edited a v200 file to add `variant: 'T32'` (anticipating v201 by
hand) would have their explicit choice silently overwritten with the default
'T16'. Not a bug given the documented design, but worth a one-line user-facing
warning emission so the editor can surface it.

**Fix:** Optional — return a flag (`overwroteVariant: boolean`) from
`migrateV200ToV201` that the load modal can show in its summary.

---

_Reviewed: 2026-04-29_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
