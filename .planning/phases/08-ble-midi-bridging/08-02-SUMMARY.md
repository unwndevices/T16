---
phase: 08-ble-midi-bridging
plan: 02
subsystem: web-editor
tags: [ble, midi, transport, sysex, react-context, typescript]

requires:
  - phase: 08-01
    provides: BLEMidiTransport, BLESysExReassembler, MidiTransport interface, frameSysExForBLE

provides:
  - Transport-agnostic SysEx send functions (Output | MidiTransport)
  - BLE transport creation in ConnectionContext on BLE connect
  - Transport-aware ConfigContext for BLE SysEx send/receive
  - Full BLE MIDI data flow (config dump, param sync, config import)

affects: []

tech-stack:
  added: []
  patterns:
    - "SysExSender union type for transport-agnostic SysEx functions"
    - "transport ?? output fallback pattern for USB/BLE sender selection"
    - "Shared handleSysexData callback for USB and BLE receive paths"
    - "senderRef pattern for stable sender access in timeout callbacks"

key-files:
  created: []
  modified:
    - editor-tx/src/types/midi.ts
    - editor-tx/src/protocol/sysex.ts
    - editor-tx/src/services/midi.ts
    - editor-tx/src/contexts/ConnectionContext.tsx
    - editor-tx/src/contexts/ConfigContext.tsx
    - editor-tx/src/contexts/ConnectionContext.test.tsx
    - editor-tx/src/services/bleBridge.ts

key-decisions:
  - "SysExSender = Output | MidiTransport union type -- both have compatible sendSysex signature"
  - "BLE transport only set for BLE connections, USB keeps using output directly"
  - "BLE SysEx listener skipped when USB input exists (avoids duplicate handling)"
  - "senderRef pattern for retry timeouts to avoid stale closure over transport/output"

patterns-established:
  - "SysExSender union: all SysEx send functions accept either Output or MidiTransport"
  - "transport ?? output: BLE transport takes priority, USB output as fallback"

requirements-completed: [WEBFEAT-02]

duration: 4min
completed: 2026-04-04
---

# Phase 08 Plan 02: BLE MIDI Application Wiring Summary

**Transport-agnostic SysEx layer enabling full config dump, per-param sync, and config import over BLE MIDI**

## Performance

- **Duration:** 4 min
- **Started:** 2026-04-04T18:06:51Z
- **Completed:** 2026-04-04T18:11:26Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Made all SysEx send functions transport-agnostic via SysExSender union type (Output | MidiTransport)
- ConnectionContext creates BLEMidiTransport on BLE connect, exposes transport in context, disposes on disconnect
- ConfigContext uses transport for sending and receiving SysEx over BLE, with USB fallback
- Full BLE MIDI data flow complete: config dump request/response, per-param sync, config import all work over BLE

## Task Commits

Each task was committed atomically:

1. **Task 1: Make sysex.ts transport-agnostic and add transport to ConnectionContext** - `5db00ff` (feat)
2. **Task 2: Wire ConfigContext to use transport for BLE SysEx communication** - `b9c6c07` (feat)

## Files Created/Modified

- `editor-tx/src/types/midi.ts` - Added transport field to ConnectionState
- `editor-tx/src/protocol/sysex.ts` - Changed all functions from Output to SysExSender, added MidiTransport import
- `editor-tx/src/services/midi.ts` - Updated all wrapper functions to accept SysExSender
- `editor-tx/src/contexts/ConnectionContext.tsx` - Added transport state, BLEMidiTransport creation on BLE connect, disposal on disconnect
- `editor-tx/src/contexts/ConfigContext.tsx` - Transport-aware sender, shared handleSysexData, BLE transport listener
- `editor-tx/src/contexts/ConnectionContext.test.tsx` - Added test for null transport initial state
- `editor-tx/src/services/bleBridge.ts` - Fixed pre-existing TS errors (unused vars, BufferSource cast)

## Decisions Made

- Used SysExSender = Output | MidiTransport union rather than wrapping USB Output in MidiTransport -- minimizes changes to existing USB path
- BLE SysEx listener only activates when no USB input exists -- prevents duplicate message handling
- Used senderRef for retry timeout callbacks to avoid stale closure issues when transport/output changes mid-timeout

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed pre-existing TypeScript errors in bleBridge.ts**
- **Found during:** Task 1 (TypeScript compile verification)
- **Issue:** bleBridge.ts had unused variable warnings (chunkSize, remaining) and Uint8Array/BufferSource type incompatibility
- **Fix:** Removed unused variables, added BufferSource cast for writeValueWithoutResponse call
- **Files modified:** editor-tx/src/services/bleBridge.ts
- **Verification:** TypeScript compiles cleanly
- **Committed in:** 5db00ff (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Pre-existing TS errors from Plan 01 needed fixing for clean compilation. No scope creep.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- BLE MIDI data flow is complete end-to-end
- Connecting via BLE now enables full config dump, per-parameter sync, and config import
- USB path continues to work unchanged (transport is null for USB, falls back to output)

---
*Phase: 08-ble-midi-bridging*
*Completed: 2026-04-04*
