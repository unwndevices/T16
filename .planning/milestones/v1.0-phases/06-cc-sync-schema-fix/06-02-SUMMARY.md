---
phase: 06-cc-sync-schema-fix
plan: 02
subsystem: protocol
tags: [sysex, midi, cc, react, reducer]

requires:
  - phase: 06-01
    provides: pal field in schema and types
provides:
  - Dedicated 5-byte CC SysEx send path (sendCCParamUpdate)
  - UPDATE_CC_PARAM reducer action for atomic chs/ids updates
  - Dashboard CC handlers using updateCCParam
affects: []

tech-stack:
  added: []
  patterns: [atomic CC param update pattern, domain-specific SysEx functions]

key-files:
  created: []
  modified:
    - editor-tx/src/protocol/sysex.ts
    - editor-tx/src/services/midi.ts
    - editor-tx/src/types/midi.ts
    - editor-tx/src/contexts/ConfigContext.tsx
    - editor-tx/src/pages/Dashboard/Dashboard.tsx
    - editor-tx/src/services/midi.test.ts
    - editor-tx/src/contexts/ConfigContext.test.tsx

key-decisions:
  - "CC updates always send channel+id together (atomic) matching firmware SetCCParam"
  - "Removed DOMAIN.BANK_CC from generic UPDATE_PARAM path to prevent broken 4-byte sends"

patterns-established:
  - "Domain-specific SysEx functions: sendCCParamUpdate alongside generic sendParamUpdate"
  - "ACK tracking uses cc-prefixed keys (cc-{bank}-{ccIndex}) to avoid collisions with generic param keys"

requirements-completed: [PROTO-02]

duration: 3min
completed: 2026-04-04
---

# Phase 06 Plan 02: CC Per-Parameter Sync Fix Summary

**Dedicated 5-byte CC SysEx send path replacing broken 4-byte generic path, with atomic channel+id updates**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-04T17:17:13Z
- **Completed:** 2026-04-04T17:20:13Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Added sendCCParamUpdate function that sends 5-byte payload matching firmware expectation [DOMAIN.BANK_CC, bank, ccIndex, channel, id]
- Wired UPDATE_CC_PARAM reducer action that atomically updates both chs[] and ids[] arrays
- Updated Dashboard CcMappingTab to always send channel+id together via updateCCParam
- Removed DOMAIN.BANK_CC from generic UPDATE_PARAM path to prevent broken 4-byte sends
- Added ACK tracking with retry logic for CC-specific keys

## Task Commits

Each task was committed atomically:

1. **Task 1: Add CC SysEx send function and types** - `027b2e5` (feat)
2. **Task 2: Wire CC reducer, context callback, and Dashboard handlers** - `df837a6` (feat)

## Files Created/Modified
- `editor-tx/src/protocol/sysex.ts` - Added sendCCParamUpdate with 5-byte CC payload
- `editor-tx/src/services/midi.ts` - Added service wrapper for sendCCParamUpdate
- `editor-tx/src/types/midi.ts` - Added UPDATE_CC_PARAM action type and updateCCParam method
- `editor-tx/src/contexts/ConfigContext.tsx` - Added reducer case, callback, and removed CC from generic path
- `editor-tx/src/pages/Dashboard/Dashboard.tsx` - Updated CcMappingTab to use updateCCParam
- `editor-tx/src/services/midi.test.ts` - Added test for 5-byte CC payload
- `editor-tx/src/contexts/ConfigContext.test.tsx` - Added test for UPDATE_CC_PARAM reducer

## Decisions Made
- CC updates always send channel+id together (atomic) matching firmware SetCCParam expectation
- Removed DOMAIN.BANK_CC from generic UPDATE_PARAM reducer case to ensure CC only flows through dedicated path

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- CC per-parameter sync is fixed, matching firmware 5-byte expectation
- Phase 06 plans complete (01: pal field, 02: CC sync)

---
*Phase: 06-cc-sync-schema-fix*
*Completed: 2026-04-04*
