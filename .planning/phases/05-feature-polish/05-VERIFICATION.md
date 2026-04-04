---
phase: 05-feature-polish
verified: 2026-04-04T02:55:00Z
status: gaps_found
score: 5/7 must-haves verified
re_verification: false
gaps:
  - truth: "Web app is installable as PWA with service worker caching static assets"
    status: failed
    reason: "vite-plugin-pwa and @types/web-bluetooth are declared in package.json but not installed in node_modules. TypeScript compilation fails (TS2688: Cannot find type definition files). Production build fails (ERR_MODULE_NOT_FOUND for vite-plugin-pwa). No service worker or manifest is generated."
    artifacts:
      - path: "editor-tx/vite.config.ts"
        issue: "VitePWA plugin import fails at build time -- package not installed"
      - path: "editor-tx/tsconfig.json"
        issue: "types array references vite-plugin-pwa/client and @types/web-bluetooth which are not in node_modules"
    missing:
      - "Run `cd editor-tx && npm install --legacy-peer-deps` to install vite-plugin-pwa and @types/web-bluetooth"
      - "Verify `npx tsc --noEmit` passes after install"
      - "Verify `npx vite build` produces dist/manifest.webmanifest and dist/sw.js"
  - truth: "BLE MIDI connection available via connectBLE() in ConnectionContext"
    status: partial
    reason: "connectBLE() is wired and calls the BLE service, but leaves input/output as null after connection. No MIDI messages can flow over BLE -- the characteristic notifications are not bridged to the app's MIDI message handling. The SUMMARY acknowledges this as intentional (SysEx chunking blocker), but the success criterion says 'can configure the device over BLE' which is not achieved."
    artifacts:
      - path: "editor-tx/src/contexts/ConnectionContext.tsx"
        issue: "connectBLE sets isConnected=true but input/output remain null -- no MIDI I/O over BLE"
    missing:
      - "Bridge BLE characteristic notifications to a synthetic Input-like object, or document BLE as non-functional for configuration until SysEx chunking is implemented"
human_verification:
  - test: "Visual note grid rendering"
    expected: "4x4 grid shows colored cells with note names and MIDI numbers, colors change per scale degree"
    why_human: "Visual rendering, HSL color accuracy, responsive layout"
  - test: "MIDI monitor real-time display"
    expected: "Connect T16 via USB, play keys -- Monitor page shows NoteOn/NoteOff/CC messages with colored bars"
    why_human: "Requires physical MIDI device and real-time interaction"
  - test: "PWA install prompt on mobile"
    expected: "After fixing npm install, visiting the deployed site on mobile Chrome shows install prompt"
    why_human: "Requires deployed build, mobile browser, and PWA install flow"
  - test: "Offline banner appearance"
    expected: "Disconnecting network shows yellow banner below nav"
    why_human: "Requires browser network toggle and visual inspection"
---

# Phase 05: Feature Polish Verification Report

**Phase Goal:** Differentiator features that make the configurator feel complete -- visual note mapping, mobile access, and real-time MIDI feedback
**Verified:** 2026-04-04T02:55:00Z
**Status:** gaps_found
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Note grid displays 16 cells in 4x4 layout showing MIDI note name and number for current bank | VERIFIED | NoteGrid.tsx renders 16 gridcell divs with noteName/noteNumber spans, calls computeNoteMap with bank params |
| 2 | Cells are colored by scale degree using HSL hue rotation (root at 270deg purple) | VERIFIED | NoteGrid.tsx line 52: `hue = ((degree * 360) / intervals.length + 270) % 360`, applied as inline HSLA |
| 3 | Grid updates reactively when bank/scale/root/octave/flip settings change | VERIFIED | NoteGrid reads `config.banks[selectedBank]` from useConfig -- React re-renders on any config change |
| 4 | computeNoteMap matches firmware SetNoteMap algorithm for all 19 scales | VERIFIED | scales.ts port matches firmware logic; 25 unit tests pass covering chromatic, ionian, pentatonic, flips |
| 5 | MIDI monitor page displays incoming CC, Note On, Note Off messages in real-time | VERIFIED | Monitor.tsx uses useMidiMonitor(input), renders MessageRow per message with type-colored borders and value bars |
| 6 | Web app is installable as PWA with service worker caching static assets | FAILED | vite-plugin-pwa not installed -- build fails with ERR_MODULE_NOT_FOUND |
| 7 | BLE MIDI connection available via connectBLE() in ConnectionContext | PARTIAL | Code wired but input/output remain null after BLE connect -- no MIDI data flows over BLE |

**Score:** 5/7 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `editor-tx/src/constants/scales.ts` | Scale intervals, computeNoteMap, getScaleDegree | VERIFIED | 138 lines, exports all required functions, 19 scale intervals matching firmware |
| `editor-tx/src/constants/scales.test.ts` | Unit tests for scale algorithms | VERIFIED | 25 tests, all passing |
| `editor-tx/src/components/NoteGrid/NoteGrid.tsx` | 4x4 CSS Grid note visualizer | VERIFIED | 75 lines, role=grid/gridcell, HSL coloring, aria-labels |
| `editor-tx/src/hooks/useMidiMonitor.ts` | MIDI message listener hook | VERIFIED | 74 lines, captures CC/NoteOn/NoteOff, 100-message cap, pause/clear |
| `editor-tx/src/hooks/useMidiMonitor.test.ts` | Hook unit tests | VERIFIED | 10 tests, all passing |
| `editor-tx/src/pages/Monitor/Monitor.tsx` | Monitor page component | VERIFIED | 150 lines, role=log, value bars with role=meter, empty state |
| `editor-tx/src/services/ble.ts` | BLE MIDI connection and parser | VERIFIED | 83 lines, parseBLEMidiPacket, connectBLE, GATT UUIDs |
| `editor-tx/src/services/ble.test.ts` | BLE parser tests | VERIFIED | Tests exist and pass |
| `editor-tx/vite.config.ts` | VitePWA plugin config | BROKEN | Code present but vite-plugin-pwa not installed -- import fails at runtime |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| NoteGrid.tsx | scales.ts | import computeNoteMap, getScaleDegree | WIRED | Line 2-6 imports all required functions |
| NoteGrid.tsx | useConfig.ts | useConfig() | WIRED | Line 1 import, line 11 destructures config + selectedBank |
| Dashboard.tsx | NoteGrid.tsx | `<NoteGrid />` rendered | WIRED | Line 21 import, line 96 rendered |
| Dashboard.tsx | scales.ts | import computeNoteMap | WIRED | Line 19 imports SCALES, NOTE_NAMES, getNoteNameWithOctave, computeNoteMap |
| Monitor.tsx | useMidiMonitor.ts | useMidiMonitor(input) | WIRED | Line 3 import, line 109 called |
| Monitor.tsx | useConnection.ts | useConnection() | WIRED | Line 2 import, line 108 destructures input |
| App.tsx | Monitor.tsx | route path 'monitor' | WIRED | Line 5 import, line 14 route entry |
| NavBar.tsx | /monitor | NavLink | WIRED | Line 59 NavLink to="/monitor" with MdEqualizer icon |
| vite.config.ts | vite-plugin-pwa | VitePWA plugin | NOT_WIRED | Import exists but package not installed in node_modules |
| ConnectionContext.tsx | ble.ts | connectBLE import | WIRED | Line 5 import, line 84 useCallback, line 131 in context value |
| NavBar.tsx | navigator.onLine | Offline banner | WIRED | Line 13 useState, online/offline event listeners, line 97 conditional render |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| NoteGrid.tsx | noteMap | computeNoteMap(bank params) | Yes -- algorithmic computation from config state | FLOWING |
| Monitor.tsx | messages | useMidiMonitor(input) | Yes -- WebMIDI event listeners produce MidiMessage objects | FLOWING |
| NavBar.tsx offline | isOffline | navigator.onLine + window events | Yes -- browser API | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| All tests pass | `npx vitest run` | 70/70 tests pass (7 test files) | PASS |
| TypeScript compiles | `npx tsc --noEmit` | TS2688: Cannot find type definition for vite-plugin-pwa/client and @types/web-bluetooth | FAIL |
| Production build | `npx vite build` | ERR_MODULE_NOT_FOUND for vite-plugin-pwa | FAIL |
| scales.ts exports | `npx vitest run src/constants/scales.test.ts` | 25/25 pass | PASS |
| useMidiMonitor tests | `npx vitest run src/hooks/useMidiMonitor.test.ts` | 10/10 pass | PASS |
| BLE parser tests | `npx vitest run src/services/ble.test.ts` | 7/7 pass | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| WEBFEAT-01 | 05-01 | Note grid visualizer showing 4x4 key mapping with scale degree colors | SATISFIED | NoteGrid component renders 4x4 grid with HSL degree coloring, firmware-matching algorithm |
| WEBFEAT-02 | 05-03 | PWA support -- service worker, manifest, offline capability, mobile BLE configuration | BLOCKED | Code written but vite-plugin-pwa not installed; BLE connects but cannot send/receive MIDI data |
| WEBFEAT-05 | 05-02 | Live MIDI monitor with CC value visualization | SATISFIED | Monitor page at /monitor with real-time message log, CC/velocity bars, pause/clear controls |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| editor-tx/tsconfig.json | 16 | Types array references packages not in node_modules | BLOCKER | TypeScript compilation fails entirely |
| editor-tx/vite.config.ts | 3 | Import of vite-plugin-pwa which is not installed | BLOCKER | Production build fails |
| editor-tx/src/contexts/ConnectionContext.tsx | 84-96 | connectBLE sets isConnected=true but leaves input/output null | WARNING | BLE appears connected but no MIDI data flows -- misleading UX |
| editor-tx/src/pages/Monitor/Monitor.tsx | 8-13 | getNoteNameWithOctave defined inline instead of importing from @/constants/scales | INFO | Duplicate of function in scales.ts; not a bug but unnecessary duplication |

### Human Verification Required

### 1. Visual Note Grid Rendering
**Test:** Open Dashboard, select different banks/scales, observe NoteGrid
**Expected:** 4x4 grid shows colored cells with correct note names, colors rotate by scale degree with purple at root
**Why human:** Visual rendering, HSL color accuracy, responsive layout

### 2. MIDI Monitor Real-Time Display
**Test:** Connect T16 via USB, play keys, open /monitor page
**Expected:** NoteOn/NoteOff/CC messages appear in real-time with colored borders and value bars
**Why human:** Requires physical MIDI device and real-time interaction

### 3. PWA Install Flow
**Test:** After fixing npm install, deploy and visit on mobile Chrome
**Expected:** Install prompt appears, app installs to home screen, works offline
**Why human:** Requires deployed build, mobile browser, PWA install flow

### 4. Offline Banner
**Test:** Toggle network off in browser DevTools
**Expected:** Yellow banner appears below nav with offline message
**Why human:** Requires browser network simulation and visual inspection

### Gaps Summary

Two gaps block full goal achievement:

1. **npm packages not installed (BLOCKER):** `vite-plugin-pwa` and `@types/web-bluetooth` are declared in package.json but absent from node_modules. This causes TypeScript compilation failure and production build failure. The fix is simply running `npm install --legacy-peer-deps`. All PWA code is correctly written -- this is a dependency installation gap, not a code gap.

2. **BLE MIDI bridging incomplete (PARTIAL):** The `connectBLE()` method in ConnectionContext calls the BLE service and sets `isConnected=true`, but does not create Input/Output objects. No MIDI messages can flow over BLE. The SUMMARY acknowledges this as intentional due to a SysEx chunking blocker, but the phase success criterion states the app "can configure the device over BLE" which is not yet achievable. This may be acceptable as a known limitation if documented.

Both gaps share a root cause related to Plan 03 (PWA/BLE). Plans 01 and 02 are fully verified.

---

_Verified: 2026-04-04T02:55:00Z_
_Verifier: Claude (gsd-verifier)_
