---
phase: 01-protocol-data-foundation
plan: 03
subsystem: protocol
tags: [sysex, midi, command-handler, firmware, esp32]

requires:
  - phase: 01-01
    provides: SysEx protocol constants (SysExProtocol.hpp)
  - phase: 01-02
    provides: ConfigManager API for config access and per-parameter setters
provides:
  - SysExHandler class with validated command dispatch
  - Version handshake (protocol + firmware version)
  - Config dump/load via structured SysEx
  - Per-parameter updates dispatched by domain
  - ACK response framing
affects: [01-04, phase-2-firmware-extraction, phase-3-web-editor]

tech-stack:
  added: []
  patterns: [validated-dispatch, domain-based-param-addressing, ack-response-pattern]

key-files:
  created:
    - src/SysExHandler.hpp
    - src/SysExHandler.cpp
  modified: []

key-decisions:
  - "Byte layout: data[0]=F0, data[1]=manufacturer ID, data[2]=cmd, data[3]=sub -- added debug log to verify at runtime"
  - "CC domain uses 5-byte payload (domain+bank+ccIndex+channel+id) vs 4-byte for other domains"
  - "CalibrationReset left as TODO stub -- CalibrationManager extraction is Phase 2 (D-07)"

patterns-established:
  - "Validated dispatch: check length and manufacturer ID before processing any command"
  - "ACK response: MANUFACTURER_ID + cmd + SUB_ACK + status for bidirectional confirmation"
  - "Domain addressing: DOMAIN_GLOBAL/BANK_KB/BANK_CC for per-parameter routing"

requirements-completed: [PROTO-02, PROTO-03, PROTO-04, PROTO-05]

duration: 1min
completed: 2026-04-03
---

# Phase 01 Plan 03: SysEx Handler Summary

**Structured SysEx command handler replacing magic-byte dispatch with validated, domain-routed command processing for version handshake, config dump/load, and per-parameter updates**

## Performance

- **Duration:** 1 min
- **Started:** 2026-04-03T13:44:27Z
- **Completed:** 2026-04-03T13:45:54Z
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments
- SysExHandler validates manufacturer ID and minimum message length before processing (PROTO-04)
- Version handshake responds with protocol version and firmware version (PROTO-05)
- Config dump serializes full config and sends via MIDI; config load deserializes and applies to RAM (PROTO-03)
- Per-parameter updates dispatch to ConfigManager setters by domain (global, bank keyboard, bank CC) without flash writes (PROTO-02)
- All constants from SysEx:: namespace -- no magic bytes remain

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement SysExHandler with validated command dispatch** - `099a45a` (feat)

## Files Created/Modified
- `src/SysExHandler.hpp` - Class declaration with constructor taking ConfigManager& and MidiProvider&
- `src/SysExHandler.cpp` - Full implementation: ProcessSysEx dispatch, version handshake, config dump/load, per-param set, calibration reset stub, ACK responses

## Decisions Made
- Byte layout follows arduino_midi_library convention (data[0]=F0, data[1]=first byte after F0) with debug log for runtime verification
- CC domain requires 5-byte payload (extra byte for CC id) vs 4-byte for global and bank keyboard domains
- CalibrationReset is a logging stub with TODO referencing Phase 2 (D-07) for CalibrationManager extraction

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

- `src/SysExHandler.cpp` line ~177: `HandleCalibrationReset()` logs but does not call calibration.Delete() or ESP.restart(). Intentional -- CalibrationManager extraction is Phase 2 scope (D-07). Documented with TODO comment.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- SysExHandler ready to be wired into main.cpp (Plan 04 scope)
- All protocol commands (version, config, param, calibration) have handler implementations
- CalibrationReset stub will be completed when CalibrationManager is extracted in Phase 2

## Self-Check: PASSED

- src/SysExHandler.hpp: FOUND
- src/SysExHandler.cpp: FOUND
- 01-03-SUMMARY.md: FOUND
- Commit 099a45a: FOUND

---
*Phase: 01-protocol-data-foundation*
*Completed: 2026-04-03*
