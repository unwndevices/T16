# Technology Stack

**Project:** T16 Firmware Refactor + Web Configurator Rewrite
**Researched:** 2026-04-03

## Recommended Stack

### Firmware Core

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| PlatformIO | latest | Build system | Constraint -- already in use, custom board `unwn_s3` defined here, no migration | HIGH |
| Arduino Framework | ESP32 v2.x | Runtime | Constraint -- staying on PlatformIO/Arduino. v2.x is the stable PlatformIO-supported line (uses ESP-IDF 4.x). v3.x requires pioarduino fork and has FastLED RMT driver issues | HIGH |
| C++17 | - | Language standard | Already enabled via Arduino ESP32. Use namespaces, constexpr, structured bindings -- follow eisei patterns | HIGH |

### Firmware Libraries

| Library | Version | Purpose | Why | Confidence |
|---------|---------|---------|-----|------------|
| ArduinoJson | ^7.0.3 | Config serialization | Already in use, v7 has superior memory model (JsonDocument auto-sizes). Keep current | HIGH |
| FastLED | ^3.6.0 | LED control | Already in use. Pin to 3.6.x -- 3.9.x has ESP32-S3 RMT driver regressions with Arduino Core 2.x. Upgrade only after validating on hardware | HIGH |
| Adafruit TinyUSB | 3.3.0 | USB MIDI device class | Already in use, stable | HIGH |
| NimBLE-Arduino | ^1.4.0 | BLE stack | Already in use. v2.x exists with API refactoring but requires Arduino Core 3.x -- stay on 1.4.x until Core migration | HIGH |
| BLE-MIDI | ^2.2.0 | BLE MIDI transport | Already in use, depends on NimBLE 1.x | HIGH |
| arduino_midi_library | feat/v5.1.0 | MIDI protocol | Already in use from feature branch. Monitor for stable 5.x release | MEDIUM |
| StreamUtils | ^1.8.0 | Stream buffering | Already in use for JSON I/O, keep | HIGH |
| Unity | PlatformIO built-in | Unit testing | PlatformIO's native test framework. Supports `[env:native]` for host-side testing of pure logic (config parsing, protocol encoding, state machines) without hardware | HIGH |

### Web Core

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| TypeScript | ~5.8.3 | Language | Match DROP. Strict mode enabled. Eliminates prop-types, catches SysEx byte errors at compile time | HIGH |
| React | ^19.1.0 | UI framework | Match DROP. React 19 is stable (19.0.4 latest patch). Needed for Radix compatibility | HIGH |
| Vite | ^7.0.0 | Bundler | Match DROP (already on Vite 7). Vite 8 exists (uses Rolldown) but is too new -- DROP hasn't migrated either. Vite 7 is the stable production choice | HIGH |
| @vitejs/plugin-react | ^4.5.2 | React fast refresh | Match DROP version | HIGH |
| React Router | ^7.13.1 | Client routing | T16 has 3 routes (dashboard, upload, manual). v7 is stable, simplified imports (no separate react-router-dom package). DROP doesn't use routing but T16 needs it | HIGH |

### Web Design System

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Custom design system | - | UI primitives | Match DROP architecture. Port DROP's `design-system/` (Button, Card, Modal, Input, Select, StatusIndicator, Toast, layouts) to T16 | HIGH |
| @radix-ui/react-slider | ^1.3.5 | Slider primitive | Match DROP. Radix provides accessible, unstyled primitives. Slider is critical for T16's CC/parameter editing | HIGH |
| @radix-ui/react-select | latest | Select primitive | Accessible dropdown for bank/mode/scale selection | HIGH |
| @radix-ui/react-dialog | latest | Dialog primitive | For confirmation dialogs, config backup/restore modals | HIGH |
| @radix-ui/react-tabs | latest | Tab primitive | For multi-bank configuration switching | HIGH |
| CSS custom properties | - | Theming | Match DROP's `tokens/index.css` approach. No CSS-in-JS runtime. Design tokens as CSS variables | HIGH |
| lucide-react | ^1.7.0 | Icons | Match DROP. Tree-shakable, TypeScript-native. Replaces react-icons (which bundles entire icon sets) | HIGH |
| @fontsource/inter | ^5.0.17 | Typography | Already in use, keep. Self-hosted for offline/PWA use | HIGH |

### Web Device Communication

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| webmidi | ^3.1.14 | Web MIDI API | Already in use, bump to latest 3.x. Core device communication -- SysEx config sync | HIGH |
| esptool-js | ^0.5.7 | Browser firmware flashing | Bump from 0.4.3 to match DROP's approach. Latest stable with better error handling | HIGH |

### Web PWA

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| vite-plugin-pwa | ^1.2.0 | PWA support | Match DROP. Enables offline caching, install prompt, mobile BLE configuration use case | HIGH |

### Web Testing

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Vitest | ^4.1.2 | Test runner | Native Vite integration, no separate config. Jest-compatible API. Standard for Vite projects in 2026 | HIGH |
| @testing-library/react | latest | Component testing | Standard for React component tests, works with Vitest | HIGH |
| @testing-library/user-event | latest | Interaction testing | Realistic user event simulation for UI tests | MEDIUM |

### Web Linting / Quality

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| ESLint | ^9.29.0 | Linting | Match DROP. v9 flat config is the standard, v10 exists but too new. Legacy eslintrc is deprecated | HIGH |
| typescript-eslint | ^8.34.1 | TypeScript lint rules | Match DROP version. Uses `defineConfig()` helper | HIGH |
| eslint-plugin-react-hooks | ^5.2.0 | React hooks rules | Match DROP | HIGH |
| eslint-plugin-react-refresh | ^0.4.20 | HMR validation | Match DROP | HIGH |

### Infrastructure

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| GitHub Pages | - | Web hosting | Constraint -- already in use, free, CI-friendly. Configure with `base` path in Vite | HIGH |
| GitHub Actions | - | CI pipeline | Build + lint + test for both firmware and web. PlatformIO has official GitHub Action | HIGH |
| Node.js | >=20.19 | Web dev runtime | Vite 7 requirement. Pin via `.nvmrc` | HIGH |

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| UI framework | Custom design system + Radix | Chakra UI v3 | Chakra v2 is EOL. v3 exists but is a complete rewrite (Ark UI + Panda CSS). Would introduce new abstractions rather than matching DROP. Custom system gives full control and cross-project consistency |
| UI framework | Custom design system + Radix | shadcn/ui | Opinionated Tailwind dependency. T16 and DROP use CSS custom properties, not utility classes. shadcn would introduce style system mismatch |
| Icons | lucide-react | react-icons | react-icons bundles entire icon families even with tree-shaking. lucide-react is individually tree-shakable and already used in DROP |
| Bundler | Vite 7 | Vite 8 | Vite 8 (Rolldown-based) released recently. DROP hasn't migrated. Too early for production -- wait for ecosystem maturity |
| Test framework | Vitest | Jest | Jest requires separate transform config for TypeScript/ESM. Vitest shares Vite config natively. No reason to use Jest in a Vite project |
| State management | React Context (split) | Zustand / Jotai | T16's state is device-bound (config lives on the ESP32). The problem isn't Context vs. external stores -- it's that one context does everything. Split into ConnectionContext, ConfigContext, SyncContext. No external state library needed |
| Routing | React Router v7 | TanStack Router | React Router v7 is battle-tested for simple SPA routing. TanStack Router adds type-safe routing overhead for only 3 routes. Overkill |
| BLE library | NimBLE 1.4.x | NimBLE 2.x | v2.x requires Arduino Core 3.x which isn't stable on PlatformIO. Stay on 1.4.x |
| FastLED | 3.6.x | 3.9.x | 3.9.x has documented RMT driver regressions on ESP32-S3 with Arduino Core 2.x. Not worth the risk for existing working LED code |
| Charting | Remove (apexcharts) | uplot | T16 uses apexcharts for MIDI monitor. Evaluate whether the MIDI monitor feature is needed. If yes, uplot (used in DROP) is lighter. If not, remove entirely |
| HTTP client | Remove | axios | axios appears unused in T16 editor. Remove it |
| Prop validation | TypeScript | prop-types | TypeScript makes prop-types redundant. Remove prop-types |

## Firmware Architecture Libraries (New Patterns, Not NPM)

These aren't external libraries -- they're architectural patterns to implement, following eisei.

| Pattern | Inspired By | Purpose |
|---------|-------------|---------|
| TaskScheduler | eisei `TaskScheduler.hpp` | Replace flat `loop()` with scheduled tasks at different intervals (LED render at 30fps, input poll at 1kHz, MIDI read every loop) |
| Service classes | eisei `CommService`, `PresetManager`, `CalibrationManager` | Extract from main.cpp: `MidiService`, `InputService`, `LedService`, `ConfigService`, `SliderService` |
| Command handlers | eisei `serial_commands/` directory | Structured SysEx command dispatch with command ID enum, replacing magic byte matching |
| Signal (keep) | Already in T16 `Signal.hpp` | Qt-style signal/slot for decoupled event handling. Already good -- keep and extend |
| Namespaces | eisei `namespace eisei` | Wrap T16 code in `namespace t16` to avoid global pollution |

## Installation

### Web (new project)

```bash
# Core
npm install react react-dom react-router webmidi esptool-js lucide-react @fontsource/inter

# Radix primitives (install as needed)
npm install @radix-ui/react-slider @radix-ui/react-select @radix-ui/react-dialog @radix-ui/react-tabs

# PWA
npm install vite-plugin-pwa

# Dev dependencies
npm install -D typescript @types/react @types/react-dom @vitejs/plugin-react vite eslint @eslint/js typescript-eslint eslint-plugin-react-hooks eslint-plugin-react-refresh globals

# Testing
npm install -D vitest @testing-library/react @testing-library/user-event jsdom
```

### Firmware (platformio.ini changes)

```ini
; Add native test environment
[env:native]
platform = native
build_flags = -std=c++17
test_framework = unity
lib_ignore =
    Adafruit TinyUSB Library
    NimBLE-Arduino
    BLE-MIDI
    FastLED
```

### Vite Config (match DROP patterns)

```typescript
// vite.config.ts
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { VitePWA } from 'vite-plugin-pwa'
import path from 'path'

export default defineConfig({
  plugins: [react(), VitePWA({ registerType: 'prompt' })],
  resolve: {
    alias: {
      '@/design-system': path.resolve(__dirname, './src/design-system'),
      '@/components': path.resolve(__dirname, './src/components'),
      '@/services': path.resolve(__dirname, './src/services'),
      '@/contexts': path.resolve(__dirname, './src/contexts'),
      '@/hooks': path.resolve(__dirname, './src/hooks'),
      '@/utils': path.resolve(__dirname, './src/utils'),
      '@/types': path.resolve(__dirname, './src/types'),
    }
  },
  base: process.env.NODE_ENV === 'production' ? '/T16/' : '/',
  build: {
    outDir: 'dist',
    sourcemap: false,
    rollupOptions: {
      output: {
        manualChunks: {
          vendor: ['react', 'react-dom'],
        }
      }
    }
  }
})
```

### tsconfig.app.json (match DROP)

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
    "noEmit": true,
    "baseUrl": ".",
    "paths": {
      "@/design-system/*": ["src/design-system/*"],
      "@/components/*": ["src/components/*"],
      "@/services/*": ["src/services/*"],
      "@/contexts/*": ["src/contexts/*"],
      "@/hooks/*": ["src/hooks/*"],
      "@/utils/*": ["src/utils/*"],
      "@/types/*": ["src/types/*"]
    }
  },
  "include": ["src"]
}
```

## Key Decisions Summary

| Decision | Rationale |
|----------|-----------|
| Match DROP's exact dependency versions | Cross-project consistency. Known working combinations. Reduces cognitive overhead when switching between T16 and DROP codebases |
| Stay on Arduino Core 2.x / NimBLE 1.4.x / FastLED 3.6.x | PlatformIO stable support. FastLED 3.9.x has ESP32-S3 RMT regressions. NimBLE 2.x needs Core 3.x. Upgrade all three together later as a dedicated effort |
| Split Context, not replace with external store | The problem is a god-context, not React Context itself. Device-bound state (config on ESP32) doesn't benefit from Zustand/Jotai patterns |
| Port DROP design system, not adopt Chakra v3 or shadcn | Custom system matches the existing unwn design language. No vendor dependency. Full control over components |
| Remove: axios, prop-types, @emotion/*, framer-motion, @chakra-ui/* | All are Chakra v2 dependencies or unused. TypeScript replaces prop-types. CSS custom properties replace CSS-in-JS |
| Add PlatformIO `[env:native]` | Enables host-side unit tests for config parsing, SysEx protocol encoding, state machine logic -- no hardware needed |

## Sources

- [React Versions](https://react.dev/versions) -- React 19.0.4 latest patch (Jan 2026)
- [Vite Releases](https://vite.dev/releases) -- Vite 7 stable, Vite 8 uses Rolldown
- [Vite 7 Announcement](https://vite.dev/blog/announcing-vite7) -- Requires Node.js 20.19+
- [Radix Primitives Releases](https://www.radix-ui.com/primitives/docs/overview/releases) -- radix-ui 1.4.3
- [ESLint v10 Release](https://www.infoq.com/news/2026/04/eslint-10-release/) -- v10 removes legacy eslintrc, v9 is current stable
- [typescript-eslint](https://typescript-eslint.io/packages/typescript-eslint/) -- defineConfig() helper
- [Vitest](https://vitest.dev/) -- v4.1.2 latest
- [esptool-js npm](https://www.npmjs.com/package/esptool-js) -- v0.5.7
- [webmidi npm](https://www.npmjs.com/package/webmidi) -- v3.1.14
- [lucide-react npm](https://www.npmjs.com/package/lucide-react) -- v1.7.0
- [vite-plugin-pwa npm](https://www.npmjs.com/package/vite-plugin-pwa) -- v1.2.0
- [React Router npm](https://www.npmjs.com/package/react-router) -- v7.13.1
- [NimBLE-Arduino GitHub](https://github.com/h2zero/NimBLE-Arduino) -- v2.x needs Core 3.x
- [FastLED ESP32-S3 RMT issues](https://github.com/fastled/fastled/issues/2147) -- 3.9.x regressions with Core 2.x
- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html) -- Unity framework, native env
- [PlatformIO Espressif32](https://docs.platformio.org/en/latest/platforms/espressif32.html) -- v2.x stable, v3.x via pioarduino
- DROP reference repo (`/home/unwn/git/DROP/package.json`) -- exact versions verified
- eisei reference repo (`/home/unwn/git/unwn/eisei/daisy/`) -- architecture patterns verified
