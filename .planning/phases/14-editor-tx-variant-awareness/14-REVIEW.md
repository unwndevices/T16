---
phase: 14-editor-tx-variant-awareness
reviewed: 2026-04-29T22:33:21Z
depth: standard
files_reviewed: 22
files_reviewed_list:
  - editor-tx/src/assets/firmwares/release_notes.json
  - editor-tx/src/components/CrossVariantAdaptDialog/CrossVariantAdaptDialog.tsx
  - editor-tx/src/components/CrossVariantAdaptDialog/index.ts
  - editor-tx/src/components/KeyboardGrid/KeyboardGrid.tsx
  - editor-tx/src/components/KeyboardGrid/index.ts
  - editor-tx/src/components/NavBar/NavBar.tsx
  - editor-tx/src/components/NoteGrid/NoteGrid.tsx
  - editor-tx/src/components/SliderCard/SliderCard.tsx
  - editor-tx/src/components/VariantIndicator/VariantIndicator.tsx
  - editor-tx/src/components/VariantIndicator/index.ts
  - editor-tx/src/constants/capabilities.ts
  - editor-tx/src/constants/scales.ts
  - editor-tx/src/contexts/ConfigContext.tsx
  - editor-tx/src/hooks/useVariant.ts
  - editor-tx/src/pages/Layout/Layout.tsx
  - editor-tx/src/pages/Upload/FlashOverrideDialog.tsx
  - editor-tx/src/pages/Upload/Upload.tsx
  - editor-tx/src/protocol/sysex.ts
  - editor-tx/src/services/adaptConfigForVariant.ts
  - editor-tx/src/services/configValidator.ts
  - editor-tx/src/services/midi.ts
  - editor-tx/src/types/midi.ts
  - editor-tx/src/types/variant.ts
  - editor-tx/vite.config.ts
findings:
  critical: 0
  warning: 4
  info: 7
  total: 11
status: issues_found
---

# Phase 14: Code Review Report

**Reviewed:** 2026-04-29T22:33:21Z
**Depth:** standard
**Files Reviewed:** 22 (test files and CSS modules excluded per agent rules)
**Status:** issues_found

## Summary

Phase 14 introduces variant awareness across editor-tx: a runtime variant + capabilities context, a parametric `KeyboardGrid`, a capabilities SysEx handshake, an adapt-on-import dialog, and an Upload variant gate. The architecture is sound — `useVariant()` is properly typed, `parseCapabilitiesPayload` is strict, and the precedence chain (handshake > config.variant > localStorage > 'T16') is explicit.

Findings cluster around three themes:

1. **Capabilities reframe is incorrect.** The first parse attempt in `ConfigContext.handleSysexData` duplicates the status byte into the JSON window — only the fallback view succeeds. This works in practice because every emission tested matches the fallback shape, but the "two-attempt parse" defensive design is silently single-attempt.
2. **Custom-scale degree coloring is broken** in `NoteGrid` for `custom_scale2` (intervals > 11). This is pre-existing logic in `getScaleDegree`, but `NoteGrid.tsx` was rewritten in 14-03 and the bug ships in the new file scope.
3. **Adapt-then-load toast is misleading** — for v201 there are no per-key arrays to truncate, so "keys 17–32 discarded" is technically vacuous despite `adaptConfigForVariant` being a no-op beyond variant rewrite. Documented in code, but the user-visible string contradicts behavior.

Other findings are robustness items: a retry-timer leak in `updateParam`/`updateCCParam`, an unused dev-error contract in `KeyboardGrid`, and stylistic consistency (`isVariant` vs `ALL_VARIANTS.includes`).

No critical issues — no security gaps, no auth issues, no data-loss paths.

## Warnings

### WR-01: Capabilities reframe duplicates status byte; first parse attempt is dead code

**File:** `editor-tx/src/contexts/ConfigContext.tsx:298-307`
**Issue:** The reframed view written for `parseCapabilitiesPayload` builds a buffer where index 2 holds the "status byte" (`msg.payload[0]`), then `reframed.set(msg.payload, 3)` copies the *entire* payload starting at index 3. This means the byte at `msg.payload[0]` appears twice: once as the synthetic status (`reframed[2]`) and once as the first JSON byte (`reframed[3]`). `parseCapabilitiesPayload` slices from index 3 and treats those bytes as JSON, so the JSON window now has an extra leading byte equal to the status byte.

If the firmware emits `[status=0x00, '{', '"', ...]` as the payload, the reframed JSON becomes `[0x00, '{', ...]` which `JSON.parse` rejects (NUL byte). The fallback `buildCapabilitiesView(msg.payload)` then re-runs with the raw payload at offset 3, which only succeeds if the payload itself starts with valid JSON (no status prefix). Net effect: the comment block describes a "reframe-then-fallback" two-attempt design but in reality only one of the two paths can succeed for any given firmware emission shape — the other is dead.

**Fix:** Either (a) skip the duplicated byte by copying `msg.payload.slice(1)` into `reframed[3..]`:

```typescript
const reframed = new Uint8Array(3 + (msg.payload.length - 1))
reframed[0] = msg.cmd
reframed[1] = msg.sub
reframed[2] = msg.payload[0] ?? 0
reframed.set(msg.payload.slice(1), 3)
```

…or (b) drop the reframe attempt entirely and call `parseCapabilitiesPayload(buildCapabilitiesView(msg.payload))` directly, since the firmware shape is now stable and the fallback handles the no-status-byte case.

### WR-02: Custom-scale degree coloring fails when intervals exceed 11 semitones

**File:** `editor-tx/src/constants/scales.ts:142` (function `getScaleDegree`); consumed by `editor-tx/src/components/NoteGrid/NoteGrid.tsx:46`
**Issue:** `getScaleDegree` does `((midiNote - rootNote) % 12 + 12) % 12` to compute a 0–11 semitone offset, then `intervals.indexOf(semitoneFromRoot)`. For `custom_scale2` the default intervals are `[0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45]` — most values are ≥ 12. The 0–11 reduction collapses note positions like `12, 15, 18, ...` to `0, 3, 6, ...` for matching, but the *interval list* still contains the un-reduced values, so `indexOf` returns -1 for everything except the literal `0`, `3`, `6`, `9` entries. Result: cells in the NoteGrid that `computeNoteMap` placed at valid in-scale positions are rendered as `out of scale` (gray/muted) and lose the hue ramp.

This bug pre-exists Phase 14 in `getScaleDegree`, but `NoteGrid.tsx` was rewritten in 14-03 and the file is in review scope. Doubling the grid for T32 makes the visible breakage more obvious.

**Fix:** Reduce the *intervals* to mod-12 before lookup, or skip mod reduction when the interval list spans multiple octaves:

```typescript
export function getScaleDegree(
  midiNote: number,
  rootNote: number,
  intervals: readonly number[],
): number | null {
  const semitoneFromRoot = ((midiNote - rootNote) % 12 + 12) % 12
  const degree = intervals.findIndex((iv) => iv % 12 === semitoneFromRoot)
  return degree === -1 ? null : degree
}
```

Add a unit test against `custom_scale2` defaults asserting non-null degrees for the placed notes.

### WR-03: `adaptConfigForVariant` only rewrites `variant`, but Layout toast asserts data was discarded

**File:** `editor-tx/src/services/adaptConfigForVariant.ts:19-30` and `editor-tx/src/pages/Layout/Layout.tsx:17-21`
**Issue:** For v201, `adaptConfigForVariant` is documented as a variant-rewrite no-op (no per-key arrays exist to pad/truncate). The `Layout.handleAdapt` toast tells the user "Config adapted: T32 → T16 (keys 17–32 discarded)" — but nothing was discarded. The config that round-trips through the dialog and `sendFullConfig` is byte-identical to the file the user loaded except for the `variant` field. The user gets a destructive-sounding confirmation for a benign operation.

This is correctness-of-UX rather than a logic bug, but it will cause real confusion when v202 lands and the toast becomes accurate for some configs but not others — there's no way to tell which.

**Fix:** Either (a) make the toast describe what `adaptConfigForVariant` actually did (compare input vs output), or (b) gate the destructive copy on a future schema version. Minimal change:

```typescript
const adaptedKeys = adaptConfigForVariant(...)
const sameShape = JSON.stringify({...fileConfig, variant: deviceVariant}) === JSON.stringify(adaptedKeys)
const toastMsg = sameShape
  ? `Config variant rewritten: ${fileVariant} → ${deviceVariant} (no per-key data changed in v${fileConfig.version}).`
  : (fileVariant === 'T16'
      ? 'Config adapted: T16 → T32 (16 keys padded with defaults).'
      : 'Config adapted: T32 → T16 (keys 17–32 discarded).')
```

Or scope the warning copy to the dialog body (which is shown *before* commit) and use a neutral confirmation toast.

### WR-04: Param-update retry timers leak after disconnect / unmount

**File:** `editor-tx/src/contexts/ConfigContext.tsx:351-408` (`updateParam` and `updateCCParam`)
**Issue:** `updateParam` and `updateCCParam` schedule nested `setTimeout`s for ACK retry / failure logging. The timeout IDs are not stored, and the effect cleanup that detaches MIDI listeners (`useEffect` lines 480-491, 494-502) doesn't cancel pending timers. On disconnect or unmount:

- The outer 500 ms timer still fires; it then schedules an inner 500 ms timer; the inner fires another 500 ms later — both reading `senderRef.current` (now null) and logging "no ACK, retrying…" / "sync failed after retry" warnings *after the user has already disconnected*.
- `pendingParamTimestamps` is never cleared on disconnect, so entries accumulate across reconnects (the `entries().next()` pop in `isParamAck` matches arbitrary stale entries to fresh ACKs, corrupting RTT measurements).

Phase 14 did not introduce this code, but the disconnect-clear effect (lines 508-521) was added in 14-01 and explicitly excludes the timer/timestamp cleanup.

**Fix:** Track timeout IDs in a ref-Set, cancel them in the disconnect-clear effect, and clear `pendingParamTimestamps`:

```typescript
const pendingTimers = useRef<Set<ReturnType<typeof setTimeout>>>(new Set())
// in updateParam:
const t1 = setTimeout(() => {
  pendingTimers.current.delete(t1)
  if (!pendingParamTimestamps.current.has(key)) return
  // ... existing retry logic, store t2 the same way
}, 500)
pendingTimers.current.add(t1)

// in disconnect-clear effect:
if (!isConnected && wasConnectedRef.current) {
  pendingTimers.current.forEach(clearTimeout)
  pendingTimers.current.clear()
  pendingParamTimestamps.current.clear()
  // ... existing dispatches
}
```

## Info

### IN-01: `isParamAck` pops arbitrary entry instead of matching by key

**File:** `editor-tx/src/contexts/ConfigContext.tsx:327-338`
**Issue:** On ACK, the code pops `timestamps.entries().next()` — the *first* (insertion-order) pending entry — and reports its RTT. If two updates fire close together and ACKs arrive in a different order, the RTT log mis-attributes timing. Functional sync still works (the retry timer cleanup uses the correct key), but logged RTT values are unreliable.
**Fix:** Include the parameter key in the ACK SubResponse from firmware (or correlate by next-received-update assumption) so the editor can match precisely. Until then, document the log as "approximate".

### IN-02: `KeyboardGrid` dev-mode contract logs but does not throw

**File:** `editor-tx/src/components/KeyboardGrid/KeyboardGrid.tsx:13-16`
**Issue:** Comment reads `// Defensive: keys must equal cols * rows. Throw in dev for early catch.` — implementation calls `console.error` only. A wrong-shaped grid silently renders an inconsistent layout instead of failing loudly during development.
**Fix:** Replace `console.error` with `throw new Error(...)` in the `import.meta.env.DEV` branch, or update the comment to reflect log-only behavior.

### IN-03: `parseCapabilitiesPayload` uses spread for byte → string conversion

**File:** `editor-tx/src/services/midi.ts:201`
**Issue:** `String.fromCharCode(...Array.from(data.slice(3)))` spreads the entire payload into a function call. Capability payloads are tiny (~50 bytes) so safe in practice, but spreading large arrays into varargs can hit `Maximum call stack size exceeded` on misbehaving inputs. Use `TextDecoder` for both safety and to handle UTF-8 correctly:
```typescript
const json = new TextDecoder('utf-8').decode(data.slice(3))
```

### IN-04: `derivedVariant` recomputes on every render and reads `localStorage` synchronously

**File:** `editor-tx/src/contexts/ConfigContext.tsx:226-237`
**Issue:** The IIFE recomputes the variant every render. When `state.runtimeVariant && state.isHandshakeConfirmed` is the resolved branch this is cheap, but the localStorage fallback runs a sync I/O call on every render that lands there (offline path). Wrap in `useMemo` keyed on the inputs:
```typescript
const derivedVariant = useMemo<Variant>(() => { /* ... */ }, [state.runtimeVariant, state.isHandshakeConfirmed, state.config.variant])
```

### IN-05: `setVariant` identity changes thrash SysEx listeners

**File:** `editor-tx/src/contexts/ConfigContext.tsx:253-270`, `:480-502`
**Issue:** `setVariant`'s deps include `state.isHandshakeConfirmed`, so its identity flips on handshake. `handleSysexData` lists `setVariant` as a dep, so its identity flips. The two SysEx listener effects depend on `handleSysexData`, so both detach and re-attach on every handshake transition (and on every offline picker click). Functional, but causes momentary listener gaps and adds noise to React DevTools profiling.
**Fix:** Use a ref for `isHandshakeConfirmed` inside `setVariant`, or split the post-variant-set capability dispatch out of the callback so `setVariant` is a stable function.

### IN-06: `VariantIndicator` casts before validating

**File:** `editor-tx/src/components/VariantIndicator/VariantIndicator.tsx:55-58`
**Issue:** `if (ALL_VARIANTS.includes(v as Variant)) { setVariant(v as Variant) }` — the cast circumvents the type check the runtime guard is meant to enforce. The codebase already exports `isVariant` from `@/types/variant`; use it for consistency with `parseCapabilitiesPayload` (which validates the same set):
```typescript
if (isVariant(v)) setVariant(v)
```

### IN-07: `Upload` may briefly emit a controlled-`<select>` warning on variant change

**File:** `editor-tx/src/pages/Upload/Upload.tsx:166-179, 296-313`
**Issue:** When `selectedVariant` changes, `selectedFirmwareVersion` is still the previous variant's value during the same render. `selectedFirmware` falls back to `availableFirmwares[0]` (correct UX), but the native `<select>` is rendered with `value={selectedFirmware?.version ?? ''}` — and that version may not match `selectedFirmwareVersion`. Then the reset effect fires and aligns state. React typically warns on uncontrolled→controlled (or value-not-in-options) transitions during this window. Tests passed because vitest doesn't render the version select; manual UAT may show a one-frame console warning.
**Fix:** Derive the displayed value from `selectedFirmware?.version` and write back to state synchronously when computing the fallback (e.g., compute the resolved version in a `useMemo` and use it for both the `<select value>` and the source of truth, dropping the reset effect entirely).

---

_Reviewed: 2026-04-29T22:33:21Z_
_Reviewer: Claude (gsd-code-reviewer, inline due to Task tool unavailability)_
_Depth: standard_
