---
phase: 04-integration-ci
plan: 02
subsystem: ui
tags: [ajv, json-schema, validation, migration, config-import, config-export]

requires:
  - phase: 03-web-rewrite
    provides: TypeScript ConfigContext, services layer, types
  - phase: 01-protocol-data-foundation
    provides: JSON schema, T16Configuration type, config format v200
provides:
  - configValidator service with ajv-based schema validation
  - v103-to-v200 config migration function
  - importConfig and exportConfig in ConfigContext
  - .topo file export with _schema_version field
affects: [04-integration-ci, settings-ui, backup-restore]

tech-stack:
  added: [ajv]
  patterns: [ajv/dist/2020 for JSON Schema draft 2020-12, prepareImport migrate-then-validate pipeline]

key-files:
  created:
    - editor-tx/src/services/configValidator.ts
    - editor-tx/src/services/configValidator.test.ts
  modified:
    - editor-tx/src/contexts/ConfigContext.tsx
    - editor-tx/src/contexts/ConfigContext.test.tsx
    - editor-tx/src/types/midi.ts

key-decisions:
  - "Used ajv/dist/2020 instead of ajv default for JSON Schema draft 2020-12 support"
  - "Consolidated sendFullConfig import into existing midi service import block"

patterns-established:
  - "Config import pipeline: prepareImport detects version, migrates if needed, validates against schema"
  - "Export format: .topo file with _schema_version and _exported_at metadata fields"

requirements-completed: [WEBFEAT-04]

duration: 3min
completed: 2026-04-04
---

# Phase 04 Plan 02: Config Import Validation Summary

**Ajv-based config import validation with JSON schema checking, v103 migration, and .topo export with schema version field**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-04T00:01:36Z
- **Completed:** 2026-04-04T00:05:05Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Config validation against JSON schema with field-level error messages (e.g., "banks.0.scale")
- v103 flat-format config migration to v200 nested format with bank data preservation
- importConfig in ConfigContext with migrate-then-validate pipeline and auto-send to device
- exportConfig producing .topo file download with _schema_version: 200 metadata

## Task Commits

Each task was committed atomically:

1. **Task 1: Create configValidator service with ajv validation and v103 migration** - `36c0fa7` (feat)
2. **Task 2: Add importConfig and exportConfig to ConfigContext** - `535215d` (feat)

## Files Created/Modified
- `editor-tx/src/services/configValidator.ts` - Ajv validation, migrateV103, prepareImport
- `editor-tx/src/services/configValidator.test.ts` - 9 unit tests for validation/migration/import
- `editor-tx/src/contexts/ConfigContext.tsx` - importConfig and exportConfig callbacks
- `editor-tx/src/contexts/ConfigContext.test.tsx` - Updated mocks for new dependencies
- `editor-tx/src/types/midi.ts` - Extended ConfigContextValue with importConfig/exportConfig
- `editor-tx/package.json` - Added ajv dependency

## Decisions Made
- Used `ajv/dist/2020` import path because the t16 schema uses JSON Schema draft 2020-12 which the default ajv export does not support
- Consolidated the `sendFullConfig` import into the existing midi service import block to avoid duplicate imports

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Changed ajv import path to ajv/dist/2020**
- **Found during:** Task 1 (configValidator implementation)
- **Issue:** Schema uses `$schema: "https://json-schema.org/draft/2020-12/schema"` which default ajv does not support
- **Fix:** Import from `ajv/dist/2020` instead of `ajv`
- **Files modified:** editor-tx/src/services/configValidator.ts
- **Verification:** All 9 tests pass
- **Committed in:** 36c0fa7

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Necessary for JSON Schema draft 2020-12 compatibility. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Config validation service ready for use in Settings UI import flow
- Export function ready to wire to export button in UI
- All tests passing (28/28 across full suite)

## Self-Check: PASSED

All files exist. All commits verified.

---
*Phase: 04-integration-ci*
*Completed: 2026-04-04*
