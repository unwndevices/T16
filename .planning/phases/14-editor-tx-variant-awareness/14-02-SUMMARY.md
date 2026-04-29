---
phase: 14-editor-tx-variant-awareness
plan: "14-02"
subsystem: protocol
tags: [sysex, capabilities, midi, react, handshake]
requires:
  - phase: 11-hardware-abstraction-layer
    provides: "CMD_CAPABILITIES = 0x07 + JSON capabilities response"
  - phase: 14-editor-tx-variant-awareness
    provides: "ConfigContext setVariant/setCapabilities (Plan 14-01)"
provides:
  - "CMD.CAPABILITIES = 0x07 in protocol/sysex.ts + requestCapabilities builder"
  - "isCapabilitiesResponse(cmd, sub) predicate + parseCapabilitiesPayload strict decoder"
  - "Capabilities handshake wired into ConfigProvider's shared SysEx handler (NOT ConnectionContext, due to provider order)"
affects: [14-04, 14-05]
tech-stack:
  added: []
  patterns: [strict-validation parser returning null on any malformed input]
key-files:
  created:
    - editor-tx/src/services/capabilitiesPayload.test.ts
  modified:
    - editor-tx/src/protocol/sysex.ts
    - editor-tx/src/services/midi.ts
    - editor-tx/src/contexts/ConfigContext.tsx
key-decisions:
  - "Capabilities request fires once after isVersionResponse arrives (Research §12 race-condition mitigation), tracked via capabilitiesRequestedRef"
  - "ConfigProvider owns the dispatch site instead of ConnectionContext — provider order in main.tsx is Toast > Connection > Config, and ConfigProvider already owns the shared SysEx receive path. ConnectionContext cannot useConfig() (parent context)"
  - "parseCapabilitiesPayload is strict: rejects unknown variant strings, missing capabilities object, wrong-typed fields, malformed JSON, truncated buffers — all return null"
  - "Two-attempt parse: first with [cmd, sub, status, ...json] reframed view, then with payload-as-direct-json fallback to handle either firmware emission shape"
patterns-established:
  - "Sysex predicate signature: (cmd: number, sub: number) => boolean — matches isConfigResponse / isVersionResponse"
requirements-completed: [EDITOR-01]
duration: ~20min
completed: 2026-04-30
---

# Plan 14-02: CMD_CAPABILITIES handshake wire-in

Editor-tx now requests CMD_CAPABILITIES (0x07) once after a successful version handshake and routes the response (variant + capabilities JSON) into ConfigContext. On parse failure, the connection stays alive — `console.warn` fires and isHandshakeConfirmed remains false (offline picker takes over).

## Highlights

- 7 vitest cases for parseCapabilitiesPayload (T16/T32 happy paths, malformed JSON, unknown variant, missing capabilities, wrong-type field, truncated buffer).
- Wired into ConfigProvider's existing handleSysexData callback alongside the version + config branches — no new SysEx listener.

## Deviation from plan

- **Dispatch site:** Plan 14-02 Task 4 specified ConnectionContext as the dispatch site. After confirming provider order (Toast > Connection > Config in main.tsx), moved to ConfigProvider — ConnectionContext cannot call useConfig(), and ConfigProvider already owns the shared SysEx receive path on both USB and BLE. Plan caveat documented this contingency.
