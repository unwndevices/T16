# Phase 14 — UI Review

**Audited:** 2026-04-29
**Baseline:** `14-UI-SPEC.md` (Radix custom design-system + tokens.css; no shadcn)
**Screenshots:** not captured (no dev server detected on :3000 or :5173 — code-only audit)

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 4/4 | All locked strings rendered verbatim; no generic labels added beyond spec-mandated "Cancel". |
| 2. Visuals | 3/4 | Hierarchy and destructive emphasis are correct, but `VariantIndicator` offline state diverges from the "small chip" contract — flattens chip + Select + helper inline instead of a popover trigger. |
| 3. Color | 4/4 | Accent reserved correctly; only one defensive `#ffffff` fallback (inside `var(--white, #ffffff)`); no new tokens. |
| 4. Typography | 3/4 | All sizes used as declared, but `Upload.module.css:18` uses `--text-lg` (Display, 28px) for the page title — spec explicitly says "Display ... Not used in Phase 14" and Section heading should be `--text-md`. |
| 5. Spacing | 4/4 | Token scale honored throughout; KeyboardGrid `max-width 280px` (T16) / `100%` + `min-width 280px` (T32) and 44px tap targets all match the contract. |
| 6. Experience Design | 3/4 | Modal focus traps and override gating are clean; **KeyboardGrid lacks the spec's loading skeleton and no-config error state** (spec lines 164-167) — NoteGrid renders unconditionally with no fallback. |

**Overall: 21/24**

---

## Top 3 Priority Fixes

1. **KeyboardGrid missing loading/error states** — During handshake or when no config is available, the grid renders with whatever stale `noteMap` exists or could mount empty; spec mandates `<Skeleton>` x `TOTAL_KEYS` while waiting and the locked copy `Couldn't load layout for {variant}. Reconnect the device or import a config.` on failure. **Fix:** Add `loading?: boolean` and `error?: string | null` props to `KeyboardGrid` (or add a sibling wrapper in `NoteGrid`); when loading, render `Skeleton` cells dimensioned to the same 44px×44px cell box; when `error`, render the locked error copy in a `--gray-200` placeholder card.
2. **Upload page title overrides the typography contract** — `Upload.module.css:18` sets `.title` to `var(--text-lg)` (28px Display), but `14-UI-SPEC.md` line 58 declares Display "Not used in Phase 14" and Section heading is `--text-md`. **Fix:** Change `.title` `font-size` to `var(--text-md)` to match the contract; if a Display size truly is wanted for the page H1, update the UI-SPEC instead so the contract stays the source of truth.
3. **VariantIndicator offline state isn't a chip-popover** — Spec section "Variant Indicator — Placement Contract" says "Click target: opens the offline variant picker popover (Radix `<Select>` from `@/design-system/Select`)." The implementation renders the chip only when `isHandshakeConfirmed`; when offline it flattens a hidden `tooltipAnchor`, an inline `<Select>`, helper text, and a separate `offlineLabel` span side-by-side in the NavBar — there is no chip and no popover trigger. The orphan `.offline` CSS rule (defined, never applied) confirms the divergence. **Fix:** Render the offline branch as an italic chip (use the existing `.chip` + a new `.offline` modifier) that acts as the Radix `Select.Trigger`; move the helper text into the popover's content area, not into the navbar layout.

---

## Detailed Findings

### Pillar 1: Copywriting (4/4)

Every contract string from `14-UI-SPEC.md` lines 95-167 is reproduced verbatim:

- `CrossVariantAdaptDialog.tsx:51` — `Variant mismatch` (modal title) ✓
- `CrossVariantAdaptDialog.tsx:53` — `This config is for {fileVariant}. You're connected to {deviceVariant}.` ✓
- `CrossVariantAdaptDialog.tsx:30-32` — both T16→T32 and T32→T16 body lines verbatim, with the destructive variant routed through the `styles.destructive` class ✓
- `CrossVariantAdaptDialog.tsx:58,61` — `Cancel` / `Adapt and load` ✓
- `Layout.tsx:19-20` — both success toasts verbatim ✓
- `FlashOverrideDialog.tsx:43-45` — `Flash a different variant?` and full body copy verbatim ✓
- `FlashOverrideDialog.tsx:49,56` — `Keep {deviceVariant}` / `Flash {targetVariant} anyway` ✓
- `Upload.tsx:225,287` — unavailable-firmware copy verbatim (used both as toast and as inline warning) ✓
- `VariantIndicator.tsx:16-19` — chip label and tooltip copy verbatim, including offline suffix ✓
- `VariantIndicator.tsx:53-62` — picker label `Edit layout for`, options `T16 (16 keys, 4×4)` / `T32 (32 keys, 4×8)`, helper `Connect a device to override this automatically.` ✓
- `SliderCard.tsx:33,34,38` — capability-hidden placeholder copy `Not available on {variant}` / `is hardware-specific to T16` / `availability will be confirmed when a device connects` verbatim ✓

No generic "Submit / Click Here / Save" labels found. The lone "Cancel" string is spec-mandated.

### Pillar 2: Visuals (3/4)

Strengths:
- Modal hierarchy uses `DialogTitle` / `DialogDescription` primitives consistently.
- Destructive copy in the truncation modal renders in `--color-error` per spec (`CrossVariantAdaptDialog.tsx:55`).
- Override CTA exposes destructiveness via `styles.destructive` (border + text in `--color-error` — `FlashOverrideDialog.module.css:1-4`).
- Variant indicator chip uses `--text-xs` semibold, `--radius-sm`, `--space-xs/sm` padding per spec.

WARNING — VariantIndicator offline branch (`VariantIndicator.tsx:44-65`) does not match the "Placement Contract":
- Spec: a small chip whose **click target opens the offline variant picker popover**.
- Implementation: when offline, the chip is replaced by a hidden `tooltipAnchor` (`width:0;height:0`), an always-visible Radix `<Select>`, helper text rendered inline, and a separate italic `offlineLabel` span — all flattened into the navbar layout.
- The `.offline` CSS class (`VariantIndicator.module.css:32-36`) defining the italic gray-500 chip background is dead code (no JSX consumer).
- Visual side-effect: in the offline state the navbar shows a permanent dropdown + descriptive paragraph rather than a compact chip with a popover.

WARNING — Upload page exposes a native `<select>` element (`Upload.tsx:296-313`) for firmware version selection, sitting alongside design-system `<Select>` for variant. Native select inherits OS-level styling and breaks the otherwise-uniform Radix surface. (Pre-existing — not introduced by Phase 14, but Phase 14 places the new variant Select directly above it which makes the inconsistency more visible.)

### Pillar 3: Color (4/4)

- Accent (`--primary-700`) appears only on:
  - `VariantIndicator.module.css:28` — chip active state at 10% alpha via `color-mix` (matches spec exactly)
  - The Button `variant="primary"` primitives in both dialogs
  - Pre-existing `.highlights` and `.progressFill` styles in Upload (use `--primary-800`, the existing button hover token, not new accent introductions)
- `--color-error` reserved correctly:
  - `CrossVariantAdaptDialog.module.css:2` — truncation copy
  - `FlashOverrideDialog.module.css:2-3` — override confirm CTA border + text
  No accent or destructive surfaces leak elsewhere in the new code.
- Hardcoded color audit: only `var(--white, #ffffff)` at `Upload.module.css:90` (defensive fallback inside a token reference — acceptable). No `rgb(`, `rgba(`, or bare hex values introduced in any Phase 14 file.
- `--color-warning` is reused at `Upload.module.css:55-56` for the unavailable-firmware banner. Spec line 90 says "`--color-warning` is not introduced into Phase 14 surfaces." Mild deviation, but justifiable: this surface communicates "build artifact unavailable" which is genuinely warning semantics, not error. Flagging as informational, not deducting.

### Pillar 4: Typography (3/4)

Sizes in use across Phase 14 surfaces: `--text-xs`, `--text-sm`, `--text-md`, `--text-lg` — all four declared sizes appear.

WARNING — `Upload.module.css:16-22` `.title` uses `var(--text-lg)` (Display, 28px). Spec line 58 explicitly: "Display | `--text-lg` | ... | Not used in Phase 14". The Upload page H1 should use Section heading (`--text-md`) per the contract, or the spec needs to be relaxed to allow `--text-lg` for page H1s. As-shipped, the page-title size is the only direct contract violation in this pillar.

Weights: only `--weight-semibold` (600) and the implicit `--weight-regular` (400 default) appear. Within the 2-weight cap.

Fonts: `--font-heading` (Poppins) on titles/section headings, `--font-body` (Inter) elsewhere. Consistent.

### Pillar 5: Spacing (4/4)

KeyboardGrid layout (`KeyboardGrid.module.css`):
- `gap: var(--space-sm)` ✓ matches NoteGrid convention
- `max-width: 280px` (default / T16) ✓
- `[data-keys="32"] { max-width: 100%; min-width: 280px }` ✓ matches T32 contract
- `@media (max-width: 480px)` preserves `min-width: 280px` so cells stay ≥ 44px during overflow scroll ✓
- `.cell { min-height: 44px; min-width: 44px; padding: var(--space-sm); border-radius: var(--radius-md) }` ✓

Upload page spacing uses only declared tokens (`--space-xs/sm/md/lg/2xl`). No arbitrary `[Npx]` values found in any new module CSS.

VariantIndicator chip uses `var(--space-xs) var(--space-sm)` padding — exactly what spec section "Variant Indicator — Placement Contract" prescribes.

Dialog actions use `gap: var(--space-sm)` and `margin-top: var(--space-md)` in both modal stylesheets — matches spec spacing scale row "md" usage.

### Pillar 6: Experience Design (3/4)

Strengths:
- **Focus management:** Both dialogs (`CrossVariantAdaptDialog.tsx:34-41`, `FlashOverrideDialog.tsx:27-33`) trap initial focus on the safe `Cancel` ref via `useEffect` + `setTimeout(0)` to defer past the Radix portal mount. Matches spec "Focus on open: trapped on `[Cancel]` (the safe default)".
- **ESC closes:** `Dialog.onOpenChange` in both modals routes a falsy `open` to `onCancel()` (no accidental adapt / no accidental override). Matches spec "ESC closes (cancels)".
- **Override gate correctly bypassed offline:** `Upload.tsx:232` only sets `overrideOpen` when `isHandshakeConfirmed && selectedVariant !== deviceVariant`. Matches spec "If no device connected, no confirmation step."
- **Toast on success:** `Layout.tsx:17-21` dispatches the success toast with the correct directional copy after `confirmAdaptation()`. Matches spec.
- **Capability-hidden placeholder shipped** (`SliderCard.tsx:33-38`) using the locked copy.
- **First-load default + handshake takeover:** `Upload.tsx:139-149` syncs `selectedVariant` to `deviceVariant` exactly once (`handshakeAppliedRef`) and only if the user hasn't manually picked. Clean implementation of D14.4.

WARNINGS:
1. **Loading state missing in KeyboardGrid** — `NoteGrid.tsx:38-82` renders the grid unconditionally; there is no skeleton placeholder while the handshake is pending. Spec line 165 explicitly calls for "render `<Skeleton>` from design-system at the same dimensions as one cell, repeated `TOTAL_KEYS` times". `grep -rn "Skeleton" src/components/KeyboardGrid src/components/NoteGrid` returns no matches.
2. **No-config error state missing** — Spec line 167 locks `Couldn't load layout for {variant}. Reconnect the device or import a config.` Grep finds no occurrence in source.
3. **KeyboardGrid invariant violation is dev-only** — `KeyboardGrid.tsx:13-16` logs to `console.error` only when `import.meta.env.DEV`. In production a `cols * rows !== keys` mismatch (e.g., a future variant with non-multiple-of-4 keys) will silently render the wrong cell count. Consider either always-on `console.warn` or a visible error fallback.
4. **"(detected)" hint is concatenated, not styled** — Spec line 140 calls for the suffix in `--gray-500`. `Upload.tsx:184-194` builds the option label as a plain string `'T16 (detected)'` — Radix `<Select>` renders it as a single text node with no color split. Functionally readable; visually it doesn't differentiate the hint.

---

## Files Audited

- `editor-tx/src/components/KeyboardGrid/KeyboardGrid.tsx`
- `editor-tx/src/components/KeyboardGrid/KeyboardGrid.module.css`
- `editor-tx/src/components/NoteGrid/NoteGrid.tsx`
- `editor-tx/src/components/VariantIndicator/VariantIndicator.tsx`
- `editor-tx/src/components/VariantIndicator/VariantIndicator.module.css`
- `editor-tx/src/components/CrossVariantAdaptDialog/CrossVariantAdaptDialog.tsx`
- `editor-tx/src/components/CrossVariantAdaptDialog/CrossVariantAdaptDialog.module.css`
- `editor-tx/src/components/SliderCard/SliderCard.tsx` (capability-hidden placeholder)
- `editor-tx/src/pages/Upload/Upload.tsx`
- `editor-tx/src/pages/Upload/Upload.module.css`
- `editor-tx/src/pages/Upload/FlashOverrideDialog.tsx`
- `editor-tx/src/pages/Upload/FlashOverrideDialog.module.css`
- `editor-tx/src/pages/Layout/Layout.tsx`
- `editor-tx/src/components/NavBar/NavBar.tsx` (placement check)
- `editor-tx/src/design-system/tokens.css` (token resolution check)
- `.planning/phases/14-editor-tx-variant-awareness/14-UI-SPEC.md` (audit baseline)

Registry safety: not applicable — no `components.json` (Phase 14 introduces no third-party registries).
