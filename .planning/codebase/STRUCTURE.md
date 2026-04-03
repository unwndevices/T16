# Codebase Structure

**Analysis Date:** 2026-04-03

## Directory Layout

```
T16/
├── src/                        # ESP32 firmware (PlatformIO, Arduino framework)
│   ├── main.cpp                # Application entry point + all orchestration logic
│   ├── pinout.h                # Hardware pin definitions (REV_A / REV_B variants)
│   ├── Configuration.hpp/cpp   # Config data structures + save/load
│   ├── Performance.hpp         # Loop rate / core load monitoring
│   ├── Scales.hpp              # Musical scale definitions + note mapping
│   └── Libs/                   # Hardware abstraction libraries
│       ├── Adc.hpp/cpp         # ADC reading with mux, calibration, filtering
│       ├── Button.hpp          # Debounced button with click/long-press state machine
│       ├── DataManager.hpp     # LittleFS JSON persistence
│       ├── Keyboard.hpp        # 16-key FSR keyboard with velocity/aftertouch
│       ├── MidiProvider.hpp/cpp # Multi-transport MIDI (USB, BLE, TRS)
│       ├── Ota.hpp             # OTA update support
│       ├── Signal.hpp          # Signal/slot event system
│       ├── Timer.hpp           # Simple timer utility
│       ├── TouchSlider.hpp/cpp # 7-sensor capacitive touch slider
│       ├── Types.hpp           # Shared type definitions
│       └── Leds/               # LED subsystem
│           ├── LedManager.hpp  # LED orchestration (4x4 matrix + 7 slider + 1 status)
│           └── patterns/       # LED animation patterns
│               ├── Pattern.hpp       # Abstract base class
│               ├── NoBlur.hpp        # Keyboard mode pattern
│               ├── TouchBlur.hpp     # XY pad mode pattern
│               ├── Strips.hpp        # Strips mode pattern
│               ├── Strum.hpp         # Strum mode pattern
│               ├── QuickSettings.hpp # Quick settings UI pattern
│               ├── WaveTransition.hpp # Mode transition animation
│               ├── Droplet.hpp       # Unused/decorative pattern
│               ├── Sea.hpp           # Unused/decorative pattern
│               └── Sea2.hpp          # Unused/decorative pattern
├── editor-tx/                  # Web configurator (React + Vite)
│   ├── package.json            # Dependencies: React 18, Chakra UI v2, WebMidi.js
│   ├── vite.config.js          # Vite build config
│   ├── index.html              # SPA entry HTML
│   ├── .eslintrc.cjs           # ESLint config
│   ├── .htaccess               # Apache hosting rules
│   └── src/
│       ├── main.jsx            # React mount + Chakra theme + MidiProvider wrapper
│       ├── App.jsx             # React Router setup (3 routes)
│       ├── layouts/
│       │   └── RootLayout.jsx  # NavBar + Outlet
│       ├── pages/
│       │   ├── Dashboard.jsx   # Main config view (tabs: Keyboard, Scales, CC, Settings)
│       │   ├── Keyboard.jsx    # Bank-specific keyboard settings
│       │   ├── Scales.jsx      # Custom scale editor
│       │   ├── ControlChange.jsx # CC mapping editor
│       │   ├── Settings.jsx    # Global device settings
│       │   ├── Upload.jsx      # Firmware update via esptool-js
│       │   ├── QuickStart.jsx  # User manual / quickstart guide
│       │   └── TopoT16.jsx    # Device visualization
│       ├── components/
│       │   ├── MidiProvider.jsx    # God-context: MIDI connection, config state, sync
│       │   ├── SerialProvider.jsx  # Web Serial API context (unused in current flow)
│       │   ├── NavBar.jsx          # Top navigation bar
│       │   ├── AppNav.jsx          # App navigation component
│       │   ├── Footer.jsx          # Page footer
│       │   ├── BankSelector.jsx    # Bank switching UI
│       │   ├── MidiMonitor.jsx     # Live MIDI message display
│       │   ├── Monitor.jsx         # Connection monitor
│       │   ├── SkeletonLoader.jsx  # Loading placeholder
│       │   ├── TopoT16Svg.jsx      # SVG device illustration
│       │   ├── KeyCard.jsx         # Keyboard config card
│       │   ├── CcCard.jsx          # CC config card
│       │   ├── SelectCard.jsx      # Dropdown select UI card
│       │   ├── SliderCard.jsx      # Slider input UI card
│       │   ├── ToggleCard.jsx      # Toggle switch UI card
│       │   ├── NumberCard.jsx      # Number input UI card
│       │   ├── CustomInputs.jsx    # Custom form inputs
│       │   ├── EditableScaleDisplay.jsx # Editable scale note grid
│       │   └── ScaleDisplay.jsx    # Read-only scale display
│       └── assets/
│           ├── firmwares/          # Bundled firmware binaries + release notes
│           │   ├── release_notes.json
│           │   ├── t16_v1.0.0.bin
│           │   ├── t16_v1.0.1.bin
│           │   ├── t16_v1.0.2.bin
│           │   └── t16_v1.0.3.bin
│           └── *.webp, *.svg       # UI images and icons
├── data/                       # LittleFS filesystem data (uploaded to device flash)
│   ├── calibration_data.json   # Key calibration min/max values
│   └── configuration_data.json # Device configuration
├── include/                    # PlatformIO include directory (empty, has README)
├── lib/                        # PlatformIO lib directory (empty, has README)
├── test/                       # PlatformIO test directory (empty, has README)
├── platformio.ini              # PlatformIO project config (ESP32-S3, custom board)
├── .github/workflows/
│   └── deploy.yml              # GitHub Actions deployment
├── .vscode/
│   ├── extensions.json         # Recommended VS Code extensions
│   ├── settings.json           # VS Code settings
│   └── tasks.json              # VS Code tasks
├── T16.code-workspace          # VS Code workspace file
└── .gitignore
```

## Directory Purposes

**`src/`:**
- Purpose: ESP32 firmware source code
- Contains: C++ source files, header-only libraries
- Key files: `main.cpp` (all orchestration), `Configuration.hpp/cpp` (data model)

**`src/Libs/`:**
- Purpose: Reusable hardware abstraction classes
- Contains: Header-only and .hpp/.cpp pairs for peripherals
- Key files: `MidiProvider.hpp/cpp` (MIDI transport), `Keyboard.hpp` (input), `Adc.hpp/cpp` (analog)

**`src/Libs/Leds/`:**
- Purpose: LED rendering system with pluggable pattern animations
- Contains: `LedManager.hpp` (orchestrator) and `patterns/` subdirectory
- Key files: `Pattern.hpp` (base class), mode-specific patterns

**`editor-tx/src/`:**
- Purpose: React web application for device configuration
- Contains: JSX components, pages, layouts, assets

**`editor-tx/src/components/`:**
- Purpose: All React components -- providers, UI cards, navigation (flat, no subdirectories)
- Contains: Context providers mixed with UI components
- Key files: `MidiProvider.jsx` (central state), `SerialProvider.jsx` (unused serial context)

**`editor-tx/src/pages/`:**
- Purpose: Route-level page components
- Contains: Dashboard (main), Upload, QuickStart
- Note: `Keyboard.jsx`, `Scales.jsx`, `ControlChange.jsx`, `Settings.jsx` are tab panels within Dashboard, not standalone routes

**`data/`:**
- Purpose: LittleFS filesystem image uploaded to ESP32 flash
- Contains: JSON config files
- Generated: No (manually created templates)
- Committed: Yes

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Firmware entry (`setup()` + `loop()`)
- `editor-tx/src/main.jsx`: Web app React mount point
- `editor-tx/src/App.jsx`: Router definition

**Configuration:**
- `platformio.ini`: Build config, board, dependencies, build flags
- `editor-tx/package.json`: Web dependencies
- `editor-tx/vite.config.js`: Vite build settings
- `src/pinout.h`: Hardware pin mapping (conditional on REV_A/REV_B)

**Core Logic:**
- `src/main.cpp`: Mode switching, input callbacks, slider processing, SysEx handling
- `src/Configuration.hpp/cpp`: Config data structures + JSON serialization
- `src/Libs/MidiProvider.cpp`: Multi-transport MIDI output
- `src/Libs/Keyboard.hpp`: FSR keyboard state machine + XY/strip calculations
- `editor-tx/src/components/MidiProvider.jsx`: WebMIDI connection, config state, SysEx protocol

**Testing:**
- `test/`: Empty (PlatformIO test directory, unused)
- No web tests exist

## Naming Conventions

**Firmware Files:**
- Classes: PascalCase filenames matching class name (`MidiProvider.hpp`, `DataManager.hpp`)
- Config/data: PascalCase (`Configuration.hpp`, `Performance.hpp`)
- C headers: lowercase (`pinout.h`)
- Pattern: `.hpp` for headers (most are header-only), `.cpp` for implementations

**Web Files:**
- Components: PascalCase (`MidiProvider.jsx`, `NavBar.jsx`, `SelectCard.jsx`)
- Pages: PascalCase (`Dashboard.jsx`, `Upload.jsx`)
- Layouts: PascalCase (`RootLayout.jsx`)
- Assets: lowercase with underscores (`keyboard_layout.webp`, `release_notes.json`)

**Directories:**
- Firmware: PascalCase (`Libs/`, `Leds/`)
- Web: lowercase (`components/`, `pages/`, `layouts/`, `assets/`)

## Where to Add New Code

**New Firmware Feature/Service:**
- Create class in `src/Libs/` (e.g., `src/Libs/NewService.hpp`)
- Include and instantiate in `src/main.cpp`
- Currently no service registration pattern -- instances are global variables

**New LED Pattern:**
- Create header in `src/Libs/Leds/patterns/NewPattern.hpp`
- Inherit from `Pattern` base class (`src/Libs/Leds/patterns/Pattern.hpp`)
- Include in `src/Libs/Leds/LedManager.hpp`
- Instantiate in `src/main.cpp`

**New Web Page (Route):**
- Create page component in `editor-tx/src/pages/NewPage.jsx`
- Add route in `editor-tx/src/App.jsx`

**New Web Page (Dashboard Tab):**
- Create component in `editor-tx/src/pages/NewTab.jsx`
- Add tab in `editor-tx/src/pages/Dashboard.jsx`

**New Web UI Component:**
- Create in `editor-tx/src/components/NewComponent.jsx`
- No barrel exports -- import directly by path

**New Config Parameter:**
- Add field to struct in `src/Configuration.hpp`
- Add save/load in `src/Configuration.cpp`
- Add to `config` state default in `editor-tx/src/components/MidiProvider.jsx`
- Add UI control in appropriate page component

## Special Directories

**`data/`:**
- Purpose: LittleFS filesystem image -- JSON config templates
- Generated: No
- Committed: Yes
- Note: Uploaded to ESP32 flash via PlatformIO `uploadfs` command

**`.pio/` (gitignored):**
- Purpose: PlatformIO build artifacts, downloaded libraries
- Generated: Yes
- Committed: No

**`editor-tx/node_modules/` (gitignored):**
- Purpose: npm dependencies
- Generated: Yes
- Committed: No

**`editor-tx/src/assets/firmwares/`:**
- Purpose: Bundled firmware binaries for browser-based flashing via esptool-js
- Generated: No (manually placed after builds)
- Committed: Yes
- Note: Binary files committed to repo -- increases repo size

## Comparison with Reference Repo Structures

**eisei firmware (`/home/unwn/git/unwn/eisei/daisy/`) has:**
- Dedicated service files: `CommService.cpp/hpp`, `CalibrationManager.cpp/hpp`, `PresetManager.cpp/hpp`, `FirmwareUpdateService.cpp/hpp`
- Separated command handlers: `serial_commands/DiagnosticCommands.cpp`, `ParameterCommands.cpp`, `PresetCommands.cpp`, `SlotCommands.cpp`
- HAL wrapper: `daisy_eisei.cpp/hpp` encapsulating hardware platform
- Engine class: `EiseiEngine.cpp/hpp` for core audio processing
- T16 lacks all of this separation -- everything is in `main.cpp`

**DROP web app (`/home/unwn/git/DROP/src/`) has:**
- `services/` directory: `DataModel/`, `DatumPersistence/`, `DeviceBridge/`, `LuaEngine/`, `PixelArt/`
- `hooks/` directory: Custom React hooks
- `contexts/` directory: Separated from components
- `types/` directory: TypeScript type definitions
- `utils/` directory: Shared utilities
- `design-system/` directory: Reusable UI primitives with tokens
- `tools/` directory: Feature-specific tool modules
- Component subdirectories with barrel exports (`index.ts`)
- T16 web editor has none of this organization

**Recommended T16 Structure (Post-Refactor):**

Firmware:
```
src/
├── main.cpp              # Slim: init + loop calling services
├── pinout.h
├── Configuration.hpp/cpp
├── Scales.hpp
├── Services/             # NEW: Extracted from main.cpp
│   ├── ModeManager.hpp   # Mode switching logic
│   ├── InputProcessor.hpp # Key/slider/button callback routing
│   └── CommandHandler.hpp # SysEx command protocol
├── Libs/                 # Hardware abstraction (keep as-is)
└── Leds/                 # Move out of Libs
```

Web:
```
editor-tx/src/
├── main.jsx
├── App.jsx
├── contexts/             # NEW: Extract from components
│   └── MidiContext.jsx
├── hooks/                # NEW
│   └── useMidi.js
├── services/             # NEW
│   └── sysex.js
├── components/           # Reorganize by feature
│   ├── ui/               # Generic cards, inputs
│   ├── midi/             # MIDI-specific components
│   └── layout/           # NavBar, Footer
├── pages/
├── layouts/
└── assets/
```

---

*Structure analysis: 2026-04-03*
