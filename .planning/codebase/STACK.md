# Technology Stack

**Analysis Date:** 2026-04-03

## Languages

**Primary:**
- C++17 (Arduino dialect) - Firmware (`src/`)
- JavaScript (JSX) - Web editor (`editor-tx/`)

**Secondary:**
- C - Pin definitions (`src/pinout.h`)
- JSON - Configuration serialization (ArduinoJson on firmware, native JS in editor)

## Runtime

**Firmware:**
- ESP32-S3 (Espressif32 platform via PlatformIO)
- Arduino framework
- Custom board definition: `unwn_s3` (referenced in `platformio.ini`, board JSON not found in repo -- likely in PlatformIO custom boards path)
- Hardware revisions: `REV_A` and `REV_B` via compile-time flags (active: `REV_B`)

**Web Editor:**
- Node.js (version not pinned -- no `.nvmrc` or `engines` field)
- Browser runtime targeting Chrome/Edge (requires Web Serial API and Web MIDI API)

**Package Managers:**
- npm (editor-tx)
- PlatformIO lib_deps (firmware)
- Lockfile: `package-lock.json` status unknown, no lock pinning strategy visible

## Frameworks

**Core:**
- Arduino Framework (ESP32) - Firmware runtime
- React 18.2.0 - Web editor UI (`editor-tx/package.json`)
- Chakra UI 2.8.2 - Component library (`editor-tx/package.json`)
- React Router DOM 6.21.3 - Client-side routing (`editor-tx/src/App.jsx`)

**Build/Dev:**
- PlatformIO - Firmware build system (`platformio.ini`)
- Vite 5.0.8 - Web editor bundler (`editor-tx/vite.config.js`)
- `@vitejs/plugin-react` 4.2.1 - React fast refresh

**Testing:**
- Not detected (no test framework in either firmware or editor)

## Key Dependencies

### Firmware (PlatformIO lib_deps in `platformio.ini`)

**MIDI:**
- `arduino_midi_library` v5.1.0 (feat branch) - Core MIDI protocol parsing (`src/Libs/MidiProvider.hpp`)
- `Adafruit TinyUSB Library` 3.3.0 - USB MIDI device class (`src/Libs/MidiProvider.hpp`)
- `BLE-MIDI` 2.2.0 - Bluetooth MIDI transport (`src/Libs/MidiProvider.hpp`)
- `NimBLE-Arduino` 1.4.0 - BLE stack for ESP32 (BLE-MIDI dependency)

**Hardware I/O:**
- `FastLED` 3.6.0 - LED matrix and slider LED control (`src/Libs/Leds/LedManager.hpp`)

**Data/Storage:**
- `ArduinoJson` 7.0.3 - Configuration serialization to LittleFS (`src/Libs/DataManager.hpp`)
- `StreamUtils` 1.8.0 - Stream buffering for JSON operations (`src/Libs/DataManager.hpp`)

**OTA (present but not active in main loop):**
- WiFi/ArduinoOTA - Included in `src/Libs/Ota.hpp` but not called from `setup()`/`loop()`

### Web Editor (`editor-tx/package.json`)

**Critical:**
- `webmidi` 3.1.8 - Web MIDI API wrapper, primary device communication (`editor-tx/src/components/MidiProvider.jsx`)
- `esptool-js` 0.4.3 - Browser-based firmware flashing via Web Serial (`editor-tx/src/pages/Upload.jsx`)

**UI:**
- `@chakra-ui/react` 2.8.2 + `@chakra-ui/icons` 2.1.1 - UI components
- `@emotion/react` 11.11.3 + `@emotion/styled` 11.11.0 - CSS-in-JS (Chakra dependency)
- `framer-motion` 11.0.3 - Animations (Chakra dependency)
- `react-icons` 5.0.1 - Icon library
- `@fontsource/inter` 5.0.17 + `@fontsource/poppins` 5.0.12 - Self-hosted fonts

**Charting:**
- `apexcharts` 3.45.2 + `react-apexcharts` 1.4.1 - Data visualization (MIDI monitor)

**Misc:**
- `axios` 1.7.2 - HTTP client (usage unclear -- may be unused)
- `prop-types` 15.8.1 - Runtime prop validation (used throughout components)

## Configuration

**Firmware Build Flags:**
- `-DUSE_TINYUSB` - Enable TinyUSB for USB MIDI
- `-DCORE_DEBUG_LEVEL=5` (dev) / `=0` (release) - ESP-IDF log level
- `-DREV_B` - Hardware revision selector
- `-Os` - Optimize for size

**Firmware Build Environments (in `platformio.ini`):**
- `esp32s3` - Development (debug logging enabled)
- `release` - Production (debug logging disabled)

**Filesystem:**
- `board_build.filesystem = littlefs` - On-device config/calibration storage
- Config files stored on LittleFS: `/calibration_data.json`, `/configuration_data.json`

**Web Editor:**
- Minimal Vite config (`editor-tx/vite.config.js`): React plugin + polling watcher
- No path aliases configured
- No environment variables used
- Custom Chakra theme defined inline in `editor-tx/src/main.jsx`
- `.htaccess` for Apache SPA routing (`editor-tx/.htaccess`)

**Linting:**
- ESLint 8.55 with `eslint-plugin-react`, `eslint-plugin-react-hooks`, `eslint-plugin-react-refresh`
- Legacy `.eslintrc` config format (not flat config)
- No Prettier configured
- No firmware linting/formatting tools

**Serial Monitor:**
- Baud rate: 115200
- Filter: `esp32_exception_decoder`

## Platform Requirements

**Development:**
- PlatformIO CLI or IDE (VSCode extension)
- Node.js + npm (for editor-tx)
- Chrome/Edge browser (Web Serial + Web MIDI API required)

**Production:**
- Firmware runs on custom ESP32-S3 board (`unwn_s3`)
- Web editor deployed to shared hosting via FTP (Apache with `.htaccess`)
- No PWA support (unlike DROP reference repo)

## Comparison with Reference Repos

### vs. eisei (Daisy firmware)

| Aspect | T16 | eisei |
|--------|-----|-------|
| Build system | PlatformIO/Arduino | Make/CMake (libDaisy) |
| Language standard | Arduino C++ (no namespace) | C++17 with namespaces |
| Task scheduling | Single-threaded `loop()` | `TaskScheduler` with timed tasks (`daisy/TaskScheduler.hpp`) |
| Serial commands | None (SysEx-only config) | Structured `SerialCommandManager` with categorized command files (`serial_commands/`) |
| Parameter management | Global structs (`Configuration.hpp`) | `ParameterRegistry` with typed `ParamId` enum |
| Config persistence | LittleFS JSON files | Flash presets with structured serialization |
| Code organization | Flat includes in `main.cpp`, header-only libs | Namespaced modules, separate `.cpp`/`.hpp` |

### vs. DROP (React web app)

| Aspect | T16 editor-tx | DROP |
|--------|--------------|------|
| React version | 18.2.0 | 19.1.0 |
| Language | JavaScript (JSX) | TypeScript (TSX) |
| UI framework | Chakra UI 2.x | Custom design system with Radix primitives |
| Vite version | 5.0.8 | 7.0.0 |
| ESLint | v8 legacy config | v9 flat config with typescript-eslint |
| Path aliases | None | `@/components`, `@/services`, `@/hooks`, etc. |
| PWA support | No | Yes (vite-plugin-pwa, workbox) |
| Build config | Minimal | Manual chunks, source maps, production base path |
| esptool-js | 0.4.3 | 0.5.6 |
| Device comms | WebMidi (SysEx) | WebMidi + Web Serial + WebDFU |

### vs. unwn-core (C++ monorepo)

| Aspect | T16 firmware | unwn-core |
|--------|-------------|-----------|
| Build system | PlatformIO | CMake with presets |
| Code style | No enforcement | clang-format + clang-tidy planned |
| Testing | None | Catch2 unit tests |
| Naming | Mixed (PascalCase classes, camelCase methods) | Standardizing on camelCase methods, PascalCase types |
| Documentation | None | Doxygen-style `///` comments planned |

---

*Stack analysis: 2026-04-03*
