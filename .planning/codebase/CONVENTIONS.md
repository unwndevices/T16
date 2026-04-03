# Coding Conventions

**Analysis Date:** 2026-04-03

## Firmware (C++ / PlatformIO)

### Naming Patterns

**Files:**
- PascalCase for all `.hpp` and `.cpp` files: `Configuration.hpp`, `DataManager.hpp`, `MidiProvider.cpp`
- Exception: `pinout.h` uses lowercase (legacy C-style header)
- Header-only classes are common -- most `Libs/` files are `.hpp` with inline implementations

**Classes:**
- PascalCase: `MidiProvider`, `LedManager`, `DataManager`, `TouchSlider`
- Nested enums inside classes use PascalCase: `Key::State`, `Button::State`, `Keyboard::Lut`

**Functions/Methods:**
- PascalCase for public methods: `Init()`, `Update()`, `SetPosition()`, `GetVelocity()`
- PascalCase for free functions: `ProcessKey()`, `OnBankChange()`, `ApplyConfiguration()`
- Private methods also PascalCase: `CalcXY()`, `GenerateLUTs()`, `CombineBuffers()`

**Variables:**
- snake_case for local variables and globals: `slider_mode`, `current_chord`, `note_map`
- snake_case for struct fields: `base_octave`, `midi_trs`, `velocity_curve`
- Underscore-prefixed for private class members: `_config`, `_adc`, `_bank`, `_keys`
- No prefix for some private members (inconsistent): `slew`, `threshold`, `state`

**Constants/Defines:**
- SCREAMING_SNAKE_CASE for `#define`: `PIN_LED`, `NUM_LEDS`, `BANK_AMT`
- Mixed: some `const` use snake_case (`kMatrixWidth` k-prefix), some SCREAMING_SNAKE (`CC_AMT`)

**Enums:**
- SCREAMING_SNAKE_CASE values: `KEYBOARD`, `STRUM`, `XY_PAD`, `IDLE`, `PRESSED`
- Enum types are PascalCase: `Mode`, `SliderMode`

### Comparison with eisei (Target Style)

The eisei repo demonstrates a more mature pattern:
- **Namespaces:** eisei uses `namespace eisei { }` to scope classes -- T16 has no namespaces
- **Member naming:** eisei uses trailing underscore for private members: `isCalibrating_`, `hw_`, `shouldSendUpdate_` -- T16 uses leading underscore `_config` or no prefix at all
- **Header guards:** eisei uses `#pragma once` -- T16 uses `#ifndef`/`#define` guards
- **Initialization:** eisei uses `= default` constructors and brace initialization `{nullptr}` -- T16 uses explicit constructors with member initializer lists
- **Separation of concerns:** eisei has proper `.cpp`/`.hpp` pairs (e.g., `CalibrationManager.cpp`/`.hpp`, `EiseiEngine.cpp`/`.hpp`) -- T16 puts most implementation inline in headers
- **Static members:** eisei avoids defining static members in headers -- T16 defines statics in headers (e.g., `Key::press_threshold` in `Keyboard.hpp`, `Pattern::currentPalette` in `Pattern.hpp`), which will break with multiple translation units

### Code Style

**Formatting:**
- Allman brace style (opening brace on new line)
- 4-space indentation
- No formatter config file detected (no `.clang-format`)

**Linting:**
- No C++ linter configuration detected

### Import Organization

**Firmware pattern (`src/main.cpp`):**
- Includes are interleaved with global variable declarations (non-standard)
- Pattern: `#include` followed immediately by constructing the global object
- No separation between system, library, and project headers

**Recommended order (from eisei):**
1. Project headers
2. Library headers
3. System/STL headers

### Error Handling

**Strategy:** Minimal -- mostly log-and-continue
- `log_d()` macro for debug logging (ESP-IDF)
- No error return codes from most functions
- `Init()` methods return `void` except `TouchSlider::Init()` which returns `bool`
- Hardware failures trigger LED indicators + `ESP.restart()`

### Comments

**Usage:**
- Sparse inline comments, mostly for section markers: `// Data`, `// STARTUP ANIMATION`
- TODO comments present: `// TODO proper message`, `// TODO make it for multiple muxes`
- No doc comments / Doxygen

### Module Design

**Pattern:** Header-only classes with implementation inline
- Most classes in `src/Libs/` are single `.hpp` files
- Only `Adc`, `MidiProvider`, `TouchSlider`, and `Configuration` have separate `.cpp` files
- Free functions defined directly in headers (e.g., `wu_pixel()` in `Pattern.hpp`, `fmap()` in `Keyboard.hpp`)
- Global variables defined in headers (e.g., `leds_plus_safety_pixel` in `LedManager.hpp`)

**Exports:** No module system -- relies on `#include` ordering in `main.cpp`

### Signal/Event Pattern

**Custom Signal class** (`src/Libs/Signal.hpp`):
- Template-based observer pattern: `Signal<Args...>`
- `Connect()` / `Disconnect()` / `DisconnectAll()` / `Emit()`
- Used for button state changes: `onStateChanged.Connect(&ProcessButton)`
- Uses `std::map<uint8_t, Slot>` internally with auto-incrementing IDs
- Helper macros: `CONNECT_SLOT_1`, `CONNECT_SLOT_2` for member function binding

### LED Pattern System

**Inheritance hierarchy:**
- `Pattern` (abstract base) in `src/Libs/Leds/patterns/Pattern.hpp`
- Concrete patterns: `TouchBlur`, `NoBlur`, `Strips`, `Strum`, `QuickSettings`, `WaveTransition`
- Pure virtual: `RunPattern()` returns `bool` (false = transition complete)
- `LedManager` owns `currentPattern` and `nextPattern` pointers
- Pattern transitions via `TransitionToPattern()` which creates `new WaveTransition()`

---

## Web Editor (React / Vite)

### Naming Patterns

**Files:**
- PascalCase for components: `KeyCard.jsx`, `MidiProvider.jsx`, `SelectCard.jsx`
- PascalCase for pages: `Dashboard.jsx`, `Keyboard.jsx`, `Settings.jsx`
- PascalCase for layouts: `RootLayout.jsx`
- Entry point lowercase: `main.jsx`

**Components:**
- PascalCase function names: `SelectCard`, `ToggleCard`, `KeyCard`
- Mix of `export default function` and named `export function` in same files

**Variables/Functions:**
- camelCase for handlers: `handleChange`, `handleToggle`, `onSysex`
- camelCase for state: `isConnected`, `selectedBank`, `ccMessages`

### Comparison with DROP (Target Style)

DROP demonstrates a more mature web pattern:
- **TypeScript:** DROP uses `.tsx` -- T16 uses `.jsx` with `prop-types` runtime validation
- **Flat config ESLint:** DROP uses modern `eslint.config.js` -- T16 uses legacy `.eslintrc.cjs`
- **Directory structure:** DROP uses nested component directories (`components/DeviceBridge/`, `components/UI/SettingsModal/`) -- T16 uses flat `components/` with all files at top level
- **CSS approach:** DROP uses dedicated `.css` files per component -- T16 relies entirely on Chakra UI inline props
- **Design system:** DROP has a `design-system/` module with reusable primitives -- T16 uses Chakra components directly
- **Context pattern:** DROP uses `SettingsContext` with proper provider/hook separation -- T16 exports the raw context object from `MidiProvider.jsx`
- **Services layer:** DROP separates business logic into `services/` -- T16 puts all MIDI logic in the provider component

### Code Style

**Formatting:**
- 4-space indentation
- Single quotes for JS strings
- No semicolons
- No `.prettierrc` detected

**Linting:**
- ESLint with `eslint:recommended`, `plugin:react`, `plugin:react-hooks`
- Config: `editor-tx/.eslintrc.cjs`
- React Refresh plugin for HMR

### Import Organization

**Order (observed in pages):**
1. Chakra UI components
2. Local components
3. React hooks and context

**No path aliases configured.**

### State Management

**Pattern:** React Context via `MidiContext`
- Single monolithic provider: `editor-tx/src/components/MidiProvider.jsx`
- Holds ALL application state: connection, config, sync status, MIDI I/O
- ~500 lines -- does too much (connection, serialization, config management, firmware checking)
- No custom hooks for consuming context -- raw `useContext(MidiContext)` everywhere

### Component Patterns

**Presentational components:**
- Props-based with `PropTypes` validation at bottom of file
- Chakra UI components used exclusively for layout/styling
- `SkeletonLoader` wrapper for loading states
- Sync indicator pattern: colored dot (`Circle`) showing sync status

**Page components:**
- Consume `MidiContext` directly
- Define `handleChange` locally for config mutations
- Inline data arrays (scales, note names) defined at module level

### Theming

**Setup:** Chakra UI `extendTheme` in `editor-tx/src/main.jsx`
- Custom color palettes: `primary`, `secondary`, `accent`
- Fonts: Poppins (headings), Inter (body)
- Full gray scale override for consistent neutrals

---

*Convention analysis: 2026-04-03*
