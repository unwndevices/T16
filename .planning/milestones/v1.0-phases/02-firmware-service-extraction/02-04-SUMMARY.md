---
phase: 02-firmware-service-extraction
plan: 04
subsystem: firmware
tags: [c++, service-extraction, dependency-injection, namespace, calibration, input-processing]

# Dependency graph
requires:
  - phase: 02-firmware-service-extraction
    provides: ModeManager, ConfigManager, shared Types.hpp, split Scales/Keyboard/LedManager
provides:
  - InputProcessor service for key/strum/quick-settings event processing
  - SliderProcessor service for slider mode processing
  - ButtonHandler service for button event dispatch via ModeManager
  - CalibrationService with timeout-based hardware test (FWBUG-04 fix)
affects: [02-firmware-service-extraction, 04-sysex-protocol]

# Tech tracking
tech-stack:
  added: []
  patterns: [service constructor injection with references, strum state accessors for cross-service communication, lambda marker callbacks]

key-files:
  created:
    - src/services/InputProcessor.hpp
    - src/services/InputProcessor.cpp
    - src/services/SliderProcessor.hpp
    - src/services/SliderProcessor.cpp
    - src/services/ButtonHandler.hpp
    - src/services/ButtonHandler.cpp
    - src/services/CalibrationService.hpp
    - src/services/CalibrationService.cpp
  modified:
    - src/Libs/Leds/LedManager.hpp
    - src/Libs/Leds/LedManager.cpp

key-decisions:
  - "SliderProcessor takes InputProcessor reference for strum state access in STRUMMING mode"
  - "ButtonHandler uses ModeManager.cycleSliderMode() instead of inline allowed_modes arrays (D-04)"
  - "CalibrationService creates its own DataManager for calibration persistence"
  - "Palette array passed as pointer to InputProcessor constructor for onBankChange palette lookup"

patterns-established:
  - "Cross-service state access via public getters (InputProcessor strum state)"
  - "Lambda callbacks for marker registration bridging member functions to std::function"

requirements-completed: [FWARCH-01, FWARCH-04, FWBUG-03, FWBUG-04]

# Metrics
duration: 10min
completed: 2026-04-03
---

# Phase 02 Plan 04: Input Processing and Calibration Services Summary

**Four service classes extracted from main.cpp: InputProcessor (key/strum/quick-settings), SliderProcessor (slider modes), ButtonHandler (button dispatch via ModeManager), CalibrationService (hardware test with 3s timeout fix)**

## Performance

- **Duration:** 10 min
- **Started:** 2026-04-03T20:58:36Z
- **Completed:** 2026-04-03T21:08:52Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- Extracted all application logic from main.cpp free functions into 4 service classes with dependency injection
- Fixed FWBUG-04: HardwareTest now uses 3-second timeout per key instead of infinite `while (!test_passed)` loop
- Fixed FWBUG-03: Dead XY_PAD branch (main.cpp lines 870-873) not carried into any service -- eliminated during extraction
- All services use ModeManager for mode queries and slider mode cycling (D-04)

## Task Commits

Each task was committed atomically:

1. **Task 1: Extract InputProcessor service** - `1665696` (feat)
2. **Task 2: Extract SliderProcessor, ButtonHandler, and CalibrationService** - `27f2e3e` (feat)

## Files Created/Modified
- `src/services/InputProcessor.hpp` - InputProcessor class with key/strum/QS processing and config application
- `src/services/InputProcessor.cpp` - Implementation with ConfigManager-based config access
- `src/services/SliderProcessor.hpp` - SliderProcessor class with update() and onSliderModeChanged()
- `src/services/SliderProcessor.cpp` - All slider mode processing extracted from ProcessSlider/ProcessSliderButton
- `src/services/ButtonHandler.hpp` - ButtonHandler class with handleButton() dispatch
- `src/services/ButtonHandler.cpp` - Button logic using ModeManager for mode cycling
- `src/services/CalibrationService.hpp` - CalibrationService with 3s timeout constant
- `src/services/CalibrationService.cpp` - HardwareTest and CalibrationRoutine with FWBUG-04 fix
- `src/Libs/Leds/LedManager.hpp` - Moved constructor/destructor out of header (unique_ptr fix)
- `src/Libs/Leds/LedManager.cpp` - Added default constructor/destructor definitions

## Decisions Made
- SliderProcessor holds InputProcessor reference for strum state access (getCurrentChord/getCurrentBaseNote/getCurrentKeyIdx) -- needed for STRUMMING slider mode
- ButtonHandler uses ModeManager.cycleSliderMode() replacing inline allowed_modes array cycling from ProcessButton
- Palette array passed as CRGBPalette16* to InputProcessor constructor since palette[] is currently a main.cpp global
- CalibrationService creates its own DataManager instance for calibration file I/O rather than taking a reference

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed LedManager inline constructor causing unique_ptr<Pattern> incomplete type error**
- **Found during:** Task 1 (build verification)
- **Issue:** LedManager.hpp had inline constructor `LedManager() {};` which triggered `std::unique_ptr<Pattern>` destructor instantiation with forward-declared `Pattern` type -- caused compile error in any TU including LedManager.hpp
- **Fix:** Moved constructor and destructor declarations to header, definitions (`= default`) to LedManager.cpp where Pattern is fully defined
- **Files modified:** src/Libs/Leds/LedManager.hpp, src/Libs/Leds/LedManager.cpp
- **Verification:** All service .o files compile successfully
- **Committed in:** 1665696 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Pre-existing incomplete type error blocked compilation of any new service including LedManager.hpp. Fix was minimal and necessary.

## Issues Encountered
- Pre-existing linker errors (Palettes.hpp multiple definitions, BLE-MIDI/TinyUSB duplicate symbols, esptoolpy packaging) prevent final binary linking. Source compilation (.cpp to .o) succeeds cleanly for all services. These are not caused by this plan's changes.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None - all services contain complete extracted logic from main.cpp.

## Next Phase Readiness
- All 4 service classes ready for AppEngine wiring in Plan 05
- main.cpp free functions still exist but are shadowed by service methods
- Plan 05 will create AppEngine to own all services and replace main.cpp orchestration
- Pre-existing linker errors need resolution before firmware can be flashed (out of scope)

---
*Phase: 02-firmware-service-extraction*
*Completed: 2026-04-03*

## Self-Check: PASSED

All files exist. All commits verified.
