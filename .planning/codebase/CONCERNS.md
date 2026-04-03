# Codebase Concerns

**Analysis Date:** 2026-04-03

## Tech Debt

**God-file main.cpp (887 lines):**
- Issue: `src/main.cpp` contains all application logic -- mode processing, slider handling, button handling, SysEx protocol, calibration routine, configuration application, and the main loop. No separation of concerns.
- Files: `src/main.cpp`
- Impact: Every change risks breaking unrelated functionality. Impossible to test individual features. Difficult for multiple contributors. Contrast with eisei which separates into `EiseiEngine`, `CallbackHandlers`, `SerialCommandManager`, `CalibrationManager`, etc.
- Fix approach: Extract into dedicated modules: `SliderController`, `ModeManager`, `SysExHandler`, `CalibrationRoutine`, `ButtonHandler`. Each with its own `.hpp/.cpp` pair. Follow eisei's pattern of domain-specific managers.

**Implementation in header files:**
- Issue: Nearly all firmware classes are implemented entirely in `.hpp` files with full method bodies. `Keyboard.hpp` (513 lines), `LedManager.hpp` (312 lines), `Scales.hpp` (165 lines), `Button.hpp` (192 lines), `DataManager.hpp` (174 lines), and all pattern files contain implementation in headers.
- Files: `src/Libs/Keyboard.hpp`, `src/Libs/LedManager.hpp`, `src/Libs/Button.hpp`, `src/Libs/DataManager.hpp`, `src/Scales.hpp`, all `src/Libs/Leds/patterns/*.hpp`
- Impact: Longer compile times (full recompilation on any header change). Name collision risk from global-scope definitions. Static variable definitions in headers (`Key::press_threshold`, `Pattern::currentPalette`, `loopCount` in `Performance.hpp`) risk ODR violations if included from multiple translation units. Currently works only because `main.cpp` is effectively the only `.cpp` that includes them.
- Fix approach: Move implementations to `.cpp` files. Keep only declarations and small inline methods in headers. This is partially done for `Adc`, `MidiProvider`, and `TouchSlider` -- extend to all classes.

**Global state everywhere:**
- Issue: Core application state is scattered across global variables in `main.cpp` and `Configuration.cpp`. `marker`, `current_chord`, `current_base_note`, `current_key_idx`, `current_qs_option`, `current_value_length`, `slider_mode` are all globals in `main.cpp`. `cfg`, `kb_cfg[]`, `cc_cfg[]`, `parameters`, `qs`, `calibration_data` are globals in `Configuration.cpp`. Mutable arrays `scales[]`, `note_map[]`, `current_chord_mapping[]` are globals in `Scales.hpp`.
- Files: `src/main.cpp` (lines 48, 71, 98-100, 154-155), `src/Configuration.cpp` (lines 3-8), `src/Scales.hpp` (lines 32, 100-101)
- Impact: Any function can mutate any state. Race conditions between main loop (core 1) and ADC task (core 0) are possible since `Keyboard::Update()` runs on the main loop but reads `Adc` values written from a FreeRTOS task on core 1. No mutex or atomic protection.
- Fix approach: Encapsulate state into classes. Create an `AppState` or `T16Controller` class that owns all mutable state. Use `volatile` or atomics for cross-core shared data. Consider a proper state machine pattern.

**Duplicated slider mode cycling logic:**
- Issue: The `ProcessButton` function contains three nearly identical blocks of code for cycling through allowed slider modes (lines 401-452). Each block creates a local `allowed_modes[]` array and iterates to find the current mode.
- Files: `src/main.cpp` (lines 396-453)
- Impact: Adding a new mode requires editing three places. Easy to introduce inconsistencies.
- Fix approach: Extract a `CycleSliderMode(SliderMode* allowed, int count)` helper function, or use a mode-to-allowed-slider-modes lookup table.

**Duplicated MIDI send pattern:**
- Issue: Every `MidiProvider::Send*` method repeats the same `if (midiBle)` / `if (midiOut)` guard pattern for USB, BLE, and Serial. 10+ methods each have the same 3-way dispatch.
- Files: `src/Libs/MidiProvider.cpp` (all Send* methods)
- Impact: Adding a new transport (e.g., WiFi MIDI) requires editing every method. Missing a guard causes silent bugs.
- Fix approach: Abstract MIDI transports into a list/array of interfaces. Loop over active transports. eisei's `CommService` pattern is a good reference.

**Configuration save/load is field-by-field with no schema:**
- Issue: `SaveConfiguration` and `LoadConfiguration` manually serialize each field with string keys. Adding a new config field requires changes in 4+ places: struct definition, save, load, and quick settings mapping. Quick settings uses magic index numbers (0-11) to map to config fields.
- Files: `src/Configuration.cpp` (lines 10-49, 83-130), `src/Configuration.hpp`
- Impact: Extremely fragile. Mismatch between save and load field names causes silent data loss. Version migration (`cfg.version != 103`) just overwrites config.
- Fix approach: Use a reflection/serialization pattern. Consider macros or a config descriptor table that maps field names to struct offsets. eisei's `DatumPersistence` pattern handles this more robustly.

**MidiProvider context in web app is a 500-line monolith:**
- Issue: `editor-tx/src/components/MidiProvider.jsx` is a single 499-line context provider that handles MIDI connection, SysEx serialization/deserialization, config state management, sync status tracking, firmware version checking, config file import/export, and toast notifications.
- Files: `editor-tx/src/components/MidiProvider.jsx`
- Impact: Impossible to test individual concerns. State updates trigger unnecessary re-renders across the entire app.
- Fix approach: Split into separate contexts/hooks: `useMidiConnection()`, `useConfigSync()`, `useFirmwareUpdate()`. Follow DROP's pattern with dedicated `services/` and `contexts/` directories.

**Web app lacks TypeScript:**
- Issue: All web code is plain JSX with PropTypes for runtime type checking only. DROP uses TypeScript throughout.
- Files: All `editor-tx/src/**/*.jsx` files
- Impact: No compile-time type safety. Config shape mismatches between firmware and web are caught only at runtime (if at all). The config object structure is duplicated as a default value in `MidiProvider.jsx` with no type definition.
- Fix approach: Migrate to TypeScript. Define shared types for the config schema. This is the single highest-impact change for web reliability.

## Known Bugs

**Unreachable XY_PAD branch in main loop:**
- Symptoms: The `loop()` function has two `else if (cfg.mode == Mode::XY_PAD)` blocks (lines 822 and 870). The second one at line 870 is unreachable.
- Files: `src/main.cpp` (line 870)
- Trigger: Always -- dead code.
- Workaround: None needed; the first block handles XY_PAD correctly. The second block duplicates `SetPosition` which is already called.

**HardwareTest infinite loop:**
- Symptoms: If any key fails the hardware test, the function enters `while (!test_passed) { delay(100); }` which loops forever since `test_passed` is never set back to `true`.
- Files: `src/main.cpp` (lines 585-608)
- Trigger: A broken key sensor on boot.
- Workaround: Physical power cycle. Device becomes unresponsive.

**Memory leak in LedManager transitions:**
- Symptoms: `TransitionToPattern` and `UpdateTransition` allocate `new WaveTransition()` without ever freeing the previous pattern.
- Files: `src/Libs/Leds/LedManager.hpp` (lines 233-246)
- Trigger: Every mode change leaks a `WaveTransition` object. Over extended use, heap fragmentation and eventual crash.
- Workaround: None.

**TouchSlider::SetPosition(uint8_t, uint8_t) is a no-op:**
- Symptoms: The `SetPosition(uint8_t intPosition, uint8_t numPositions)` method computes a value but never assigns it to `lastPosition`.
- Files: `src/Libs/TouchSlider.cpp` (lines 126-130)
- Trigger: Called to set slider to a quantized position -- the position is silently ignored.
- Workaround: Use the float version `SetPosition(float)` instead.

## Security Considerations

**Hardcoded WiFi credentials in OTA module:**
- Risk: `src/Libs/Ota.hpp` contains plaintext WiFi SSIDs and passwords on lines 18-20. This file is committed to git.
- Files: `src/Libs/Ota.hpp` (lines 18-20)
- Current mitigation: The OTA module is not included in the active build (not `#include`d from `main.cpp`). But credentials are in the repository.
- Recommendations: Remove credentials from source. Use a `.env` or config file excluded from git. If OTA is re-enabled, use a provisioning flow instead.

**No SysEx authentication or validation:**
- Risk: Any MIDI device on the same bus can send SysEx to overwrite the entire device configuration, trigger calibration, or request a config dump. The `ProcessSysEx` function in `main.cpp` accepts commands based only on magic byte sequences with no authentication.
- Files: `src/main.cpp` (lines 687-724)
- Current mitigation: None. Physical access to USB or BLE is required, which provides some natural protection.
- Recommendations: Add a device-specific manufacturer ID prefix to SysEx messages. Consider a simple handshake or nonce for destructive operations (calibration wipe).

**Unsanitized SysEx deserialization:**
- Risk: `config.DeserializeFromBuffer` in `ProcessSysEx` (line 713) passes raw SysEx data directly to ArduinoJson deserialization with no size limit validation. A malformed or oversized payload could cause heap overflow.
- Files: `src/main.cpp` (line 713), `src/Libs/DataManager.hpp` (lines 98-104)
- Current mitigation: ArduinoJson has built-in size limits via `JsonDocument`, but the default `JsonDocument` has no explicit capacity limit in v7.
- Recommendations: Validate SysEx payload length before deserialization. Set an explicit `JsonDocument` capacity.

**Web config file upload has no validation:**
- Risk: `uploadConfig` in `MidiProvider.jsx` (line 427) parses any `.topo` file and directly sets it as the device config with no schema validation.
- Files: `editor-tx/src/components/MidiProvider.jsx` (lines 427-462)
- Current mitigation: JSON.parse will throw on invalid JSON, but valid JSON with wrong structure is accepted silently.
- Recommendations: Validate uploaded config against expected schema before applying. Check for required fields and value ranges.

## Performance Bottlenecks

**DataManager reads/writes full JSON on every field save:**
- Problem: Each call to `SaveVar` loads the entire JSON file from LittleFS, modifies one field, then writes the entire file back. `SaveConfiguration` calls `SaveVar` 8 times in a row, plus `SaveArray` twice and `SaveBanksArray` once, resulting in 11 full file read-write cycles.
- Files: `src/Libs/DataManager.hpp` (lines 22-27), `src/Configuration.cpp` (lines 10-49)
- Cause: No in-memory document caching. Every operation is a full filesystem round-trip.
- Improvement path: Load the document once into memory, make all modifications, then write once. Or batch all saves into a single `SaveJsonDocument` call.

**ADC task runs in a tight loop with no yield:**
- Problem: `Adc::Update` runs `while(1) { ReadValues(); }` on core 1 with no `vTaskDelay`. This starves other tasks on core 0 (BLE stack) and wastes CPU.
- Files: `src/Libs/Adc.cpp` (lines 131-140)
- Cause: The `vTaskDelay(1)` is commented out on line 138.
- Improvement path: Uncomment or add a small delay. At 16 channels with sequential reads, the effective scan rate is already limited by `analogRead` timing (~100us per read). A 1ms delay between full scans would not noticeably affect latency.

## Fragile Areas

**Scales.hpp global mutable arrays:**
- Files: `src/Scales.hpp`
- Why fragile: `scales[]`, `note_map[]`, `current_chord_mapping[]`, and `strum_chords[]` are global mutable arrays defined in a header. `SetNoteMap` and `SetCustomScale` mutate them directly. The `scales[CUSTOM1]` and `scales[CUSTOM2]` entries are overwritten at runtime by `SetCustomScale`.
- Safe modification: Do not add new includes of `Scales.hpp` from any additional `.cpp` files -- will cause linker errors from duplicate symbol definitions.
- Test coverage: None.

**Quick Settings index mapping:**
- Files: `src/Configuration.cpp` (lines 51-81)
- Why fragile: `LoadQuickSettings` and `SaveQuickSettings` use hardcoded array indices (0-11) to map between `qs.settings[]` and config struct fields. Adding or reordering a quick setting requires updating both functions in lockstep with the `QuickSettingsData` struct and the `ProcessQuickSettings` function in `main.cpp`.
- Safe modification: Always update `LoadQuickSettings`, `SaveQuickSettings`, and `QuickSettingsData` together.
- Test coverage: None.

**Configuration version migration:**
- Files: `src/main.cpp` (lines 786-791)
- Why fragile: Version mismatch handling (line 786) simply sets `cfg.version = 103` and overwrites the saved config. No actual migration logic. Users upgrading firmware may lose their configuration silently.
- Safe modification: Implement incremental migration functions per version bump.
- Test coverage: None.

## Scaling Limits

**SysEx config transfer limited to ~4KB:**
- Current capacity: `SerializeToBuffer` uses a 4098-byte buffer (line 702 in `main.cpp`). `SysExMaxSize` is 2048 in `MidiProvider.hpp`.
- Limit: SysEx max size in `CustomSettings` (2048 bytes) is smaller than the buffer used for serialization (4098 bytes). Large configs may be truncated silently.
- Scaling path: Increase `SysExMaxSize` or implement chunked SysEx transfer. The JSON config is currently ~1-2KB but grows with custom scales and additional banks.

**Fixed 4-bank limit:**
- Current capacity: `BANK_AMT = 4` in `Configuration.hpp`.
- Limit: Adding banks requires changes to `Configuration.hpp`, `Configuration.cpp`, `MidiProvider.jsx` (default config state), and potentially `LedManager` (palette array).
- Scaling path: Make bank count configurable. Store bank data in a dynamic structure rather than fixed arrays.

**Signal class uses uint8_t for slot IDs:**
- Current capacity: Max 255 slots per signal.
- Limit: `nextSlotId` wraps at 255, causing ID collisions. Not likely in practice but architecturally unsound.
- Files: `src/Libs/Signal.hpp` (line 17)
- Scaling path: Use `uint16_t` or check for collisions.

## Dependencies at Risk

**Chakra UI v2 (web app):**
- Risk: Chakra UI v2 is in maintenance mode. v3 has breaking changes (different API, different theming). DROP appears to use a custom design system instead.
- Impact: No new features. May become incompatible with newer React versions.
- Migration plan: Consider migrating to Chakra v3, or to a custom design system following DROP's `design-system/` pattern. Could also use `@unwn-core` shared components if available.

**NimBLE-Arduino v1.4.0:**
- Risk: Pinned to `^1.4.0`. NimBLE has moved to v2.x with API changes.
- Impact: Missing BLE improvements and bug fixes. May conflict with newer ESP-IDF versions.
- Migration plan: Test upgrade to latest NimBLE. The BLE-MIDI library also needs compatibility verification.

**React 18 (web app):**
- Risk: React 19 is current. The app uses older patterns (no use of Suspense, no server components consideration).
- Impact: Low immediate risk. React 18 is well-supported.
- Migration plan: Upgrade alongside TypeScript migration and Chakra replacement.

## Missing Critical Features

**No serial command interface:**
- Problem: The firmware has no serial command system for diagnostics, debugging, or configuration. Serial is commented out in `setup()` (line 728). eisei has a full `SerialCommandManager` with modular command categories (`DiagnosticCommands`, `PresetCommands`, `ParameterCommands`, `SlotCommands`).
- Blocks: Runtime debugging, factory diagnostics, automated testing.

**No firmware update service over USB/BLE:**
- Problem: OTA updates require WiFi (`Ota.hpp`) which has hardcoded credentials and is disabled. The web app has an `Upload.jsx` page that uses `esptool-js` requiring the device to be in bootloader mode. eisei has a dedicated `FirmwareUpdateService` that works over serial.
- Blocks: Seamless firmware updates. Users must hold a button during power-on.

**No task scheduling:**
- Problem: Everything runs in `loop()` with no timing control. LED updates, keyboard scanning, slider reading, and MIDI processing all happen every loop iteration. eisei uses a `TaskScheduler` for controlled timing.
- Blocks: Power optimization, consistent timing, priority-based processing.

**Web app has no PWA support:**
- Problem: No service worker, no manifest, no offline capability. DROP has `PWAService.ts` and full offline support.
- Blocks: Mobile usage (configurator on phone while device is connected via BLE).

**No automated testing anywhere:**
- Problem: Zero test files in both firmware and web app. No test framework configured.
- Blocks: Confident refactoring. Regression detection.

## Test Coverage Gaps

**Entire firmware is untested:**
- What's not tested: All firmware logic -- key processing, MIDI output, slider behavior, configuration serialization, scale mapping, LED patterns.
- Files: All `src/**/*.{hpp,cpp}`
- Risk: Any refactor could break MIDI output, key velocity curves, or configuration persistence without detection.
- Priority: High. PlatformIO supports native unit testing. Core logic (scale mapping, LUT generation, slew limiter, config serialization) can be tested without hardware.

**Web app has no tests:**
- What's not tested: MIDI connection flow, SysEx serialization/deserialization, config state management, sync status calculation.
- Files: All `editor-tx/src/**/*.jsx`
- Risk: Config changes sent to device could silently corrupt settings. The `deserializeSysex` function and `updateSyncStatus` are critical paths with no coverage.
- Priority: High. The SysEx serialization/deserialization and config state management are the most critical paths to test.

**No linting enforcement:**
- What's not tested: Code style consistency. `.eslintrc.cjs` exists but no pre-commit hooks or CI enforce it. No Prettier config. No `clang-format` for firmware.
- Files: `editor-tx/.eslintrc.cjs`
- Risk: Inconsistent code style across the codebase. No automated quality gates.
- Priority: Medium. Add `.clang-format` for firmware, configure Prettier for web, add pre-commit hooks.

---

*Concerns audit: 2026-04-03*
