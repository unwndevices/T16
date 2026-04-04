---
phase: 04-integration-ci
plan: 04
subsystem: ui
tags: [esptool-js, web-serial, bootloader, sysex, performance, react-hooks]

requires:
  - phase: 04-03
    provides: "requestBootloader SysEx helper and CMD.BOOTLOADER constant"
  - phase: 03-web-editor
    provides: "TypeScript web editor with ConfigContext, ConnectionContext, and Upload page"
provides:
  - "useBootloader hook with auto-bootloader state machine"
  - "Upload page with one-click firmware update flow"
  - "Per-parameter sync round-trip timing via performance.now()"
affects: [phase-05, firmware-update-ux]

tech-stack:
  added: []
  patterns: ["auto-bootloader state machine hook", "performance.now() round-trip measurement with retry"]

key-files:
  created:
    - editor-tx/src/hooks/useBootloader.ts
  modified:
    - editor-tx/src/pages/Upload/Upload.tsx
    - editor-tx/src/pages/Upload/Upload.module.css
    - editor-tx/src/contexts/ConfigContext.tsx

key-decisions:
  - "ESP32-S3 VID 0x303A / PID 0x1001 used for auto-detecting bootloader serial port"
  - "500ms ACK timeout with single retry before marking param sync as failed"

patterns-established:
  - "Auto-bootloader pattern: SysEx command -> wait disconnect -> Web Serial connect -> flash"
  - "Round-trip timing pattern: pendingParamTimestamps ref map with performance.now() deltas"

requirements-completed: [WEBFEAT-03]

duration: 3min
completed: 2026-04-04
---

# Phase 04 Plan 04: Upload Auto-Bootloader and Sync Timing Summary

**One-click firmware update via SysEx bootloader command with Web Serial auto-connect, plus per-parameter sync round-trip measurement logged to console**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-04T00:10:00Z
- **Completed:** 2026-04-04T00:13:05Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments
- useBootloader hook implements full state machine: idle -> entering_bootloader -> bootloader_ready -> uploading -> success/error
- Upload page rewritten with auto-bootloader flow, manual BOOT button fallback link, version display, and downgrade warning
- ConfigContext measures per-parameter sync round-trip via performance.now() with console.debug logging
- 500ms ACK timeout with single retry prevents silent sync failures

## Task Commits

Each task was committed atomically:

1. **Task 1: Create useBootloader hook and rewrite Upload page** - `4447fec` (feat)
2. **Task 2: Add per-parameter sync round-trip timing to ConfigContext** - `604175b` (feat)
3. **Task 3: Verify Upload page auto-bootloader UX and sync timing** - checkpoint (human-verify, approved)

## Files Created/Modified
- `editor-tx/src/hooks/useBootloader.ts` - Auto-bootloader state machine hook with ESPLoader integration
- `editor-tx/src/pages/Upload/Upload.tsx` - Upload page with one-click firmware update flow and manual fallback
- `editor-tx/src/pages/Upload/Upload.module.css` - Styles for fallback link and version info
- `editor-tx/src/contexts/ConfigContext.tsx` - Per-parameter sync round-trip timing with retry logic

## Decisions Made
- ESP32-S3 USB Serial/JTAG VID 0x303A / PID 0x1001 used for auto-detecting the bootloader serial port after device reset
- 500ms ACK timeout with single retry balances responsiveness with reliability for param sync measurement

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Auto-bootloader firmware update flow complete, ready for end-to-end testing in Phase 5
- Per-parameter sync timing available in dev console for performance validation
- All Phase 04 plans now complete

## Self-Check: PASSED

All files verified present. All commit hashes found in git history.

---
*Phase: 04-integration-ci*
*Completed: 2026-04-04*
