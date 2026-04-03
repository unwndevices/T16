---
phase: 01-protocol-data-foundation
plan: 02
subsystem: firmware
tags: [arduinojson, littlefs, config-persistence, esp32, migration]

# Dependency graph
requires: []
provides:
  - ConfigManager class with load-once/save-once flash persistence
  - Per-parameter RAM mutation with dirty flag and idle flush
  - v103 to v200 config migration (non-destructive)
  - SerializeToBuffer/DeserializeFromBuffer for SysEx config dump/load
affects: [01-01, 02-firmware-services, 04-integration]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "RAM-resident config with dirty flag and idle-timer flush (2s)"
    - "JsonDocument ephemeral at load/save boundaries only"
    - "v200 JSON format: global fields nested under 'global' key"
    - "#pragma once header guards (eisei convention)"
    - "Trailing underscore for private members (eisei convention)"

key-files:
  created:
    - src/ConfigManager.hpp
    - src/ConfigManager.cpp
  modified: []

key-decisions:
  - "PopulateStructsFromDoc handles both v103 flat format and v200 nested format for robustness during migration"
  - "MigrateIfNeeded handles version range 100-199 (not just 103) to cover all v1xx variants"
  - "SetGlobalParam uses integer field IDs (not string keys) matching SysExProtocol addressing scheme"

patterns-established:
  - "ConfigManager pattern: load once into structs, mutate in RAM, flush on idle"
  - "Allman brace style, 4-space indent, trailing underscore for private members"

requirements-completed: [FWARCH-06, FWFEAT-03]

# Metrics
duration: 2min
completed: 2026-04-03
---

# Phase 1 Plan 02: ConfigManager Summary

**Load-once/save-once ConfigManager replacing per-field DataManager, with dirty-flag idle flush and v103-to-v200 migration**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-03T13:37:51Z
- **Completed:** 2026-04-03T13:39:30Z
- **Tasks:** 2
- **Files created:** 2

## Accomplishments
- ConfigManager holds ConfigurationData, KeyModeData[4], and ControlChangeData[4] in RAM -- no flash access for reads or per-parameter mutations
- Idle flush pattern: MarkDirty records timestamp, CheckIdleFlush writes to flash only after 2s of no changes
- v103-to-v200 migration moves global fields from root level into "global" nested object, preserves all user data
- SerializeToBuffer/DeserializeFromBuffer enable SysEx full config dump/load without touching flash

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement ConfigManager with load-once/save-once persistence** - `76b52ea` (feat)
2. **Task 2: Implement v103 to v200 config migration** - included in `76b52ea` (migration fully implemented alongside ConfigManager)

## Files Created/Modified
- `src/ConfigManager.hpp` - Class declaration with full API: Init, Save, CheckIdleFlush, SetGlobalParam, SetBankParam, SetCCParam, SerializeToBuffer, DeserializeFromBuffer, MigrateIfNeeded
- `src/ConfigManager.cpp` - Complete implementation: LoadFromFlash, SaveToFlash, PopulateStructsFromDoc, PopulateDocFromStructs, MigrateV103ToV200, idle flush, per-parameter setters

## Decisions Made
- PopulateStructsFromDoc handles both v103 flat format and v200 nested format, so LoadFromFlash works correctly even before migration runs
- MigrateIfNeeded accepts any version in 100-199 range (not just 103), covering the v102 default in Configuration.hpp
- Used integer field IDs for SetGlobalParam/SetBankParam to align with the SysEx per-parameter addressing scheme from 01-RESEARCH.md

## Deviations from Plan

None - plan executed exactly as written. Task 2 (migration) was implemented alongside Task 1 since the migration methods are integral to ConfigManager and share PopulateStructsFromDoc. This resulted in a single commit covering both tasks rather than two separate commits.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- ConfigManager ready for integration with SysExProtocol (plan 01-01) via SetGlobalParam/SetBankParam for per-parameter updates
- ConfigManager ready for Phase 2 service extraction -- can replace DataManager usage in main.cpp
- SerializeToBuffer/DeserializeFromBuffer ready for SysEx full config dump/load commands

## Self-Check: PASSED

---
*Phase: 01-protocol-data-foundation*
*Completed: 2026-04-03*
