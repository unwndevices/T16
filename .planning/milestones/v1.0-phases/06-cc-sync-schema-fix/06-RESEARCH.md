# Phase 6: CC Sync & Schema Fix - Research

**Researched:** 2026-04-04
**Domain:** SysEx per-parameter sync protocol (CC domain) + JSON schema alignment
**Confidence:** HIGH

## Summary

Phase 6 closes two concrete bugs identified in the v1.0 milestone audit: (1) CC per-parameter sync payload mismatch where the web editor sends 4 bytes but firmware expects 5, causing all CC mapping edits to be silently dropped, and (2) a missing `pal` (palette) field in the JSON schema that causes config export-reimport to fail when the config originated from a device dump.

Both bugs are fully understood from code review. The CC payload issue is a design mismatch in `sendParamUpdate` -- it accepts a single `value` parameter, but the CC domain needs two values (channel AND id) per CC slot. The firmware's `SetCCParam(bank, ccIndex, channel, id)` correctly expects both. The schema issue is straightforward: firmware serializes `"pal"` per bank in `PopulateDocFromStructs`, but the schema has `additionalProperties: false` without listing `pal`, so ajv validation rejects device-originated configs.

**Primary recommendation:** Add a dedicated `sendCCParamUpdate` function to the web protocol layer that sends 5 payload bytes (domain, bank, ccIndex, channel, id), update the Dashboard CC handlers to call it, add `pal` to schema + TypeScript types, and update the ConfigContext reducer to handle CC updates with the new field mapping.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None -- discuss phase was skipped per user request.

### Claude's Discretion
All implementation choices are at Claude's discretion -- discuss phase was skipped per user request. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

### Deferred Ideas (OUT OF SCOPE)
None -- discuss phase skipped.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PROTO-02 | Editor can send per-parameter updates via SysEx (single value change, <100ms round-trip) | CC domain payload mismatch fully diagnosed: web sends 4 bytes, firmware expects 5. Fix requires dedicated CC send function + firmware CC field ID constants. |
| WEBFEAT-04 | Config backup import validates against schema before applying | Schema missing `pal` field causes ajv rejection of device-originated configs. Fix requires schema + type + validator updates. |
</phase_requirements>

## Architecture Patterns

### Bug 1: CC Payload Mismatch (PROTO-02)

**Root cause:** The web `sendParamUpdate(output, domain, bank, field, value)` signature assumes all domains use the same 4-byte payload format: `[domain, bankIndex, fieldId, value]`. But the firmware CC domain needs 5 bytes: `[domain, bankIndex, ccIndex, channel, id]` because `SetCCParam` takes both channel and id as separate parameters.

**Current web code flow (broken):**
```
Dashboard.CcMappingTab.handleChannelChange(ccIndex, channel)
  -> updateParam(DOMAIN.BANK_CC, selectedBank, ccIndex * 2, channel)
    -> midiSendParamUpdate(output, DOMAIN.BANK_CC, bank, ccIndex*2, channel)
      -> output.sendSysex(MANUFACTURER_ID, [CMD.PARAM, SUB.REQUEST, 0x02, bank, ccIndex*2, channel])
```

The web sends: `[domain=0x02, bank, ccIndex*2, channel]` = 4 payload bytes.
Firmware expects: `[domain=0x02, bank, ccIndex, channel, id]` = 5 payload bytes.

The firmware explicitly checks `if (payloadLen < 5)` for CC domain and rejects shorter payloads (SysExHandler.cpp line 177-181).

**Additional problem:** The web uses `ccIndex * 2` / `ccIndex * 2 + 1` field encoding to distinguish channel vs id updates, but the firmware expects the raw ccIndex plus BOTH channel and id in one message. The firmware always sets both values together via `SetCCParam(bank, ccIndex, channel, id)`.

**Fix approach:** The firmware design is correct (atomic CC update of channel+id pair). The web needs to match it:

1. Add `sendCCParamUpdate(output, bank, ccIndex, channel, id)` to `sysex.ts` that sends 5 payload bytes
2. Add corresponding wrapper in `midi.ts` service layer
3. Change `ConfigContextValue.updateParam` signature or add a dedicated `updateCCParam(bank, ccIndex, channel, id)` method
4. Update `CcMappingTab` handlers to always send both channel and id together
5. Update the `configReducer` to handle CC updates on the `chs` and `ids` arrays directly

**Firmware CC constants needed in web protocol:** Currently `SysExProtocol.hpp` has no `FIELD_CC_*` constants because CC uses a different addressing scheme (ccIndex + channel + id). The web just needs to know the CC_AMT (8) for validation.

### Bug 2: Schema Missing 'pal' Field (WEBFEAT-04)

**Root cause:** Firmware `PopulateDocFromStructs` serializes `bankObj["pal"] = banks_[i].palette` (ConfigManager.cpp line 442). The JSON schema's bank item definition has `additionalProperties: false` but does NOT list `pal` in properties. When a device config dump is exported and reimported, ajv rejects it because `pal` is an unexpected property.

**Firmware struct:** `KeyModeData` has `uint8_t palette = 0` (Configuration.hpp line 19). It's a valid, used field (LED color palette per bank).

**Fix approach (schema-first):**
1. Add `"pal": { "type": "integer", "minimum": 0, "maximum": 7 }` to schema bank item properties
2. Add `"pal"` to the bank `required` array
3. Re-run `schema/generate-types.sh` to regenerate `config.ts` with `pal` field
4. Update `DEFAULT_BANK` in ConfigContext.tsx to include `pal: 0` (matching firmware default)
5. Update `DEFAULT_CONFIG` banks to include `pal` per bank
6. Update `configValidator.test.ts` test fixtures to include `pal`
7. Update `midi.test.ts` fixtures if they contain bank objects

**Palette range:** Looking at `KeyModeData`, palette defaults to 0. The firmware has palette indices for LED patterns. Looking at the bank index default `banks_[i].palette = bankObj["pal"] | (uint8_t)i`, each bank defaults to its own index (0-3). Maximum should be verified but 0-7 is a safe range based on the LED pattern system having up to 8 palettes.

### ConfigContext Reducer Fix

The current `UPDATE_PARAM` action in the reducer handles `DOMAIN.BANK_CC` by using `BANK_FIELD_MAP[field]` -- but CC fields aren't in that map. The reducer silently does nothing for CC updates. This needs a new action type or extended handling:

```typescript
// New action for CC-specific updates
| { type: 'UPDATE_CC_PARAM'; bank: number; ccIndex: number; channel: number; id: number }
```

The reducer should update `config.banks[bank].chs[ccIndex]` and `config.banks[bank].ids[ccIndex]` directly.

### Files to Modify

| File | Changes |
|------|---------|
| `schema/t16-config.schema.json` | Add `pal` to bank properties and required |
| `editor-tx/src/types/config.ts` | Regenerate (or manual: add `pal: number` to bank type) |
| `editor-tx/src/protocol/sysex.ts` | Add `sendCCParamUpdate()` function |
| `editor-tx/src/services/midi.ts` | Add `sendCCParamUpdate()` wrapper |
| `editor-tx/src/types/midi.ts` | Add `UPDATE_CC_PARAM` action type, add `updateCCParam` to `ConfigContextValue` |
| `editor-tx/src/contexts/ConfigContext.tsx` | Add `updateCCParam` callback, CC reducer case, `pal` to defaults |
| `editor-tx/src/pages/Dashboard/Dashboard.tsx` | Change CC handlers to use `updateCCParam` |
| `editor-tx/src/services/configValidator.test.ts` | Add `pal` to test fixtures |
| `editor-tx/src/services/midi.test.ts` | Add CC send test |
| `editor-tx/src/contexts/ConfigContext.test.tsx` | Add CC update test |

### Recommended Project Structure (no changes)

No new directories. All changes are to existing files.

### Anti-Patterns to Avoid
- **Overloading sendParamUpdate for CC:** Do not try to encode two values into one `value` parameter. The firmware design requires both channel and id in one message. Use a dedicated function.
- **Stripping pal instead of adding to schema:** The `pal` field is a real, used configuration value. Stripping it on import would lose data. Add it to the schema.
- **Separate channel/id SysEx messages for CC:** The firmware `SetCCParam` always sets both. Sending them separately would require firmware changes and create race conditions.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Schema validation | Custom validation | ajv (already in use) | Already integrated, just needs schema update |
| Type generation | Manual type edits | `generate-types.sh` | Single source of truth from schema |

## Common Pitfalls

### Pitfall 1: Forgetting to Update Test Fixtures
**What goes wrong:** Adding `pal` to schema makes existing test configs invalid (they lack `pal`). Tests break.
**Why it happens:** `configValidator.test.ts` and `ConfigContext.test.ts` have hardcoded config objects without `pal`.
**How to avoid:** Update ALL test fixture bank objects to include `pal` field.
**Warning signs:** Test failures mentioning "additionalProperties" or "required property 'pal'".

### Pitfall 2: DEFAULT_CONFIG Missing pal
**What goes wrong:** `DEFAULT_CONFIG` in ConfigContext.tsx doesn't have `pal`, so fresh connections send schema-invalid configs.
**Why it happens:** `pal` wasn't in the schema when `DEFAULT_CONFIG` was written.
**How to avoid:** Add `pal: 0` (or bank-appropriate default) to each bank in `DEFAULT_CONFIG`.
**Warning signs:** Import validation fails on app-generated defaults.

### Pitfall 3: migrateV103 Not Adding pal
**What goes wrong:** v103 configs migrated in the web editor won't have `pal`, failing validation.
**Why it happens:** `migrateV103` passes banks through unchanged.
**How to avoid:** Ensure `migrateV103` adds `pal` to each bank if missing (matching firmware's `bankObj["pal"] | (uint8_t)i` default behavior).
**Warning signs:** v103 imports fail after schema update.

### Pitfall 4: ConfigContext parseConfigDump Not Handling pal
**What goes wrong:** Config dumps from firmware include `pal`, but the old TypeScript type didn't have it. After type update, this just works -- but the `SET_CONFIG` dispatch with device data must preserve `pal`.
**Why it happens:** `parseConfigDump` uses `JSON.parse` which preserves all fields. After schema/type update, this is fine.
**How to avoid:** Verify that the config dump -> SET_CONFIG -> state roundtrip preserves `pal`.

### Pitfall 5: CC updateParam Retry Logic
**What goes wrong:** The retry/ACK tracking in `updateParam` uses `${domain}-${bank}-${field}` as key. CC needs different keying since it sends ccIndex, not field.
**Why it happens:** CC has its own send path but must still track ACK timestamps.
**How to avoid:** Ensure `updateCCParam` has its own retry/ACK logic using appropriate key format.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 3.x (web), Unity (firmware native) |
| Config file | `editor-tx/vitest.config.ts`, `platformio.ini [env:native]` |
| Quick run command | `cd editor-tx && npx vitest run --reporter=verbose` |
| Full suite command | `cd editor-tx && npx vitest run && cd .. && pio test -e native` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROTO-02 | CC param update sends 5-byte payload | unit | `cd editor-tx && npx vitest run src/services/midi.test.ts -x` | Exists (needs CC test added) |
| PROTO-02 | CC reducer updates chs/ids arrays | unit | `cd editor-tx && npx vitest run src/contexts/ConfigContext.test.tsx -x` | Exists (needs CC test added) |
| WEBFEAT-04 | Schema validates config with pal field | unit | `cd editor-tx && npx vitest run src/services/configValidator.test.ts -x` | Exists (needs pal fixtures) |
| WEBFEAT-04 | v103 migration adds pal defaults | unit | `cd editor-tx && npx vitest run src/services/configValidator.test.ts -x` | Exists (needs migration test) |

### Sampling Rate
- **Per task commit:** `cd editor-tx && npx vitest run --reporter=verbose`
- **Per wave merge:** `cd editor-tx && npx vitest run && cd /home/unwn/git/T16 && pio test -e native`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
None -- existing test infrastructure covers all phase requirements. Tests need new cases added, not new files.

## Code Examples

### CC SysEx Send Function (new)
```typescript
// sysex.ts - new function
export function sendCCParamUpdate(
  output: Output,
  bankIndex: number,
  ccIndex: number,
  channel: number,
  id: number,
): void {
  output.sendSysex(MANUFACTURER_ID, [
    CMD.PARAM, SUB.REQUEST,
    DOMAIN.BANK_CC, bankIndex, ccIndex, channel, id
  ])
}
```

This produces a SysEx payload of 5 bytes after the header: `[0x02, bankIndex, ccIndex, channel, id]`, matching firmware expectation.

### Schema pal Addition
```json
// In bank item properties
"pal": { "type": "integer", "minimum": 0, "maximum": 7 }
// In bank item required array
"required": ["pal", "ch", "scale", "oct", "note", "vel", "at", "flip_x", "flip_y", "koala_mode", "chs", "ids"]
```

### Reducer CC Case
```typescript
case 'UPDATE_CC_PARAM': {
  const { bank, ccIndex, channel, id } = action
  const newConfig = structuredClone(state.config)
  const bankConfig = newConfig.banks[bank]
  if (bankConfig && ccIndex >= 0 && ccIndex < 8) {
    bankConfig.chs[ccIndex] = channel
    bankConfig.ids[ccIndex] = id
  }
  return { ...state, config: newConfig }
}
```

## Open Questions

1. **Palette maximum value**
   - What we know: `KeyModeData.palette` is `uint8_t`, defaults to 0. Firmware uses palette for LED color schemes.
   - What's unclear: Exact maximum. Code review suggests 0-7 based on pattern count, but not explicitly constrained in firmware.
   - Recommendation: Use `"maximum": 7` in schema (conservative). If firmware allows more, schema can be relaxed later.

2. **CC per-param ACK tracking**
   - What we know: Current `updateParam` tracks ACK by `${domain}-${bank}-${field}` key.
   - What's unclear: Whether firmware sends one ACK per CC update or needs special handling.
   - Recommendation: Firmware `HandleParamSet` sends ACK after successful `SetCCParam`, so one ACK per call. Use `cc-${bank}-${ccIndex}` as tracking key.

## Sources

### Primary (HIGH confidence)
- Direct code review of `src/SysExHandler.cpp` lines 141-197 (HandleParamSet CC path)
- Direct code review of `src/ConfigManager.cpp` lines 131-147 (SetCCParam) and 415-461 (PopulateDocFromStructs with pal)
- Direct code review of `editor-tx/src/protocol/sysex.ts` lines 68-76 (sendParamUpdate)
- Direct code review of `editor-tx/src/pages/Dashboard/Dashboard.tsx` lines 229-275 (CcMappingTab)
- Direct code review of `schema/t16-config.schema.json` (no pal in bank properties)
- v1.0 milestone audit `.planning/milestones/v1.0-MILESTONE-AUDIT.md`

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - no new libraries, all changes to existing code
- Architecture: HIGH - both bugs fully diagnosed from code review, fix approach is clear
- Pitfalls: HIGH - all pitfalls derived from direct code analysis of affected files

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable -- firmware/web codebase under our control)
