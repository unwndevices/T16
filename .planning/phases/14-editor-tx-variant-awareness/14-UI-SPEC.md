---
phase: 14
slug: editor-tx-variant-awareness
status: draft
shadcn_initialized: false
preset: none
created: 2026-04-29
---

# Phase 14 — UI Design Contract

> Visual and interaction contract for the editor-tx variant-awareness work. Additive on top of the existing Radix + tokens.css design system established in Phase 3 — this contract does NOT introduce new tokens, only declares which existing ones the new surfaces must use.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none (custom Radix-primitive design system, no shadcn) |
| Preset | not applicable |
| Component library | `@/design-system/*` — Radix-backed primitives (Button, Card, Dialog, Select, Skeleton, Slider, Switch, Tabs, Toast, Tooltip) |
| Icon library | `react-icons` (Material Design subset — `react-icons/md`) — match existing NavBar usage |
| Font | Inter 400/600 (body), Poppins 600 (headings) — already loaded in `main.tsx` |

**Source of tokens:** `editor-tx/src/design-system/tokens.css`. All declarations below reference existing CSS variables — do not introduce new ones in Phase 14.

---

## Spacing Scale

Phase 14 uses the existing 4-point scale unchanged.

| Token | Variable | Value | Usage in Phase 14 |
|-------|----------|-------|-------------------|
| xs | `--space-xs` | 4px | Inline gaps inside KeyboardGrid cells |
| sm | `--space-sm` | 8px | KeyboardGrid `gap` (matches NoteGrid) |
| md | `--space-md` | 16px | Modal body padding, dropdown spacing, variant-picker row gaps |
| lg | `--space-lg` | 24px | Dialog section padding, "(detected)" badge offset |
| xl | `--space-xl` | 32px | Page-level container gaps in Upload |
| 2xl | `--space-2xl` | 48px | Major breaks in cross-variant adapt modal |

**Exceptions:**
- KeyboardGrid cell minimum tap target: 44px (existing NoteGrid `min-height/min-width: 44px`). Reuse this — do not shrink for T32.
- KeyboardGrid `max-width`: T16 keeps `280px` cap (matches NoteGrid). T32 (4×8) removes the cap and uses `max-width: 100%` so 8 rows can render without clipping; on viewports `< 480px` the grid switches to horizontal scroll inside the parent card (not a layout reflow — preserves the 4-column "physical key" mental model).

---

## Typography

Phase 14 uses the existing 4-size, 2-weight scale. No new sizes.

| Role | Size token | px | Weight | Line Height | Phase-14 surfaces |
|------|------------|----|----|-------------|-------------------|
| Body | `--text-sm` | 14px (0.875rem) | `--weight-regular` (400) | 1.5 | Modal body copy, dropdown options, variant-picker labels |
| Label / Cell | `--text-xs` | 12px (0.75rem) | `--weight-semibold` (600) | 1.4 | KeyboardGrid cell labels, variant indicator chip |
| Section heading | `--text-md` | 20px (1.25rem) | `--weight-semibold` (600) | 1.3 | Modal title ("Variant mismatch"), Upload section header |
| Display | `--text-lg` | 28px (1.75rem) | `--weight-semibold` (600) | 1.2 | Not used in Phase 14 |

Heading font: `--font-heading` (Poppins 600). Body font: `--font-body` (Inter).

---

## Color

Phase 14 reuses the existing 60/30/10 split. No new color tokens. **Accent (`--accent-500`, yellow-green) is reserved — Phase 14 does NOT introduce new accent surfaces.**

| Role | Variable | Hex | Usage in Phase 14 |
|------|----------|-----|-------------------|
| Dominant (60%) | `--gray-50` / `--white` | `#faf9fe` / `#ffffff` | Page background, modal backdrop body, KeyboardGrid empty cell fill |
| Secondary (30%) | `--gray-100` / `--gray-200` | `#f2f0fd` / `#e8e6f3` | Card surfaces, modal panel, dropdown background, capability-hidden placeholder background |
| Accent (10%) | `--primary-700` | `#7258ec` | Primary CTAs only (see reserved-for list) |
| Destructive | `--color-error` | `#e4412c` | Cross-variant truncation warning text + "Adapt to T16" confirm-state border |

**Accent (`--primary-700`) reserved for in Phase 14:**
1. Primary modal CTA button: `[Adapt and load]` in cross-variant adapt modal
2. Primary flasher CTA: `[Flash {variant}]` button in Upload page
3. Active state of variant indicator chip near connection indicator
4. Selected option highlight in variant dropdown

**Accent (`--primary-700`) NOT used for:**
- KeyboardGrid cell hover/active (reuse existing NoteGrid `--gray-200` muted treatment)
- Variant picker offline toggle (uses `--gray-700` text)
- Capability-hidden placeholder text (uses `--gray-500`)

**Destructive (`--color-error`) reserved for in Phase 14:**
1. Truncation warning copy in cross-variant adapt modal when adapting T32 → T16
2. Confirmation step border on manual flasher override that disagrees with connected device

Status colors (`--color-success`, `--color-warning`) are not introduced into Phase 14 surfaces.

---

## Copywriting Contract

All copy is locked. Executor uses these strings verbatim — no paraphrasing.

### Variant indicator (near connection indicator)

| Element | Copy |
|---------|------|
| Indicator chip when device connected | `T16` or `T32` (uppercase, no prefix) |
| Indicator chip offline (last-used remembered) | `T16 (offline)` / `T32 (offline)` |
| Indicator chip first-load default | `T16` (per D14.4 — no "default" suffix; matches existing user expectation) |
| Tooltip on hover | `Variant: {VARIANT}. Detected from device.` (when connected) |
| Tooltip on hover | `Variant: {VARIANT}. No device connected — using last selection.` (offline) |

### Offline variant picker (settings or near indicator)

| Element | Copy |
|---------|------|
| Picker label | `Edit layout for` |
| Option 1 | `T16 (16 keys, 4×4)` |
| Option 2 | `T32 (32 keys, 4×8)` |
| Helper text below picker | `Connect a device to override this automatically.` |

### Cross-variant adapt modal (D13.1 / D14.x consumer)

Triggered when user loads a config whose `variant` differs from connected device, OR offline-selected variant.

| Element | Copy |
|---------|------|
| Modal title | `Variant mismatch` |
| Modal body line 1 | `This config is for {fileVariant}. You're connected to {deviceVariant}.` |
| Modal body line 2 (T16 → T32) | `Adapting will pad per-key settings (notes, scales, CC) from 16 to 32 keys with default values for keys 17–32. Global settings are preserved.` |
| Modal body line 2 (T32 → T16) | `Adapting will discard per-key settings for keys 17–32. This cannot be undone for the loaded file.` (rendered in `--color-error`) |
| Primary CTA | `Adapt and load` |
| Secondary CTA | `Cancel` |
| Toast on success (T16 → T32) | `Config adapted: T16 → T32 (16 keys padded with defaults).` |
| Toast on success (T32 → T16) | `Config adapted: T32 → T16 (keys 17–32 discarded).` |
| Toast on cancel | (no toast) |

### Flasher (Upload page) variant dropdown — D14.2

| Element | Copy |
|---------|------|
| Dropdown label | `Firmware variant` |
| Option 1 | `T16` |
| Option 2 | `T32` |
| Detected hint (suffix on matching option, gray-500) | ` (detected)` |
| Primary CTA — matching | `Flash {variant} firmware` |
| Primary CTA — overriding (mismatch) | `Flash {variant} firmware` (unchanged label; confirmation step gates the action) |
| Override confirmation modal title | `Flash a different variant?` |
| Override confirmation body | `Your connected device reports {deviceVariant}. You selected {targetVariant}. Flashing the wrong variant will require a recalibration and may render keys non-functional until the matching firmware is reflashed.` |
| Override confirm CTA | `Flash {targetVariant} anyway` (border + text in `--color-error`) |
| Override cancel CTA | `Keep {deviceVariant}` |
| Build artifact unavailable error | `{variant} firmware unavailable in this build. Reload the page or report this issue.` |

### Capability-hidden control empty state

When a control is hidden because the active variant lacks the capability (e.g., touch slider on T32), it must NOT be silently removed from any tabbed/grouped UI. Replace with a passive placeholder.

| Element | Copy |
|---------|------|
| Placeholder heading | `Not available on {variant}` |
| Placeholder body | `{Feature name} is hardware-specific to T16 and isn't supported on this variant.` |
| Placeholder body (offline, capabilities unknown) | `{Feature name} availability will be confirmed when a device connects.` |

Placeholder visual: `--gray-200` background, `--gray-500` text, `--radius-md`, `--space-md` padding, no icon.

### KeyboardGrid empty / loading states

| Element | Copy |
|---------|------|
| Loading skeleton (waiting on handshake) | (no copy — render `<Skeleton>` from design-system at the same dimensions as one cell, repeated `TOTAL_KEYS` times) |
| No-config error | `Couldn't load layout for {variant}. Reconnect the device or import a config.` |

---

## Component Contract — `<KeyboardGrid>`

This is the central new component (D14.3). Contract is prescriptive.

| Prop | Type | Required | Notes |
|------|------|----------|-------|
| `keys` | `number` | yes | `TOTAL_KEYS` from variant constants — 16 or 32 |
| `cols` | `number` | yes | `4` for both T16 and T32 (physical column count is invariant) |
| `rows` | `number` | yes | `keys / cols` — 4 for T16, 8 for T32 |
| `renderKey` | `(index: number) => ReactNode` | yes | Caller provides cell contents |
| `aria-label` | `string` | yes | `"Keyboard layout: {variant}, {keys} keys"` |

**Layout rules:**
- `display: grid`, `grid-template-columns: repeat({cols}, 1fr)`, `gap: var(--space-sm)`
- T16 (4×4): `max-width: 280px` (matches existing NoteGrid)
- T32 (4×8): `max-width: 100%`; on viewport `< 480px`, parent card applies `overflow-x: auto` and the grid sets `min-width: 280px` so cells stay >= 44px
- Cell minimum: 44px × 44px (tap target — non-negotiable)
- Cells inherit `KeyCard` styling — Phase 14 does NOT restyle KeyCard

**Responsive behavior at 4×8 (T32):**
- `>= 768px`: full grid renders inline with config side panels (no overflow)
- `480–767px`: grid renders full-width, single-column page stack
- `< 480px`: grid horizontally scrolls inside its card; vertical row count (8) is preserved — never reflow to 8×4 (would invert the physical mapping)

---

## Variant Indicator — Placement Contract

- Lives in the existing `<NavBar>` (`editor-tx/src/components/NavBar/NavBar.tsx`), positioned **immediately to the left of the connection indicator** (USB/BLE icons).
- Visual: small chip — `--space-xs` vertical padding, `--space-sm` horizontal padding, `--radius-sm`, `--gray-100` background, `--gray-800` text, `--text-xs` semibold.
- Active state (handshake confirmed): background swaps to `--primary-700` at 10% alpha (use `color-mix(in srgb, var(--primary-700) 10%, transparent)`), text stays `--gray-800`. No border.
- Offline state: background `--gray-100`, text `--gray-500`, italic.
- Click target: opens the offline variant picker popover (Radix `<Select>` from `@/design-system/Select`).

---

## Modal Confirm-Flow Contract

Both modals use `@/design-system/Dialog`. No new modal component.

**Cross-variant adapt modal:**
1. Trigger: file load detects variant mismatch (handshake variant differs from file `variant`)
2. Focus on open: trapped on `[Cancel]` (the safe default), not the destructive primary
3. ESC closes (cancels) — no accidental adapt
4. After Adapt: dispatch `SET_CONFIG` and emit toast (copy above)
5. After Cancel: no state change, no toast

**Flasher manual override confirmation:**
1. Trigger: user selects a variant in the flasher dropdown that doesn't match `deviceVariant` (when device is connected). If no device connected, no confirmation step.
2. Focus on open: trapped on `[Keep {deviceVariant}]` (safe default)
3. Override confirm CTA renders with `--color-error` border + text to surface destructiveness
4. Confirmation flow does NOT change for offline flashing (no detected variant to compare against)

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| (none — custom design-system) | not applicable | not applicable |

No third-party component registries are introduced in Phase 14. All new UI composes existing primitives in `editor-tx/src/design-system/`.

---

## Pre-Populated From

| Source | Decisions Used |
|--------|---------------|
| `14-CONTEXT.md` | D14.1 variant detection, D14.2 .bin selection, D14.3 KeyboardGrid, D14.4 first-load default, capability-driven hiding |
| `13-CONTEXT.md` | D13.1 cross-variant adapt modal lives in editor-tx |
| `editor-tx/src/design-system/tokens.css` | All spacing, typography, color tokens (no new tokens) |
| `editor-tx/src/components/NoteGrid/NoteGrid.module.css` | Grid template (`repeat(4, 1fr)`), 44px tap target, `max-width: 280px`, gap convention |
| `editor-tx/src/main.tsx` | Font stack (Inter + Poppins), provider tree |
| `editor-tx/src/components/NavBar/NavBar.tsx` | Icon library confirmation (`react-icons/md`) |
| User input this session | None — all answers derived from upstream artifacts |

---

## Checker Sign-Off

- [ ] Dimension 1 Copywriting: PASS
- [ ] Dimension 2 Visuals: PASS
- [ ] Dimension 3 Color: PASS
- [ ] Dimension 4 Typography: PASS
- [ ] Dimension 5 Spacing: PASS
- [ ] Dimension 6 Registry Safety: PASS

**Approval:** pending
