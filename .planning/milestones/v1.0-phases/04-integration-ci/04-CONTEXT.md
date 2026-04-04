# Phase 4: Integration & CI - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Per-parameter config sync reaches the device in under 100ms, firmware updates work without holding the bootloader button, config imports are validated against the schema, and CI validates every push. This phase wires together the protocol (Phase 1), firmware services (Phase 2), and web rewrite (Phase 3) into a working end-to-end system with automated quality gates.

</domain>

<decisions>
## Implementation Decisions

### Per-Parameter Sync Performance
- Measure round-trip time via timestamps in the web editor — log send time in ConnectionContext, measure ACK return. Display in dev console, assert in integration tests.
- Skip BLE for full config dumps — BLE used for note/CC only, full config sync requires USB connection. BLE MTU (20 bytes) makes chunked SysEx unreliable.
- No debounce on parameter sends — each param change sends immediately (single SysEx packet is small). Firmware ConfigManager already batches flash writes via 2s idle flush.
- On sync failure: retry once after 500ms timeout, then show toast notification and mark SyncIndicator red.

### Firmware Update Flow
- ESP32-S3 bootloader entry via SysEx command — firmware receives BOOTLOADER SysEx, calls `ESP.restart()` into download mode via USB strapping pins. esptool-js handles the rest.
- Two-step UX: user clicks "Update Firmware" on Upload page, SysEx triggers bootloader, then esptool-js connects via Web Serial automatically.
- Keep manual boot button instructions as fallback — show "Hold BOOT button" as a fallback link below the Update button for boards/USB hubs that don't support software bootloader entry.
- Firmware reports version via VERSION command. Upload page compares with selected binary version. Warn (not block) if downgrading.

### Config Import Validation
- TypeScript runtime type validation against config.ts types. Reject if structure doesn't match, show specific field-level error.
- Specific error messaging — "Missing field: banks[0].scale" or "Invalid value: velocity_curve must be 0-4". Show in toast with expandable details.
- Migrate then validate — if imported config has version < 200, run migration logic (same as firmware). If migration succeeds, import. If not, reject with version error.
- Keep .topo file extension — established format, users have existing exports. Add JSON schema version field to exported files.

### CI Pipeline
- Trigger on push to any branch + PR to main. Deploy only on main merge (existing behavior).
- PlatformIO in GitHub Actions — `pio run` for build verification, `pio test -e native` for unit tests.
- ESLint + Prettier for web, clang-format for firmware — fail CI on lint errors. Add .clang-format with project conventions (Allman braces, 4-space indent).
- No coverage gate — run tests, fail on test failures, don't enforce coverage percentage.

### Claude's Discretion
- Exact ESP32-S3 bootloader entry sequence (GPIO strapping vs USB-CDC reset)
- CI action versions and caching strategy
- Prettier configuration details
- Test expansion scope (which additional tests to add)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/SysExProtocol.hpp` — Full protocol constants (VERSION, CONFIG, PARAM, CALIBRATION commands)
- `src/SysExHandler.hpp/cpp` — Validated SysEx dispatch with HandleParamSet(), HandleConfigDumpRequest()
- `src/ConfigManager.hpp/cpp` — Per-parameter setters, lazy persistence with 2s idle flush
- `editor-tx/src/protocol/sysex.ts` — Mirrored protocol constants + encoding/decoding
- `editor-tx/src/services/midi.ts` — sendParamUpdate(), sendFullConfig(), parseSysExMessage()
- `editor-tx/src/contexts/ConnectionContext.tsx` — WebMIDI lifecycle, device discovery
- `editor-tx/src/contexts/ConfigContext.tsx` — Device config state, sync tracking
- `editor-tx/src/pages/Upload/Upload.tsx` — esptool-js integration (v0.6.0)
- `test/` — 7 firmware test suites (Unity), 3 web test files (Vitest + RTL)

### Established Patterns
- SysEx protocol: manufacturer ID 0x7D, command byte framing, ACK responses
- ConfigManager: integer field IDs for per-parameter setters matching SysEx addressing
- Web contexts: createContext<T|null>(null) + useX() with throw guard
- Service layer: pure functions in services/midi.ts, no React dependencies
- Test mocking: vi.mock('@/services/midi') for context tests

### Integration Points
- Firmware: AppEngine.init()/update() orchestration, SysExHandler processes incoming
- Web: ConnectionContext handles device connect/disconnect, ConfigContext handles config state
- Protocol: SysExProtocol.hpp ↔ sysex.ts constants are synchronized
- CI: .github/workflows/deploy.yml exists (FTP deploy only, needs expansion)
- Upload: esptool-js 0.6.0 with Web Serial, firmware binaries in assets/firmwares/

</code_context>

<specifics>
## Specific Ideas

No specific requirements — all recommended approaches accepted. Focus on wiring existing Phase 1-3 work together.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
