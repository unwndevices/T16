# Config Schema v201

**Effective:** 2026-04-29 (Phase 13)
**Predecessors:** v103 (legacy flat) → v200 (nested global) → **v201** (variant discriminator)
**Source of truth:** `schema/t16-config.schema.json`

## What changed from v200

1. New required top-level field: `variant: "T16" | "T32"`.
2. `version` const bumped from 200 to 201.
3. No other field changes. Per-key arrays sized by `TOTAL_KEYS` are reserved for a future schema bump (deferred per Phase 13 RESEARCH.md).

## Migration rule (v200 → v201)

Default-inject `variant` based on the migrating party's variant identity:

- **Firmware (`ConfigManager::MigrateV200ToV201`):** uses `variant::CurrentVariant::kConfig.NAME` — the compile-time identity of the binary doing the migration.
- **Editor-tx (`migrateV200ToV201` in `services/configValidator.ts`):** defaults to `'T16'` because v200 only existed in T16 builds. Callers may override via the `defaultVariant` argument.

No other fields are transformed. `version` is rewritten to `201`.

## Cross-variant load behavior

| Surface | Mismatch handling |
|---------|-------------------|
| Firmware boot (LittleFS read) | Log warning, reset structs to defaults, save v201 with current variant. **No adaptation logic on-device** (D13.4). |
| Firmware SysEx import | Reject the payload (return false); do NOT overwrite on-disk config. |
| Editor-tx file import | Validate via ajv. If schema valid but variant mismatches the connected device, the load modal offers Adapt or Cancel (D13.1). Adapt calls the pure `adaptConfigForVariant(config, targetVariant)` function. |

## Forward-version handling (D13.2)

| Surface | Behavior on `version > 201` |
|---------|------------------------------|
| Firmware | Log warning, fall back to defaults. No silent acceptance of unknown future formats. |
| Editor-tx | `prepareImport` rejects with an explicit error: `"Config version N is not supported by this editor"`. |

## What is out of scope for v201

- ConfigContext / variant-conditional UI (Phase 14).
- `.bin` selection in flasher (Phase 14).
- On-disk header (magic + checksum) for config robustness (REQUIREMENTS.md Future).
- Reversible / downgrade migrations (explicitly out of scope per REQUIREMENTS.md).
- Per-key arrays sized by `TOTAL_KEYS` (deferred — when introduced, they require an `if/then/else` size branch in the schema and a follow-up version bump).

## Reference implementation files

- **Schema:** `schema/t16-config.schema.json`
- **TypeScript types (generated):** `editor-tx/src/types/config.ts` (regenerate via `bash schema/generate-types.sh`)
- **Editor-tx validator:** `editor-tx/src/services/configValidator.ts` (`migrateV200ToV201`, `adaptConfigForVariant`, `prepareImport`)
- **Firmware migration:** `src/ConfigManager.cpp` (`MigrateV200ToV201`, `MigrateIfNeeded`, `PopulateDocFromStructs`)
- **Native unit tests:** `test/test_migration/test_main.cpp` (run via `pio test -e native_test`)

## Test commands

```bash
# Firmware migration tests
pio test -e native_test -f test_migration

# Editor-tx migration + validation tests
cd editor-tx && npm test -- configValidator

# Schema↔types consistency check
bash schema/generate-types.sh && node schema/validate-types.js

# Full CI parity
pio run -e t16_release && pio run -e t32_release && pio test -e native_test
cd editor-tx && npm run typecheck && npm test && npm run build
```
