---
phase: 14-editor-tx-variant-awareness
plan: "14-04"
subsystem: ui
tags: [react, dialog, radix, variant, adapt, copy-locked]
requires:
  - phase: 14-editor-tx-variant-awareness
    provides: "useVariant() (14-01), ConfigContext pendingAdaptation state (14-01)"
provides:
  - "VariantIndicator chip + offline picker (NavBar)"
  - "CrossVariantAdaptDialog with locked UI-SPEC copy"
  - "adaptConfigForVariant pure service module + tests"
  - "ConfigContext.importConfig now routes variant mismatches to the adapt modal"
  - "Layout.tsx mounts the dialog and emits the success toast"
affects: []
tech-stack:
  added: []
  patterns: [locked copy from UI-SPEC, autoFocus on safe default via setTimeout for Radix portal]
key-files:
  created:
    - editor-tx/src/components/VariantIndicator/VariantIndicator.tsx
    - editor-tx/src/components/VariantIndicator/VariantIndicator.module.css
    - editor-tx/src/components/VariantIndicator/VariantIndicator.test.tsx
    - editor-tx/src/components/VariantIndicator/index.ts
    - editor-tx/src/components/CrossVariantAdaptDialog/CrossVariantAdaptDialog.tsx
    - editor-tx/src/components/CrossVariantAdaptDialog/CrossVariantAdaptDialog.module.css
    - editor-tx/src/components/CrossVariantAdaptDialog/CrossVariantAdaptDialog.test.tsx
    - editor-tx/src/components/CrossVariantAdaptDialog/index.ts
    - editor-tx/src/services/adaptConfigForVariant.ts
    - editor-tx/src/services/adaptConfigForVariant.test.ts
  modified:
    - editor-tx/src/components/NavBar/NavBar.tsx
    - editor-tx/src/pages/Layout/Layout.tsx
    - editor-tx/src/contexts/ConfigContext.tsx
    - editor-tx/src/services/configValidator.ts (re-exports adaptConfigForVariant for backward compat)
key-decisions:
  - "All dialog/picker copy is verbatim from 14-UI-SPEC.md §Copywriting Contract"
  - "T32→T16 (truncating) body line 2 uses --color-error styling for destructive emphasis"
  - "Cancel autoFocus via cancelRef + setTimeout(0) — Radix Dialog portal mounts asynchronously"
  - "Toast emission lives in Layout.tsx (consumer of useConfig/useToast), not in ConfigContext — keeps ConfigContext UI-free"
  - "adaptConfigForVariant moved to its own module; configValidator.ts re-exports for backward compat (avoids duplication)"
patterns-established:
  - "Locked copy contract: exact strings asserted via getByText to detect unintended copy edits"
  - "Pure adapt function for cross-variant routing — single point of change when v202+ adds per-key arrays"
requirements-completed: [EDITOR-01, EDITOR-02]
duration: ~35min
completed: 2026-04-30
---

# Plan 14-04: Variant indicator + offline picker + adapt modal

Users now see their connected variant at all times in the NavBar, can switch variants offline via the chip's Radix Select, and get a locked-copy adapt modal when loading a config that doesn't match the connected device.

## Highlights

- 5 vitest cases for VariantIndicator (offline label, connected T16/T32 chip text + data-variant + disabled state, helper copy).
- 8 vitest cases for CrossVariantAdaptDialog (title, body interpolation, T16→T32 vs T32→T16 body copy + destructive class, Cancel autoFocus, Adapt/Cancel handlers, ESC).
- 5 vitest cases for adaptConfigForVariant (idempotent same-variant returns same ref, T16↔T32 rewrite, no input mutation, round-trip identity).

## Deviation from plan

- **Design-system API:** Plan 14-04 Task 5 referenced `Dialog.Title` / `Dialog.Body` notation. The actual design-system uses `Dialog`, `DialogContent`, `DialogTitle`, `DialogDescription` named exports. Used the actual API.
- **Toast API:** Plan referenced `addToast({ message, kind })`. Actual API is `toast({ title, variant })`. Used actual.
- **Select API:** Plan referenced `<Select.Item>` children. Actual takes `options` array. Used actual.
- **Mount site:** Plan 14-04 Task 7 said App.tsx, but App.tsx is router-only. Mounted in Layout.tsx (inside ConfigProvider + ToastProvider, outside the routed pages).
- **VariantIndicator structure:** When offline, chip is a Select trigger; when handshake-confirmed, chip is a disabled `<button>` with tooltip explaining "device wins". Render branches by isHandshakeConfirmed rather than rendering the same JSX with conditional disabled prop.
