---
phase: 05-feature-polish
plan: 03
subsystem: pwa
tags: [pwa, ble-midi, service-worker, web-bluetooth, offline]

# Dependency graph
requires:
  - phase: 03-web-editor-rewrite
    provides: TypeScript codebase, ConnectionContext, NavBar, design system tokens
provides:
  - PWA support with service worker and web app manifest
  - BLE MIDI service with packet parser and connection logic
  - Offline indicator banner in NavBar
  - PWA icon placeholder assets (192x192, 512x512)
affects: [future BLE SysEx chunking, mobile deployment]

# Tech tracking
tech-stack:
  added: [vite-plugin-pwa v1.2.0, @types/web-bluetooth]
  patterns: [BLE MIDI packet parsing, PWA autoUpdate registration, offline detection via navigator.onLine]

key-files:
  created:
    - editor-tx/src/services/ble.ts
    - editor-tx/src/services/ble.test.ts
    - editor-tx/public/icon-192.png
    - editor-tx/public/icon-512.png
  modified:
    - editor-tx/vite.config.ts
    - editor-tx/tsconfig.json
    - editor-tx/src/contexts/ConnectionContext.tsx
    - editor-tx/src/types/midi.ts
    - editor-tx/src/components/NavBar/NavBar.tsx
    - editor-tx/src/components/NavBar/NavBar.module.css
    - editor-tx/package.json

key-decisions:
  - "Used --legacy-peer-deps for vite-plugin-pwa (Vite 8 not yet in peer dep list)"
  - "Solid-color placeholder PNGs for PWA icons -- proper branded icons can replace later"
  - "BLE connectBLE() sets isConnected but leaves input/output null -- full MIDI bridging deferred"

patterns-established:
  - "BLE MIDI packet parsing: strip timestamp bytes to extract raw MIDI messages"
  - "Offline detection: useState + useEffect with online/offline window events"

requirements-completed: [WEBFEAT-02]

# Metrics
duration: 5min
completed: 2026-04-04
---

# Phase 05 Plan 03: PWA & BLE MIDI Summary

**PWA installable app with workbox service worker, BLE MIDI connection service with tested packet parser, and offline indicator banner**

## Tasks Completed

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 (TDD) | Install vite-plugin-pwa, BLE service + tests, PWA icons | 4037dfa (RED), 570b326 (GREEN) | ble.ts, ble.test.ts, icon-192.png, icon-512.png |
| 2 | PWA manifest config, BLE in ConnectionContext, offline banner | ee0e9dc | vite.config.ts, ConnectionContext.tsx, NavBar.tsx |

## Implementation Details

### BLE MIDI Service (ble.ts)
- Exports `parseBLEMidiPacket(data: DataView)` that strips BLE MIDI timestamp framing to extract raw MIDI messages
- Exports `connectBLE()` that uses Web Bluetooth API to connect to "Topo T16" BLE device
- Standard GATT UUIDs: service `03b80e5a-ede8-4b33-a751-6ce34ec4c700`, characteristic `7772e5db-3868-4112-a1a9-f2669d106bf3`
- 7 unit tests covering single messages, multi-message packets, empty packets, and various MIDI types

### PWA Configuration
- VitePWA plugin with `registerType: 'autoUpdate'` -- service worker auto-updates on new deployments
- Manifest: "T16 Configurator", theme_color #6246ea, standalone display, 192/512 PNG icons
- Workbox precaches all static assets (js, css, html, png, svg, woff2)
- Build produces `dist/manifest.webmanifest`, `dist/sw.js`, `dist/workbox-*.js`

### ConnectionContext BLE Integration
- Added `connectBLE()` method alongside existing USB `connect()`
- Handles GATT disconnection events to reset connected state
- Note: BLE input/output remain null -- full MIDI message bridging over BLE is a future task (BLE SysEx chunking blocker in STATE.md)

### Offline Banner
- NavBar detects offline state via `navigator.onLine` + window event listeners
- Yellow warning banner below nav: "You're offline. Device communication requires a connection."
- Uses `role="status"` and `aria-live="polite"` for accessibility

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added @types/web-bluetooth to tsconfig.json types**
- Found during: Task 2 TypeScript check
- Issue: Web Bluetooth API types (BluetoothDevice, BluetoothRemoteGATTCharacteristic) not recognized
- Fix: Added `@types/web-bluetooth` to tsconfig.json types array (package was already installed as transitive dep)
- Files modified: editor-tx/tsconfig.json

## Verification Results

- TypeScript: `npx tsc --noEmit` passes with zero errors
- Tests: 35/35 pass (5 test files including new ble.test.ts)
- Build: `npx vite build` succeeds, produces manifest.webmanifest with correct T16 branding
- Service worker: sw.js generated with 51 precache entries

## Known Stubs

- **BLE MIDI message bridging**: `connectBLE()` in ConnectionContext sets `isConnected=true` but does not wire BLE characteristic notifications to the app's MIDI message handling. Full BLE MIDI I/O requires SysEx chunking implementation (STATE.md blocker). This is intentional -- per-parameter updates work within BLE MTU, full config dumps need fragmentation support.
- **PWA icons**: Solid purple (#6246ea) placeholder PNGs. Should be replaced with proper T16 branded icons.
