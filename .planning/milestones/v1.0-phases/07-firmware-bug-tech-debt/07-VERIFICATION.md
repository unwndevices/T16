---
phase: 07-firmware-bug-tech-debt
verified: 2026-04-04T18:00:00Z
status: passed
score: 8/8 must-haves verified
---

# Phase 07: Firmware Bug Fix & Tech Debt Verification Report

**Phase Goal:** Verify/complete LedManager pattern leak fix, implement calibration/factory reset SysEx commands, clean up code duplication and dead code.
**Verified:** 2026-04-04T18:00:00Z
**Status:** passed
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | UpdateTransition does not overwrite currentPattern_ unconditionally -- uses else branch | VERIFIED | LedManager.cpp lines 259-269: else clause present, WaveTransition only created in else block |
| 2 | Calibration SysEx command deletes calibration file, sends ACK, and restarts device | VERIFIED | SysExHandler.cpp lines 207-223: LittleFS.remove("/calibration_data.json"), SendAck, delay(100), esp_restart() |
| 3 | Factory reset SysEx command deletes both config and calibration files, sends ACK, and restarts device | VERIFIED | SysExHandler.cpp lines 225-245: removes both /configuration_data.json and /calibration_data.json, SendAck, delay(100), esp_restart() |
| 4 | CMD_FACTORY_RESET constant (0x06) exists in SysExProtocol.hpp | VERIFIED | SysExProtocol.hpp line 14: `constexpr uint8_t CMD_FACTORY_RESET = 0x06;` |
| 5 | Calibration button sends CMD_CALIBRATION SysEx to device | VERIFIED | Dashboard.tsx line 368: `if (output) requestCalibration(output)` -- wired through midi.ts to sysex.ts requestCalibration |
| 6 | Factory reset button sends CMD_FACTORY_RESET SysEx to device | VERIFIED | Dashboard.tsx line 395: `if (output) requestFactoryReset(output)` -- wired through midi.ts to sysex.ts requestFactoryReset |
| 7 | Monitor.tsx imports getNoteNameWithOctave from scales.ts instead of defining locally | VERIFIED | Monitor.tsx line 6: `import { getNoteNameWithOctave } from '@/constants/scales'` -- no local NOTE_NAMES or function definition |
| 8 | SYNC_CONFIRMED type variant and reducer case are removed | VERIFIED | grep for SYNC_CONFIRMED in editor-tx/src/ returns zero matches |

**Score:** 8/8 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/Libs/Leds/LedManager.cpp` | Fixed UpdateTransition with else clause | VERIFIED | else clause at line 265, WaveTransition inside else block |
| `src/SysExProtocol.hpp` | CMD_FACTORY_RESET constant | VERIFIED | Line 14: 0x06 |
| `src/SysExHandler.hpp` | HandleFactoryReset declaration | VERIFIED | Line 27 |
| `src/SysExHandler.cpp` | Working HandleCalibrationReset and HandleFactoryReset implementations | VERIFIED | Both use LittleFS.remove, SendAck, delay(100), esp_restart() |
| `editor-tx/src/protocol/sysex.ts` | FACTORY_RESET constant and request functions | VERIFIED | CMD.FACTORY_RESET: 0x06, requestCalibration, requestFactoryReset exported |
| `editor-tx/src/services/midi.ts` | Service-layer wrappers | VERIFIED | requestCalibration and requestFactoryReset exported, delegate to sysex functions |
| `editor-tx/src/pages/Dashboard/Dashboard.tsx` | Wired button handlers | VERIFIED | Both buttons call service functions guarded by `if (output)` |
| `editor-tx/src/pages/Monitor/Monitor.tsx` | Import from scales.ts, no local duplication | VERIFIED | Import on line 6, no local NOTE_NAMES constant |
| `editor-tx/src/types/midi.ts` | ConfigAction without SYNC_CONFIRMED | VERIFIED | No SYNC_CONFIRMED in file |
| `editor-tx/src/contexts/ConfigContext.tsx` | Reducer without SYNC_CONFIRMED case | VERIFIED | No SYNC_CONFIRMED in file |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| SysExHandler.cpp | SysExProtocol.hpp | CMD_FACTORY_RESET constant | WIRED | case SysEx::CMD_FACTORY_RESET at line 83 dispatches to HandleFactoryReset |
| SysExHandler.cpp ProcessSysEx | HandleFactoryReset | switch case dispatch | WIRED | Lines 83-88: case dispatches to HandleFactoryReset() |
| Dashboard.tsx | services/midi.ts | requestCalibration/requestFactoryReset import | WIRED | Line 19 imports both, lines 368 and 395 call them |
| services/midi.ts | protocol/sysex.ts | sysex function import | WIRED | Lines 14-15 import aliased functions, lines 98-106 delegate to them |

### Data-Flow Trace (Level 4)

Not applicable -- firmware handlers are command receivers (no data rendering), and button handlers send outbound SysEx (no data display).

### Behavioral Spot-Checks

Step 7b: SKIPPED (firmware code requires device hardware; editor requires browser runtime with WebMIDI).

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| FWBUG-01 | 07-01-PLAN, 07-02-PLAN | LedManager UpdateTransition pattern leak bug | SATISFIED | else clause in LedManager.cpp prevents unconditional overwrite of currentPattern_ |

No REQUIREMENTS.md file exists in the project. Requirement FWBUG-01 was declared in both plan frontmatters and is satisfied by the UpdateTransition fix.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| None | - | - | - | No anti-patterns detected in modified files |

### Human Verification Required

### 1. LedManager Pattern Transition Visual Behavior

**Test:** Switch banks on the T16 device and observe LED transitions
**Expected:** WaveTransition plays when switching banks; when nextPattern_ is ready, it replaces currentPattern_ without a flash of WaveTransition
**Why human:** Requires physical hardware with LED matrix to observe visual behavior

### 2. Calibration Reset via Editor

**Test:** Connect T16 to web editor, navigate to Dashboard Settings tab, click "Calibrate" confirmation button
**Expected:** Device receives SysEx, deletes calibration data, restarts, and enters calibration routine on boot
**Why human:** Requires physical device connected via WebMIDI to verify end-to-end SysEx flow

### 3. Factory Reset via Editor

**Test:** Connect T16 to web editor, navigate to Dashboard Settings tab, click "Factory Reset" confirmation button
**Expected:** Device receives SysEx, deletes both config and calibration files, restarts with default configuration
**Why human:** Requires physical device connected via WebMIDI to verify end-to-end SysEx flow and data deletion

---

_Verified: 2026-04-04T18:00:00Z_
_Verifier: Claude (gsd-verifier)_
