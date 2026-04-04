---
phase: 07-firmware-bug-tech-debt
plan: 01
subsystem: firmware
tags: [led-patterns, sysex, littlefs, factory-reset, calibration]

# Dependency graph
requires:
  - phase: 01-firmware-foundation
    provides: SysExHandler, SysExProtocol, ConfigManager, LedManager.cpp extraction
provides:
  - Fixed UpdateTransition pattern leak bug (FWBUG-01)
  - Working calibration reset SysEx handler
  - Working factory reset SysEx handler with CMD_FACTORY_RESET (0x06) protocol constant
affects: [editor-tx calibration/reset UI buttons]

# Tech tracking
tech-stack:
  added: []
  patterns: [ACK-delay-restart pattern for destructive SysEx commands]

key-files:
  created: []
  modified:
    - src/Libs/Leds/LedManager.cpp
    - src/SysExProtocol.hpp
    - src/SysExHandler.hpp
    - src/SysExHandler.cpp

key-decisions:
  - "Follow HandleBootloaderRequest pattern (ACK, delay, restart) for calibration and factory reset"

patterns-established:
  - "Destructive SysEx commands: send ACK, delay(100) for USB flush, then esp_restart()"

requirements-completed: [FWBUG-01]

# Metrics
duration: 2min
completed: 2026-04-04
---

# Phase 07 Plan 01: Firmware Bug Fix & SysEx Reset Handlers Summary

**Fixed UpdateTransition pattern leak bug and implemented calibration/factory reset SysEx command handlers with LittleFS file deletion and device restart**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-04T17:38:41Z
- **Completed:** 2026-04-04T17:40:30Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Fixed FWBUG-01: UpdateTransition no longer unconditionally overwrites currentPattern_ -- uses else clause to only create WaveTransition when nextPattern_ is not ready
- Implemented HandleCalibrationReset: deletes calibration_data.json, sends ACK, restarts device
- Implemented HandleFactoryReset: deletes both configuration_data.json and calibration_data.json, sends ACK, restarts device
- Added CMD_FACTORY_RESET (0x06) protocol constant and ProcessSysEx switch dispatch

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix LedManager UpdateTransition logic bug (FWBUG-01)** - `624732f` (fix)
2. **Task 2: Implement calibration and factory reset SysEx handlers** - `431d346` (feat)

## Files Created/Modified
- `src/Libs/Leds/LedManager.cpp` - Fixed UpdateTransition else clause
- `src/SysExProtocol.hpp` - Added CMD_FACTORY_RESET = 0x06 constant
- `src/SysExHandler.hpp` - Added HandleFactoryReset declaration
- `src/SysExHandler.cpp` - Implemented HandleCalibrationReset and HandleFactoryReset, added LittleFS include and factory reset switch case

## Decisions Made
- Followed HandleBootloaderRequest pattern (ACK -> delay(100) -> esp_restart) for both calibration and factory reset handlers -- consistent with existing codebase convention

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Calibration and factory reset commands are now functional in firmware -- editor buttons can be wired to send these SysEx commands
- FWBUG-01 resolved -- pattern transitions work correctly

---
*Phase: 07-firmware-bug-tech-debt*
*Completed: 2026-04-04*
