---
phase: 14-editor-tx-variant-awareness
plan: "14-05"
subsystem: ui
tags: [react, dialog, esptool, variant, capability-hiding, build-bundle]
requires:
  - phase: 14-editor-tx-variant-awareness
    provides: "useVariant() (14-01), capabilities handshake (14-02)"
provides:
  - "Upload page variant dropdown with (detected) suffix on matching option"
  - "FlashOverrideDialog with destructive-styled CTA gating cross-variant flash"
  - "SliderCard capabilityKey='touchSlider' prop + capability-hidden empty-state placeholder"
  - "release_notes.json gains variant field on every entry + T32 placeholder entry (unavailable: true)"
  - "Test setup stubs for ResizeObserver, Element pointer-capture, scrollIntoView"
affects: []
tech-stack:
  added: []
  patterns: [unavailable-firmware locked-copy fallback, capability-hidden empty state]
key-files:
  created:
    - editor-tx/src/pages/Upload/FlashOverrideDialog.tsx
    - editor-tx/src/pages/Upload/FlashOverrideDialog.module.css
    - editor-tx/src/pages/Upload/FlashOverrideDialog.test.tsx
    - editor-tx/src/pages/Upload/Upload.test.tsx
    - editor-tx/src/components/SliderCard/SliderCard.test.tsx
    - editor-tx/src/assets/firmwares/t32_v1.0.0.bin (2-byte placeholder)
  modified:
    - editor-tx/src/pages/Upload/Upload.tsx (rewrite for variant + override gate)
    - editor-tx/src/components/SliderCard/SliderCard.tsx (capabilityKey gating)
    - editor-tx/src/components/SliderCard/SliderCard.module.css
    - editor-tx/src/assets/firmwares/release_notes.json
    - editor-tx/src/test/setup.ts (Radix-required jsdom stubs)
    - editor-tx/vite.config.ts (assetsInclude for .placeholder files)
key-decisions:
  - "Override gate fires ONLY when isHandshakeConfirmed && selectedVariant !== deviceVariant — offline cross-variant flashing skips modal"
  - "T32 binary ships as a 2-byte .bin placeholder + entry marked unavailable: true. Locked 'firmware unavailable in this build' copy renders when user selects T32 — flash button disabled. Once Phase 12 hardware bring-up produces a tagged binary, drop unavailable from release_notes.json."
  - "selectedFirmware version derives from availableFirmwares[0] in state; reset on variant change via dedicated useEffect with eslint-disable for set-state-in-effect (legitimate sync-with-prop pattern)"
  - "Vite's static URL resolution emits hashed assets only for filenames it can statically analyze; the T32 placeholder doesn't currently bundle, but isn't needed because the unavailable: true flag prevents flash attempts"
patterns-established:
  - "Capability hiding via prop opt-in: SliderCard takes optional capabilityKey='touchSlider'; existing callers unaffected"
  - "Unavailable firmware: release_notes entry with unavailable: true + UI fallback copy + button disable"
  - "Module-level static assets: ALL_FIRMWARES parsed once at import, no setState-in-effect"
requirements-completed: [EDITOR-04]
duration: ~40min
completed: 2026-04-30
---

# Plan 14-05: Upload page variant + capability hiding + T32 placeholder

The Upload page is now variant-aware: dropdown defaults to detected variant with `(detected)` suffix, override modal gates cross-variant flash with destructive CTA, and SliderCard renders the capability-hidden placeholder on T32. T32 firmware ships as a placeholder until Phase 12 hardware bring-up tags a real binary.

## Highlights

- 7 vitest cases for FlashOverrideDialog (title, body interpolation, Keep T16/Flash T32 anyway labels, destructive class, Cancel autoFocus, ESC, Confirm).
- 5 vitest cases for Upload page (default variant offline/with handshake, T32 unavailable warning, no-modal-when-matching, no-modal-when-offline).
- 4 vitest cases for SliderCard capability hiding (slider visible when true, placeholder visible with handshake/offline body copy, default behaviour without prop).
- Test setup hardened with Radix-required jsdom stubs (ResizeObserver, pointer-capture, scrollIntoView) — unblocks Slider/Dialog/Select primitives in jsdom.

## Caveats / Deferrals

- **T32 binary not in dist bundle.** Vite/rolldown's static URL resolver in `new URL('../../assets/firmwares/${selectedFirmware.fileName}', import.meta.url)` emits hashed assets only for the t16 .bin filenames it can statically analyze from the parsed release_notes.json strings — the t32 placeholder filename appears in the JSON but rolldown's analyzer doesn't pick it up. Investigated multiple workarounds (import.meta.glob, explicit `?url` imports, assetsInclude) — none successfully ship the placeholder through to dist. Deferred because:
  1. The user-facing flow doesn't depend on a real T32 .bin (the unavailable: true flag triggers the locked warning copy and disables flash).
  2. Phase 12 hardware bring-up will produce a real tagged T32 firmware that will bundle naturally once dropped into release_notes.json with unavailable cleared.
  3. Investigating rolldown bundler internals is out of scope for Phase 14.
- **Koala mode hiding** (mentioned in plan must_haves): no discoverable koala_mode toggle exists in editor-tx today, so capability gating for it is deferred until that toggle lands.
- **Manual hardware UAT** for the override flow + capability hiding deferred to milestone close alongside Phase 12.

## Final state

- `npx tsc --noEmit`: clean
- `npx vitest run`: 164/164 pass
- `npm run lint`: 22 errors (1 below pre-Phase-14 baseline of 23)
- `npm run build`: succeeds
