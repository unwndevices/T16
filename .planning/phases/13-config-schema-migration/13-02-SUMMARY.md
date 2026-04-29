# Plan 13-02 — Editor-tx v200→v201 Migration + Variant Adapter Summary

**Status:** complete
**Commits:** 1 (`8e46033`) — Task 1 and Task 2 are tightly coupled (test file imports new exports), single atomic commit
**Wave:** 2

## What changed

| File | Change |
|------|--------|
| `editor-tx/src/services/configValidator.ts` | Added `V200Config` type, `migrateV200ToV201`, `adaptConfigForVariant`. Rewrote `prepareImport` dispatch to handle v1xx/v200/v201/v202+ branches. |
| `editor-tx/src/services/configValidator.test.ts` | Added 3 new `describe` blocks (`migrateV200ToV201`, `adaptConfigForVariant`, `prepareImport (v200→v201)`) and 3 new tests in `validateConfig`. Updated 2 pre-existing assertions to v201 expectations. |

## prepareImport dispatch table

| Input version | Path | migrated flag | Output |
|---------------|------|---------------|--------|
| `201` | `validateConfig(data)` | `false` | data as-is if valid |
| `> 201` | reject | `false` | `{valid: false, errors: [{field: 'version', message: 'Config version N is not supported by this editor'}]}` |
| `200` | `migrateV200ToV201` → `validateConfig` | `true` | v201 with `variant: 'T16'` (default) |
| `< 200` | `migrateV103` → `migrateV200ToV201` → `validateConfig` | `true` | v201 with `variant: 'T16'` (default) |
| any unhandled | guard | `false` | `{valid: false, errors: [{field: 'version', message: 'Unhandled version'}]}` |

`prepareImport` does not currently take a `defaultVariant` argument — Phase 14's modal will pass it through `adaptConfigForVariant` after the import returns. This matches D13.1: editor-tx prompts the user for variant adaptation as a separate UI step from import.

## Test count delta

| Suite | Before | After | Delta |
|-------|--------|-------|-------|
| `validateConfig` | 6 | 9 | +3 (variant rejection tests) |
| `migrateV103` | 3 | 3 | 0 |
| `migrateV200ToV201` | 0 | 5 | +5 (new) |
| `adaptConfigForVariant` | 0 | 3 | +3 (new) |
| `prepareImport` | 3 | 3 | 0 (existing tests stay; one assertion adjusted from `version: 200` to `version: 201` because v103 → v201 is now the chain output) |
| `prepareImport (v200→v201)` | 0 | 3 | +3 (new) |
| **Total configValidator** | **12** | **26** | **+14** |
| **Full editor-tx suite** | 89 | 103 | +14 |

## Verification

- `cd editor-tx && npm run typecheck` → exit 0
- `cd editor-tx && npm test -- --run configValidator` → 26 passed
- `cd editor-tx && npm test -- --run` → 103 passed (all suites)
- `cd editor-tx && npm run build` → exit 0 (precache 51 entries, 1141.41 KiB)
- `cd editor-tx && npm run lint` → 21 problems (15 errors, 6 warnings); confirmed pre-existing by stashing changes and re-running lint — same 21 problems, untouched by Plan 13-02.

## Deviations

### Plan deviation 1: introduced `V200Config` type
The plan said "do NOT modify migrateV103" except to keep it producing v200 output. But after Plan 13-01 regenerated types, `migrateV103`'s return type `T16Configuration | null` was incompatible with v200 output (because `T16Configuration.version: 201` is a literal). The minimum-impact fix: introduce a dedicated `V200Config` type and change `migrateV103`'s return type to `V200Config | null`. The function body is unchanged, the v200 contract is preserved, and TypeScript is happy. This closes the typecheck regression Plan 13-01 flagged in its SUMMARY.

### Plan deviation 2: removed `migrateV103` instruction step (4) as a no-op
Plan 13-02 Task 1 step 4 said to "update migrateV103 return value to include variant" then immediately reversed itself with "Wait — migrateV103 MUST stay producing v200 output. Do NOT add variant inside migrateV103." Followed the corrected guidance — no `variant` injected in `migrateV103`.

### Plan deviation 3: pre-existing assertions adjusted
Two existing tests had implicit v200 assumptions:
- `validateConfig` "missing global field" test fixture had `{ version: 200, banks: ... }`. Updated to `{ version: 201, variant: 'T16', banks: ... }` so it actually exercises "missing global" without tripping the version/variant rules first.
- `prepareImport` "v103 well-formed" test asserted `result.config!.version === 200`. Since `prepareImport` now chains v103 → v200 → v201, the output is 201. Updated assertion.

These were necessary side effects of the schema bump, not behavioral changes to the tests' intent.

### Test changes pre-existing tests:
- `'skips migration for version >= 200, just validates'` renamed to `'skips migration for version === 201, just validates'` because the `>= 200` branch no longer exists in `prepareImport` (200 is now its own migration branch).

## Files committed

```
editor-tx/src/services/configValidator.ts
editor-tx/src/services/configValidator.test.ts
```
