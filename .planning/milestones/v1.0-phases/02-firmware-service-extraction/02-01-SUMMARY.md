---
phase: 02-firmware-service-extraction
plan: 01
subsystem: firmware
tags: [c++, enums, namespace, service-extraction, mode-management, scales]

# Dependency graph
requires:
  - phase: 01-foundation
    provides: PlatformIO project structure and build configuration
provides:
  - ModeManager service class with mode state and slider mode cycling
  - Shared Types.hpp with Mode and SliderMode enums in namespace t16
  - Properly split Scales module (.hpp/.cpp) safe for multi-TU inclusion
affects: [02-firmware-service-extraction, 04-sysex-protocol]

# Tech tracking
tech-stack:
  added: []
  patterns: [namespace t16 for new service code, trailing underscore for private members, pragma once headers, extern declarations for shared data arrays]

key-files:
  created:
    - src/Types.hpp
    - src/services/ModeManager.hpp
    - src/services/ModeManager.cpp
    - src/Scales.cpp
  modified:
    - src/Libs/Types.hpp
    - src/Libs/Keyboard.hpp
    - src/Libs/Adc.cpp
    - src/main.cpp
    - src/Scales.hpp

key-decisions:
  - "Kept enums unscoped with global using declarations for backward compatibility -- avoids touching 75+ references in main.cpp"
  - "Used namespace t16 with using declarations to bridge old and new code gradually"

patterns-established:
  - "Service files go in src/services/ with .hpp/.cpp pairs"
  - "Trailing underscore for private members in service classes (eisei convention)"
  - "Namespace t16 for all new service code"
  - "Data arrays extern-declared in headers, defined in .cpp files"

requirements-completed: [FWARCH-01, FWARCH-03, FWARCH-04]

# Metrics
duration: 8min
completed: 2026-04-03
---

# Phase 02 Plan 01: Types and ModeManager Summary

**ModeManager service with mode/slider-mode state machine, shared Types.hpp with namespaced enums, and Scales.hpp split into proper .hpp/.cpp compilation units**

## Performance

- **Duration:** 8 min
- **Started:** 2026-04-03T20:42:42Z
- **Completed:** 2026-04-03T20:51:19Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments
- Extracted Mode and SliderMode enums into shared src/Types.hpp under namespace t16 with backward-compatible global using declarations
- Created ModeManager service with mode state, allowed slider modes per mode, cycle logic, and default slider mode resolution
- Split Scales.hpp into declaration-only header and separate .cpp with all data arrays and function implementations

## Task Commits

Each task was committed atomically:

1. **Task 1: Create shared Types.hpp and ModeManager service** - `6cb2c45` (feat)
2. **Task 2: Split Scales.hpp into .hpp/.cpp** - `74bac18` (refactor)

## Files Created/Modified
- `src/Types.hpp` - Shared enums (Mode, SliderMode) and Vector2 in namespace t16
- `src/services/ModeManager.hpp` - ModeManager class declaration with mode and slider mode accessors
- `src/services/ModeManager.cpp` - ModeManager implementation with allowed slider modes per mode and cycle logic
- `src/Scales.cpp` - Scale data arrays and SetNoteMap/SetChordMapping implementations
- `src/Scales.hpp` - Now declaration-only: enums, extern arrays, function prototypes
- `src/Libs/Types.hpp` - Redirect to new src/Types.hpp
- `src/Libs/Keyboard.hpp` - Removed Mode enum definition, now includes Types.hpp
- `src/Libs/Adc.cpp` - Fixed case-sensitive include (adc.hpp -> Adc.hpp)
- `src/main.cpp` - Removed SliderMode enum, now includes Types.hpp

## Decisions Made
- Kept enums unscoped with global using declarations for backward compatibility -- avoids touching 75+ references in main.cpp during this plan
- Used namespace t16 with using declarations to bridge old and new code gradually
- Preserved Vector2 in Types.hpp even though it's currently unused (may be needed by future services)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed case-sensitive include paths**
- **Found during:** Task 1 (build verification)
- **Issue:** `src/Libs/Adc.cpp` and `src/Libs/Keyboard.hpp` include `adc.hpp` (lowercase) but file is `Adc.hpp` (PascalCase). Linux is case-sensitive, causing fatal compile error.
- **Fix:** Changed includes to `Adc.hpp` to match actual filename
- **Files modified:** src/Libs/Adc.cpp, src/Libs/Keyboard.hpp
- **Verification:** Compilation passes (no fatal error on include)
- **Committed in:** 6cb2c45 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Pre-existing case-sensitivity bug prevented build verification. Fix was minimal and necessary.

## Issues Encountered
- PlatformIO build environment has pre-existing linker errors from library conflicts (BLE-MIDI/NimBLE multiple definitions, TinyUSB duplicate symbols, missing Python certifi module). These are NOT caused by this plan's changes and affect the main repo equally. Source compilation (all .cpp -> .o) succeeds cleanly; only the final link step fails due to library issues.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- ModeManager service is ready for consumption by InputProcessor, SliderProcessor, and ButtonHandler (Plan 04)
- Shared Types.hpp establishes the pattern for additional shared types
- Scales module is now safe for inclusion from multiple .cpp files
- Pre-existing linker errors need resolution before firmware can be flashed (out of scope for this plan)

---
*Phase: 02-firmware-service-extraction*
*Completed: 2026-04-03*

## Self-Check: PASSED

All files exist. All commits verified.
