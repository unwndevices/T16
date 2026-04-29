# Phase 13 — Config Schema & Migration: Context

**Phase goal:** Config schema carries a `variant` discriminator and migrates v200 configs forward without data loss.
**Requirements:** SCHEMA-01, SCHEMA-02, SCHEMA-03, SCHEMA-04
**Depends on:** Phase 12 (T32 firmware functional and persisting calibration)

---

## Locked Decisions

### D13.1 — Cross-variant load: warn + prompt in editor-tx [SCHEMA-02]
- Firmware never adapts mismatched configs. Adaptation logic lives **only** in editor-tx.
- When the user loads a config whose `variant` differs from the connected device:
  - Editor-tx shows a modal: *"This config is for {fileVariant}, you're connected to {deviceVariant} — [Adapt and load] / [Cancel]"*.
  - **Adapt → T32:** pad per-key arrays from 16 to 32 with default values; preserve global settings.
  - **Adapt → T16:** truncate per-key arrays from 32 to 16 (keys 0–15 kept, keys 16–31 discarded); explicit warning that data is lost.
  - **Cancel:** do nothing; user keeps the file unchanged.
- No silent extend/truncate; no hard reject.

### D13.2 — Schema version: v201
- Bump from v200 by one. Migration rule: v200 → v201 default-injects `"variant": "T16"` (the only existing variant before v1.1).
- Firmware writes v201 on next save after migration.
- Forward-compat: firmware logs a warning and falls back to defaults on versions higher than its own (defensive — no silent acceptance of unknown future formats).

### D13.3 — Native PlatformIO `test/` env for migration tests
- Add `[env:native_test]` with `platform = native`, `test_framework = unity`.
- Test target: the migration transform function (host-compiled, no ESP32 needed).
- Wire into the GitHub Actions matrix from Phase 10 (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`, **`native_test`**).
- This is the firmware repo's first automated test infrastructure — set the precedent carefully (file layout: `test/test_migration/test_migration.cpp`).

### D13.4 — Firmware rejects variant-mismatched configs (falls back to defaults)
- Pairs with D13.1. Firmware logic on `LoadConfig`:
  1. Parse JSON; read `variant` field.
  2. If `variant` ≠ `CurrentVariant::kName` (e.g., T32 firmware reading a `"variant": "T16"` file): log warning, call `ApplyDefaults()`, return.
  3. If `variant` matches: proceed with normal load + per-variant array sizing via `kConfig.TOTAL_KEYS`.
- Single source of truth for adaptation = editor-tx. Avoids duplicating extend/truncate logic in firmware.
- Calibration files are independent (per-variant filenames per D12.2) so this rejection only affects `/configuration_data.json`.

---

## Schema Shape (v201)

```jsonc
{
  "version": 201,
  "variant": "T16" | "T32",          // SCHEMA-01: required
  "keyMap":   [/* TOTAL_KEYS items */], // SCHEMA-02: variant-bound size
  "scales":   [/* TOTAL_KEYS items */],
  "ccConfig": [/* TOTAL_KEYS items */],
  // ... existing global fields unchanged
}
```

## ajv Validator (`editor-tx/src/services/configValidator.ts`) [SCHEMA-04]

- `variant`: `enum: ["T16", "T32"]`, required.
- Per-key arrays: `minItems`/`maxItems` derived dynamically from variant — emit one schema per variant or use ajv `if/then` to branch on the discriminator.
- Reject on missing `variant` or wrong-sized arrays at import time, before the cross-variant prompt fires.

---

## Implementation Notes for Researcher / Planner

- **Migration rule (SCHEMA-03):** single hand-written transform in C++ (`Configuration::MigrateV200ToV201` or similar) and a parallel TypeScript transform in editor-tx. ArduinoJson + plain function only — no `UniversalConfiguration` or TLV machinery (explicitly out of scope per REQUIREMENTS.md).
- **Per-variant array sizing on firmware:** when reading v201, the load path must clamp/check array length against `CurrentVariant::kConfig.TOTAL_KEYS`. Anything else triggers the variant-mismatch rejection in D13.4 (since a T32 file on T16 would have 32-element arrays even with matching `variant`, that should be a different — but adjacent — error path).
- **Editor-tx adapt logic:** factor into a pure function (`adaptConfigForVariant(config, targetVariant): AdaptedConfig`) so it's unit-testable independent of the modal. Vitest tests in editor-tx are the natural home.
- **CI:** the native unit test env runs on Linux; ensure ArduinoJson compiles standalone (it does, but stub out anything Arduino-specific behind `#ifdef ARDUINO`).
- **Schema docs:** add a short `docs/schema-v201.md` (or update existing protocol doc) describing the discriminator and the migration rule. Editor-tx imports the same JSON Schema definition for ajv.

## Out of Scope for Phase 13

- ConfigContext / variant-conditional UI in editor-tx → Phase 14
- `.bin` selection in flasher → Phase 14
- On-disk header (magic + checksum) for config robustness → REQUIREMENTS.md Future
- Reversible/downgrade migrations → explicitly out of scope

## Open Items for Researcher

1. Confirm PlatformIO `platform = native` works on this repo's PlatformIO version and that ArduinoJson compiles natively (it does on recent versions — verify lockfile state).
2. Decide whether to use ajv `if/then/else` discriminator or generate per-variant schemas at build time. The former is simpler; the latter gives clearer error messages.
3. Locate the existing v200 config schema definition (firmware + editor) and identify all fields that become per-variant — the migration transform must touch every one.
4. Naming: should the migration function be `MigrateV200ToV201` or a generic `MigrateConfig(fromVersion)` dispatcher? With one rule today, the simple name is fine; revisit if v202 lands.
