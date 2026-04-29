# Phase 14 — Editor-tx Variant Awareness: Context

**Phase goal:** The web configurator detects the connected variant, renders the matching keyboard/calibration UI, and flashes the correct `.bin`.
**Requirements:** EDITOR-01, EDITOR-02, EDITOR-03, EDITOR-04
**Depends on:** Phase 11 (capabilities in handshake), Phase 13 (`variant` in config + ajv validator)

---

## Locked Decisions

### D14.1 — Variant detection: handshake authoritative, config fallback [EDITOR-01]
- `ConfigContext` exposes `variant: 'T16' | 'T32'` derived in this order:
  1. Connected device handshake `variant` field (from D11.4).
  2. Loaded config file's `variant` field (from D13.2 schema v201).
  3. Last-used variant from localStorage.
  4. Default `'T16'` (per D14.4).
- When connected, the device wins. If a user loads a T16 config while connected to a T32, the cross-variant prompt from D13.1 fires.
- Components consume variant via a typed hook (`useVariant()`), not raw `useContext`.

### D14.2 — `.bin` selection: auto-detect with manual override [EDITOR-04]
- `pages/Upload.tsx` (or current React equivalent) shows a variant dropdown.
- Default value: connected device's variant if known; otherwise `'T16'`.
- User can override (e.g., to flash T32 firmware onto a chip currently running T16, or onto a fresh chip).
- Build artifacts: `t16_release.bin` and `t32_release.bin` available in the deploy bundle; flasher resolves the URL by variant.
- esptool-js call unchanged except for the binary URL.

### D14.3 — Parametric `<KeyboardGrid>` component [EDITOR-02 / EDITOR-03]
- Single `<KeyboardGrid keys={TOTAL_KEYS} cols={4} rows={TOTAL_KEYS / 4} renderKey={...} />` component.
- T16 → 4×4, T32 → 4×8, future variants drop in with no new component.
- Per-key array editors (note maps, scales, CC assignments, calibration view) read `variant.TOTAL_KEYS` from the context and render exactly that many rows/cards.
- Calibration view branches on `variant` for the count only — same component for both.

### D14.4 — First-load default: T16, no prompt
- With no device connected and no config loaded, the editor renders T16 UI immediately.
- Matches the `default_envs = t16_release` decision (D10.1) and the existing user base's expectations.
- A discreet variant picker (in app settings or near the connection indicator) lets a user switch to T32 layout offline if they want to author a T32 config without hardware. localStorage remembers the choice for next session.

---

## Capability-Driven Hiding (HAL-04 consumer)

- Editor-tx reads `capabilities` from the handshake (D11.4) and hides controls for features the device doesn't support:
  - `capabilities.touchSlider === false` → hide slider mode UI on T32.
  - `capabilities.koalaMode === false` → hide Koala mode toggle on T32.
- When variant is known but no device is connected (offline editing), use a hardcoded fallback table keyed by variant. (Keeps offline editing functional; documented as best-effort until handshake confirms.)

## Implementation Notes for Researcher / Planner

- **`ConfigContext` shape:** add `variant`, `capabilities`, `setVariant` (for offline picker). Keep the typed hook (`useVariant`) close to the context definition.
- **Cross-variant adapt logic (D13.1 dependency):** the `adaptConfigForVariant()` pure function lives here in editor-tx; called from the modal flow. Vitest covers extend/truncate behavior.
- **`<KeyboardGrid>` styling:** verify T32's 4×8 grid fits the existing layout breakpoints. May need horizontal scroll on narrow screens, or a responsive switch to 8×4. Capture the constraint in the plan.
- **Flasher dropdown UX:** show device-detected variant prominently with a small "(detected)" hint; manual override should require a confirmation step if it disagrees with the connected device, to prevent accidental cross-flashing.
- **Build/deploy:** GitHub Pages bundle must ship both `.bin` files. Update the deploy step to include `t16_release.bin` + `t32_release.bin`.
- **Type definitions:** `Variant` type lives in a shared types module (`editor-tx/src/types/variant.ts`); imported by ConfigContext, validator, flasher.

## Out of Scope for Phase 14

- T32-specific feature redesign (Koala mode rework on 32 keys) — explicitly deferred per REQUIREMENTS.md.
- New variant-specific visualizations (e.g., 8-column waveform displays) — only the keyboard layout and calibration view branch.
- Flasher protocol changes — only the binary URL resolution differs.

## Open Items for Researcher

1. Confirm the editor-tx repo is the TypeScript/React 19 + Radix design system version (per CLAUDE.md target). If still on JS/Chakra, scope the variant work for the current stack and note the migration as a separate axis.
2. Locate the existing `ConfigContext` and document the seam for adding `variant` without disrupting current consumers.
3. Where the `t16_release.bin` URL is currently resolved in `Upload.tsx` — confirm a single point of change for D14.2.
4. Layout audit: does any current screen assume 4×4 explicitly (CSS grid template, hardcoded 16 SVG elements, etc.)? Each becomes a Phase 14 work item.
