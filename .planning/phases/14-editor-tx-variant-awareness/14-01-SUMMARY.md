---
phase: 14-editor-tx-variant-awareness
plan: "14-01"
subsystem: state
tags: [react, context, typescript, variant, capabilities]
requires:
  - phase: 13-config-schema-migration
    provides: "T16Configuration.variant + DEFAULT_CONFIG.variant='T16'"
provides:
  - "Variant + Capabilities types (editor-tx/src/types/variant.ts)"
  - "TOTAL_KEYS_BY_VARIANT, FALLBACK_CAPABILITIES, VARIANT_STORAGE_KEY constants"
  - "ConfigContext gains runtimeVariant/capabilities/isHandshakeConfirmed/pendingAdaptation state"
  - "useVariant() typed hook returning {variant, capabilities, totalKeys, isHandshakeConfirmed, setVariant, setCapabilities}"
  - "Variant precedence: handshake > config.variant > localStorage > 'T16'"
affects: [14-02, 14-03, 14-04, 14-05]
tech-stack:
  added: []
  patterns: [typed hook over raw useContext, precedence-based derived state]
key-files:
  created:
    - editor-tx/src/types/variant.ts
    - editor-tx/src/constants/capabilities.ts
    - editor-tx/src/hooks/useVariant.ts
    - editor-tx/src/hooks/useVariant.test.tsx
  modified:
    - editor-tx/src/types/midi.ts
    - editor-tx/src/contexts/ConfigContext.tsx
key-decisions:
  - "ConfigContext exported as a named export so useVariant can import the context object directly"
  - "localStorage writes wrapped in try/catch (Safari private mode + quota)"
  - "Disconnect-clear effect uses wasConnectedRef to skip the initial mount transition (prevents test/programmatic state from being clobbered)"
patterns-established:
  - "Typed hook + named context export: useX() throws if outside Provider; ConfigContext is named export not default-only"
  - "Precedence-derived state: derivedVariant computed inline in provider with explicit fallback chain"
requirements-completed: [EDITOR-01]
duration: ~25min
completed: 2026-04-30
---

# Plan 14-01: Variant types + ConfigContext.variant + useVariant hook

Established the variant + capabilities seams in editor-tx state. ConfigContext now exposes a derived `variant` computed from a strict precedence chain (handshake > config.variant > localStorage > 'T16'), plus a `capabilities` shape and `isHandshakeConfirmed` flag. The `useVariant()` hook is the only sanctioned entry point — direct `useContext(ConfigContext)` for variant is reserved for the provider itself.

## Highlights

- 6 vitest cases pass (precedence default, config wins over localStorage, setVariant flips runtimeVariant, handshake flips both flags, totalKeys derivation, error outside provider).
- Bug found and fixed: the existing disconnect-clear effect fired on initial mount and overwrote handshake state set by tests/programmatic code. Now gated on a was-connected ref.

## Caveats

- The plan's "config.variant T16 wins over localStorage" test had to be reframed: with DEFAULT_CONFIG.variant='T16', config always wins absent handshake, regardless of localStorage. Documented in the test's comment.
