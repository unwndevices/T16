# Phase 14 — Research: Editor-tx Variant Awareness

**Date:** 2026-04-29
**Status:** Complete

> What do we need to know to PLAN this phase well? Summary of editor-tx code seams, Phase 11/13 outputs that Phase 14 consumes, and concrete code-level guidance for the planner.

---

## 1. Stack Confirmation (Open Item #1 from CONTEXT.md)

The editor-tx repo is **TypeScript + React + custom Radix design-system + CSS modules**, completed in Phase 3.

- Confirmed paths:
  - `editor-tx/src/contexts/ConfigContext.tsx` — typed React reducer/provider, exports `DEFAULT_CONFIG: T16Configuration`.
  - `editor-tx/src/contexts/ConnectionContext.tsx` — owns USB/BLE MIDI Output handle.
  - `editor-tx/src/design-system/` — `Button`, `Card`, `Dialog`, `Select`, `Skeleton`, `Slider`, `Switch`, `Tabs`, `Toast`, `Tooltip`. Use these for all new UI.
  - `editor-tx/src/hooks/useConfig.ts`, `useConnection.ts` — typed context consumers (no raw `useContext` in app code).
  - `editor-tx/src/types/config.ts` — generated from `schema/t16-config.schema.json` via `schema/generate-types.sh`.
  - `editor-tx/src/services/configValidator.ts` — ajv-based import validator (`prepareImport`).
  - `editor-tx/src/protocol/sysex.ts` — wire-level SysEx command builders (`CMD`, `SUB`, `DOMAIN`).
  - `editor-tx/src/services/midi.ts` — re-exports + `parseSysExMessage`, `parseConfigDump`, `isConfigResponse`.
  - `editor-tx/src/components/NoteGrid/NoteGrid.{tsx,module.css}` — reference grid layout (4×4, 280px max-width, 44px tap target).
  - `editor-tx/src/components/NavBar/NavBar.tsx` — connection indicator location; variant chip lives here per UI-SPEC.
  - `editor-tx/src/pages/Upload/Upload.tsx` — line 149: `firmwareUrl = new URL(\`../../assets/firmwares/${selectedFirmware.fileName}\`, import.meta.url)`. Single binary URL resolution point — D14.2 is a one-line change to filename selection.
  - `editor-tx/src/assets/firmwares/` — currently holds `t16_v*.bin`. T32 binary will be added via Phase 10's build matrix output.

No JS/Chakra remnants in `src/` — Open Item #1 closed, scope is the TypeScript/Radix stack.

## 2. Existing `ConfigContext` Shape (Open Item #2)

`ConfigContext.tsx` exports `ConfigContextValue` (defined in `types/midi.ts`). The reducer dispatches `ConfigAction` for global/bank/cc/full-config updates. Phase 13 Plan 13-01 (Wave 1) adds `variant: 'T16'` to `DEFAULT_CONFIG` and bumps `version` to 201 — that is the seam Phase 14 builds on. Phase 14 adds:

- New context field: `variant: Variant` (top-level on context value, derived from `config.variant` plus runtime overrides).
- New context field: `capabilities: Capabilities` (from handshake or fallback).
- New context action: `setVariant(v: Variant)` for offline picker.
- New typed hook: `useVariant()` in `editor-tx/src/hooks/useVariant.ts`.

Variant precedence is computed inside the provider effect (per D14.1 priority list).

## 3. Flasher Single Point of Change (Open Item #3)

`pages/Upload/Upload.tsx` lines 25-150 hold the firmware list (`FIRMWARES` array) and `selectedFirmware.fileName` resolution. The variant dropdown adds a filter on this list (or a parallel T32 list). Confirmed: `firmwareUrl` is constructed from `selectedFirmware.fileName` only — the variant dropdown rewrites `selectedFirmware` selection, no other call site changes.

## 4. Hardcoded "16" Audit (Open Item #4)

```
editor-tx/src/constants/scales.ts:71  // 16-key note map docstring
editor-tx/src/constants/scales.ts:97  const noteMap = new Array<number>(16)
editor-tx/src/constants/scales.ts:102 for (let i = 0; i < 16; i++) {
editor-tx/src/components/NoteGrid/NoteGrid.module.css:3  grid-template-columns: repeat(4, 1fr); max-width: 280px
editor-tx/src/components/CcCard/CcCard.tsx                — CC count is per-bank (8), not per-key
editor-tx/src/pages/Dashboard/Dashboard.tsx:147           max={16}  (custom-scale degree picker — global, NOT per-key; leave as 16)
```

**Per-key surfaces that must parametrize on `TOTAL_KEYS`:**
1. `computeNoteMap()` in `constants/scales.ts` — accept `totalKeys: number` parameter; replace literal `16`.
2. `NoteGrid` component CSS — column count stays 4, but `max-width` rule must branch per UI-SPEC §"Spacing Scale Exceptions".
3. The new `<KeyboardGrid>` component (D14.3) — entirely new file, replaces the inline grid wrapper in NoteGrid.
4. Any per-key array editor that maps over a fixed `[0..15]` range — none currently exist beyond NoteGrid; CcCard is per-bank-CC (8 CCs/bank), unrelated to key count.

`Dashboard.tsx` line 147 (`max={16}`) is a degree-count cap for custom scales — global, not per-key. Leave unchanged.

## 5. Phase 11 / Phase 13 Outputs Phase 14 Depends On

### Phase 11 (HAL — already complete)
- `src/SysExProtocol.hpp` line 16: `constexpr uint8_t CMD_CAPABILITIES = 0x07;`
- `src/SysExHandler.cpp` lines 91, 290: handler emits ASCII JSON payload `{"variant":"T16"|"T32","capabilities":{"touchSlider":true|false,"koalaMode":true|false}}` in response to `[CMD_CAPABILITIES, SUB_REQUEST]` request.
- Editor-tx has not yet wired this command — Phase 14 adds the request + response parser.

### Phase 13 (executing in parallel — assumed landed before Phase 14)
- `schema/t16-config.schema.json` carries `version: const 201` and required `variant: enum ["T16","T32"]`.
- `editor-tx/src/types/config.ts` regenerated with `variant: "T16" | "T32"` on `T16Configuration`.
- `editor-tx/src/contexts/ConfigContext.tsx` `DEFAULT_CONFIG.variant = 'T16'` and `version: 201`.
- `editor-tx/src/services/configValidator.ts` rejects non-enum variants and wrong-sized per-key arrays.
- D13.1 cross-variant prompt is a Phase 14 implementation: editor side detects mismatch (handshake variant vs file variant) and shows the modal whose copy is locked in 14-UI-SPEC.

## 6. CMD_CAPABILITIES Wire-In Plan

Editor-side request:
```ts
// editor-tx/src/protocol/sysex.ts
export const CMD = { ..., CAPABILITIES: 0x07 } as const
export function requestCapabilities(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.CAPABILITIES, SUB.REQUEST])
}
```

Editor-side response parser:
```ts
// editor-tx/src/services/midi.ts (alongside parseConfigDump)
export function isCapabilitiesResponse(parsed: ParsedSysEx): boolean {
  return parsed.cmd === CMD.CAPABILITIES && parsed.sub === SUB.RESPONSE
}
export function parseCapabilitiesPayload(data: Uint8Array): {
  variant: Variant
  capabilities: Capabilities
} | null {
  // Bytes [3..end] are ASCII JSON (firmware emits `{"variant":"...","capabilities":{...}}`)
  const json = String.fromCharCode(...data.slice(3))
  const parsed = JSON.parse(json) as { variant: string; capabilities: Capabilities }
  if (parsed.variant !== 'T16' && parsed.variant !== 'T32') return null
  return parsed as { variant: Variant; capabilities: Capabilities }
}
```

Connection seam: when the SysEx version handshake arrives (`isVersionResponse`), the existing handler in `ConnectionContext.tsx` should also dispatch `requestCapabilities()`. The capabilities response then drives the `setVariant()` + `setCapabilities()` actions in `ConfigContext`.

## 7. `<KeyboardGrid>` Component Contract (D14.3)

New file: `editor-tx/src/components/KeyboardGrid/KeyboardGrid.{tsx,module.css}`. UI-SPEC §"Component Contract — `<KeyboardGrid>`" prescribes the API and layout rules. Refactor `NoteGrid` to render its cells via `<KeyboardGrid>`, passing `renderKey={(i) => <NoteCell index={i} ... />}`. Keep cell content in NoteGrid; KeyboardGrid only owns the grid container CSS + responsive breakpoints.

## 8. `adaptConfigForVariant()` Pure Function (D13.1 / D14.x consumer)

Lives in `editor-tx/src/services/adaptConfigForVariant.ts`. Pure, no side effects — vitest covers extend (T16→T32: pad per-key arrays from 16 to 32 with defaults) and truncate (T32→T16: drop indices 16..31). Only `custom_scale1`/`custom_scale2` are per-key today (16 elements), so adapt initially handles those plus `variant` rewrite. Future per-key arrays (note maps in v202+) will plug into the same function.

```ts
// editor-tx/src/services/adaptConfigForVariant.ts
export function adaptConfigForVariant(
  config: T16Configuration,
  target: Variant,
): T16Configuration { /* pure pad/truncate */ }
```

Modal copy is locked in 14-UI-SPEC §"Cross-variant adapt modal".

## 9. Capability-Driven Hiding

Touch slider and Koala mode are the only Phase 14 hide candidates. Surfaces:
- `editor-tx/src/components/SliderCard/SliderCard.tsx` — render the placeholder when `capabilities.touchSlider === false`. Use UI-SPEC's "Capability-hidden control empty state" copy.
- Any "Koala mode" toggle in the bank/dashboard editor (locate at plan time via grep `koala_mode`).

Offline fallback (no device connected): hardcoded table keyed by variant in `editor-tx/src/constants/capabilities.ts`:
```ts
export const FALLBACK_CAPABILITIES: Record<Variant, Capabilities> = {
  T16: { touchSlider: true,  koalaMode: true },
  T32: { touchSlider: false, koalaMode: false },
}
```

## 10. Build/Deploy Bundle (D14.2 secondary)

`editor-tx/src/assets/firmwares/` currently holds T16 binaries. Phase 14 adds T32 binaries (sourced from Phase 10's `pio run -e t32_release` output) and updates `release_notes.json` to list both. Vite resolves them at build time via `import.meta.url`, so the deploy step requires no other change.

## 11. Validation Architecture

Per `workflow.nyquist_validation = true`, plans MUST include:

- **Component-level (Vitest):**
  - `adaptConfigForVariant.test.ts` — extend + truncate + idempotency cases.
  - `KeyboardGrid.test.tsx` — renders `keys/cols × cols` grid; aria-label correct; respects `cols=4` invariant.
  - `parseCapabilitiesPayload.test.ts` — happy path + malformed JSON + bad variant string.
  - `useVariant.test.tsx` — precedence: handshake > config > localStorage > default.
- **Integration (manual hardware test on T16 + T32):**
  - Connect T16 → indicator chip shows `T16`, NoteGrid renders 4×4.
  - Connect T32 → chip shows `T32`, NoteGrid renders 4×8, slider tab replaced with placeholder.
  - Load T16 config while connected to T32 → adapt modal fires with locked copy.
  - Upload page: detected variant suffix appears on matching dropdown option; mismatch override fires confirmation modal.
- **Static:**
  - `npm run typecheck` clean.
  - `npm run lint` clean (no new ESLint warnings).
  - `npm run build` produces a bundle that includes both `t16_*.bin` and `t32_*.bin` under the assets dir.

Hardware-dependent UAT items (T32 unit) remain `validation_deferred` until milestone v1.1 close (matches Phase 12 batching policy).

## 12. Risks / Landmines

- **Race condition:** capabilities request fires before MIDI input handler is bound → response lost. Mitigation: dispatch `requestCapabilities()` from inside the `version_response` handler (proves MIDI plumbing is up).
- **localStorage `quotaExceeded`** for variant memory — wrap in try/catch, fallback to default `T16`. Single key only (`t16-editor.lastVariant`), trivially small.
- **JSON parse failure** on capabilities payload — return null, log warn, fall back to handshake-less default `T16`. Do not crash the connection.
- **NoteGrid existing consumers** — Dashboard imports `NoteGrid` and assumes 4×4 visual. After Phase 14, on T32 the grid grows 8 rows tall — re-test page layout breakpoints on Dashboard at 480/768/1024px viewport widths.

## 13. Anti-Patterns to Avoid

- Storing `variant` in two places (raw `useContext` access vs `useVariant()` hook). Always go through the typed hook. Direct context import in app code is a lint smell.
- Mutating `config.variant` directly — only the reducer's `SET_CONFIG` action or the new `SET_VARIANT` action may write it.
- Hardcoding `16`/`32` anywhere in Phase 14 surfaces — read from `config.variant` → `TOTAL_KEYS` derived constant or context capabilities.
- New CSS tokens — UI-SPEC explicitly forbids new tokens; reuse existing `--space-*`, `--gray-*`, `--primary-700`, `--color-error`.

---

## RESEARCH COMPLETE
