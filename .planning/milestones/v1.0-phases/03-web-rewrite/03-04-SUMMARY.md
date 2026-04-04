---
phase: 03-web-rewrite
plan: 04
subsystem: ui
tags: [react, typescript, radix, css-modules, dashboard, tabs, responsive, design-system]

requires:
  - phase: 03-02
    provides: Design system primitives (Button, Card, Select, Switch, Slider, Tabs, Dialog, Tooltip, Skeleton)
  - phase: 03-03
    provides: Context providers (useConnection, useConfig, useToast) and MIDI service layer
provides:
  - App shell (NavBar, Footer, SyncIndicator, root Layout) with responsive design
  - Dashboard page with 4 tabbed sections (Keyboard, Custom Scales, CC Mapping, Settings)
  - 7 config card components (BankSelector, SelectCard, SliderCard, ToggleCard, NumberCard, KeyCard, CcCard)
  - Empty state with UI-SPEC copywriting for disconnected users
affects: [03-05, 03-06]

tech-stack:
  added: []
  patterns:
    - "Config card components wrap design system primitives with per-parameter sync dots"
    - "Dashboard uses sub-components per tab (KeyboardTab, CustomScalesTab, etc.) to keep file organized"
    - "All param updates use DOMAIN/FIELD constants from @/protocol/sysex"
    - "Responsive grid: 1 column mobile, 2 columns tablet+"

key-files:
  created:
    - editor-tx/src/components/NavBar/NavBar.tsx
    - editor-tx/src/components/Footer/Footer.tsx
    - editor-tx/src/components/SyncIndicator/SyncIndicator.tsx
    - editor-tx/src/pages/Layout/Layout.tsx
    - editor-tx/src/pages/Dashboard/Dashboard.tsx
    - editor-tx/src/components/BankSelector/BankSelector.tsx
    - editor-tx/src/components/SelectCard/SelectCard.tsx
    - editor-tx/src/components/SliderCard/SliderCard.tsx
    - editor-tx/src/components/ToggleCard/ToggleCard.tsx
    - editor-tx/src/components/NumberCard/NumberCard.tsx
    - editor-tx/src/components/KeyCard/KeyCard.tsx
    - editor-tx/src/components/CcCard/CcCard.tsx
  modified: []

key-decisions:
  - "Used react-router v7 imports (not react-router-dom) matching package.json"
  - "Old JSX files shadow new directories for module resolution; used explicit paths to avoid conflicts during migration"
  - "Custom scale updates use convention-based field IDs (100+ for scale1, 116+ for scale2) pending proper protocol support"
  - "Dashboard sub-components (KeyboardTab, SettingsTab, etc.) keep single file manageable"

patterns-established:
  - "Config card pattern: wrap design-system primitive in Card with label, sync dot, and typed props"
  - "Tab sub-component pattern: each tab section is a private component in the Dashboard file"
  - "Responsive layout: CSS grid with 1fr mobile, 1fr 1fr tablet+ media query at 768px"

requirements-completed: [WEBFEAT-06]

duration: 7min
completed: 2026-04-03
---

# Phase 03 Plan 04: Layout Shell and Dashboard Summary

**App shell (NavBar, Footer, Layout) and Dashboard with 4 tabbed config sections (Keyboard, Scales, CC, Settings) consuming design system and context hooks**

## Performance

- **Duration:** 7 min
- **Started:** 2026-04-03T22:15:14Z
- **Completed:** 2026-04-03T22:22:14Z
- **Tasks:** 3
- **Files modified:** 36

## Accomplishments
- Complete app shell: sticky NavBar with icon nav links, tooltips, sync indicator, and connect/disconnect button; Footer with brand text; root Layout with max-width 960px content area
- 7 config card components (BankSelector, SelectCard, SliderCard, ToggleCard, NumberCard, KeyCard, CcCard) wrapping design system primitives with typed props and per-parameter sync dots
- Dashboard page with 4 functional tabs matching all existing features: Keyboard config, Custom Scale editing, CC Mapping, and Settings with calibration/factory reset dialogs
- Empty state with UI-SPEC copywriting and skeleton placeholders when disconnected
- All param updates use correct DOMAIN and FIELD constants from @/protocol/sysex
- Responsive grid layout matching UI-SPEC (single column mobile, two-column desktop)

## Task Commits

Each task was committed atomically:

1. **Task 1: Create Layout shell (NavBar, Footer, SyncIndicator, RootLayout)** - `7e47ceb` (feat)
2. **Task 2: Create config card components** - `9123270` (feat)
3. **Task 3: Create Dashboard page with 4 tabbed sections** - `393984b` (feat)

## Files Created/Modified
- `editor-tx/src/components/NavBar/NavBar.tsx` - Sticky nav with icon links, tooltips, sync indicator, connect button
- `editor-tx/src/components/NavBar/NavBar.module.css` - NavBar styling with responsive breakpoints
- `editor-tx/src/components/Footer/Footer.tsx` - Static footer with brand text
- `editor-tx/src/components/SyncIndicator/SyncIndicator.tsx` - 3-state dot (synced/unsynced/disconnected)
- `editor-tx/src/pages/Layout/Layout.tsx` - Root layout with NavBar + Outlet + Footer
- `editor-tx/src/components/BankSelector/BankSelector.tsx` - 4 bank buttons (A-D)
- `editor-tx/src/components/SelectCard/SelectCard.tsx` - Card + Select with sync dot
- `editor-tx/src/components/SliderCard/SliderCard.tsx` - Card + Slider with value display
- `editor-tx/src/components/ToggleCard/ToggleCard.tsx` - Card + Switch toggle
- `editor-tx/src/components/NumberCard/NumberCard.tsx` - Card with increment/decrement
- `editor-tx/src/components/KeyCard/KeyCard.tsx` - Key display card for 4x4 grid
- `editor-tx/src/components/CcCard/CcCard.tsx` - CC mapping card with channel/ID controls
- `editor-tx/src/pages/Dashboard/Dashboard.tsx` - Dashboard with 4 tabbed sections
- `editor-tx/src/pages/Dashboard/Dashboard.module.css` - Responsive grid layouts

## Decisions Made
- Used `react-router` v7 imports (not `react-router-dom`) to match the actual installed package
- Used explicit file paths (`@/components/NavBar/NavBar`) instead of barrel imports to avoid shadowing by old `.jsx` files during migration
- Dashboard sub-components (KeyboardTab, SettingsTab, etc.) defined as private functions in the same file to keep tab logic co-located
- Custom scale field IDs use convention-based encoding (100+ for scale1, 116+ for scale2) as a placeholder pending proper protocol support

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed react-router-dom import to react-router**
- **Found during:** Task 1 (NavBar/Layout creation)
- **Issue:** Plan specified react-router-dom imports, but package.json has react-router v7 (which bundles DOM APIs)
- **Fix:** Changed imports from 'react-router-dom' to 'react-router'
- **Files modified:** NavBar.tsx, Layout.tsx
- **Verification:** TypeScript compiles without errors
- **Committed in:** 7e47ceb (Task 1 commit)

**2. [Rule 3 - Blocking] Used explicit paths to avoid old JSX file shadowing**
- **Found during:** Task 1 (Layout creation)
- **Issue:** `@/components/NavBar` resolves to old NavBar.jsx instead of NavBar/index.ts
- **Fix:** Import from `@/components/NavBar/NavBar` explicitly
- **Files modified:** Layout.tsx, Dashboard.tsx
- **Verification:** TypeScript compiles without errors
- **Committed in:** 7e47ceb, 393984b

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes necessary for TypeScript compilation during migration period. No scope creep.

## Issues Encountered
None beyond the auto-fixed deviations above.

## Known Stubs
- `Dashboard.tsx` calibration button onClick: sends no actual calibration SysEx command (placeholder `setCalibrationOpen(false)`) -- will be wired in Phase 4 when SysEx calibration protocol is integrated
- `Dashboard.tsx` factory reset button onClick: sends no actual reset SysEx command -- same as above
- `CustomScalesTab` uses convention-based field IDs (100+/116+) for custom scale note updates -- proper protocol mapping needed in Phase 4

## Next Phase Readiness
- Layout shell and Dashboard are ready for router integration (Plan 05)
- All config card components available for reuse in other pages
- Design system, contexts, and Dashboard pages form a complete editing UI

---
*Phase: 03-web-rewrite*
*Completed: 2026-04-03*
