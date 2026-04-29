---
phase: 14-editor-tx-variant-awareness
fixed: 2026-04-29T22:50:00Z
review_path: .planning/phases/14-editor-tx-variant-awareness/14-REVIEW.md
fix_scope: critical_warning
iteration: 1
findings_in_scope: 4
fixed: 4
skipped: 0
deferred: 7
status: all_fixed
---

# Phase 14: Code Review Fix Report

**Source review:** `14-REVIEW.md` (11 findings — 0 critical, 4 warning, 7 info)
**Scope this pass:** Critical + Warning (`fix_scope: critical_warning`)
**Iterations:** 1 (no re-review run; all in-scope items resolved on first pass)

## Summary

All 4 warning findings from the Phase 14 review were fixed and committed atomically.
Info findings (IN-01..IN-07) were deferred per the default scope contract — they
are tracked below for follow-up but were not touched in this pass.

| ID | Title | Status | Commit |
|----|-------|--------|--------|
| WR-01 | Capabilities reframe duplicates status byte | Fixed | `5f35e94` |
| WR-02 | `getScaleDegree` fails for multi-octave custom scales | Fixed | `090ec55` |
| WR-03 | Adapt toast asserts data discarded when v201 adapt is no-op | Fixed | `104c8d8` |
| WR-04 | Param-update retry timers leak after disconnect/unmount | Fixed | `851e7a3` |
| IN-01..IN-07 | Robustness / stylistic items | Deferred | — |

## Fixes Applied

### WR-01 — Capabilities reframe drops duplicated status byte
**Commit:** `5f35e94`
**File:** `editor-tx/src/contexts/ConfigContext.tsx` (capability handler block)

The reframed Uint8Array placed `msg.payload[0]` at the synthetic status slot
(`reframed[2]`) and then copied the FULL payload starting at index 3, so
`msg.payload[0]` appeared twice and the JSON window started with a status byte
(typically `0x00`) that `JSON.parse` rejected. The "two-attempt parse" was
silently single-attempt because only the fallback view ever produced valid JSON.

Now the reframe path slices `msg.payload.slice(1)` into the JSON window so the
status byte is not duplicated. The fallback path is untouched and still handles
the no-status-prefix shape that future firmware may emit. The inline comment
documents both shapes explicitly to prevent regression.

### WR-02 — `getScaleDegree` mod-12 reduces intervals
**Commit:** `090ec55`
**Files:** `editor-tx/src/constants/scales.ts:142`, `editor-tx/src/constants/scales.test.ts`

`getScaleDegree` reduced the lookup key mod-12 but compared against un-reduced
interval entries via `indexOf`, so any pitch class only reachable through an
interval `>= 12` returned `null`. For `custom_scale2` defaults
`[0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45]`, every interval
beyond index 3 failed lookup. The visible breakage was NoteGrid cells landing
in-scale by `computeNoteMap` but rendering as out-of-scale (gray, no hue ramp)
— amplified on T32's doubled grid.

Fix: `findIndex` with both sides reduced mod-12. Added a `multi-octave custom
scales (WR-02)` describe block in `scales.test.ts` that sweeps every default
`custom_scale2` interval and asserts non-null degree, plus a negative case for
a pitch class outside the scale's class set. Test count: 170 (+5 from this fix).

### WR-03 — Layout adapt toast describes actual diff
**Commit:** `104c8d8`
**File:** `editor-tx/src/pages/Layout/Layout.tsx`

For v201, `adaptConfigForVariant` is a pure variant rewrite — no per-key arrays
exist to pad/truncate. The "keys 17–32 discarded" toast was destructive copy
applied to a benign rename, misleading users into thinking data had been lost.

`handleAdapt` now computes the adapted config and compares it to the
"variant-only-rewrite baseline" (`{...fileConfig, variant: deviceVariant}`).
When they match (the v201 reality today), the toast reads
`Config variant rewritten: T32 → T16 (no per-key data changed in v201).`
The destructive copy is preserved verbatim for the future v202+ branch where
`adaptConfigForVariant` actually prunes per-key data — the toast will then
become accurate automatically.

### WR-04 — Cancel param-update retry timers on disconnect/unmount
**Commit:** `851e7a3`
**File:** `editor-tx/src/contexts/ConfigContext.tsx`

`updateParam` and `updateCCParam` scheduled nested 500ms outer + 500ms inner
`setTimeout`s for ACK retry / failure logging. Timer IDs were never stored, so
the disconnect-clear effect added in 14-01 could not cancel them. Effects:

- Outer + inner timers continued firing after disconnect, reading
  `senderRef.current` (now `null`) and emitting stale "no ACK, retrying..." /
  "sync failed after retry" warnings.
- `pendingParamTimestamps` was never cleared on disconnect, accumulating stale
  entries across reconnects. Since `isParamAck` pops the first map entry (see
  IN-01), a fresh ACK could be credited with a stale send time, corrupting the
  RTT debug log.

Fix: a new `pendingTimers` ref-Set tracks every outer/inner timer ID; each
timer self-removes from the Set when it fires; the disconnect-clear effect
calls `pendingTimers.current.forEach(clearTimeout)` and clears
`pendingParamTimestamps` alongside the existing capability/device-config reset.
A second `useEffect` ensures the same cleanup runs on full provider unmount,
covering the case where the provider tears down without a disconnect transition.

## Deferred Findings (Info — out of scope this pass)

These remain documented in `14-REVIEW.md` and were intentionally not touched
because the default scope is Critical + Warning. Re-run with `--all` to address.

- **IN-01** — `isParamAck` pops arbitrary entry (first-in-map) instead of
  matching by parameter key. Mis-attributes RTT in concurrent updates.
  Note: the WR-04 fix clears the map on disconnect, removing one source of
  cross-session corruption, but the per-ACK matching issue remains.
- **IN-02** — `KeyboardGrid` dev-mode contract logs `console.error` despite
  comment promising a throw.
- **IN-03** — `parseCapabilitiesPayload` uses `String.fromCharCode(...spread)`;
  swap to `TextDecoder` for safety + UTF-8 correctness.
- **IN-04** — `derivedVariant` IIFE recomputes every render and reads
  `localStorage` synchronously on the offline path; wrap in `useMemo`.
- **IN-05** — `setVariant` identity flips on `isHandshakeConfirmed`, thrashing
  SysEx listener attach/detach.
- **IN-06** — `VariantIndicator` casts before validating; should use the
  exported `isVariant` guard.
- **IN-07** — `Upload` may emit a one-frame controlled-`<select>` value
  warning when `selectedVariant` changes before `selectedFirmwareVersion`.

## Validation

| Gate | Result |
|------|--------|
| `npm run typecheck` (editor-tx) | clean |
| `npm test` (editor-tx vitest) | 170/170 pass (+5 from WR-02 regression block) |
| `pio run -e t16_debug -e t32_debug` | both SUCCESS |
| `pio test -e native_test` | 81/84 pass — 2 failures in `test_config_manager` (`test_init_default_version` expects `200`, got `201`) are pre-existing and unrelated to Phase 14 web-editor changes; not introduced by this fix pass. |
| ESLint | unchanged warning count for modified files (all warnings on `ConfigContext.tsx` are pre-existing react-refresh / exhaustive-deps notes) |

## Notes

- Phase 13 WR-03 (param-ACK FIFO / first-in-map matching) was explicitly
  flagged as a separate protocol issue and is **not** re-attempted here.
  IN-01 is the editor-side surface of the same underlying contract; both
  await the firmware-side correlation key change.
- Commits use the project convention `fix(14): WR-NN — <summary>` (matches
  `fix(13)` precedent in `git log`).
- No firmware source files were modified; the `native_test` pre-existing
  failures predate this fix pass.

---

_Generated: 2026-04-29T22:50:00Z_
_Workflow: gsd-code-review-fix 14 --auto_
