# Pitfalls Research

**Domain:** ESP32-S3 MIDI controller firmware refactor + web configurator rewrite
**Researched:** 2026-04-03
**Confidence:** HIGH (based on codebase analysis and domain experience)

## Critical Pitfalls

### Pitfall 1: Config Schema Divergence Between Firmware and Web

**What goes wrong:**
The firmware and web editor independently define the config shape. The firmware uses C++ structs (`ConfigurationData`, `KeyModeData`, `ControlChangeData`) with field names like `base_octave`, `velocity_curve`, `aftertouch_curve`. The JSON serialization uses abbreviated keys (`oct`, `vel`, `at`, `ch`). The web editor hardcodes default config objects in `MidiProvider.jsx` with those same abbreviated keys. There is no shared schema -- both sides are hand-maintained copies. During a refactor, one side will change a field name, add a field, or reorder fields without updating the other. The result is silent config corruption: the device accepts the JSON, applies defaults for missing fields, and the user's settings vanish.

**Why it happens:**
No shared type definition or schema exists. The firmware serializes with string keys in `Configuration.cpp`, the web editor matches by convention. The current version field (`cfg.version = 103`) has no migration logic -- a version mismatch just overwrites with defaults.

**How to avoid:**
- Define the config schema exactly once. Use a JSON Schema file or a TypeScript type definition that generates firmware validation code.
- At minimum: create a `config-schema.ts` file in the web app that is the canonical source of truth. Document the JSON key names the firmware uses. Write a test that serializes a firmware config and validates it against the web schema.
- Implement proper version migration: when `cfg.version` does not match expected, run a migration function per version increment rather than overwriting.

**Warning signs:**
- Config values reset to defaults after a firmware update
- Web editor shows stale or wrong values after sync
- `DeserializeFromBuffer` succeeds but fields are zero/default
- No test exists that round-trips a config through both firmware serialization and web deserialization

**Phase to address:**
SysEx Protocol phase. Define the schema before building the new protocol. Migration logic must ship with the first firmware build that changes config format.

---

### Pitfall 2: Breaking MIDI Output During Service Extraction

**What goes wrong:**
The firmware refactor extracts logic from a monolithic 887-line `main.cpp` into service classes. During extraction, the timing relationship between keyboard scanning, note processing, and MIDI output changes. Specifically: `Key::Update()` runs a state machine (IDLE -> STARTED -> PRESSED -> AFTERTOUCH -> RELEASED) that emits signals. The signal callbacks (`ProcessKey`, `ProcessChord`, etc.) directly call `midi_provider.SendNoteOn()`. If the extraction changes when these callbacks fire relative to the main loop, or if a service class introduces async processing, notes get dropped, stuck, or duplicated. Stuck notes are the worst -- a note-on without a matching note-off causes an audible drone until the device is power-cycled.

**Why it happens:**
The current code works because everything runs synchronously in `loop()`. The `keyboard.Update()` call triggers signals that immediately call `ProcessKey()` which immediately sends MIDI -- all in the same loop iteration. Extracting into services risks adding indirection (message queues, deferred callbacks, task boundaries) that breaks this synchronous guarantee.

**How to avoid:**
- Extract services one at a time, not all at once. Start with the least MIDI-critical service (LED patterns).
- Preserve the synchronous signal-to-MIDI-send path. Do not introduce async processing between key state change and MIDI output.
- Add a "MIDI smoke test" early: a minimal test that verifies note-on is always followed by note-off for every key press. Can be done with a MIDI loopback (USB MIDI out -> USB MIDI in on a computer running a monitor).
- Keep `note_pool`, `chord_pool`, and `strum_pool` tracking intact. These arrays ensure NoteOff is sent for every NoteOn. If they get lost in the refactor, stuck notes will happen.

**Warning signs:**
- Notes sustaining after key release (stuck notes)
- Missing notes during fast playing
- Aftertouch messages arriving after note-off
- Note pool arrays not being cleared on mode switch

**Phase to address:**
Firmware Service Extraction phase. This must be the highest-risk, most-careful phase. Validate MIDI output after each service extraction, not at the end.

---

### Pitfall 3: Velocity Curve Regression

**What goes wrong:**
The T16's velocity curves translate FSR (force-sensitive resistor) pressure into MIDI velocity values (0-127). These curves are tuned by feel -- they determine how the instrument responds to the player. During refactoring, if the ADC reading path, calibration data application, or velocity curve lookup is modified, the instrument's playing feel changes. This is not a "bug" that shows up in tests -- it is a subjective regression that makes the instrument feel wrong to the player. Users who have muscle memory for the current response will immediately notice.

**Why it happens:**
Velocity curves involve multiple stages: raw ADC read -> calibration min/max mapping -> curve application -> MIDI value. The calibration data (`calibration_data.minVal[16]`, `calibration_data.maxVal[16]`) is per-key and stored in LittleFS. The curve functions are in `Keyboard.hpp`. Refactoring any of these stages or changing their order produces different output for the same physical input.

**How to avoid:**
- Capture reference data before refactoring: for each velocity curve type, record input ADC values and expected MIDI velocity output. This becomes a regression test.
- Do not refactor `Key::Update()`, the velocity curve functions, or `Adc` reading simultaneously. Change one, verify, then change the next.
- The ADC task runs on core 0 while `Keyboard::Update()` runs on core 1. This cross-core data sharing currently has no mutex. Adding one during refactoring could change timing. If you add synchronization, profile the impact on scan rate.

**Warning signs:**
- Players report the instrument "feels different" after a firmware update
- Velocity values cluster at extremes (all 127 or all 1) instead of smooth gradation
- Aftertouch becomes jumpy or unresponsive

**Phase to address:**
Firmware Service Extraction phase. Extract keyboard/velocity as a standalone service with its own unit tests before touching anything else in the signal path.

---

### Pitfall 4: SysEx Protocol Change Bricks Existing Devices

**What goes wrong:**
The new structured SysEx protocol is incompatible with the old magic-byte protocol (`data[2] == 127 && data[3] == 7`). Users who update firmware but not the web editor (or vice versa) get a broken config experience: the editor sends commands the firmware does not understand, or the firmware responds with a format the editor cannot parse. The device works fine as a MIDI controller, but it becomes unconfigurable.

**Why it happens:**
The web editor is hosted on GitHub Pages and updates independently from firmware. A user may flash new firmware then visit the cached old editor, or use a bookmarked old editor URL. The firmware has no way to negotiate protocol version with the editor.

**How to avoid:**
- Add a version/capability handshake at connection time. The editor already sends an identity request (`[0x7e, 0x7f, 0x06, 0x01]`). Extend the response to include protocol version. The editor then adapts its behavior.
- Support the old protocol alongside the new one for at least one firmware release cycle. The firmware should recognize both old magic-byte commands and new structured commands.
- The web editor should detect firmware protocol version and show a "firmware update recommended" banner rather than silently failing.
- Never remove old protocol handling from firmware without at least one version that supports both.

**Warning signs:**
- Editor connects but shows blank/default config
- "Config dump request" sent but no response received
- SysEx handler receives data but none of the `if` branches match

**Phase to address:**
SysEx Protocol phase. Design the protocol with backward compatibility from the start. Implement the handshake before changing any command formats.

---

### Pitfall 5: LittleFS Flash Wear During Development

**What goes wrong:**
The current `SaveConfiguration` does 11 filesystem read-write cycles per save (each `SaveVar` call loads the full JSON, modifies one field, writes the full file). During development with frequent config changes, this accelerates flash wear on the ESP32-S3. LittleFS has wear leveling, but the underlying NOR flash has ~100K write cycles per sector. Heavy development testing with per-parameter saves can approach this limit on a single device.

More immediately: the 11 read-write cycles take ~3 seconds, making the config sync feel sluggish and blocking the main loop (no MIDI processing during save).

**Why it happens:**
`DataManager::SaveVar` was written for simplicity, not efficiency. Each call is self-contained: open file, parse JSON, modify, serialize, write, close. No batching.

**How to avoid:**
- Batch config saves: load the JSON document once, modify all fields, write once. This is a 10x reduction in flash writes and should bring save time under 300ms.
- Keep the config document in RAM (ESP32-S3 has ~512KB SRAM). Only write to flash on explicit save or on a dirty-flag timeout (e.g., 5 seconds after last change).
- For per-parameter SysEx: update the in-memory config immediately (instant response), defer flash write with a debounce timer.
- Consider tracking a "dirty" flag per config section rather than saving everything on every change.

**Warning signs:**
- Config save takes more than 500ms (measured from SysEx receive to response)
- ESP32 `log_d` shows file I/O errors
- Config data becomes corrupted (LittleFS sector failure)

**Phase to address:**
Firmware Service Extraction phase (DataManager rewrite). Must be done before per-parameter SysEx, since per-parameter sync would multiply the write frequency.

---

### Pitfall 6: Web Rewrite Loses Existing Editor Features

**What goes wrong:**
A full rewrite from JavaScript/Chakra to TypeScript/Radix starts from scratch. Features that existed in the old editor get forgotten because they were buried in the 500-line MidiProvider or in obscure UI components. Common casualties: config file download/upload (`.topo` files), demo mode, CC message monitoring, sync status indicators, firmware version checking, and the specific toast notification flows users rely on for feedback.

**Why it happens:**
The rewrite starts with architecture (contexts, hooks, services) and core flows (connect, sync). "Secondary" features get deferred to "later" and then forgotten. The old codebase is discarded before all features are catalogued.

**How to avoid:**
- Before writing any new code, create a feature inventory of the existing editor. Walk through every function exported from `MidiProvider` context: `connect`, `disconnect`, `sendSysex`, `sendConfig`, `downloadConfig`, `uploadConfig`, `startFullCalibration`, `setDemo`, `updateSyncStatus`. Walk through every page and component.
- Use the feature inventory as a checklist. Mark each feature as "must have for parity", "improve", or "remove". Nothing gets removed without an explicit decision.
- Build the new editor page-by-page, verifying feature parity against the checklist before moving to the next page.

**Warning signs:**
- "We will add that later" appearing more than twice in planning
- Users of the old editor reporting missing features after the new editor ships
- The new editor has fewer routes than the old one

**Phase to address:**
Web Rewrite phase. The feature inventory should be created in the planning/research phase (now), and the checklist should gate the "done" criteria for the web rewrite phase.

---

### Pitfall 7: Cross-Core Race Conditions After Refactoring

**What goes wrong:**
The ADC task runs on core 0 (`xTaskCreatePinnedToCore` in `Adc.cpp`), continuously writing FSR values. `Keyboard::Update()` runs on core 1 (main loop), reading those same values. Currently there is no synchronization -- it works by accident because the reads/writes are naturally atomic for single `uint16_t` values on the ESP32-S3 (32-bit aligned reads are atomic on Xtensa). During refactoring, if config data or state structures are shared across cores (e.g., a service on core 0 reads `cfg` while the SysEx handler on core 1 writes it), non-atomic reads will cause torn data: half-old, half-new config values applied simultaneously.

**Why it happens:**
The current code avoids the problem by running almost everything on one core. The ADC task is the only cross-core boundary, and it only writes `uint16_t` values. Refactoring may introduce new FreeRTOS tasks or move services to different cores for performance.

**How to avoid:**
- Keep all config mutation on a single core. The SysEx handler, config load/save, and `ApplyConfiguration()` should all run on the same core.
- If services need cross-core communication, use FreeRTOS queues or semaphores, not shared mutable state.
- Document which core each service runs on. Make it a conscious architectural decision, not an accident.
- For the ADC values specifically: use `std::atomic<uint16_t>` or `volatile` with compiler barriers.

**Warning signs:**
- Intermittent config corruption that only happens under specific timing
- Key velocity values occasionally reading as 0 or max during normal play
- Crash in `ApplyConfiguration()` that cannot be reproduced reliably

**Phase to address:**
Firmware Service Extraction phase. Decide core assignment as part of the architecture, before writing service code.

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Keep magic-byte SysEx alongside new protocol indefinitely | No migration pain | Two code paths to maintain, doubles SysEx handler complexity | Only for one firmware version transition; remove old protocol after |
| Skip unit tests during service extraction | Faster initial extraction | No regression safety net; velocity curve or note mapping bugs discovered by users | Never -- extract + test together or don't extract |
| Copy-paste MidiProvider.jsx logic into new TypeScript services | Quick web rewrite progress | Carries forward the god-object pattern in a new language | Never -- decompose during migration, not after |
| In-memory config without flash write-back timer | Simpler implementation | Power loss = lost config | Only if you add explicit "save" UX or save on mode switch/disconnect |
| Hardcode 4 banks in the new architecture | Matches current behavior | Same scaling ceiling as today | Acceptable for MVP if the data structures are at least array-based (not 4 named variables) |

## Integration Gotchas

Common mistakes when connecting firmware, web editor, and MIDI hosts.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| WebMIDI SysEx | Assuming SysEx is always available -- Chrome requires `sysex: true` permission and user gesture. Some browsers block it entirely. | Check `navigator.requestMIDIAccess({ sysex: true })` result. Show clear error if denied. Test in Firefox (no SysEx support). |
| BLE MIDI SysEx | Sending SysEx packets larger than BLE MTU (typically 20 bytes default, 512 after negotiation). Large configs get silently truncated. | Negotiate MTU on connection. Implement chunked SysEx for payloads over MTU. Verify chunk reassembly on both sides. |
| esptool-js firmware flashing | Assuming Web Serial and WebMIDI can coexist on the same USB port. They cannot -- one must release the port before the other can claim it. | Disconnect MIDI before entering flash mode. After flashing, prompt user to reconnect MIDI. Handle the port-in-use error gracefully. |
| LittleFS config persistence | Assuming config file survives firmware flash. It does -- LittleFS partition is separate from app partition. But a partition table change (e.g., resizing app partition) will erase it. | Never change partition table without migration tooling. Document the partition layout. |
| ArduinoJson v7 | Using `JsonDocument` without size awareness. v7 uses dynamic allocation by default (no fixed `StaticJsonDocument<N>`). On ESP32-S3 with ~512KB SRAM, large documents can fragment heap. | Set explicit capacity or use `JsonDocument doc; doc.overflowPolicy(DeserializationOption::FailOnOverflow)`. Monitor free heap during config operations. |

## Performance Traps

Patterns that work at small scale but fail as config complexity grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Full config JSON round-trip on every parameter change | 3-second lag between knob turn and device response | In-memory config with debounced flash writes; per-parameter SysEx sends only changed field | Already broken -- 3-second sync is the primary UX complaint |
| Re-rendering entire React tree on any config change | UI jank when adjusting sliders, delayed visual feedback | Split config context into granular contexts (connection, per-bank config, global config) so changes only re-render affected components | Noticeable with 4 banks x 10+ parameters; worse if bank count increases |
| JSON serialization for SysEx payloads | CPU time on ESP32-S3 for JSON parse/serialize; unnecessary overhead for single-field updates | Binary encoding for per-parameter updates (command byte + param ID + value = 4-6 bytes vs 1KB+ JSON) | Already suboptimal; JSON overhead dominates the 3-second save time |
| ADC tight loop without yield on core 0 | BLE stack starvation; intermittent BLE disconnects; increased power draw | Add `vTaskDelay(1)` between scan cycles; the commented-out delay at line 138 of Adc.cpp should be restored | Manifests as flaky BLE connections, especially on iOS |

## Security Mistakes

Domain-specific security issues for MIDI controllers with web configurators.

| Mistake | Risk | Prevention |
|---------|------|------------|
| No SysEx manufacturer ID prefix | Any MIDI device on the bus can impersonate config commands; a rogue MIDI message could wipe calibration | Use a registered MIDI manufacturer ID or a unique 3-byte prefix. Validate prefix on receive before processing. |
| Unbounded `DeserializeFromBuffer` | Malformed SysEx payload causes heap overflow on ESP32-S3, crashing the device | Validate payload length before calling `deserializeJson`. Set `JsonDocument` capacity limit to 4KB. |
| No config file schema validation on upload | A crafted `.topo` file could set out-of-range values (e.g., MIDI channel 255, velocity curve index beyond array bounds) causing firmware crashes or undefined behavior | Validate every field against allowed ranges on both firmware (after deserialize) and web (before send). Clamp to valid ranges. |
| Hardcoded WiFi credentials in `Ota.hpp` | Credentials in git history even if file is removed | Already in git history. If OTA is re-enabled, use build-time environment variables or a provisioning flow. |

## UX Pitfalls

Common user experience mistakes in MIDI controller configurators.

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No visual feedback during config sync | User changes a setting and sees nothing happen for 3 seconds, thinks it did not work, changes it again | Show immediate optimistic update in UI; show sync indicator (spinner/checkmark) per parameter; revert on failure |
| Requiring page reload after firmware flash | User flashes firmware, device reboots, but the web editor loses connection and shows stale state | Auto-reconnect after firmware flash with a "reconnecting..." state. Poll for device availability. |
| Scale visualizer not matching device state | The note grid shows chromatic when the device is set to pentatonic -- broken `ScaleVisualizer` was identified as an existing issue | Derive visualizer state directly from the synced config, not from separate UI state |
| No undo for config changes | User accidentally changes velocity curve for all banks and cannot revert | Keep a config history stack (last 5 states). Offer "undo last change" button. The `.topo` download already provides manual backup. |
| Hiding connection errors behind generic toasts | "Connection failed" toast with no actionable information | Show specific errors: "SysEx permission denied -- click to learn how to enable", "Device not found -- is it plugged in?", "BLE not supported in this browser" |

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **Service extraction:** Services compile and run -- but verify all Signal connections are re-established. Check that `Key::onStateChanged`, `Button::onStateChanged`, and `TouchSlider::onSensorTouched` are connected to the right handlers after extraction.
- [ ] **Per-parameter SysEx:** Single parameter updates work -- but verify that switching banks sends the correct bank's parameters, not the previous bank's cached values.
- [ ] **Config migration:** Version bump from 103 to 104 works -- but verify that a device with version 102 (or corrupt/missing version field) also migrates correctly, not just the happy path.
- [ ] **Web TypeScript migration:** Components render with correct types -- but verify that SysEx byte encoding/decoding produces identical bytes to the old JavaScript implementation. Off-by-one in typed array handling is common.
- [ ] **LED pattern extraction:** Patterns display correctly -- but verify that `TransitionToPattern` no longer leaks memory (the existing `WaveTransition` leak). The fix must ship with the extraction, not after.
- [ ] **BLE MIDI:** Connection works in Chrome on desktop -- but verify on iOS Safari (BLE Web API differences), Android Chrome, and with MTU negotiation for SysEx payloads.
- [ ] **Config backup/restore:** Download `.topo` file works -- but verify that uploading a config from a different firmware version triggers migration, not silent corruption.
- [ ] **Quick Settings:** On-device quick settings save correctly -- but verify that changing a setting via quick settings and then opening the web editor shows the updated value (full round-trip sync).

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Config schema divergence (corrupted device config) | LOW | Device still functions with default config. Flash firmware with factory-reset-on-boot flag, or send "reset to defaults" SysEx from editor. |
| Stuck MIDI notes after refactor | LOW | Send All Notes Off (CC 123) on all channels. Add an explicit "panic" button to the editor. Firmware should send All Notes Off on mode switch. |
| Velocity curve regression | MEDIUM | Revert to last known-good firmware. Capture ADC-to-velocity reference data to prevent recurrence. |
| SysEx protocol incompatibility | MEDIUM | Keep old editor version accessible at a versioned URL (e.g., `/v1/`). Firmware can support both protocols temporarily. |
| Flash wear / config corruption | HIGH | LittleFS format and recalibration required. User loses all config. Mitigate by implementing config backup before the refactor ships. |
| Cross-core race condition (intermittent crash) | HIGH | Difficult to diagnose. Add heap canary checks and watchdog timer. Log last-known-good state before crash. Core dump analysis via ESP-IDF monitor. |

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Config schema divergence | SysEx Protocol Design | Round-trip test: firmware serialize -> web deserialize -> web serialize -> firmware deserialize = identical config |
| Breaking MIDI output | Firmware Service Extraction | MIDI loopback test after each service extraction: every note-on has matching note-off, velocity values match expected curves |
| Velocity curve regression | Firmware Service Extraction | Capture ADC-to-velocity lookup table before refactor; compare after refactor; values must be identical |
| SysEx protocol bricks devices | SysEx Protocol Design | Test new firmware with old editor AND new editor with old firmware; both must function (possibly degraded) |
| LittleFS flash wear | Firmware Service Extraction (DataManager rewrite) | Measure write count per config save: must be 1 (not 11). Measure save duration: must be < 500ms |
| Web rewrite loses features | Web Rewrite (planning gate) | Feature inventory checklist: every function in old MidiProvider context has equivalent in new codebase |
| Cross-core race conditions | Firmware Service Extraction (architecture decision) | Document core assignment per service. Use ThreadSanitizer-equivalent (ESP-IDF has `CONFIG_FREERTOS_CHECK_MUTEX_GIVEN_BY_OWNER`) |

## Sources

- Codebase analysis: `src/main.cpp`, `src/Configuration.cpp`, `src/Configuration.hpp`, `src/Libs/DataManager.hpp`, `editor-tx/src/components/MidiProvider.jsx`
- Known bugs and concerns: `.planning/codebase/CONCERNS.md`
- Architecture gaps: `.planning/codebase/ARCHITECTURE.md`
- ESP32-S3 flash endurance: NOR flash ~100K write cycles per sector (manufacturer spec)
- WebMIDI SysEx browser support: Chrome requires explicit permission; Firefox does not support SysEx
- BLE MIDI MTU limitations: default 20 bytes, negotiable to 512 (Bluetooth 4.2+ spec)

---
*Pitfalls research for: T16 MIDI controller firmware refactor + web configurator rewrite*
*Researched: 2026-04-03*
