# Phase 3: Web Rewrite - Context

**Gathered:** 2026-04-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Full TypeScript rewrite of the web configurator. Replace Chakra UI v2 with a custom design system using Radix primitives and CSS custom properties. Split the MidiProvider god-context into ConnectionContext and ConfigContext. Reorganize codebase by feature domain. Ensure consistent, polished UI across all pages.

</domain>

<decisions>
## Implementation Decisions

### Design System
- **D-01:** Replace Chakra UI v2 with **Radix UI primitives + CSS custom properties** — matching DROP's approach. Custom design system with reusable components (Button, Card, Modal, Select, Switch, Slider).
- **D-02:** CSS approach uses **CSS custom properties with CSS modules** — no CSS-in-JS runtime. Design tokens defined as CSS variables in a central theme file.
- **D-03:** Typography uses **Inter (body) and Poppins (headings)** — same fonts, self-hosted via @fontsource (already installed).
- **D-04:** Color system uses the existing **primary/secondary/accent palettes** from the current Chakra theme, converted to CSS custom properties.

### Architecture
- **D-05:** MidiProvider split into **ConnectionContext** (WebMIDI lifecycle, device discovery, connection state) and **ConfigContext** (device config, sync status, parameter mutations).
- **D-06:** Custom hooks for context consumption — **useConnection()** and **useConfig()** instead of raw useContext.
- **D-07:** MIDI communication logic extracted to a **services/midi.ts** service layer — protocol encoding, SysEx send/receive, config serialization.
- **D-08:** Codebase organized by **feature domain** — contexts/, hooks/, services/, components/, types/, pages/.

### Toolchain
- **D-09:** Upgrade to **React 19 + Vite 7 + ESLint 9 flat config** — matching DROP's toolchain.
- **D-10:** All files converted to **TypeScript with strict mode** — no .js/.jsx, no PropTypes.
- **D-11:** Path aliases configured — **@/components**, **@/services**, **@/hooks**, **@/contexts**, **@/types**.

### Claude's Discretion
- Component-level design decisions (specific Radix primitive usage, animation approach)
- Responsive breakpoint values
- State management within ConfigContext (reducer vs useState)
- ESLint rule selection for flat config

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `editor-tx/src/types/config.ts` — Generated TypeScript config types (from Phase 1)
- `editor-tx/src/protocol/sysex.ts` — SysEx protocol constants and helpers (from Phase 1)
- `editor-tx/src/components/MidiProvider.jsx` — Current god-context (to be split)
- `editor-tx/src/main.jsx` — Chakra theme with color palettes (to be converted to CSS vars)

### Established Patterns
- Current app uses React Router DOM for routing (Dashboard, Upload, Manual pages)
- Chakra UI inline props for all styling
- Single context pattern with raw useContext

### Integration Points
- `editor-tx/src/App.jsx` — Router configuration
- `editor-tx/src/main.jsx` — App entry point with providers
- `editor-tx/package.json` — Dependencies to add/remove

</code_context>

<specifics>
## Specific Ideas

- Follow DROP's design system structure with a `design-system/` directory
- Match DROP's component organization pattern (barrel exports per domain)
- Use existing T16 color palettes (unwn brand colors) for the design system
- Keep the current page structure (Dashboard, Upload, Manual/QuickStart)

</specifics>

<deferred>
## Deferred Ideas

- PWA support — Phase 5
- Note grid visualizer — Phase 5
- MIDI monitor visualization — Phase 5
- Config backup import/export — Phase 4

</deferred>
