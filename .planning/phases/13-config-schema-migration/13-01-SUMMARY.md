# Plan 13-01 — Schema Bump + Types Regen + DEFAULT_CONFIG Summary

**Status:** complete
**Commits:** 3 (`172c07f`, `308a4dd`, `d84d092`)
**Wave:** 1 (parallel slot, executed inline sequential)

## What changed

| File | Change |
|------|--------|
| `schema/t16-config.schema.json` | `version` const 200 → 201; added required `variant: enum["T16","T32"]`; root `required` now `["version","variant","global","banks"]`; `additionalProperties: false` preserved |
| `editor-tx/src/types/config.ts` | Regenerated via `bash schema/generate-types.sh`. `T16Configuration` now declares `version: 201` and `variant: "T16" \| "T32"` |
| `editor-tx/src/contexts/ConfigContext.tsx` | `DEFAULT_CONFIG.version: 200 → 201`, added `variant: 'T16'`. Also updated unrelated `_schema_version: 200 → 201` in `exportConfig` (consistent with the bump) |

## Verification

- `node -e "..."` schema invariant check: PASS (`schema OK`)
- `node schema/validate-types.js`: PASS (25 properties match)
- `head -1 editor-tx/src/types/config.ts`: contains `// DO NOT EDIT — generated`
- `grep -c "version: 201" editor-tx/src/types/config.ts`: 1
- `grep -c 'variant: "T16" | "T32"' editor-tx/src/types/config.ts`: 1
- `grep -c "version: 201," editor-tx/src/contexts/ConfigContext.tsx`: 1
- `grep -c "variant: 'T16'," editor-tx/src/contexts/ConfigContext.tsx`: 1

## Deviations

1. **`json-schema-to-typescript` was not in `editor-tx/package.json`**. Installed it transiently with `npm install --no-save --legacy-peer-deps json-schema-to-typescript` so `bash schema/generate-types.sh` could run. The dep is required by the regen script but never declared — this is a pre-existing repo gap, not a plan-13-01 introduction. Recommend adding it as a `devDependencies` in a follow-up.

2. **Typecheck (Task 3 acceptance criterion) transiently fails between Plans 13-01 and 13-02.** `editor-tx/src/services/configValidator.ts:66` returns `{ version: 200, ... }` from `migrateV103` typed as `T16Configuration`, which is now incompatible with the regenerated `version: 201` literal. Plan 13-02 explicitly preserves `migrateV103`'s v200 output and chains it through `migrateV200ToV201` — at which point the type contract is restored. The acceptance criterion is satisfied at the end of Wave 2 (post-13-02), not at the end of Plan 13-01 alone. This is an inter-plan dependency the original plan did not surface.

3. **`_schema_version: 200 → 201` in `exportConfig`** was outside the literal Task 3 actions but is required for consistency with the schema bump (otherwise exported configs claim v200 but contain a v201 shape). Bumped it.

## Inter-plan handoff

- Plan 13-02 must update `migrateV103`'s return type so the typecheck regression closes. The minimal fix: change the literal in `migrateV103`'s `return { version: 200, ... }` to a type cast (`as unknown as T16Configuration`) OR adjust the function signature to return `Record<string, unknown>` since the chained `migrateV200ToV201` re-shapes it. Plan 13-02's "do NOT modify migrateV103" instruction conflicts with the typecheck criterion and needs a small, principled deviation in 13-02 to land cleanly.
- Plan 13-03 reads `kConfig.NAME` which is `"T16"` for native_test (Plan 13-04 sets `-DT16`). The schema's `variant: enum["T16","T32"]` matches this exactly.

## Files committed

```
schema/t16-config.schema.json
editor-tx/src/types/config.ts
editor-tx/src/contexts/ConfigContext.tsx
```
