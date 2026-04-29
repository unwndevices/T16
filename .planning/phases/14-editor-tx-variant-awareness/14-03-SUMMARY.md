---
phase: 14-editor-tx-variant-awareness
plan: "14-03"
subsystem: ui
tags: [react, css-modules, parametric, grid, variant]
requires:
  - phase: 14-editor-tx-variant-awareness
    provides: "useVariant() hook returning totalKeys (Plan 14-01)"
provides:
  - "Parametric <KeyboardGrid> component (cols/rows/keys/renderKey + aria-label)"
  - "computeNoteMap parametrized on totalKeys (defaults to 16 for backward compat)"
  - "NoteGrid refactored to compose <KeyboardGrid> via renderKey, reads totalKeys/variant from useVariant()"
affects: [future per-key editors (calibration, etc.)]
tech-stack:
  added: []
  patterns: [render-prop composition for variant-parametric grids]
key-files:
  created:
    - editor-tx/src/components/KeyboardGrid/KeyboardGrid.tsx
    - editor-tx/src/components/KeyboardGrid/KeyboardGrid.module.css
    - editor-tx/src/components/KeyboardGrid/index.ts
    - editor-tx/src/components/KeyboardGrid/KeyboardGrid.test.tsx
    - editor-tx/src/components/NoteGrid/NoteGrid.test.tsx
  modified:
    - editor-tx/src/components/NoteGrid/NoteGrid.tsx
    - editor-tx/src/components/NoteGrid/NoteGrid.module.css
    - editor-tx/src/constants/scales.ts
    - editor-tx/src/constants/scales.test.ts
key-decisions:
  - "cols=4 invariant for both T16 and T32 (physical mapping, UI-SPEC §Component Contract). T32 = 4 cols × 8 rows."
  - "computeNoteMap flip math generalized: lastRow = rows-1 instead of literal 3 (correct for T32 8-row grid)"
  - "KeyboardGrid is purely presentational; consumers wire useVariant() to fill totalKeys"
  - "data-keys attribute drives T32-specific CSS (max-width override)"
patterns-established:
  - "Parametric grid via render-prop: <KeyboardGrid renderKey={(i) => ReactNode} />"
  - "CSS module with attribute-driven variant overrides: .grid[data-keys=\"32\"] { ... }"
requirements-completed: [EDITOR-02, EDITOR-03]
duration: ~30min
completed: 2026-04-30
---

# Plan 14-03: Parametric KeyboardGrid + NoteGrid + computeNoteMap

The keyboard editor now branches on `totalKeys` from useVariant(). On T32 it renders a 4×8 grid (32 cells); on T16 it renders 4×4 (16 cells). Future per-key editors compose <KeyboardGrid> the same way.

## Highlights

- 6 vitest cases for KeyboardGrid (T16/T32 cell counts, aria-label, cols invariant, renderKey indices, dev assertion).
- 4 cases extending scales.test.ts for the new totalKeys parameter.
- 4 cases for NoteGrid covering T16/T32 cell count, T32 aria-label, MIDI label rendering.

## Deviation from plan

- The plan's "first 16 elements of a 32-key map equal a 16-key map" test does not hold for non-Chromatic scales because flip math depends on row count. Reframed the test to assert the structural property that is invariant: for no-flip Chromatic, the 32-key map continues the chromatic sequence (24..55 from C2).
