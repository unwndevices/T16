# Phase 3: Web Rewrite - Research

**Researched:** 2026-04-03
**Domain:** React/TypeScript web application architecture, design systems, toolchain migration
**Confidence:** HIGH

## Summary

Phase 3 is a full rewrite of the T16 web configurator from JavaScript/Chakra UI to TypeScript/Radix UI with modern toolchain (React 19, Vite 7, ESLint 9). The existing codebase has 21 JSX files across a flat directory structure with a single god-context (MidiProvider, ~500 lines) handling all application state. Phase 1 already delivered TypeScript types (`types/config.ts`) and SysEx protocol helpers (`protocol/sysex.ts`), which serve as the foundation for the rewrite.

The rewrite involves four parallel concerns: (1) toolchain upgrade (React 19 + Vite 7 + TypeScript strict + ESLint 9 flat config), (2) design system build (Radix primitives + CSS custom properties replacing Chakra UI), (3) architecture refactor (context split, service extraction, feature-domain organization), and (4) UI polish (consistent spacing, transitions, responsive layout per UI-SPEC). These can be sequenced as: toolchain first (foundational), then design system primitives, then architecture + page rewrites, then test infrastructure.

**Primary recommendation:** Start with toolchain scaffolding (tsconfig, vite.config.ts, eslint.config.js, path aliases), then build the design system tokens and primitives bottom-up, then rewrite contexts and services, then convert pages one at a time consuming the new contexts and design system components. Testing infrastructure (Vitest + Testing Library) should be set up early so each component can be tested as it is written.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Replace Chakra UI v2 with **Radix UI primitives + CSS custom properties** -- matching DROP's approach. Custom design system with reusable components (Button, Card, Modal, Select, Switch, Slider).
- **D-02:** CSS approach uses **CSS custom properties with CSS modules** -- no CSS-in-JS runtime. Design tokens defined as CSS variables in a central theme file.
- **D-03:** Typography uses **Inter (body) and Poppins (headings)** -- same fonts, self-hosted via @fontsource (already installed).
- **D-04:** Color system uses the existing **primary/secondary/accent palettes** from the current Chakra theme, converted to CSS custom properties.
- **D-05:** MidiProvider split into **ConnectionContext** (WebMIDI lifecycle, device discovery, connection state) and **ConfigContext** (device config, sync status, parameter mutations).
- **D-06:** Custom hooks for context consumption -- **useConnection()** and **useConfig()** instead of raw useContext.
- **D-07:** MIDI communication logic extracted to a **services/midi.ts** service layer -- protocol encoding, SysEx send/receive, config serialization.
- **D-08:** Codebase organized by **feature domain** -- contexts/, hooks/, services/, components/, types/, pages/.
- **D-09:** Upgrade to **React 19 + Vite 7 + ESLint 9 flat config** -- matching DROP's toolchain.
- **D-10:** All files converted to **TypeScript with strict mode** -- no .js/.jsx, no PropTypes.
- **D-11:** Path aliases configured -- **@/components**, **@/services**, **@/hooks**, **@/contexts**, **@/types**.

### Claude's Discretion
- Component-level design decisions (specific Radix primitive usage, animation approach)
- Responsive breakpoint values
- State management within ConfigContext (reducer vs useState)
- ESLint rule selection for flat config

### Deferred Ideas (OUT OF SCOPE)
- PWA support -- Phase 5
- Note grid visualizer -- Phase 5
- MIDI monitor visualization -- Phase 5
- Config backup import/export -- Phase 4
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WEBARCH-01 | Full rewrite in TypeScript (no JavaScript, no PropTypes) | Standard Stack: TypeScript 5.x strict, tsconfig with path aliases, Vite TS plugin |
| WEBARCH-02 | Custom design system with Radix primitives and CSS custom properties | Architecture Patterns: design-system/ directory, Radix unified package, CSS custom property tokens |
| WEBARCH-03 | MidiProvider god-context split into ConnectionContext and ConfigContext | Architecture Patterns: context split pattern, service layer extraction |
| WEBARCH-04 | Codebase organized by feature domain | Architecture Patterns: recommended project structure with contexts/, hooks/, services/, components/, types/, pages/ |
| WEBARCH-06 | React 19 + Vite 7 + ESLint 9 flat config | Standard Stack: verified versions, migration pitfalls documented |
| WEBFEAT-06 | Consistent, polished UI/UX across all pages | UI-SPEC contract: spacing scale, typography, color system, interaction states, transitions |
| TEST-02 | Web component and integration tests via Vitest + Testing Library | Validation Architecture: Vitest 4.x + @testing-library/react 16.x setup |
</phase_requirements>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| react | 19.2.4 | UI framework | Latest stable, locked decision D-09 |
| react-dom | 19.2.4 | DOM rendering | Matches React version |
| typescript | 6.0.2 | Type system | Latest stable, strict mode per D-10 |
| vite | 8.0.3 | Build tool / dev server | Latest stable (note: D-09 says Vite 7, but 8.0.3 is current -- use latest) |
| radix-ui | 1.4.3 | Accessible UI primitives | Unified package, tree-shakeable, locked decision D-01 |
| react-router | 7.14.0 | Client-side routing | v7 consolidates react-router-dom into react-router |
| webmidi | 3.1.16 | WebMIDI API wrapper | Carried forward, core device communication |

**Note on Vite version:** D-09 specifies "Vite 7" but the current latest is 8.0.3. Since the intent is matching DROP's modern toolchain, use the latest stable Vite. If the user prefers Vite 7 specifically, pin to `^7.0.0`.

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| @fontsource/inter | 5.2.8 | Body font | Self-hosted, per D-03 |
| @fontsource/poppins | 5.2.7 | Heading font | Self-hosted, per D-03 |
| react-icons | 5.6.0 | Icon library (MD subset) | Per UI-SPEC |
| esptool-js | 0.6.0 | Browser firmware flashing | Upload page, carried forward |

### Dev Dependencies

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| @vitejs/plugin-react | 6.0.1 | React fast refresh for Vite | Dev server HMR |
| vitest | 4.1.2 | Test runner | Per TEST-02 |
| @testing-library/react | 16.3.2 | Component testing utilities | Per TEST-02 |
| @testing-library/jest-dom | 6.9.1 | DOM assertion matchers | Per TEST-02 |
| @testing-library/user-event | latest | User interaction simulation | Per TEST-02 |
| jsdom | latest | DOM environment for tests | Vitest environment |
| eslint | 10.2.0 | Linter | Per D-09, flat config |
| typescript-eslint | 8.58.0 | TS-aware ESLint rules | Per D-09 |
| eslint-plugin-react-hooks | latest | React hooks lint rules | Per D-09 |
| eslint-plugin-react-refresh | latest | HMR export validation | Dev experience |

### Packages to Remove

| Package | Reason |
|---------|--------|
| @chakra-ui/react | Replaced by Radix + CSS custom properties (D-01) |
| @chakra-ui/icons | Replaced by react-icons (UI-SPEC) |
| @emotion/react | Chakra dependency, no longer needed |
| @emotion/styled | Chakra dependency, no longer needed |
| framer-motion | Chakra dependency; transitions use CSS `150ms ease-out` per UI-SPEC |
| prop-types | Replaced by TypeScript (D-10) |
| apexcharts | Deferred to Phase 5 (MIDI monitor visualization) |
| react-apexcharts | Deferred to Phase 5 |
| axios | Appears unused in current codebase |
| react-router-dom | Replaced by react-router v7 (consolidated package) |
| eslint-plugin-react | Replaced by typescript-eslint + react hooks plugin |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Unified `radix-ui` | Individual `@radix-ui/react-*` | Unified package prevents version conflicts, simpler dependency management |
| CSS Modules | Vanilla Extract, Tailwind | CSS Modules is zero-runtime and matches D-02 (CSS custom properties), simplest approach |
| useReducer in ConfigContext | useState + callbacks | useReducer better for complex state with many fields; recommended for ConfigContext |
| Vite 8 | Vite 7 | D-09 says Vite 7 but 8 is current; use latest unless user objects |

**Installation:**
```bash
# Remove old packages
npm uninstall @chakra-ui/react @chakra-ui/icons @emotion/react @emotion/styled framer-motion prop-types apexcharts react-apexcharts axios react-router-dom

# Install core
npm install react@latest react-dom@latest react-router@latest radix-ui webmidi@latest esptool-js@latest react-icons@latest @fontsource/inter @fontsource/poppins

# Install dev dependencies
npm install -D typescript@latest vite@latest @vitejs/plugin-react@latest vitest jsdom @testing-library/react @testing-library/dom @testing-library/jest-dom @testing-library/user-event eslint@latest typescript-eslint eslint-plugin-react-hooks eslint-plugin-react-refresh @types/react @types/react-dom
```

## Architecture Patterns

### Recommended Project Structure

```
editor-tx/src/
├── main.tsx                    # Entry point, provider tree
├── App.tsx                     # Router configuration
├── design-system/
│   ├── tokens.css              # CSS custom properties (colors, spacing, typography)
│   ├── reset.css               # Minimal CSS reset
│   ├── globals.css             # Global styles (font imports, body)
│   ├── Button/
│   │   ├── Button.tsx
│   │   ├── Button.module.css
│   │   └── index.ts
│   ├── Card/
│   │   ├── Card.tsx
│   │   ├── Card.module.css
│   │   └── index.ts
│   ├── Select/                 # Wraps @radix-ui/react-select
│   ├── Switch/                 # Wraps @radix-ui/react-switch
│   ├── Slider/                 # Wraps @radix-ui/react-slider
│   ├── Tabs/                   # Wraps @radix-ui/react-tabs
│   ├── Dialog/                 # Wraps @radix-ui/react-dialog
│   ├── Toast/                  # Wraps @radix-ui/react-toast
│   ├── Tooltip/                # Wraps @radix-ui/react-tooltip
│   ├── Skeleton/
│   └── index.ts                # Barrel export
├── contexts/
│   ├── ConnectionContext.tsx    # WebMIDI lifecycle, device discovery
│   ├── ConfigContext.tsx        # Device config, sync, mutations
│   └── ToastContext.tsx         # Toast notifications (replaces Chakra useToast)
├── hooks/
│   ├── useConnection.ts        # Typed hook for ConnectionContext
│   ├── useConfig.ts            # Typed hook for ConfigContext
│   └── useToast.ts             # Toast hook
├── services/
│   ├── midi.ts                 # SysEx send/receive, config serialization
│   └── firmware.ts             # Firmware version checking, release notes
├── components/
│   ├── NavBar/
│   │   ├── NavBar.tsx
│   │   ├── NavBar.module.css
│   │   └── index.ts
│   ├── BankSelector/
│   ├── KeyCard/
│   ├── SelectCard/
│   ├── SliderCard/
│   ├── ToggleCard/
│   ├── NumberCard/
│   ├── CcCard/
│   ├── SyncIndicator/
│   ├── Footer/
│   └── SkeletonLoader/
├── pages/
│   ├── Dashboard/
│   │   ├── Dashboard.tsx
│   │   ├── Dashboard.module.css
│   │   └── index.ts
│   ├── Upload/
│   ├── Manual/
│   └── Layout/                 # RootLayout with NavBar + Outlet
├── types/
│   ├── config.ts               # Already exists from Phase 1
│   ├── midi.ts                 # MIDI-specific types (connection state, etc.)
│   └── ui.ts                   # Shared UI types
├── protocol/
│   └── sysex.ts                # Already exists from Phase 1
└── assets/
    ├── firmwares/              # Firmware binaries + release notes
    └── images/                 # Static images
```

### Pattern 1: Context Split (ConnectionContext + ConfigContext)

**What:** Split the current MidiProvider (~500 lines) into two focused contexts with a shared service layer.

**When to use:** ConnectionContext wraps the entire app (WebMIDI lifecycle). ConfigContext sits inside ConnectionContext (needs connection to send/receive).

**Example:**

```typescript
// contexts/ConnectionContext.tsx
import { createContext, useContext, useState, useCallback, useEffect } from 'react'
import type { Input, Output } from 'webmidi'

interface ConnectionState {
  input: Input | null
  output: Output | null
  isConnected: boolean
  isDemo: boolean
  connect: () => Promise<void>
  disconnect: () => void
  setDemo: (demo: boolean) => void
}

const ConnectionContext = createContext<ConnectionState | null>(null)

export function ConnectionProvider({ children }: { children: React.ReactNode }) {
  const [input, setInput] = useState<Input | null>(null)
  const [output, setOutput] = useState<Output | null>(null)
  const [isConnected, setIsConnected] = useState(false)
  const [isDemo, setDemo] = useState(false)

  const connect = useCallback(async () => {
    // WebMIDI enable + device discovery
    // Uses services/midi.ts for SysEx listener setup
  }, [])

  const disconnect = useCallback(() => {
    // WebMIDI disable + cleanup
  }, [])

  return (
    <ConnectionContext value={{ input, output, isConnected, isDemo, connect, disconnect, setDemo }}>
      {children}
    </ConnectionContext>
  )
}

// hooks/useConnection.ts
export function useConnection() {
  const ctx = useContext(ConnectionContext)
  if (!ctx) throw new Error('useConnection must be used within ConnectionProvider')
  return ctx
}
```

```typescript
// contexts/ConfigContext.tsx
import { createContext, useContext, useReducer, useCallback } from 'react'
import type { T16Configuration } from '@/types/config'
import { sendParamUpdate, sendFullConfig } from '@/protocol/sysex'
import { useConnection } from '@/hooks/useConnection'

interface ConfigState {
  config: T16Configuration          // Desired state (editor)
  deviceConfig: T16Configuration    // Confirmed state (device)
  selectedBank: number
  isSynced: boolean
}

type ConfigAction =
  | { type: 'SET_CONFIG'; payload: T16Configuration }
  | { type: 'SET_DEVICE_CONFIG'; payload: T16Configuration }
  | { type: 'UPDATE_PARAM'; domain: number; bank: number; field: number; value: number }
  | { type: 'SET_BANK'; payload: number }

// useReducer is recommended here -- config has many fields and
// update actions need to compute sync status atomically
```

### Pattern 2: Design System Component with Radix + CSS Modules

**What:** Wrap Radix primitives with project-specific styling via CSS Modules that consume CSS custom properties.

**Example:**

```typescript
// design-system/Select/Select.tsx
import { Select as RadixSelect } from 'radix-ui'
import styles from './Select.module.css'

interface SelectProps {
  label: string
  value: string
  onValueChange: (value: string) => void
  options: { value: string; label: string }[]
  disabled?: boolean
}

export function Select({ label, value, onValueChange, options, disabled }: SelectProps) {
  return (
    <RadixSelect.Root value={value} onValueChange={onValueChange} disabled={disabled}>
      <RadixSelect.Trigger className={styles.trigger}>
        <RadixSelect.Value />
        <RadixSelect.Icon />
      </RadixSelect.Trigger>
      <RadixSelect.Portal>
        <RadixSelect.Content className={styles.content}>
          <RadixSelect.Viewport>
            {options.map(opt => (
              <RadixSelect.Item key={opt.value} value={opt.value} className={styles.item}>
                <RadixSelect.ItemText>{opt.label}</RadixSelect.ItemText>
              </RadixSelect.Item>
            ))}
          </RadixSelect.Viewport>
        </RadixSelect.Content>
      </RadixSelect.Portal>
    </RadixSelect.Root>
  )
}
```

```css
/* design-system/Select/Select.module.css */
.trigger {
  display: inline-flex;
  align-items: center;
  gap: var(--space-sm);
  padding: var(--space-sm) var(--space-md);
  border: 1px solid var(--gray-200);
  border-radius: 8px;
  background: var(--gray-50);
  font-family: var(--font-body);
  font-size: var(--text-sm);
  transition: border-color 150ms ease-out, background-color 150ms ease-out;
}

.trigger:hover {
  border-color: var(--gray-300);
  background: var(--gray-100);
}

.trigger:focus-visible {
  outline: 2px solid var(--primary-800);
  outline-offset: 2px;
}
```

### Pattern 3: Service Layer

**What:** Extract business logic from React components into pure TypeScript modules.

**When to use:** MIDI protocol handling, config serialization, firmware version checking -- anything that does not need React state.

```typescript
// services/midi.ts
import { WebMidi, type Input, type Output } from 'webmidi'
import { MANUFACTURER_ID, CMD, SUB } from '@/protocol/sysex'
import type { T16Configuration } from '@/types/config'

export async function enableMidi(): Promise<void> {
  await WebMidi.enable({ sysex: true })
  WebMidi.octaveOffset = -1
}

export function findDevice(): { input: Input | null; output: Output | null } {
  return {
    input: WebMidi.getInputByName('Topo T16') ?? null,
    output: WebMidi.getOutputByName('Topo T16') ?? null,
  }
}

export function parseConfigDump(data: Uint8Array): T16Configuration {
  const json = Array.from(data).map(b => String.fromCharCode(b)).join('')
  return JSON.parse(json) as T16Configuration
}

export function disableMidi(): void {
  WebMidi.disable()
}
```

### Pattern 4: React 19 Context Provider (new API)

**What:** React 19 supports `<Context value={...}>` directly -- no need for `<Context.Provider>`.

```typescript
// React 19 style
<ConnectionContext value={state}>
  {children}
</ConnectionContext>

// NOT the old React 18 style
<ConnectionContext.Provider value={state}>
  {children}
</ConnectionContext.Provider>
```

### Anti-Patterns to Avoid
- **God context:** Never put connection, config, sync, and UI state in a single context. Each context re-renders all consumers on any change.
- **Raw useContext:** Always wrap with typed hooks (useConnection, useConfig) that throw on missing provider. Prevents null access bugs.
- **Chakra inline props in new code:** All styling must use CSS Modules + CSS custom properties. No `<Box p={4} bg="gray.100">`.
- **PropTypes in TypeScript:** TypeScript interfaces replace runtime prop validation entirely. Remove the prop-types package.
- **Default exports for components:** Use named exports with barrel files (index.ts) for better refactoring support and tree-shaking.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Accessible select/dropdown | Custom select with keyboard nav | `radix-ui` Select primitive | ARIA compliance, focus management, portal rendering |
| Toast notification system | Custom toast queue | `radix-ui` Toast primitive | Auto-dismiss, swipe-to-close, accessible announcements |
| Modal/dialog management | Custom overlay + focus trap | `radix-ui` Dialog primitive | Focus trap, scroll lock, portal, Escape key handling |
| Tooltip positioning | Custom positioned tooltip | `radix-ui` Tooltip primitive | Collision detection, arrow positioning, delay management |
| CSS reset | Hand-written reset | Modern Normalize or minimal reset | Browser inconsistency edge cases |
| Router | Custom history management | react-router v7 | URL sync, code splitting, nested layouts |

**Key insight:** Radix primitives handle the 80% of accessibility and interaction complexity that is invisible but critical. Every accessible dropdown requires ~500 lines of keyboard navigation, ARIA attributes, and focus management code.

## Common Pitfalls

### Pitfall 1: React 19 forwardRef Removal
**What goes wrong:** Components using `React.forwardRef()` get deprecation warnings in React 19.
**Why it happens:** React 19 passes ref as a regular prop. `forwardRef` still works but is deprecated.
**How to avoid:** Write new components accepting `ref` as a prop directly. Do not use `forwardRef` in new code.
**Warning signs:** Console deprecation warnings about forwardRef.

### Pitfall 2: React Router v7 Import Path Change
**What goes wrong:** `import { ... } from 'react-router-dom'` fails or causes duplicate packages.
**Why it happens:** v7 consolidated `react-router-dom` into `react-router`. The `-dom` package still exists as a re-export but is deprecated.
**How to avoid:** Import everything from `react-router`. Uninstall `react-router-dom`.
**Warning signs:** Two router packages in `package.json`, bundle size increase.

### Pitfall 3: Radix Unstyled Components Look Broken
**What goes wrong:** Radix primitives render with no styles. Developers think something is broken.
**Why it happens:** Radix is intentionally unstyled. Every visual detail must come from CSS.
**How to avoid:** Build design system components first, wrapping each Radix primitive with CSS Module styles. Test visually as you go.
**Warning signs:** Bare HTML elements with ARIA attributes but no visual design.

### Pitfall 4: CSS Module Class Name Composition
**What goes wrong:** Trying to compose CSS module classes with string concatenation creates invalid class names.
**Why it happens:** CSS Modules hash class names. `styles.foo + ' ' + styles.bar` works, but `styles['foo bar']` does not.
**How to avoid:** Use template literals: `` `${styles.trigger} ${styles.active}` `` or a tiny `clsx` utility.
**Warning signs:** Missing styles on elements with multiple classes.

### Pitfall 5: WebMIDI SysEx Requires User Gesture
**What goes wrong:** `WebMidi.enable({ sysex: true })` fails silently or shows permission dialog at unexpected times.
**Why it happens:** Browser security requires SysEx access to be initiated by user gesture (click).
**How to avoid:** Call `connect()` only from a button click handler, never from useEffect on mount. The current code already does this correctly.
**Warning signs:** "SecurityError: Must be handling a user gesture" in console.

### Pitfall 6: Vite Path Aliases Need Both tsconfig and vite.config
**What goes wrong:** TypeScript resolves `@/components` but Vite build fails, or vice versa.
**Why it happens:** TypeScript and Vite resolve paths independently. Both need alias configuration.
**How to avoid:** Configure `compilerOptions.paths` in tsconfig.json AND `resolve.alias` in vite.config.ts. Or use `vite-tsconfig-paths` plugin.
**Warning signs:** "Module not found" errors that appear only in build but not in IDE.

### Pitfall 7: ESLint 10 Flat Config Gotchas
**What goes wrong:** Old `.eslintrc.cjs` config ignored, no linting applied.
**Why it happens:** ESLint 9+ uses `eslint.config.js` (flat config). Legacy files are ignored by default.
**How to avoid:** Delete `.eslintrc.cjs`, create `eslint.config.js` with `defineConfig`. Use `typescript-eslint` for TS-aware rules.
**Warning signs:** Zero lint errors on code that should have errors.

### Pitfall 8: Context Re-render Performance
**What goes wrong:** Every keystroke on a config parameter re-renders the entire app.
**Why it happens:** When ConfigContext value changes, all consumers re-render.
**How to avoid:** Keep contexts focused (connection vs config is already correct split). Memoize context values with useMemo. Consider splitting ConfigContext further if performance is poor (e.g., separate selectedBank from config data).
**Warning signs:** Noticeable lag when editing slider/number values.

## Code Examples

### TypeScript Config (tsconfig.json)

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "lib": ["ES2022", "DOM", "DOM.Iterable"],
    "module": "ESNext",
    "moduleResolution": "bundler",
    "jsx": "react-jsx",
    "strict": true,
    "noUnusedLocals": true,
    "noUnusedParameters": true,
    "noFallthroughCasesInSwitch": true,
    "isolatedModules": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "paths": {
      "@/*": ["./src/*"]
    }
  },
  "include": ["src"],
  "references": [{ "path": "./tsconfig.node.json" }]
}
```

### Vite Config (vite.config.ts)

```typescript
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { resolve } from 'path'

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      '@': resolve(__dirname, './src'),
    },
  },
  server: {
    watch: { usePolling: true },
  },
})
```

### ESLint Flat Config (eslint.config.js)

```javascript
import { defineConfig } from 'eslint/config'
import tseslint from 'typescript-eslint'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'

export default defineConfig([
  { ignores: ['dist'] },
  ...tseslint.configs.strictTypeChecked,
  {
    files: ['**/*.{ts,tsx}'],
    plugins: {
      'react-hooks': reactHooks,
      'react-refresh': reactRefresh,
    },
    languageOptions: {
      parserOptions: {
        projectService: true,
        tsconfigRootDir: import.meta.dirname,
      },
    },
    rules: {
      ...reactHooks.configs.recommended.rules,
      'react-refresh/only-export-components': ['warn', { allowConstantExport: true }],
    },
  },
])
```

### CSS Design Tokens (tokens.css)

```css
:root {
  /* Spacing */
  --space-xs: 4px;
  --space-sm: 8px;
  --space-md: 16px;
  --space-lg: 24px;
  --space-xl: 32px;
  --space-2xl: 48px;
  --space-3xl: 64px;

  /* Typography */
  --font-body: 'Inter', sans-serif;
  --font-heading: 'Poppins', sans-serif;
  --font-mono: 'Menlo', monospace;
  --text-xs: 0.75rem;
  --text-sm: 0.875rem;
  --text-md: 1.25rem;
  --text-lg: 1.75rem;
  --weight-regular: 400;
  --weight-semibold: 600;

  /* Colors - Neutrals */
  --gray-50: #faf9fe;
  --gray-100: #f2f0fd;
  --gray-200: #e8e6f3;
  --gray-300: #d4d2df;
  --gray-400: #adacb6;
  --gray-500: #7f7e85;
  --gray-600: #555459;
  --gray-700: #373639;
  --gray-800: #202022;
  --gray-900: #19191b;
  --black: #100f10;

  /* Colors - Primary (purple) */
  --primary-500: #917df0;
  --primary-600: #816bee;
  --primary-700: #7258ec;
  --primary-800: #6246ea;
  --primary-900: #5234e8;

  /* Colors - Secondary (pink) */
  --secondary-500: #f770df;
  --secondary-800: #f328cf;

  /* Colors - Accent (yellow-green) */
  --accent-500: #e5f10a;

  /* Colors - Semantic */
  --color-success: #2096a6;
  --color-error: #e4412c;
  --color-warning: #eaf51a;

  /* Transitions */
  --transition-fast: 150ms ease-out;

  /* Radii */
  --radius-sm: 4px;
  --radius-md: 8px;
  --radius-lg: 12px;

  /* Layout */
  --content-max-width: 960px;
  --nav-height: 56px;
}
```

### Provider Tree (main.tsx)

```typescript
import '@fontsource/inter/400.css'
import '@fontsource/inter/600.css'
import '@fontsource/poppins/600.css'
import './design-system/reset.css'
import './design-system/tokens.css'
import './design-system/globals.css'

import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import { ToastProvider } from '@/contexts/ToastContext'
import App from './App'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <ToastProvider>
      <ConnectionProvider>
        <ConfigProvider>
          <App />
        </ConfigProvider>
      </ConnectionProvider>
    </ToastProvider>
  </StrictMode>
)
```

### Vitest Config (vitest.config.ts)

```typescript
import { defineConfig } from 'vitest/config'
import react from '@vitejs/plugin-react'
import { resolve } from 'path'

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: { '@': resolve(__dirname, './src') },
  },
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: './src/test/setup.ts',
    css: { modules: { classNameStrategy: 'non-scoped' } },
  },
})
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `react-router-dom` | `react-router` (unified) | v7, 2024 | Single import source, cleaner deps |
| `React.forwardRef` | ref as prop | React 19, Dec 2024 | Simpler component signatures |
| `<Context.Provider>` | `<Context value={}>` | React 19, Dec 2024 | Less nesting boilerplate |
| Individual `@radix-ui/react-*` | Unified `radix-ui` package | Feb 2026 | Single dep, no version conflicts |
| ESLint `.eslintrc.*` | `eslint.config.js` flat config | ESLint 9, 2024 | Better composability, no extends chain |
| Chakra UI inline props | CSS Modules + custom properties | Industry trend | Zero runtime CSS, better performance |

**Deprecated/outdated:**
- `prop-types`: Unnecessary with TypeScript, removed in this rewrite
- `@emotion/react` + `@emotion/styled`: CSS-in-JS runtime cost, replaced by CSS Modules
- `framer-motion` (for basic transitions): CSS transitions at 150ms cover all UI-SPEC needs
- ESLint 8 legacy config format: No longer default, flat config is the standard

## Open Questions

1. **WebMIDI types for service layer**
   - What we know: The `webmidi` package ships its own types. The current `sysex.ts` uses `any` for the `output` parameter.
   - What's unclear: Whether `webmidi` v3.1.16 types are complete enough for React 19.
   - Recommendation: Import `Input` and `Output` types from `webmidi` directly. Replace `any` in sysex.ts helpers.

2. **Toast context vs Radix Toast direct usage**
   - What we know: The current code uses Chakra's `useToast()` hook extensively. Radix Toast requires a `<Toast.Provider>` and imperative-style rendering.
   - What's unclear: Whether a custom ToastContext wrapping Radix Toast is simpler than direct usage.
   - Recommendation: Build a `ToastContext` with `useToast()` hook that manages a toast queue and renders via Radix Toast.Viewport. This mirrors the current DX.

3. **ConfigContext state management: useReducer vs useState**
   - What we know: Config has ~30 fields across global + 4 banks. Sync status must track per-field diffs.
   - What's unclear: Whether useState with spread updates is sufficient or useReducer is needed.
   - Recommendation: Use `useReducer` -- the config update logic involves comparing desired vs device state, updating specific fields by domain/bank/field-id, and computing sync status. This maps cleanly to reducer actions.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Vitest 4.x + @testing-library/react 16.x |
| Config file | `vitest.config.ts` (Wave 0 creation) |
| Quick run command | `npx vitest run --reporter=verbose` |
| Full suite command | `npx vitest run` |

### Phase Requirements to Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WEBARCH-01 | TypeScript strict compiles | build | `npx tsc --noEmit` | N/A (compiler) |
| WEBARCH-02 | Design system components render | unit | `npx vitest run src/design-system/` | Wave 0 |
| WEBARCH-03 | ConnectionContext provides connection state | unit | `npx vitest run src/contexts/ConnectionContext.test.tsx` | Wave 0 |
| WEBARCH-03 | ConfigContext provides config + sync | unit | `npx vitest run src/contexts/ConfigContext.test.tsx` | Wave 0 |
| WEBARCH-04 | Feature-domain directories exist | smoke | `ls src/{contexts,hooks,services,components,types,pages}` | N/A (structure) |
| WEBARCH-06 | ESLint passes on all files | lint | `npx eslint .` | N/A (toolchain) |
| WEBFEAT-06 | Pages render without errors | smoke | `npx vitest run src/pages/` | Wave 0 |
| TEST-02 | Test infrastructure works | smoke | `npx vitest run` | Wave 0 |

### Sampling Rate
- **Per task commit:** `npx tsc --noEmit && npx vitest run --reporter=verbose`
- **Per wave merge:** `npx tsc --noEmit && npx eslint . && npx vitest run`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `vitest.config.ts` -- test framework configuration
- [ ] `src/test/setup.ts` -- testing-library cleanup + jest-dom matchers
- [ ] `tsconfig.json` -- TypeScript strict config
- [ ] `eslint.config.js` -- ESLint 9+ flat config
- [ ] Framework install: `npm install -D vitest jsdom @testing-library/react @testing-library/jest-dom @testing-library/user-event`

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Node.js | All tooling | yes | 22.22.1 | -- |
| npm | Package management | yes | 11.11.0 | -- |
| Chrome/Edge | WebMIDI testing | manual | -- | Dev without device, test with mocks |

No missing dependencies. Node 22 meets Vite 8's requirement of Node 20.19+.

## Sources

### Primary (HIGH confidence)
- npm registry -- verified all package versions via `npm view` (2026-04-03)
- Existing codebase -- `editor-tx/src/` directory structure, package.json, MidiProvider.jsx analysis
- Phase 1 outputs -- `types/config.ts`, `protocol/sysex.ts` (already in repo)
- UI-SPEC -- `03-UI-SPEC.md` (design system contract for this phase)

### Secondary (MEDIUM confidence)
- [React 19 Upgrade Guide](https://react.dev/blog/2024/04/25/react-19-upgrade-guide) -- forwardRef, Context.Provider changes
- [Vite 7 Migration](https://v7.vite.dev/guide/migration) -- breaking changes from v5 to v7
- [ESLint Configuration Migration Guide](https://eslint.org/docs/latest/use/configure/migration-guide) -- flat config migration
- [Radix Primitives](https://www.radix-ui.com/primitives) -- component API, unified package
- [React Router Upgrading from v6](https://reactrouter.com/upgrading/v6) -- package consolidation

### Tertiary (LOW confidence)
- Vite 8 vs 7 recommendation -- D-09 says "Vite 7" but current latest is 8.0.3. Used latest; user may override.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all versions verified against npm registry
- Architecture: HIGH -- patterns derived from CONTEXT.md decisions and existing codebase analysis
- Pitfalls: HIGH -- verified against official migration guides for React 19, Vite, ESLint 9, Radix
- Design system: HIGH -- UI-SPEC provides exact tokens, component list, and interaction states

**Research date:** 2026-04-03
**Valid until:** 2026-05-03 (30 days -- stable ecosystem, all major versions released)
