# External Integrations

**Analysis Date:** 2026-04-03

## APIs & External Services

### Web MIDI API (Primary device communication)
- **What:** Browser-to-device MIDI communication over USB
- **SDK/Client:** `webmidi` 3.1.8 (`editor-tx/src/components/MidiProvider.jsx`)
- **Auth:** None (browser permission prompt)
- **Protocol:** MIDI SysEx messages for configuration transfer
- **Device name filter:** Hardcoded to `"Topo T16"` (line 187-188 in `MidiProvider.jsx`)
- **SysEx message types used:**
  - `[0x7e, 0x7f, 0x06, 0x01]` - Version request
  - `[0x7e, 0x7f, 0x07, 0x03]` - Configuration dump request
  - `[0x7e, 0x7f, 0x07, 0x05, ...data]` - Configuration write (JSON payload as byte array)
  - `[0x7e, 0x7f, 0x07, 0x06]` - Full calibration trigger

### Web Serial API (Firmware flashing)
- **What:** Browser-based ESP32 firmware upload
- **SDK/Client:** `esptool-js` 0.4.3 (`editor-tx/src/pages/Upload.jsx`)
- **Auth:** None (browser permission prompt)
- **Baud rate:** 921600 (for flashing)
- **Flash address:** `0x10000`
- **Note:** Also used by `SerialProvider.jsx` for text-based serial at 115200 baud, but this provider does not appear to be actively used in the current app routing

### Web Serial API (Serial monitor - inactive)
- **What:** Text-based serial communication
- **SDK/Client:** Native `navigator.serial` (`editor-tx/src/components/SerialProvider.jsx`)
- **Auth:** None (browser permission prompt)
- **Baud rate:** 115200
- **Status:** Provider exists but is not imported/used in `main.jsx` or any active route

## Data Storage

**Firmware On-Device (LittleFS):**
- Filesystem: LittleFS on ESP32-S3 flash
- Files:
  - `/calibration_data.json` - Per-key ADC min/max calibration values
  - `/configuration_data.json` - Full device configuration (mode, MIDI settings, bank configs, scales)
- Client: `DataManager` class (`src/Libs/DataManager.hpp`) wrapping ArduinoJson 7.x + LittleFS
- Configuration version tracked as integer (current: 103, checked in `src/main.cpp` line 786)

**Web Editor (Browser-side):**
- No persistent storage (no localStorage, no IndexedDB)
- Configuration lives in React state only (`MidiProvider.jsx`)
- File export/import via `.topo` files (JSON format, download/upload in `MidiProvider.jsx` lines 414-462)

**Firmware Binaries (Bundled):**
- Location: `editor-tx/src/assets/firmwares/`
- Files: `t16_v1.0.0.bin` through `t16_v1.0.3.bin`
- Metadata: `release_notes.json` (version, release date, highlights, bugfixes, improvements)
- Bundled into Vite build via `import.meta.url` dynamic resolution

**File Storage:** Local filesystem only (no cloud storage)

**Caching:** None

## MIDI Transport Layer

The firmware supports three simultaneous MIDI transports, managed by `MidiProvider` (`src/Libs/MidiProvider.hpp`):

**USB MIDI:**
- Stack: Adafruit TinyUSB Library 3.3.0
- Always active
- Device name: "Topo T16"
- Used for both MIDI data and SysEx configuration

**BLE MIDI:**
- Stack: NimBLE-Arduino 1.4.0 + BLE-MIDI 2.2.0
- Toggled via `cfg.midi_ble` configuration flag
- Broadcasts as BLE MIDI peripheral

**Serial/TRS MIDI:**
- Stack: HardwareSerial with arduino_midi_library v5.1.0
- Toggled via `cfg.midi_trs` configuration flag
- Supports Type A/B TRS via `cfg.trs_type` flag
- Pins: RX=44, TX=43, TX2=42 (defined in `src/pinout.h`)
- MIDI Thru (passthrough) supported via `cfg.passthrough`

**SysEx Configuration Protocol:**
- Max SysEx size: 2048 bytes (defined in `CustomSettings` struct, `src/Libs/MidiProvider.hpp` line 14)
- Configuration serialized as JSON via ArduinoJson, transmitted as raw bytes within SysEx
- Firmware-side handlers in `src/main.cpp` `ProcessSysEx()` (line 687)
- Editor-side handlers in `editor-tx/src/components/MidiProvider.jsx` `onSysex()` (line 301)

## Authentication & Identity

**Auth Provider:** None
- No user authentication
- Device communication relies on browser permission prompts (Web Serial, Web MIDI)
- No API keys or tokens

## Monitoring & Observability

**Error Tracking:** None

**Logs:**
- Firmware: ESP-IDF `log_d()` macro (debug level, compiled out in release builds)
- Firmware: `Serial.printf()` / `Serial.println()` for calibration output
- Web editor: `console.log()` / `console.error()` throughout
- No structured logging on either side

## CI/CD & Deployment

**Web Editor Hosting:**
- Platform: Shared web hosting (Apache)
- Deployment: FTP via GitHub Actions
- Workflow: `.github/workflows/deploy.yml`
- Trigger: Push to `main` branch
- Steps: `npm install` -> `npm run build` -> FTP deploy (`SamKirkland/FTP-Deploy-Action@v4.3.5`)
- SPA routing: `.htaccess` with mod_rewrite (copied into `dist/` during build)

**Firmware Distribution:**
- Pre-compiled `.bin` files bundled in web editor (`editor-tx/src/assets/firmwares/`)
- Browser-based flashing via esptool-js (Web Serial API)
- No CI pipeline for firmware builds (PlatformIO builds are local-only)

**Secrets:**
- `FTP_SERVER`, `FTP_USER`, `FTP_PASSWORD`, `FTP_PROTOCOL` - GitHub Actions secrets for deployment

## OTA Updates (Inactive)

**ArduinoOTA:**
- Implementation exists in `src/Libs/Ota.hpp`
- Uses WiFi with hardcoded credentials (security concern -- see CONCERNS.md)
- Not called from `setup()` or `loop()` in current `main.cpp`
- Hostname set to "eisei" (copy-paste from eisei project)
- FreeRTOS task-based (`otaTask()`)

## Webhooks & Callbacks

**Incoming:** None
**Outgoing:** None

## Device Communication Architecture

```
  [Web Editor (Browser)]
        |
        |--- Web MIDI API (webmidi 3.1.8)
        |       |--- SysEx: config read/write, version check, calibration trigger
        |       |--- CC messages: real-time MIDI monitoring
        |       \--- Note messages: input visualization
        |
        \--- Web Serial API (esptool-js 0.4.3)
                \--- Firmware binary upload (bootloader mode only)

  [ESP32-S3 Firmware]
        |
        |--- USB MIDI (TinyUSB) ---- always active
        |--- BLE MIDI (NimBLE) ----- toggled by config
        \--- TRS MIDI (HW Serial) -- toggled by config
```

## Comparison with Reference Repos

### vs. eisei (device communication)

| Aspect | T16 | eisei |
|--------|-----|-------|
| Config protocol | MIDI SysEx with JSON payload | Structured serial commands (`SerialCommandManager`) |
| Command structure | Magic byte sequences (`[127, 7, 5]`) | Named text commands (`status`, `preset save`, `cal`) |
| Parameter IDs | Implicit (JSON keys) | Explicit `ParamId` enum with mapping tables |
| Real-time control | USB/BLE/TRS MIDI CC | MIDI CC + I2C (Monome II protocol) |
| Firmware update | esptool-js in browser | DFU via web app (webdfu) |
| Serial debug | `log_d()` only | Structured `SerialCommandManager` with categorized handlers |

### vs. DROP (web app integrations)

| Aspect | T16 editor-tx | DROP |
|--------|--------------|------|
| MIDI library | webmidi 3.1.8 | webmidi (similar) |
| Firmware flashing | esptool-js 0.4.3 | esptool-js 0.5.6 + webdfu 1.0.5 |
| Deployment | FTP to shared hosting | GitHub Pages |
| PWA/offline | Not supported | Full PWA with workbox caching |
| WASM modules | None | enjin2-wasm, filterbank-wasm, wasmoon (Lua) |

## Environment Configuration

**Required env vars:** None for development

**CI/CD secrets (GitHub Actions):**
- `FTP_SERVER` - Deployment target
- `FTP_USER` - FTP credentials
- `FTP_PASSWORD` - FTP credentials
- `FTP_PROTOCOL` - FTP/SFTP selection

**Firmware build requirements:**
- PlatformIO with Espressif32 platform
- Custom board `unwn_s3` must be available in PlatformIO boards path

---

*Integration audit: 2026-04-03*
