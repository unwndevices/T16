# Phase 1: Protocol & Data Foundation - Context

**Gathered:** 2026-04-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Define the shared SysEx contract between firmware and web editor, rewrite DataManager for efficient persistence (load-once/write-once), and establish a canonical config schema as the single source of truth for both codebases. This phase produces the protocol and data layer that Phases 2-5 build on.

</domain>

<decisions>
## Implementation Decisions

### Config Schema & Types
- **D-01:** Canonical config definition lives in a **JSON Schema file** — language-neutral, generates both TypeScript types and C++ struct helpers. Round-trip test validates both sides against the schema.
- **D-02:** Config structure uses **two-level max nesting** — top-level domains (`global`, `banks[]`) with flat fields within each. Keeps SysEx path addressing simple (domain + key) while matching the UI's bank-switching model.
- **D-03:** Config version migration is a **single function** (`migrateV103toV200`) — one known legacy format, no incremental chain. Future incremental migrations deferred to ADVFW-01.
- **D-04:** Custom scales **stay in the global config** section as arrays — no separate storage file. Matches current behavior, keeps schema simple.

### DataManager Rewrite
- **D-05:** In-memory config uses **hybrid pattern** — C++ structs for runtime access, JsonDocument only at load/save boundaries. A `ConfigManager` class holds the structs, marks dirty on mutation, and serializes to LittleFS on save. Natural fit for Phase 2 service extraction.
- **D-06:** Config flushes to flash on **idle detection** — changes apply to RAM instantly, write to flash when no SysEx traffic for ~2 seconds. Batches rapid edits, reduces flash wear, aligns with <100ms round-trip goal.
- **D-07:** Calibration data uses a **separate manager** — different lifecycle (write-once at factory), different schema, no dirty-flag needed. Keeps ConfigManager focused on runtime config.

### Claude's Discretion
- SysEx command framing design (manufacturer ID, command bytes, payload encoding, BLE chunking) — not discussed, Claude has full flexibility during research and planning
- Per-parameter SysEx addressing scheme (how individual field changes are encoded) — not discussed, Claude designs this during planning

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Firmware
- `src/Configuration.hpp` — Current config structs (ConfigurationData, KeyModeData, ControlChangeData, Parameters, QuickSettingsData) and extern declarations
- `src/Configuration.cpp` — Current save/load implementation with per-field DataManager calls
- `src/Libs/DataManager.hpp` — Current persistence layer (per-call filesystem round-trips, SerializeToBuffer/DeserializeFromBuffer)
- `src/main.cpp` lines 687-723 — Current SysEx handling (ProcessSysEx with magic byte matching)
- `src/Libs/MidiProvider.hpp` — Current MIDI transport facade (USB, BLE, TRS)

### Web Editor
- `editor-tx/src/components/MidiProvider.jsx` — Current SysEx send/receive, config state management, JSON serialization

### Codebase Analysis
- `.planning/codebase/ARCHITECTURE.md` — Data flow, key abstractions, cross-cutting concerns
- `.planning/codebase/CONVENTIONS.md` — Naming patterns, eisei comparison (target style)
- `.planning/codebase/STACK.md` — ArduinoJson 7.0.3, LittleFS, StreamUtils dependencies

### Project
- `.planning/REQUIREMENTS.md` — PROTO-01 through PROTO-05, FWARCH-06, WEBARCH-05, FWFEAT-03
- `.planning/ROADMAP.md` — Phase 1 success criteria (5 items)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ConfigurationData`, `KeyModeData`, `ControlChangeData` structs — field definitions map directly to JSON Schema properties
- `DataManager::SerializeToBuffer()` / `DeserializeFromBuffer()` — can inform the new ConfigManager's save/load boundary
- `MidiProvider::SendSysEx()` — existing transport layer for sending SysEx, wraps USB/BLE/TRS

### Established Patterns
- ArduinoJson 7.x `JsonDocument` for serialization — stays as the JSON layer
- LittleFS for flash storage — stays as the filesystem
- Config version field (`cfg.version`) — used for migration detection at boot
- `hasChanged` dirty flag on DataManager — similar concept reused in new ConfigManager

### Integration Points
- `ProcessSysEx()` in main.cpp — will be replaced by structured command router (Phase 2 extracts this further)
- `LoadConfiguration()` / `SaveConfiguration()` — callers in main.cpp must update to use new ConfigManager API
- `ApplyConfiguration()` — downstream consumer of loaded config, interface may change
- Web editor `MidiProvider.jsx` — config serialization must match new schema exactly

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches for protocol design and persistence patterns.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-protocol-data-foundation*
*Context gathered: 2026-04-03*
