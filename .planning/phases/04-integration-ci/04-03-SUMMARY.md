---
phase: 04-integration-ci
plan: 03
subsystem: firmware, protocol
tags: [sysex, bootloader, esp32-s3, rtc, webmidi]

requires:
  - phase: 01-protocol-data-foundation
    provides: SysEx protocol constants and handler framework
provides:
  - CMD_BOOTLOADER SysEx command (0x05) in firmware and web protocol
  - HandleBootloaderRequest firmware implementation with RTC register bootloader entry
  - requestBootloader web helper function
affects: [04-integration-ci, firmware-update-flow]

tech-stack:
  added: []
  patterns: [RTC register write for ESP32-S3 software bootloader entry]

key-files:
  created: []
  modified:
    - src/SysExProtocol.hpp
    - src/SysExHandler.hpp
    - src/SysExHandler.cpp
    - editor-tx/src/protocol/sysex.ts
    - test/test_sysex_protocol/test_main.cpp

key-decisions:
  - "100ms delay before esp_restart() to allow USB ACK flush"

patterns-established:
  - "Bootloader entry via RTC_CNTL_FORCE_DOWNLOAD_BOOT register write + esp_restart()"

requirements-completed: [FWFEAT-02]

duration: 2min
completed: 2026-04-04
---

# Phase 04 Plan 03: Bootloader SysEx Command Summary

**CMD_BOOTLOADER (0x05) added to firmware SysEx protocol with RTC register bootloader entry and web protocol mirror**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-04T00:01:50Z
- **Completed:** 2026-04-04T00:03:15Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added CMD_BOOTLOADER = 0x05 to firmware protocol constants
- Implemented HandleBootloaderRequest: sends ACK, waits 100ms for USB flush, writes RTC register for download mode, restarts ESP32
- Added BOOTLOADER: 0x05 to web protocol constants and requestBootloader() helper
- Added firmware unit test verifying CMD_BOOTLOADER constant value (12/12 tests pass)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add CMD_BOOTLOADER to firmware protocol and handler** - `bf70fcc` (feat)
2. **Task 2: Add BOOTLOADER to web protocol constants + requestBootloader function + firmware test** - `0e044cf` (feat)

## Files Created/Modified
- `src/SysExProtocol.hpp` - Added CMD_BOOTLOADER = 0x05 constant
- `src/SysExHandler.hpp` - Added HandleBootloaderRequest() declaration
- `src/SysExHandler.cpp` - Implemented bootloader entry with RTC register write, added esp_system.h and soc/rtc_cntl_reg.h includes, added CMD_BOOTLOADER case dispatch
- `editor-tx/src/protocol/sysex.ts` - Added BOOTLOADER: 0x05 to CMD, added requestBootloader() helper
- `test/test_sysex_protocol/test_main.cpp` - Added test_bootloader_command_constant, updated uniqueness/range tests

## Decisions Made
- 100ms delay between ACK and esp_restart() to ensure USB flushes the response before device disappears

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Updated test_cmd_values_in_range upper bound from 0x04 to 0x05**
- **Found during:** Task 2
- **Issue:** Existing range test capped at 0x04, would fail for CMD_BOOTLOADER = 0x05
- **Fix:** Updated all range assertions to use 0x05 upper bound and added CMD_BOOTLOADER to range check
- **Files modified:** test/test_sysex_protocol/test_main.cpp
- **Committed in:** 0e044cf (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Necessary correction to prevent test failure. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Bootloader SysEx command ready for integration with web firmware upload flow
- Web editor can now trigger bootloader mode via requestBootloader() before flashing

---
*Phase: 04-integration-ci*
*Completed: 2026-04-04*
