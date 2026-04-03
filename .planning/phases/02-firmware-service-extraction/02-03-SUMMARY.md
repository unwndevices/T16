---
phase: 02-firmware-service-extraction
plan: 03
subsystem: firmware
tags: [cpp, header-split, unique_ptr, memory-leak, led-patterns]

# Dependency graph
requires:
  - phase: 01-protocol-config-foundation
    provides: ConfigManager and DataManager foundation
provides:
  - Keyboard.hpp/.cpp proper compilation unit split
  - Button.hpp/.cpp proper compilation unit split
  - LedManager.hpp/.cpp proper compilation unit split with extern LED globals
  - Pattern.hpp/.cpp with static member definitions in .cpp
  - Memory-safe pattern transitions via unique_ptr (FWBUG-01 fix)
  - Palettes.hpp shared gradient palette definitions
affects: [02-04, 02-05, 02-06]

# Tech tracking
tech-stack:
  added: []
  patterns: [header-source-split, unique_ptr-ownership, extern-globals, pragma-once]

key-files:
  created:
    - src/Libs/Keyboard.cpp
    - src/Libs/Button.cpp
    - src/Libs/Leds/LedManager.cpp
    - src/Libs/Leds/patterns/Pattern.cpp
    - src/Libs/Leds/Palettes.hpp
  modified:
    - src/Libs/Keyboard.hpp
    - src/Libs/Button.hpp
    - src/Libs/Leds/LedManager.hpp
    - src/Libs/Leds/patterns/Pattern.hpp
    - src/main.cpp
    - src/Libs/Adc.cpp

key-decisions:
  - "Used extern declarations for LED globals instead of private members -- patterns access patternleds/matrixleds directly across 9 header files"
  - "Extracted gradient palettes to Palettes.hpp shared header -- patterns reference palette constants in constructors"
  - "Added virtual destructor to Pattern base class -- required for correct unique_ptr polymorphic deletion"

patterns-established:
  - "Header/source split: declarations in .hpp, definitions in .cpp, static members defined in .cpp only"
  - "Ownership via unique_ptr: LedManager owns patterns through unique_ptr, transitions use make_unique"
  - "Extern global pattern: LED arrays defined in LedManager.cpp, declared extern in LedManager.hpp"

requirements-completed: [FWARCH-03, FWBUG-01]

# Metrics
duration: 11min
completed: 2026-04-03
---

# Phase 02 Plan 03: Header-Only Split Summary

**Split Keyboard/Button/LedManager/Pattern into .hpp/.cpp pairs, fixed LedManager memory leak with unique_ptr pattern ownership**

## Performance

- **Duration:** 11 min
- **Started:** 2026-04-03T20:42:36Z
- **Completed:** 2026-04-03T20:53:40Z
- **Tasks:** 2
- **Files modified:** 11

## Accomplishments
- Split 4 header-only implementations into proper .hpp/.cpp compilation units safe for multi-TU inclusion
- Fixed FWBUG-01 memory leak: LedManager pattern transitions now use unique_ptr instead of raw new/delete
- Replaced all #ifndef/#define include guards with #pragma once
- Moved static member definitions out of headers to prevent multiple-definition linker errors

## Task Commits

Each task was committed atomically:

1. **Task 1: Split Keyboard.hpp and Button.hpp into .hpp/.cpp pairs** - `ada829f` (feat)
2. **Task 2: Split LedManager and Pattern, fix memory leak with unique_ptr** - `e73d92f` (feat)

## Files Created/Modified
- `src/Libs/Keyboard.hpp` - Declarations only, no function bodies
- `src/Libs/Keyboard.cpp` - All Key/Keyboard/KeyboardConfig implementations, static members, fmap()
- `src/Libs/Button.hpp` - Declarations only, no function bodies
- `src/Libs/Button.cpp` - All Button implementations
- `src/Libs/Leds/LedManager.hpp` - Declarations with extern LED globals, unique_ptr members
- `src/Libs/Leds/LedManager.cpp` - All LedManager implementations, LED array definitions, XY()
- `src/Libs/Leds/patterns/Pattern.hpp` - Declarations with extern patternleds/XY, virtual destructor
- `src/Libs/Leds/patterns/Pattern.cpp` - Static member definitions, wu_pixel functions
- `src/Libs/Leds/Palettes.hpp` - Shared gradient palette definitions (unwn_gp, topo_gp, alt_gp, acid_gp)
- `src/main.cpp` - Updated to use make_unique for pattern transitions, removed global pattern instances
- `src/Libs/Adc.cpp` - Fixed case-sensitive include (adc.hpp -> Adc.hpp)

## Decisions Made
- Used extern declarations for LED globals instead of making them private LedManager members. The patterns (9 header-only files) directly reference patternleds, matrixleds, sliderleds, and XY() as globals. Making them private members would require rewriting every pattern file and passing pointers/references throughout -- deferred to a future pattern refactor.
- Extracted gradient palette definitions (unwn_gp, topo_gp, etc.) from main.cpp to a shared Palettes.hpp header. Pattern constructors reference these palettes, and with LedManager.cpp now a separate TU, both main.cpp and LedManager.cpp need access.
- Added `virtual ~Pattern() = default` to the Pattern base class. Required for correct polymorphic deletion through unique_ptr<Pattern>.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed case-sensitive include adc.hpp -> Adc.hpp**
- **Found during:** Task 1 (Keyboard.hpp split)
- **Issue:** Keyboard.hpp and Adc.cpp included "adc.hpp" but the file is named "Adc.hpp" (PascalCase). On case-sensitive Linux filesystems, this causes a compilation failure.
- **Fix:** Changed includes to use correct casing "Adc.hpp"
- **Files modified:** src/Libs/Keyboard.hpp, src/Libs/Adc.cpp
- **Verification:** Compilation succeeds
- **Committed in:** ada829f (Task 1 commit)

**2. [Rule 3 - Blocking] Extracted gradient palettes to shared header**
- **Found during:** Task 2 (LedManager split)
- **Issue:** Pattern constructors reference gradient palette constants (unwn_gp, topo_gp, etc.) that were defined in main.cpp. With LedManager.cpp as a separate TU, these symbols were no longer visible to pattern headers included from LedManager.cpp.
- **Fix:** Created src/Libs/Leds/Palettes.hpp with the gradient palette definitions, included from Pattern.hpp
- **Files modified:** src/Libs/Leds/Palettes.hpp (new), src/Libs/Leds/patterns/Pattern.hpp, src/main.cpp
- **Verification:** Compilation succeeds
- **Committed in:** e73d92f (Task 2 commit)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both auto-fixes necessary for compilation. No scope creep.

## Issues Encountered
- Pre-existing build failure: PlatformIO TinyUSB library conflict causes linker errors (hcd_* multiple definitions). This is a library version issue unrelated to this plan's changes. All source files compile successfully; only the final link step fails due to the TinyUSB conflict. This was already broken before this plan's changes.
- Pre-existing build failure: Adc.cpp had case-sensitive include issue (adc.hpp vs Adc.hpp) that prevented compilation on Linux. Fixed as deviation.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All 4 header-only files are properly split into compilation units
- Headers are safe for multi-TU inclusion (no static definitions, no function bodies, no global variable definitions)
- LedManager pattern transitions are memory-safe via unique_ptr
- Ready for service extraction (Plans 04-06) which will include these headers from multiple .cpp files

## Self-Check: PASSED

All created files verified present. All commit hashes verified in git log.

---
*Phase: 02-firmware-service-extraction*
*Completed: 2026-04-03*
