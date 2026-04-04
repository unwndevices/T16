---
phase: 01-protocol-data-foundation
plan: 01
subsystem: protocol
tags: [json-schema, typescript, sysex, midi, code-generation]

# Dependency graph
requires: []
provides:
  - "Canonical JSON Schema for T16 config (v200 format)"
  - "Generated TypeScript config types from schema"
  - "SysEx protocol constants in C++ and TypeScript"
  - "SysEx helper functions for web editor"
affects: [01-02, 01-03, 01-04, 02-firmware, 03-web-editor]

# Tech tracking
tech-stack:
  added: [json-schema-to-typescript]
  patterns: [json-schema-as-source-of-truth, generated-types, matching-protocol-constants]

key-files:
  created:
    - schema/t16-config.schema.json
    - schema/generate-types.sh
    - editor-tx/src/types/config.ts
    - src/SysExProtocol.hpp
    - editor-tx/src/protocol/sysex.ts
  modified:
    - editor-tx/package.json
    - editor-tx/package-lock.json

key-decisions:
  - "Used json-schema-to-typescript instead of quicktype (quicktype has a codePointAt bug on Node 22)"
  - "Added additionalProperties: false to schema for strict validation"

patterns-established:
  - "Schema-first types: JSON Schema is canonical, TypeScript types are always generated"
  - "Protocol constants mirroring: SysExProtocol.hpp and sysex.ts must define identical byte values"

requirements-completed: [PROTO-01, WEBARCH-05]

# Metrics
duration: 3min
completed: 2026-04-03
---

# Phase 01 Plan 01: Protocol & Data Contracts Summary

**JSON Schema (v200) with generated TypeScript types and matching SysEx protocol constants in C++ and TypeScript**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-03T13:37:50Z
- **Completed:** 2026-04-03T13:40:32Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- JSON Schema (draft 2020-12) defines all config fields with types, ranges, and constraints for v200 format
- TypeScript config types auto-generated from schema via json-schema-to-typescript
- SysEx command framing constants (manufacturer ID 0x7D, command/sub-command bytes, domain/field IDs) defined identically in firmware C++ and web TypeScript

## Task Commits

Each task was committed atomically:

1. **Task 1: Create JSON Schema and generate TypeScript types** - `ed44509` (feat)
2. **Task 2: Create SysEx protocol constants for firmware and web** - `5996981` (feat)

## Files Created/Modified
- `schema/t16-config.schema.json` - Canonical config schema (JSON Schema draft 2020-12, v200 format)
- `schema/generate-types.sh` - Build script that generates TypeScript types from schema
- `editor-tx/src/types/config.ts` - Generated TypeScript interfaces for T16 config
- `src/SysExProtocol.hpp` - Firmware SysEx constants (namespace SysEx, #pragma once)
- `editor-tx/src/protocol/sysex.ts` - Web editor SysEx constants and helper functions
- `editor-tx/package.json` - Added json-schema-to-typescript dev dependency
- `editor-tx/package-lock.json` - Lock file updated

## Decisions Made
- Used json-schema-to-typescript instead of quicktype for TypeScript generation. quicktype-core has a `codePointAt` bug on Node.js 22. json-schema-to-typescript is TS-focused, lighter, and works correctly.
- Added `additionalProperties: false` to all schema objects for strict validation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Switched from quicktype to json-schema-to-typescript**
- **Found during:** Task 1 (TypeScript type generation)
- **Issue:** quicktype CLI and quicktype-core both crash with `s.codePointAt is not a function` on Node.js 22.22.1
- **Fix:** Installed json-schema-to-typescript as alternative, updated generate-types.sh to use it programmatically
- **Files modified:** schema/generate-types.sh, editor-tx/package.json
- **Verification:** `bash schema/generate-types.sh` succeeds, output contains correct interfaces
- **Committed in:** ed44509 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Minimal -- alternative tool produces equivalent output. json-schema-to-typescript is well-maintained and TS-focused.

## Issues Encountered
None beyond the quicktype compatibility issue documented above.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Schema and protocol contracts are defined, ready for ConfigManager (01-02) and SysEx handler (01-03) implementation
- Generated TypeScript types ready for web editor consumption in Phase 3
- No blockers

## Self-Check: PASSED

All 5 created files verified present. Both task commits (ed44509, 5996981) verified in git log.

---
*Phase: 01-protocol-data-foundation*
*Completed: 2026-04-03*
