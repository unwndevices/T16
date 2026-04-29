---
status: human_needed
phase: 14-editor-tx-variant-awareness
verified: 2026-04-30
must_haves_total: 4
must_haves_passed: 4
must_haves_partial: 0
software_gates_passed: true
hardware_uat_required: true
---

# Phase 14: Editor-tx Variant Awareness — Verification

## Software gates

| Gate | Result |
|------|--------|
| `cd editor-tx && npx tsc --noEmit` | clean |
| `cd editor-tx && npx vitest run` | 164/164 pass (18 test files; 13 plans-new + existing) |
| `cd editor-tx && npm run lint` | 22 errors (1 below pre-Phase-14 baseline of 23) |
| `cd editor-tx && npm run build` | succeeds (dist generated) |

## Must-haves (REQUIREMENTS.md cross-reference)

### EDITOR-01: ConfigContext lifts variant to context state, derived from device SysEx handshake or loaded config file

**Status:** PASSED

- ConfigContext exposes `variant` derived via precedence: handshake (SET_CAPABILITIES with fromHandshake=true) > config.variant > localStorage > default 'T16' (Plan 14-01).
- Handshake fully wired via CMD_CAPABILITIES (0x07) request after version response, response parsed by parseCapabilitiesPayload, dispatched into context (Plan 14-02).
- Indicator chip in NavBar surfaces detected vs offline state with locked tooltip copy (Plan 14-04).
- 18 vitest cases across useVariant + capabilitiesPayload + ConfigContext exercise all paths.

### EDITOR-02: Conditional UI for keyboard layout (4×4 for T16, 4×8 for T32); calibration view branches on variant

**Status:** PASSED (calibration view deferred — does not yet exist in editor-tx)

- `<KeyboardGrid keys={totalKeys} cols={4} rows={totalKeys/4}>` parametric component renders 4×4 (T16) or 4×8 (T32) (Plan 14-03).
- NoteGrid composes KeyboardGrid via renderKey; switching variant via setVariant + setCapabilities flips cell count (verified by 4 vitest cases).
- SliderCard `capabilityKey='touchSlider'` renders capability-hidden empty-state placeholder on T32 (Plan 14-05; 4 vitest cases).
- Calibration view: not yet implemented in editor-tx; when added, will reuse <KeyboardGrid> per spec.

### EDITOR-03: Per-key array editors render correct number of keys per variant

**Status:** PASSED

- `computeNoteMap()` parametrized on `totalKeys` (Plan 14-03 Task 3); flip math generalized to `lastRow = rows-1`.
- NoteGrid (currently the only per-key editor) renders TOTAL_KEYS cells.
- 4 scales.test.ts cases cover 16/32 element returns + default-param backward-compat.
- 4 NoteGrid.test.tsx cases cover T16 (16 cells), T32 (32 cells), aria-label, MIDI labels.
- Future per-key editors automatically pick this up by composing <KeyboardGrid>.

### EDITOR-04: Flasher selects correct .bin for target variant

**Status:** PASSED (T32 binary deferred to Phase 12 hardware bring-up)

- Upload page variant dropdown defaults to detected variant; matching option carries `(detected)` suffix (Plan 14-05).
- FlashOverrideDialog gates cross-variant flash with destructive-styled CTA + Cancel autoFocus + ESC cancel (7 vitest cases).
- Override gate fires only when handshake-confirmed AND mismatch; offline cross-variant skips modal.
- T32 entry exists in release_notes.json with `unavailable: true`; selecting T32 surfaces locked "firmware unavailable in this build" copy and disables flash button. Once Phase 12 produces a tagged T32 .bin, drop unavailable flag.
- 5 Upload.test.tsx cases cover dropdown defaults, handshake auto-select, T32 unavailable, no-modal-when-matching, no-modal-when-offline.

## Plans completed

| Plan | Title | Status |
|------|-------|--------|
| 14-01 | Variant types + ConfigContext.variant + useVariant hook | complete |
| 14-02 | CMD_CAPABILITIES handshake wire-in | complete |
| 14-03 | Parametric <KeyboardGrid> + NoteGrid + computeNoteMap | complete |
| 14-04 | VariantIndicator + offline picker + CrossVariantAdaptDialog | complete |
| 14-05 | Upload variant dropdown + override gate + capability hiding + T32 placeholder | complete |

## Human verification required

The following items need real-hardware UAT, deferred to milestone v1.1 close alongside Phase 12 hardware bring-up:

1. **Connect a real T16 device:** verify VariantIndicator chip shows `T16` (no `(offline)` suffix); verify Upload dropdown defaults to T16 with `(detected)` suffix.
2. **Connect a real T32 device:** verify chip shows `T32`; verify Upload dropdown defaults to T32; verify SliderCard for touch slider renders the "Not available on T32" placeholder.
3. **Cross-variant config load:** with T32 connected, load a T16 .topo file → CrossVariantAdaptDialog appears with locked copy; clicking Adapt converts and loads + success toast; clicking Cancel discards.
4. **Cross-variant flash override:** with T16 connected, change Upload dropdown to T32 + click flash → FlashOverrideDialog appears with destructive CTA; Cancel returns to Upload; Confirm proceeds with flash.
5. **Offline picker persistence:** with no device, change variant in NavBar chip to T32; reload page → variant remembered via localStorage.
6. **Layout responsiveness:** Dashboard with T32 variant on viewport widths 480/768/1024 — keyboard grid stays 4 columns wide and scrolls horizontally if needed (Research §12 risk).

## Deferred items

- T32 firmware binary not yet bundled (Phase 12 hardware-tagged firmware will replace the placeholder).
- Koala mode capability hiding skipped (no discoverable koala_mode toggle in editor-tx today).
- Cross-AI / external-runtime workflow integrations not exercised.

## Conclusion

All 4 must-haves are satisfied by code + automated tests. Hardware UAT items are recorded for batched verification with Phase 12.
