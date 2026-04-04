# Phase 5: Feature Polish - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Differentiator features that make the configurator feel complete: a visual note grid showing 4x4 key mapping with scale degree colors, PWA support for mobile/offline access with BLE configuration, and a real-time MIDI monitor with CC value visualization.

</domain>

<decisions>
## Implementation Decisions

### Note Grid Visualizer
- CSS Grid 4x4 layout matching the physical T16 key arrangement, rendered as a component in the Dashboard Keyboard tab
- Each key cell shows the MIDI note name (e.g., "C4") and note number, colored by scale degree using HSL hue rotation (root=0°, interval colors distributed across 360°)
- Reuse existing SCALES and NOTE_NAMES arrays from Dashboard.tsx — extract to a shared constants file if not already
- Grid updates reactively when bank or scale changes via ConfigContext — no manual refresh needed
- Keys outside the current scale shown in muted gray, keys in scale shown with their degree color

### PWA & Mobile Access
- Use vite-plugin-pwa with workbox for service worker generation, precache all static assets
- Web App Manifest with T16 branding (name: "T16 Configurator", short_name: "T16", theme_color matching design system primary)
- Installable on mobile Chrome/Edge — "Add to Home Screen" prompt
- Offline-capable: cached app shell works offline, device communication requires live connection (show offline indicator when no MIDI)
- BLE MIDI on mobile via Web Bluetooth API (navigator.bluetooth) — connect to "Topo T16" BLE peripheral, use Web MIDI API for message handling once connected
- BLE connection option in ConnectionContext alongside existing USB MIDI

### MIDI Monitor
- New page accessible from NavBar (/monitor route)
- Listen for CC, Note On, Note Off messages on the active WebMIDI input
- Scrolling message log — most recent at top, max 100 entries, auto-scroll
- CC values shown as horizontal bar visualization (0-127 range, colored by CC number)
- Note messages show velocity as bar width, note name (reuse getNoteNameWithOctave)
- Clear button to reset the log
- Pause/resume toggle for the message stream

### Integration & Navigation
- Note grid visualizer lives as a visual section in the Dashboard Keyboard tab, above or beside the key configuration cards
- MIDI monitor as a new top-level page with NavBar link
- PWA manifest icons generated from T16 logo (if available, otherwise placeholder)

### Claude's Discretion
- Exact HSL color mapping for scale degrees
- Web Bluetooth API integration details (GATT service UUIDs for BLE MIDI)
- MIDI monitor message formatting details
- PWA icon sizes and splash screen configuration
- Whether to use a separate MidiMonitorContext or extend ConnectionContext for raw MIDI message listening

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `editor-tx/src/pages/Dashboard/Dashboard.tsx` — SCALES (19), NOTE_NAMES (12), getNoteNameWithOctave(), VELOCITY_CURVES, CC_NAMES
- `editor-tx/src/design-system/` — Card, Tabs, Button, Tooltip, Skeleton, Toast components with CSS modules
- `editor-tx/src/contexts/ConnectionContext.tsx` — WebMIDI device discovery, input/output management
- `editor-tx/src/contexts/ConfigContext.tsx` — Device config state, bank/scale access
- `editor-tx/src/protocol/sysex.ts` — SysEx constants and helpers
- `editor-tx/src/design-system/tokens.css` — CSS custom properties (colors, spacing, typography)

### Established Patterns
- Pages in src/pages/{PageName}/{PageName}.tsx with {PageName}.module.css
- Design system components with barrel exports (index.ts)
- Context + typed hook pattern (createContext + useX() with throw guard)
- React Router v7 for routing in App.tsx
- CSS Modules for component styles, design tokens via CSS custom properties

### Integration Points
- `editor-tx/src/App.tsx` — Router configuration (add /monitor route)
- `editor-tx/src/components/NavBar/NavBar.tsx` — Navigation links (add Monitor)
- `editor-tx/vite.config.ts` — Vite plugins (add vite-plugin-pwa)
- `editor-tx/package.json` — Dependencies (add vite-plugin-pwa, workbox)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — all recommended approaches accepted. Focus on making these features feel polished and native to the existing design system.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
