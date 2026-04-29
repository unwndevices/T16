# Phase 13: Config Schema & Migration — Research

**Researched:** 2026-04-29
**Domain:** JSON schema versioning + dual-runtime migration (ArduinoJson on ESP32 firmware, ajv on the editor-tx web app)
**Confidence:** HIGH (codebase fully explored; existing v103→v200 migration provides the template)

## User Constraints (from 13-CONTEXT.md — verbatim)

### Locked Decisions
- **D13.1** — Cross-variant load: warn + prompt in editor-tx ONLY. Firmware never adapts. Modal shows source/target variant; Adapt→T32 pads per-key arrays 16→32, Adapt→T16 truncates 32→16 with explicit data-loss warning, Cancel does nothing.
- **D13.2** — Schema version v201. Migration v200→v201 default-injects `"variant": "T16"`. Firmware writes v201 on next save. Higher-than-known versions: log warning + fall back to defaults.
- **D13.3** — Native PlatformIO test env (`[env:native_test]`) with `platform = native`, `test_framework = unity`. Wired into CI matrix alongside the four firmware envs. File layout `test/test_migration/test_migration.cpp`.
- **D13.4** — Firmware rejects variant-mismatched configs by falling back to defaults (no adaptation logic on-device). Calibration files independent (per-variant filenames per D12.2) so this rejection only affects `/configuration_data.json`.

### Claude's Discretion
- Naming of the migration function (`MigrateV200ToV201` recommended over generic dispatcher — there is exactly one rule today)
- ajv discriminator implementation detail: `if/then/else` vs. per-variant generated schemas
- Whether `[env:native_test]` becomes a *new* env or whether the existing `[env:native]` env is renamed

### Deferred (out of scope for Phase 13)
- ConfigContext / variant-conditional UI in editor-tx → Phase 14
- `.bin` selection in flasher → Phase 14
- On-disk header (magic + checksum) for config robustness → REQUIREMENTS.md Future
- Reversible / downgrade migrations

## Project Constraints (from CLAUDE.md)
- PlatformIO + Arduino framework, ESP32-S3, no CMake migration.
- ArduinoJson 7 already pinned (`bblanchon/ArduinoJson@^7.0.3`).
- `additionalProperties: false` is already used in `schema/t16-config.schema.json` — preserve that strictness.
- TypeScript types are generated from the JSON Schema (`schema/generate-types.sh`) — every schema change MUST be followed by regenerating `editor-tx/src/types/config.ts`.
- Header guards via `#pragma once` for new headers; existing `Configuration.hpp` keeps `#ifndef` guards (don't churn).
- File layout convention: tests under `test/test_<area>/test_main.cpp` (existing pattern: `test/test_migration/test_main.cpp`, `test/test_config_manager/`, etc.).

## Summary

Phase 13 is a **bounded forward-only schema bump** with parallel firmware + editor-tx implementations. Almost all of the required infrastructure already exists in the repo:

- **Firmware:** `ConfigManager::MigrateIfNeeded()` already dispatches by version range, and `MigrateV103ToV200()` is the prototype the new `MigrateV200ToV201()` will mirror. `CURRENT_VERSION = 200` in `src/ConfigManager.hpp:47` is the single bump site. `ConfigurationData` already carries an `mode`-style field — `variant` will be added to the on-disk JSON via `kConfig.NAME` (compile-time) rather than as a stored runtime field, since the variant is fixed per-binary.
- **Editor-tx:** `services/configValidator.ts` already wraps ajv 2020 with `prepareImport()` doing `migrateV103` and validation. The new flow adds a `migrateV200ToV201()` step parallel to the existing `migrateV103`, plus a `variant` discriminator in the JSON schema and an `adaptConfigForVariant()` pure function consumed by a UI modal.
- **Tests:** `[env:native]` exists in `platformio.ini` with ArduinoJson + LittleFS + Arduino stubs (`test/native_stubs/`). `test/test_migration/test_main.cpp` exercises v103→v200 today; v200→v201 tests slot in next to it. CI already runs `pio test -e native` in `.github/workflows/ci.yml` (job `firmware-tests`), so D13.3 is **largely already satisfied** — the work is renaming/aliasing the env to match D13.3's `native_test` name (or accepting the existing `native` name as the fulfilled variant).

**Primary recommendation:** Reuse the existing v103→v200 migration as the template for v200→v201. Add `variant` to the schema as a top-level required string with `enum: ["T16", "T32"]`. On firmware, derive `variant` at write time from `variant::CurrentVariant::kConfig.NAME` (compile-time constant) rather than storing it in `ConfigurationData`. On editor-tx, write a pure `adaptConfigForVariant()` function tested via Vitest, consumed by the cross-variant load modal. Reuse the existing `[env:native]` env for migration tests (rename to `native_test` if D13.3 verbatim naming is required).

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| `variant` discriminator in on-disk JSON | Firmware (`ConfigManager`) | Editor-tx (writes back via SysEx) | Firmware writes v201 on next save; editor-tx writes during config import |
| `variant` derivation at write time | Firmware (`variant::CurrentVariant::kConfig.NAME`) | — | Compile-time constant; never stored in `ConfigurationData` to avoid drift |
| v200→v201 migration transform | Firmware (`MigrateV200ToV201`) **and** editor-tx (`migrateV200ToV201`) | — | Both surfaces must migrate independently; no shared code today |
| Per-variant array sizing on firmware load | Firmware (`PopulateStructsFromDoc`) | — | Already iterates `BANK_AMT`; per-key array clamps go here |
| Cross-variant load adaptation (extend / truncate) | Editor-tx (`adaptConfigForVariant`) | — | **Single source of truth per D13.4** — never duplicate to firmware |
| Cross-variant load UI prompt | Editor-tx component (modal) | — | Consumes `adaptConfigForVariant` pure function |
| Schema validation at import | Editor-tx (`configValidator.ts` ajv) | — | Firmware just rejects mismatches via `ApplyDefaults()` |
| Variant-mismatch firmware rejection | Firmware (`ConfigManager::Init` / migration path) | — | D13.4 — no adaptation, fall back to defaults |
| Migration tests | Firmware: PlatformIO Unity (native env) | Editor-tx: Vitest | Two test runners, two test suites — consistent with existing repo layout |

## Standard Stack

### Core (already pinned in this repo — do not introduce new dependencies)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ArduinoJson | `^7.0.3` (`bblanchon/ArduinoJson@^7.0.3` in `platformio.ini`) [VERIFIED: codebase] | Firmware JSON parse/serialize | Already used in `ConfigManager`; ESP32-friendly, deterministic memory |
| ajv | `^8.18.0` (`editor-tx/package.json`) [VERIFIED: codebase] | Editor-tx schema validation | Already used in `configValidator.ts`; supports `if/then/else` and JSON Schema 2020-12 |
| Unity (PlatformIO bundled) | shipped via `test_framework = unity` [VERIFIED: codebase] | Firmware native unit tests | Standard PlatformIO test runner; existing `test/test_migration/test_main.cpp` uses it |
| Vitest | `^3.x` (`editor-tx/package.json`, exact via `npm view` not run) [CITED: editor-tx/package.json] | Editor-tx unit tests | Already used in `services/configValidator.test.ts` |

### Supporting (already present)
| Library | Purpose | When to Use |
|---------|---------|-------------|
| `json-schema-to-typescript` | Generates `editor-tx/src/types/config.ts` from `schema/t16-config.schema.json` via `schema/generate-types.sh` | Run after every schema edit |
| `schema/validate-types.js` | Sanity-checks generated types against schema property names | Run in CI after type regen |

### Alternatives Considered (rejected)
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Per-variant generated schemas | Build-time codegen producing two schema files | Clearer error messages, but duplicates the schema and complicates the build; `if/then/else` is simpler and ajv supports it natively. **Chosen: `if/then/else` discriminator** |
| Storing `variant` in `ConfigurationData` (runtime struct) | Add `char variant_name[4]` member | Adds drift risk vs. compile-time `kConfig.NAME`. **Chosen: derive at serialize time** |
| New `[env:native_test]` env | Rename existing `[env:native]` to `native_test` | D13.3 names it `native_test` verbatim. Existing `[env:native]` already runs migration tests; safest path is to *rename* (single edit in `platformio.ini` + 1-line CI matrix update). **Chosen: rename to `native_test`** to satisfy D13.3 literally |

## Architecture Patterns

### System Architecture Diagram

```
                          ┌──────────────────────────────┐
   on-disk JSON           │   /configuration_data.json   │
   (LittleFS)             │   {version: 201, variant:    │
                          │    "T16" | "T32", ...}       │
                          └─────────────┬────────────────┘
                                        │
                ┌───────────────────────┼─────────────────────────┐
                │                       │                         │
                ▼                       ▼                         ▼
       Firmware load              Editor-tx import          Editor-tx export
       (ConfigManager::Init)      (prepareImport)           (sendFullConfig)
                │                       │                         │
                ▼                       ▼                         │
        version == 201?          ajv validate                     │
        ├─ no → migrate          ├─ valid    → check variant      │
        │  v200→v201             │  vs device                     │
        │  inject variant=       ├─ invalid  → reject             │
        │  kConfig.NAME          │                                │
        │                        ▼                                │
        ▼                  variant matches?                       │
        variant ==         ├─ yes → load                          │
        kConfig.NAME?      ├─ no  → modal:                        │
        ├─ yes → load      │       Adapt? → adaptConfigForVariant │
        ├─ no  → reject,   │       Cancel? → noop                 │
        │  ApplyDefaults() │                                      │
        ▼                  ▼                                      ▼
   in-memory structs   in-memory T16Configuration       SysEx → device
```

Entry points: (1) firmware boot via `ConfigManager::Init()`, (2) editor-tx file import via `prepareImport`, (3) editor-tx config dump SysEx response (already handled by `parseConfigDump`).

### Recommended Project Structure (deltas only — keep existing layout)

```
src/
├── ConfigManager.hpp          # bump CURRENT_VERSION 200 → 201
├── ConfigManager.cpp          # add MigrateV200ToV201() + variant rejection in Init/migration path
schema/
├── t16-config.schema.json     # add "variant" discriminator + version const 201
editor-tx/src/services/
├── configValidator.ts         # add migrateV200ToV201() + adaptConfigForVariant() (pure)
├── configValidator.test.ts    # extend with v200→v201 + adapt cases
test/
├── test_migration/
│   └── test_main.cpp          # extend with v200→v201 + variant-mismatch tests
docs/
└── schema-v201.md             # NEW — short doc on the discriminator + migration rule
```

### Pattern 1: Forward-only sequential migration
**What:** Each version bump adds one transform function; `MigrateIfNeeded` chains them.
**When to use:** Forward-compatibility only (this project explicitly excludes downgrades — see REQUIREMENTS.md "Out of Scope").
**Example:** Existing pattern in `src/ConfigManager.cpp:233` (`MigrateV103ToV200`). Mirror that signature exactly:
```cpp
bool ConfigManager::MigrateV200ToV201(JsonDocument& doc)
{
    // v200 has no "variant" key; v201 requires it.
    doc["variant"] = variant::CurrentVariant::kConfig.NAME;  // "T16" or "T32"
    doc["version"] = CURRENT_VERSION;                        // 201
    return true;
}
```
And in `MigrateIfNeeded`, extend the dispatch:
```cpp
if (version == 200)            { MigrateV200ToV201(doc); /* save+reload */ return true; }
if (version >= 100 && version < 200) { /* existing v103→v200 path */ }
```

### Pattern 2: Variant rejection on load (D13.4)
**What:** After parse, compare `doc["variant"]` to `kConfig.NAME`; mismatch → log + `ApplyDefaults` + return.
**Where:** Inside `MigrateIfNeeded` (after the v200→v201 dispatch) and `LoadFromFlash` (defensive — also after deserialize).
**Example:**
```cpp
const char* fileVariant = doc["variant"] | "";
if (strcmp(fileVariant, variant::CurrentVariant::kConfig.NAME) != 0) {
    log_d("Variant mismatch (file=%s, device=%s) — falling back to defaults",
          fileVariant, variant::CurrentVariant::kConfig.NAME);
    // Reset structs to defaults; do not import doc.
    global_ = ConfigurationData{};
    global_.version = CURRENT_VERSION;
    SaveToFlash();
    return false;
}
```

### Pattern 3: Pure adapter function (editor-tx)
**What:** `adaptConfigForVariant(config, targetVariant): AdaptedConfig` — no side effects, no React, no network.
**Why:** Unit-testable in isolation; the modal just calls it and dispatches the result.
**Example:**
```typescript
export function adaptConfigForVariant(
  config: T16Configuration,
  targetVariant: 'T16' | 'T32',
): T16Configuration {
  if (config.variant === targetVariant) return config
  // T16 → T32: pad arrays from 16 to 32 with defaults
  // T32 → T16: truncate arrays from 32 to 16 (data loss)
  // ... per-array branch logic, returning a new object (immutable)
}
```

### Anti-Patterns to Avoid
- **Storing `variant` in `ConfigurationData`** — risks drift between binary identity (compile-time) and on-disk record. Derive at serialize time only.
- **Sharing migration code between firmware and editor-tx** — they run in different languages; ArduinoJson and ajv have different idioms. Two parallel implementations + two test suites is correct.
- **Silent extend/truncate on firmware** — D13.4 forbids it. Anything that isn't an exact match → defaults.
- **Reversible migrations** — `MigrateV201ToV200` MUST NOT be written.
- **Editing `editor-tx/src/types/config.ts` by hand** — it's generated. Run `schema/generate-types.sh` after schema changes.
- **Generating per-variant schemas** — chosen against; use ajv `if/then/else` on the discriminator.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Schema validation in editor-tx | Custom validator walking the JSON | ajv 2020 (already in `package.json`) | Existing `validateConfig` already does it; just extend the schema |
| Schema versioning machinery | `UniversalConfiguration` / TLV / extension blocks | One hand-written `MigrateVxxxToVyyy` per bump | Explicitly out of scope per REQUIREMENTS.md and CONTEXT.md |
| Native test framework | Custom assert macros | Unity (already wired via `test_framework = unity`) | Existing `test/test_migration/test_main.cpp` uses it |
| TypeScript types from schema | Hand-written interfaces | `json-schema-to-typescript` via `schema/generate-types.sh` | Already wired; `validate-types.js` enforces consistency |

**Key insight:** Every supporting tool needed by Phase 13 is already in the repo. The work is *additive* — new code mirrors existing patterns line-for-line.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data (LittleFS) | `/configuration_data.json` may be v103, v200, or absent on existing devices | Migration path covers v103 (already) and v200 (new). Unknown versions → defaults. |
| Stored data (LittleFS) | `/calibration_data.json` (per D12.2 may be per-variant filenames) | Independent of config schema — Phase 13 does not touch calibration. Verify in implementation that `MigrateIfNeeded` does not accidentally read calibration files. |
| Build artifacts | `editor-tx/src/types/config.ts` is generated from `schema/t16-config.schema.json` | Re-run `schema/generate-types.sh` after schema edit; commit the regenerated types. |
| Live service config | None — no databases, queues, or external services in scope. | None. |
| Secrets / env vars | None. | None. |

## Common Pitfalls

### Pitfall 1: Forgetting to regenerate TypeScript types after schema edit
**What goes wrong:** `editor-tx` build fails with `Property 'variant' does not exist on type T16Configuration` or, worse, type-checks pass but the runtime ajv validation rejects valid configs because the schema and types are out of sync.
**Why it happens:** The schema is the source of truth; the types file is generated. Editing one without the other breaks the contract.
**How to avoid:** After every edit to `schema/t16-config.schema.json`, run `bash schema/generate-types.sh` and commit `editor-tx/src/types/config.ts`. Plan tasks must list both files in `files_modified`.
**Warning signs:** Vitest passes but `npm run typecheck` fails; or ajv errors fire on configs that look correct.

### Pitfall 2: Native test build pulls in ESP32 / TinyUSB / FastLED
**What goes wrong:** `pio test -e native` link errors with `undefined reference to ESP_LOG…` or duplicate symbols from TinyUSB.
**Why it happens:** Default `lib_deps` includes hardware libraries that don't compile on Linux.
**How to avoid:** `[env:native]` already sets `lib_ignore = Adafruit TinyUSB Library NimBLE-Arduino BLE-MIDI FastLED StreamUtils` — preserve this. New tests must `#include` only `ConfigManager.cpp` (which already has no hard FastLED/TinyUSB deps) and the stubs in `test/native_stubs/`.
**Warning signs:** Link errors mentioning ESP-IDF or TinyUSB symbols when running `pio test -e native_test`.

### Pitfall 3: `JsonDocument` size assumptions break on T32 vs T16
**What goes wrong:** v201 documents may grow when `BANK_AMT × per-key arrays` grows (future); a fixed-size `StaticJsonDocument` would overflow on T32.
**Why it happens:** ArduinoJson 7's `JsonDocument` is unbounded by default — but downstream callers must size their serialize buffers correctly.
**How to avoid:** `SerializeToBuffer(char* buffer, size_t maxSize)` already takes a size. Verify the SysEx send path (`MidiProvider::sendFullConfig` if applicable) uses a buffer sized for T32's worst case (2× T16). For Phase 13 specifically, `variant` is a 3-byte string — negligible. **Note for verification:** plan-checker should confirm no fixed-size `StaticJsonDocument<N>` exists in the migration path.
**Warning signs:** Truncated JSON on T32, intermittent SysEx parse failures, log messages "JsonDocument overflow".

### Pitfall 4: Variant rejection silently wipes user data
**What goes wrong:** A T32 user accidentally loads a T16 config; firmware sees mismatch, calls `ApplyDefaults()`, and saves over their existing T32 config — destroying customizations.
**Why it happens:** D13.4 says "fall back to defaults" — but the *trigger* is loading a mismatched file from SysEx, not boot.
**How to avoid:** `MigrateIfNeeded` runs at boot and reads from LittleFS — at boot, the file on disk was written by *this* firmware, so the variant must match (or the file is foreign — defaults are correct). The mismatch path only fires if a user manually copies a foreign file onto LittleFS, which is acceptable behavior. **For SysEx imports** (`DeserializeFromBuffer`), the variant check should reject *without* overwriting the on-disk file (return `false` instead of saving defaults). Plan tasks must distinguish these two paths.
**Warning signs:** A user reports "I synced a config and lost everything" after Phase 13 ships.

## Code Examples

### Adding `variant` to schema (`schema/t16-config.schema.json`)
```jsonc
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://unwn.dev/t16/config.schema.json",
  "title": "T16 Configuration",
  "type": "object",
  "properties": {
    "version": { "type": "integer", "const": 201 },
    "variant": { "type": "string", "enum": ["T16", "T32"] },
    "global": { /* unchanged */ },
    "banks": {
      "type": "array",
      "items": { /* unchanged */ },
      "minItems": 4,
      "maxItems": 4
    }
    // Per-key arrays (future SCHEMA-02 extension): if/then/else on variant
    // to set minItems/maxItems = 16 (T16) or 32 (T32). Apply only to fields
    // that are actually variant-bound today (none in v201 — all global
    // arrays are still 16 across both variants).
  },
  "required": ["version", "variant", "global", "banks"],
  "additionalProperties": false
}
```

### Editor-tx migration (`editor-tx/src/services/configValidator.ts`)
```typescript
// Source: mirrors existing migrateV103 pattern in the same file.
export function migrateV200ToV201(
  data: Record<string, unknown>,
  defaultVariant: 'T16' | 'T32' = 'T16',
): T16Configuration | null {
  if (typeof data.version !== 'number' || data.version !== 200) return null
  return {
    ...data,
    version: 201,
    variant: defaultVariant,
  } as T16Configuration
}

// Update prepareImport to dispatch through v200→v201
export function prepareImport(data: unknown): ImportResult {
  const obj = data as Record<string, unknown>
  const version = typeof obj?.version === 'number' ? obj.version : 201

  if (version < 200) { /* existing v103 path → produces v200, then fall through */ }
  if (version === 200) { /* migrateV200ToV201 → produces v201 */ }
  // version > 201 → reject (defensive: don't accept unknown future formats)
  // ... validate, return ImportResult
}
```

### Editor-tx adapter (`editor-tx/src/services/configValidator.ts` — new export)
```typescript
export function adaptConfigForVariant(
  config: T16Configuration,
  targetVariant: 'T16' | 'T32',
): T16Configuration {
  if (config.variant === targetVariant) return config
  // For v201, the only variant-bound fields are conceptual (per-key arrays
  // come later). Today: just rewrite the discriminator and emit a structurally
  // valid config for the target. When per-key arrays land, branch here.
  return { ...config, variant: targetVariant }
}
```

### Firmware migration (`src/ConfigManager.cpp`)
```cpp
bool ConfigManager::MigrateV200ToV201(JsonDocument& doc)
{
    // v200 lacks "variant"; v201 requires it. Default to current binary's variant.
    doc["variant"] = variant::CurrentVariant::kConfig.NAME;
    doc["version"] = CURRENT_VERSION;  // 201
    log_d("Migrated config v200 → v201 (variant=%s)",
          variant::CurrentVariant::kConfig.NAME);
    return true;
}

// In MigrateIfNeeded(), extend the version dispatch:
if (version == 200) {
    MigrateV200ToV201(doc);
    File outFile = LittleFS.open(CONFIG_FILE, "w");
    if (outFile) { serializeJson(doc, outFile); outFile.close(); }
    PopulateStructsFromDoc(doc);
    return true;
}
if (version == CURRENT_VERSION) {
    // v201: verify variant matches
    const char* fileVariant = doc["variant"] | "";
    if (strcmp(fileVariant, variant::CurrentVariant::kConfig.NAME) != 0) {
        log_d("Variant mismatch (file=%s, device=%s) — using defaults",
              fileVariant, variant::CurrentVariant::kConfig.NAME);
        global_ = ConfigurationData{};
        global_.version = CURRENT_VERSION;
        SaveToFlash();
        return false;
    }
    return false;  // already current, no migration needed
}
if (version > CURRENT_VERSION) {
    log_d("Config version %d > current %d — using defaults", version, CURRENT_VERSION);
    global_.version = CURRENT_VERSION;
    SaveToFlash();
    return false;
}
```

### Firmware variant emission on save (`src/ConfigManager.cpp` — `PopulateDocFromStructs`)
```cpp
void ConfigManager::PopulateDocFromStructs(JsonDocument& doc)
{
    doc["version"] = CURRENT_VERSION;
    doc["variant"] = variant::CurrentVariant::kConfig.NAME;  // NEW
    // ... existing global + banks population
}
```

### PlatformIO env (`platformio.ini` — rename existing `[env:native]` → `[env:native_test]`)
```ini
[env:native_test]
platform = native
build_flags =
	-std=c++17
	-I test/native_stubs
	-I src
	-DNATIVE_TEST
build_src_filter =
	-<*>
test_framework = unity
lib_deps =
	bblanchon/ArduinoJson@^7.0.3
lib_ignore =
	Adafruit TinyUSB Library
	NimBLE-Arduino
	BLE-MIDI
	FastLED
	StreamUtils
```
And update `.github/workflows/ci.yml` job `firmware-tests`:
```yaml
- name: Run firmware unit tests
  run: pio test -e native_test
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single-variant T16 firmware (no `variant` in config) | Variant discriminator + parallel migration | Phase 13 | Multi-binary support; cross-variant load explicit |
| `[env:native]` (Phase ≤12) | `[env:native_test]` (Phase 13) | This phase | CI matrix gains a 5th env name to match D13.3 verbatim |
| `migrateV103` only (editor-tx) | + `migrateV200ToV201` and `adaptConfigForVariant` | This phase | Editor-tx becomes single source of truth for cross-variant adaptation |

**Deprecated/outdated:** None for this phase — all infrastructure is current.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The existing `[env:native]` env can be renamed to `[env:native_test]` without breaking other tests (`test_config_manager`, `test_input_processor`, etc.) [ASSUMED — needs grep of `pio test -e native` references] | "Standard Stack" / "Code Examples" | If other tests/CI scripts hardcode `native`, the rename breaks them. Mitigation: planner instructs grep for `pio test -e native` across `.github/`, `scripts/`, `Makefile`, docs; either rename all references or keep both env names (alias). |
| A2 | Per-key variant-bound arrays do not exist in the v201 schema yet (only conceptually future) [ASSUMED from CONTEXT.md "Schema Shape" comments + REQUIREMENTS.md SCHEMA-02 wording] | "Architecture Patterns" / "Code Examples" | If SCHEMA-02 is interpreted as requiring per-key arrays in v201 itself (e.g., `keyMap[16|32]`), the schema and migration are larger. Mitigation: planner asks user to confirm; default interpretation (no new per-key arrays in v201) keeps scope tight. CONTEXT.md "Schema Shape" shows `keyMap`, `scales`, `ccConfig` fields that don't exist today — these may be aspirational. |
| A3 | Editor-tx `prepareImport` should reject `version > 201` (defensive forward-incompat) — CONTEXT D13.2 says firmware does this; symmetry is reasonable | "Code Examples" | If D13.2 only applies to firmware and editor-tx should pass through unknown future versions, the editor reject is wrong. Mitigation: explicit task with both interpretations flagged. Recommend reject for symmetry. |
| A4 | The "calibration files independent" decision (D13.4 / D12.2) means `MigrateIfNeeded` will not touch `/calibration_data*.json` [ASSUMED — codebase grep confirms `MigrateIfNeeded` only opens `CONFIG_FILE = /configuration_data.json`] | "Pitfall 4" | Already verified by reading `ConfigManager.cpp:163`. Risk: LOW. |

## Open Questions

1. **Should v201 add the per-key arrays (`keyMap`, `scales`, `ccConfig` of `TOTAL_KEYS` length)?**
   - What we know: CONTEXT.md "Schema Shape" lists them; REQUIREMENTS.md SCHEMA-02 says they "are variant-bound"; current schema has no such fields.
   - What's unclear: whether they land in v201 or are deferred to v202/Phase 14.
   - Recommendation: treat as **deferred** — v201 is *only* the discriminator + migration. Per-key arrays land when Phase 14 introduces them. Plan tasks should NOT add new array fields to the schema. Flag this as the first item to verify with user during plan checking; if user wants them in v201, add a single follow-up plan.

2. **Rename `[env:native]` to `[env:native_test]` or alias both?**
   - What we know: D13.3 names the env `[env:native_test]` literally; existing repo uses `[env:native]`.
   - What's unclear: whether downstream tooling (CI, dev docs, README) hardcodes `native`.
   - Recommendation: rename + grep for all references + update them. If grep returns >5 sites, plan-checker should flag for user input.

3. **Should `prepareImport` reject `version > 201` outright or warn-and-proceed?**
   - Recommendation: reject (symmetric with D13.2 firmware behavior).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| ArduinoJson | Firmware migration + native tests | ✓ | `^7.0.3` (lib_deps) | — |
| ajv | Editor-tx schema validation | ✓ | `^8.18.0` (package.json) | — |
| Unity | Native unit tests | ✓ | bundled with PlatformIO | — |
| Vitest | Editor-tx unit tests | ✓ | configured in `package.json` | — |
| `json-schema-to-typescript` | Type regeneration | ✓ (invoked via `schema/generate-types.sh` with `npx`) | latest | — |
| `platform = native` | PlatformIO native env compile | ✓ — env exists and runs in CI today | — | — |

**Missing dependencies:** None. All dependencies for Phase 13 are present and CI-verified.

## Validation Architecture

### Test Framework (firmware)
| Property | Value |
|----------|-------|
| Framework | Unity (PlatformIO bundled) |
| Config file | `platformio.ini` `[env:native_test]` (after rename) |
| Quick run command | `pio test -e native_test -f test_migration` |
| Full suite command | `pio test -e native_test` |

### Test Framework (editor-tx)
| Property | Value |
|----------|-------|
| Framework | Vitest 3.x |
| Config file | `editor-tx/vitest.config.ts` (or merged in vite config) |
| Quick run command | `cd editor-tx && npm test -- configValidator` |
| Full suite command | `cd editor-tx && npm test` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| SCHEMA-01 | `variant` is required in v201 schema; absent → ajv reject | unit (editor-tx) | `npm test -- configValidator -t "rejects.*missing variant"` | ❌ Wave 0 (extend `configValidator.test.ts`) |
| SCHEMA-01 | Firmware `PopulateDocFromStructs` writes `variant` | unit (firmware) | `pio test -e native_test -f test_migration -n test_serialized_output_has_variant` | ❌ Wave 0 (extend `test/test_migration/test_main.cpp`) |
| SCHEMA-02 | Variant-mismatched config → firmware falls back to defaults | unit (firmware) | `pio test -e native_test -f test_migration -n test_variant_mismatch_falls_back` | ❌ Wave 0 |
| SCHEMA-02 | `adaptConfigForVariant` round-trips T16↔T32 (size unchanged today) | unit (editor-tx) | `npm test -- configValidator -t "adaptConfigForVariant"` | ❌ Wave 0 |
| SCHEMA-03 | v200 → v201 firmware migration default-injects `variant` | unit (firmware) | `pio test -e native_test -f test_migration -n test_v200_migrated_to_v201` | ❌ Wave 0 |
| SCHEMA-03 | v200 → v201 editor-tx migration default-injects `variant` | unit (editor-tx) | `npm test -- configValidator -t "migrateV200ToV201"` | ❌ Wave 0 |
| SCHEMA-04 | ajv validator rejects missing `variant` and wrong size | unit (editor-tx) | `npm test -- configValidator -t "rejects.*variant"` | ❌ Wave 0 |
| SCHEMA-04 | ajv schema is regenerated to TypeScript types and types match | static check | `node schema/validate-types.js` | ✓ exists |

### Sampling Rate
- **Per task commit:** `pio test -e native_test -f test_migration` (firmware) / `npm test -- configValidator` (editor-tx)
- **Per wave merge:** `pio test -e native_test` + `cd editor-tx && npm test && npm run typecheck`
- **Phase gate:** Full CI matrix green (`t16_*`, `t32_*`, `native_test`, web build, web tests) before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Extend `test/test_migration/test_main.cpp` with v200→v201 + variant tests
- [ ] Extend `editor-tx/src/services/configValidator.test.ts` with migration + adapter tests
- [ ] Add a Vitest test file `editor-tx/src/services/adaptConfigForVariant.test.ts` if the export lives in a separate module (otherwise inline in `configValidator.test.ts`)

## Security Domain

> ASVS L1 applies. Firmware reads JSON from local flash (LittleFS) and from SysEx (USB/BLE/TRS). Editor-tx parses JSON loaded from user file picker and from SysEx dumps.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | n/a — local-only firmware, no auth surface |
| V3 Session Management | no | n/a |
| V4 Access Control | no | n/a — single-user device |
| V5 Input Validation | **yes** | ajv (editor-tx) + ArduinoJson size-bounded parse + variant check (firmware) |
| V6 Cryptography | no | no crypto in scope |
| V7 Error Handling | yes | log warnings without leaking memory contents (`log_d` already gated by `CORE_DEBUG_LEVEL`) |

### Known Threat Patterns for ESP32 + ArduinoJson + ajv

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Oversized JSON payload from SysEx → buffer overflow on firmware | DoS | `DeserializeFromBuffer(buffer, length)` already takes a length cap; verify the SysEx receive path enforces `length <= MAX_CONFIG_SIZE` *before* calling. |
| Malformed JSON → ArduinoJson parse failure leaks parser state | Information disclosure | Already mitigated: `error.c_str()` is logged at debug level only, never echoed back over MIDI. |
| Editor-tx loads a malicious JSON file from disk → ajv validation accepts unexpected fields | Tampering | `additionalProperties: false` in schema is already set; ensure new `variant` field also enforces `enum: ["T16", "T32"]` (closed set, not free-form string). |
| Variant spoofing — user crafts a T32 config with `"variant": "T16"` to bypass size checks | Tampering | ajv `if/then/else` on `variant` enforces per-variant array sizes; on firmware the size mismatch (e.g., 32-element bank in a T16 binary) triggers the variant rejection path even with matching variant string (CONTEXT.md "adjacent error path"). |
| Forward-version attack — user provides v999 config to trigger unknown-version code path | DoS / unexpected behavior | D13.2 firmware logs and falls back to defaults (no panic). Editor-tx `prepareImport` should reject (Open Question 3). |

### Threat Model Block (for plans to copy verbatim)

```xml
<threat_model>
  <threat id="V5.1" severity="medium" stride="Tampering">
    Variant spoofing: malicious config claims "variant": "T16" but contains
    32-key arrays (or vice versa). Firmware rejects via variant string
    mismatch; editor-tx ajv rejects via if/then/else array-size constraint.
  </threat>
  <threat id="V5.2" severity="low" stride="DoS">
    Forward-version attack (version > 201). Firmware logs and falls back to
    defaults. Editor-tx rejects with explicit error (no silent acceptance).
  </threat>
  <threat id="V5.3" severity="low" stride="Tampering">
    Oversized SysEx config payload. Existing DeserializeFromBuffer length cap
    is preserved; no new size assumptions introduced by Phase 13.
  </threat>
</threat_model>
```

No HIGH-severity threats. No blocking findings.

## Sources

### Primary (HIGH confidence)
- Codebase: `src/ConfigManager.cpp`, `src/ConfigManager.hpp`, `src/Configuration.hpp`, `src/variant_*.hpp` — read in full
- Codebase: `editor-tx/src/services/configValidator.ts`, `editor-tx/src/services/configValidator.test.ts`, `editor-tx/src/types/config.ts` — read in full
- Codebase: `schema/t16-config.schema.json`, `schema/generate-types.sh`, `schema/validate-types.js` — read in full
- Codebase: `platformio.ini` `[env:native]`, `test/test_migration/test_main.cpp`, `test/native_stubs/Arduino.h` — read in full
- Codebase: `.github/workflows/ci.yml` — verified existing matrix (4 firmware envs + native + format + web)
- `.planning/phases/13-config-schema-migration/13-CONTEXT.md` — locked decisions D13.1–D13.4
- `.planning/REQUIREMENTS.md` — SCHEMA-01 through SCHEMA-04 + Out of Scope list
- `.planning/ROADMAP.md` — Phase 13 success criteria (5 criteria)

### Secondary (MEDIUM confidence)
- ArduinoJson 7 API conventions — applied based on existing code patterns in `ConfigManager.cpp`. No external doc fetch performed in this session; all patterns mirror existing verified code.
- ajv 2020 `if/then/else` discriminator — confirmed via codebase usage of `Ajv from 'ajv/dist/2020'` in `configValidator.ts`. No fresh doc fetch.

### Tertiary (LOW confidence)
- None — all claims grounded in repo files.

## Metadata

- **Researcher:** inline orchestrator (gsd-plan-phase invoked without Task subagent capability)
- **Date:** 2026-04-29
- **Phase:** 13 (Config Schema & Migration)
- **Phase requirements:** SCHEMA-01, SCHEMA-02, SCHEMA-03, SCHEMA-04
- **Confidence:** HIGH

## RESEARCH COMPLETE
