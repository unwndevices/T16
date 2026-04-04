# Phase 7: Firmware Bug Confirmation & Tech Debt - Research

**Researched:** 2026-04-04
**Domain:** ESP32 C++ firmware / React TypeScript web editor -- bug confirmation, SysEx commands, dead code cleanup
**Confidence:** HIGH

## Summary

This phase addresses four distinct items from the v1.0 milestone audit: (1) confirming the LedManager pattern memory leak fix from Phase 2, (2) implementing calibration and factory reset SysEx commands that are currently stubs/no-ops, (3) removing the duplicated `getNoteNameWithOctave` function in Monitor.tsx, and (4) removing the `SYNC_CONFIRMED` dead code from the config reducer.

Code inspection confirms the unique_ptr pattern is correctly applied in LedManager (FWBUG-01 fix is complete). However, the `UpdateTransition()` method has a logic bug -- it unconditionally overwrites `currentPattern_` after potentially moving `nextPattern_` into it, which leaks the intended pattern. The calibration command exists in the SysEx protocol (`CMD_CALIBRATION = 0x04`) on both sides, but the firmware handler is a logging stub and the web button handler has a `// Send calibration command` comment with no actual send. Factory reset has no protocol command defined at all -- needs a new `CMD_FACTORY_RESET = 0x06` on both sides. The web tech debt items (duplication and dead code) are straightforward.

**Primary recommendation:** Fix UpdateTransition logic bug, wire calibration SysEx end-to-end, add factory reset SysEx command, then clean up web dead code/duplication.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None -- discuss phase was skipped per user request.

### Claude's Discretion
All implementation choices are at Claude's discretion. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

### Deferred Ideas (OUT OF SCOPE)
None.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FWBUG-01 | LedManager memory leak fix (unique_ptr pattern transitions) | Code inspection confirms unique_ptr is applied correctly. Pattern base class has virtual destructor. SetPattern/TransitionToPattern use make_unique. However, UpdateTransition has a logic bug that needs fixing. |
</phase_requirements>

## Architecture Patterns

### Current LedManager Pattern Ownership (Post Phase 2)

```
LedManager
  currentPattern_ : std::unique_ptr<Pattern>
  nextPattern_    : std::unique_ptr<Pattern>

SetPattern(p)            -> currentPattern_ = move(p)          // OK
TransitionToPattern(p)   -> nextPattern_ = move(p)             // OK
                            currentPattern_ = make_unique<WaveTransition>()

RunPattern()             -> if !currentPattern_->RunPattern()  // transition complete
                              currentPattern_ = move(nextPattern_)

UpdateTransition()       -> BUG: unconditionally overwrites currentPattern_
```

### UpdateTransition Logic Bug

```cpp
void LedManager::UpdateTransition()
{
    if (nextPattern_ && !nextPattern_->isTransition)
    {
        currentPattern_ = std::move(nextPattern_);  // moves pattern in
    }
    currentPattern_ = std::make_unique<WaveTransition>(Direction::DOWN);  // immediately overwrites!
}
```

This creates a new WaveTransition every call, discarding whatever was just moved. The intent appears to be an `else` clause. The fix should be:

```cpp
void LedManager::UpdateTransition()
{
    if (nextPattern_ && !nextPattern_->isTransition)
    {
        currentPattern_ = std::move(nextPattern_);
    }
    else
    {
        currentPattern_ = std::make_unique<WaveTransition>(Direction::DOWN);
    }
}
```

### SysEx Command Protocol Extension

Current firmware SysEx commands (from SysExProtocol.hpp):
- `CMD_VERSION = 0x01` -- version handshake
- `CMD_CONFIG = 0x02` -- full config dump/load
- `CMD_PARAM = 0x03` -- per-parameter update
- `CMD_CALIBRATION = 0x04` -- exists but handler is a stub
- `CMD_BOOTLOADER = 0x05` -- bootloader entry

Missing:
- `CMD_FACTORY_RESET = 0x06` -- not yet defined

### Calibration SysEx Flow

```
[Web Editor]                         [Firmware]
     |                                    |
     |-- CMD_CALIBRATION + SUB_REQUEST -->|
     |                                    | HandleCalibrationReset()
     |                                    |   Delete /calibration_data.json
     |                                    |   Send ACK
     |<-- CMD_CALIBRATION + SUB_ACK ------|
     |                                    |   esp_restart()
     |                                    |
     | (device restarts, runs calibration routine on boot when no cal data found)
```

### Factory Reset SysEx Flow

```
[Web Editor]                         [Firmware]
     |                                    |
     |-- CMD_FACTORY_RESET + SUB_REQ ---->|
     |                                    | HandleFactoryReset()
     |                                    |   Delete /configuration_data.json
     |                                    |   Delete /calibration_data.json
     |                                    |   Send ACK
     |<-- CMD_FACTORY_RESET + SUB_ACK ----|
     |                                    |   esp_restart()
     |                                    |
     | (device restarts with defaults)
```

### Anti-Patterns to Avoid
- **Missing ACK before restart:** Always send ACK and flush USB before `esp_restart()` (100ms delay, matching bootloader pattern)
- **Raw file deletion without LittleFS check:** Verify file exists before removing -- `LittleFS.exists()` then `LittleFS.remove()`

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| File deletion on LittleFS | Custom flash erase logic | `LittleFS.remove()` | Standard ESP32 API, handles wear leveling |
| SysEx framing | Manual byte array construction | Existing `SendSysEx`/`sendSysex` wrappers | Already proven in 5 other commands |

## Common Pitfalls

### Pitfall 1: ESP Restart Without USB Flush
**What goes wrong:** ACK response never reaches the web editor because `esp_restart()` kills USB before the packet sends.
**Why it happens:** USB is asynchronous -- queuing data != sending data.
**How to avoid:** Follow the `HandleBootloaderRequest` pattern: send ACK, `delay(100)`, then restart.
**Warning signs:** Web editor shows "command timed out" after device restart.

### Pitfall 2: UpdateTransition Called Without nextPattern_
**What goes wrong:** If `UpdateTransition()` is called when `nextPattern_` is null, the current `else` path (after fix) creates a WaveTransition. This is the existing behavior and appears intentional -- callers (InputProcessor::onBankChange, SliderProcessor quick settings) use it to trigger a visual transition on bank/page switch.
**Why it matters:** The fix must preserve this behavior or the bank-switch visual transition breaks.

### Pitfall 3: Calibration File Path Mismatch
**What goes wrong:** Deleting wrong file path doesn't trigger calibration on reboot.
**Why it happens:** Hard-coded paths in multiple places.
**How to avoid:** Use the same path constant. CalibrationService and DataManager both reference `/calibration_data.json`. ConfigManager uses `/configuration_data.json`.

### Pitfall 4: SYNC_CONFIRMED Removal Breaking Types
**What goes wrong:** Removing from the reducer without removing from the union type causes type error.
**How to avoid:** Remove from both `ConfigAction` union type in `src/types/midi.ts` (line 41) AND the reducer case in `src/contexts/ConfigContext.tsx` (line 160).

## Code Examples

### Current Calibration Stub (firmware)
```cpp
// src/SysExHandler.cpp line 199
void SysExHandler::HandleCalibrationReset()
{
    log_d("Calibration reset requested");
    // TODO: Wire to CalibrationManager when separated (D-07, Phase 2)
}
```

### Current Web No-ops (Dashboard.tsx)
```tsx
// Lines 363-367 -- calibration button
onClick={() => {
  // Send calibration command
  setCalibrationOpen(false)
}}

// Lines 390-394 -- factory reset button
onClick={() => {
  // Send factory reset command
  setResetOpen(false)
}}
```

### Web Duplication (Monitor.tsx)
```tsx
// Line 8 -- local NOTE_NAMES (duplicate of scales.ts)
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

// Lines 10-14 -- local getNoteNameWithOctave (duplicate of scales.ts)
function getNoteNameWithOctave(midiNote: number): string {
  const name = NOTE_NAMES[midiNote % 12]
  const octave = Math.floor(midiNote / 12) - 1
  return `${name}${octave}`
}
```

Fix: import from `@/constants/scales` and delete local copies.

### SYNC_CONFIRMED Dead Code
```typescript
// src/types/midi.ts line 41
| { type: 'SYNC_CONFIRMED' }

// src/contexts/ConfigContext.tsx lines 160-161
case 'SYNC_CONFIRMED':
  return { ...state, deviceConfig: structuredClone(state.config) }
```

Fix: remove both the type variant and the reducer case.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework (firmware) | Unity (PlatformIO native env) |
| Framework (web) | Vitest |
| Config file (firmware) | `platformio.ini` [env:native] |
| Config file (web) | `editor-tx/vitest.config.ts` |
| Quick run (firmware) | `pio test -e native` |
| Quick run (web) | `cd editor-tx && npx vitest run` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FWBUG-01 | LedManager unique_ptr transitions don't leak | manual | Visual inspection of code + existing firmware build confirms no raw new/delete | N/A -- verified by code review |
| N/A | Calibration SysEx command triggers restart | manual-only | Requires physical device with FSR keys | N/A |
| N/A | Factory reset SysEx command resets config | manual-only | Requires physical device | N/A |
| N/A | getNoteNameWithOctave import works | unit | `cd editor-tx && npx vitest run src/constants/scales.test.ts` | Exists |
| N/A | SYNC_CONFIRMED removal doesn't break types | unit | `cd editor-tx && npx tsc --noEmit` | N/A (typecheck) |
| N/A | SysEx protocol constants match | unit | `pio test -e native -f test_sysex_protocol` | Exists |
| N/A | Web calibration/reset send correct SysEx | unit | `cd editor-tx && npx vitest run src/services/midi.test.ts` | Exists |

### Sampling Rate
- **Per task commit:** `cd editor-tx && npx tsc --noEmit && npx vitest run` (web) / `pio test -e native` (firmware)
- **Per wave merge:** Full CI equivalent
- **Phase gate:** All tests green before verify

### Wave 0 Gaps
- None -- existing test infrastructure covers all phase requirements. New SysEx functions follow existing patterns with existing test files.

## Open Questions

1. **Should CalibrationService be extended or keep it in SysExHandler?**
   - What we know: CalibrationService handles the interactive calibration routine. HandleCalibrationReset just needs to delete the calibration file and restart.
   - What's unclear: Whether the handler should call CalibrationService or just use DataManager/LittleFS directly.
   - Recommendation: Keep it simple -- delete the file directly in SysExHandler using `LittleFS.remove()`. CalibrationService manages the interactive routine that runs on boot, which is a different concern.

2. **Should factory reset also trigger recalibration?**
   - What we know: Factory reset deletes both config and calibration data. On boot, missing calibration data triggers the calibration routine.
   - What's unclear: Whether users want to keep their calibration data during a factory reset (config-only reset).
   - Recommendation: Delete both files (true factory reset). If users want config-only reset, that could be a separate command in the future, but the button is labeled "Factory Reset" which implies full reset.

## Sources

### Primary (HIGH confidence)
- Direct code inspection: `src/Libs/Leds/LedManager.hpp`, `src/Libs/Leds/LedManager.cpp` -- unique_ptr pattern ownership verified
- Direct code inspection: `src/SysExHandler.cpp` -- calibration stub confirmed, factory reset absent
- Direct code inspection: `src/SysExProtocol.hpp` -- command constants, no CMD_FACTORY_RESET
- Direct code inspection: `editor-tx/src/pages/Dashboard/Dashboard.tsx` -- calibration/reset button no-ops confirmed
- Direct code inspection: `editor-tx/src/pages/Monitor/Monitor.tsx` -- getNoteNameWithOctave duplication confirmed
- Direct code inspection: `editor-tx/src/contexts/ConfigContext.tsx` + `src/types/midi.ts` -- SYNC_CONFIRMED dead code confirmed
- Phase 2 Plan 3 summary (`02-03-SUMMARY.md`) -- FWBUG-01 fix confirmed, requirements-completed includes FWBUG-01

### Secondary (MEDIUM confidence)
- v1.0 Milestone Audit (`v1.0-MILESTONE-AUDIT.md`) -- tech debt items enumerated

## Metadata

**Confidence breakdown:**
- FWBUG-01 status: HIGH -- code inspection confirms unique_ptr applied, virtual destructor present, Phase 2 summary confirms fix
- UpdateTransition bug: HIGH -- logic error visible in code, unconditional overwrite after conditional move
- Calibration/reset stubs: HIGH -- confirmed stub in firmware, no-ops in web
- Web tech debt: HIGH -- exact lines identified for duplication and dead code

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable -- firmware code changes infrequently)
