# Phase 1: Protocol & Data Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-03
**Phase:** 01-protocol-data-foundation
**Areas discussed:** Config schema & types, DataManager rewrite

---

## Config Schema & Types

### Canonical config definition source

| Option | Description | Selected |
|--------|-------------|----------|
| JSON Schema (Recommended) | Language-neutral .json schema file generates both TS types and C++ struct helpers. Round-trip test validates both sides against the schema. | ✓ |
| TypeScript as source | Config types defined in .ts, a codegen script produces C++ struct field maps from it. | |
| Manual sync + test | Keep separate C++ structs and TS types, add a round-trip integration test to catch drift. No codegen tooling. | |

**User's choice:** JSON Schema (Recommended)
**Notes:** None

### Config structure nesting

| Option | Description | Selected |
|--------|-------------|----------|
| Two-level max (Recommended) | Top-level domains (global, banks[]) with flat fields within each. Simple SysEx addressing: domain + key. | ✓ |
| Flat with prefixed keys | All fields at root level with naming prefixes (e.g., bank0_scale). Simplest SysEx but messy at 4 banks x 10+ fields. | |
| Fully nested by domain | Deep nesting like banks[0].keyboard.velocity_curve. Clean grouping but more complex path encoding in SysEx. | |

**User's choice:** Two-level max (Recommended)
**Notes:** None

### Config version migration strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Single v103→v200 (Recommended) | One migration function that maps old format to new schema. Clean, testable, matches the single known legacy format. Future incremental migrations deferred to ADVFW-01. | ✓ |
| Incremental chain | Series of step-by-step migration functions (v103→104→...→200). More flexible but over-engineered for one known source version. | |
| You decide | Claude picks the best approach during planning based on the actual config field mapping. | |

**User's choice:** Single v103→v200 (Recommended)
**Notes:** None

### Custom scales storage

| Option | Description | Selected |
|--------|-------------|----------|
| Keep in global config (Recommended) | Custom scales stay as arrays in the global section of the schema. Simple, matches current behavior, no extra files. | ✓ |
| Separate storage | Custom scales get their own LittleFS file. Allows unlimited scales in the future, but adds complexity now. | |
| You decide | Claude picks based on flash usage and schema simplicity during planning. | |

**User's choice:** Keep in global config (Recommended)
**Notes:** None

---

## DataManager Rewrite

### In-memory config pattern

| Option | Description | Selected |
|--------|-------------|----------|
| Hybrid structs + JSON (Recommended) | C++ structs for runtime, JsonDocument only at load/save boundaries. ConfigManager class holds structs, dirty-flag triggers single write. Natural fit for Phase 2 service extraction. | ✓ |
| JsonDocument in-memory | Keep ArduinoJson document loaded in RAM. Simpler code, but runtime access is string-key lookups instead of typed struct fields. | |
| You decide | Claude picks during planning based on memory constraints and code complexity. | |

**User's choice:** Hybrid structs + JSON (Recommended)
**Notes:** None

### Save trigger (flash flush)

| Option | Description | Selected |
|--------|-------------|----------|
| Flush on idle (Recommended) | Apply to RAM instantly, write to flash when no SysEx traffic for ~2s. Best flash wear reduction, batches rapid edits. | ✓ |
| Explicit Save() call | Only write when firmware explicitly calls Save(). Deterministic but caller must remember. | |
| Periodic timer (e.g. 5s) | Check dirty flag every 5 seconds and flush if dirty. Simple but writes on a fixed schedule regardless of activity. | |

**User's choice:** Flush on idle (Recommended)
**Notes:** None

### Calibration data handling

| Option | Description | Selected |
|--------|-------------|----------|
| Separate (Recommended) | Calibration keeps its own simple file manager. Different lifecycle (write-once at factory), different schema, no dirty-flag needed. | ✓ |
| Unified manager | Single ConfigManager handles both config and calibration. Fewer classes but mixes write-often and write-once data. | |
| You decide | Claude picks based on code complexity during planning. | |

**User's choice:** Separate (Recommended)
**Notes:** None

---

## Claude's Discretion

- SysEx command framing design (manufacturer ID, command bytes, payload encoding, BLE chunking)
- Per-parameter SysEx addressing scheme

## Deferred Ideas

None — discussion stayed within phase scope.
