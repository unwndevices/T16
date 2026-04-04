# Phase 1: Protocol & Data Foundation - Research

**Researched:** 2026-04-03
**Domain:** MIDI SysEx protocol design, embedded config persistence, shared schema generation
**Confidence:** HIGH

## Summary

Phase 1 replaces the T16's ad-hoc SysEx byte-matching with a structured protocol, rewrites DataManager into a load-once/write-once ConfigManager, and establishes a JSON Schema as the canonical config definition shared between firmware and web editor. The current codebase sends raw JSON over SysEx (already 7-bit safe at ~1KB), uses magic byte matching for command dispatch, and performs 11+ filesystem round-trips per save. The fix is straightforward: structured command framing with manufacturer ID prefix, a RAM-resident config with dirty-flag flush, and a JSON Schema file that generates both TypeScript types and guides C++ struct layout.

The config JSON payload is approximately 1,048 bytes in compact form. All characters are standard ASCII and inherently 7-bit safe, so no SysEx encoding/packing is needed for the JSON payload -- it can be sent as raw ASCII bytes. This significantly simplifies both firmware and web implementations.

**Primary recommendation:** Use manufacturer ID `0x7D` (non-commercial), a 2-byte command header (command + subcommand), JSON payloads as raw ASCII, and ArduinoJson 7's in-memory JsonDocument at load/save boundaries only. The JSON Schema file lives in the repo root and a build-time script generates TypeScript types from it.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Canonical config definition lives in a **JSON Schema file** -- language-neutral, generates both TypeScript types and C++ struct helpers. Round-trip test validates both sides against the schema.
- **D-02:** Config structure uses **two-level max nesting** -- top-level domains (`global`, `banks[]`) with flat fields within each. Keeps SysEx path addressing simple (domain + key) while matching the UI's bank-switching model.
- **D-03:** Config version migration is a **single function** (`migrateV103toV200`) -- one known legacy format, no incremental chain. Future incremental migrations deferred to ADVFW-01.
- **D-04:** Custom scales **stay in the global config** section as arrays -- no separate storage file. Matches current behavior, keeps schema simple.
- **D-05:** In-memory config uses **hybrid pattern** -- C++ structs for runtime access, JsonDocument only at load/save boundaries. A `ConfigManager` class holds the structs, marks dirty on mutation, and serializes to LittleFS on save. Natural fit for Phase 2 service extraction.
- **D-06:** Config flushes to flash on **idle detection** -- changes apply to RAM instantly, write to flash when no SysEx traffic for ~2 seconds. Batches rapid edits, reduces flash wear, aligns with <100ms round-trip goal.
- **D-07:** Calibration data uses a **separate manager** -- different lifecycle (write-once at factory), different schema, no dirty-flag needed. Keeps ConfigManager focused on runtime config.

### Claude's Discretion
- SysEx command framing design (manufacturer ID, command bytes, payload encoding, BLE chunking)
- Per-parameter SysEx addressing scheme (how individual field changes are encoded)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PROTO-01 | Device uses structured SysEx with unwn manufacturer ID prefix and command byte framing | SysEx framing design (manufacturer ID, command table, message format) |
| PROTO-02 | Editor can send per-parameter updates via SysEx (single value change, <100ms round-trip) | Per-parameter addressing scheme, ConfigManager dirty-flag pattern |
| PROTO-03 | Editor can request and receive full config dump for initial sync and backup/restore | Full dump command, JSON payload format, existing ~1KB payload analysis |
| PROTO-04 | Firmware validates SysEx payload length and structure before deserialization | Validation pattern, length checking, command byte verification |
| PROTO-05 | Protocol supports version handshake on connect (firmware reports protocol version to editor) | Handshake command design, version reporting format |
| FWARCH-06 | DataManager rewritten -- load once, modify in-memory, write once | ConfigManager architecture, idle-flush pattern, ArduinoJson best practices |
| WEBARCH-05 | Shared TypeScript types for config schema (single source of truth for firmware JSON keys) | JSON Schema tooling, quicktype for TS generation, schema file location |
| FWFEAT-03 | Configuration version migration -- non-destructive upgrade from v103 to new format | Migration function design, field mapping from v103 to v200 |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ArduinoJson | 7.0.3 | JSON serialization at load/save boundaries | Already in use, mature ESP32 support, pool allocator for embedded |
| LittleFS | (bundled) | Flash filesystem for config persistence | Already in use, wear-leveling built in, ESP32 standard |
| webmidi | 3.1.8 | Web MIDI API wrapper for SysEx send/receive | Already in use, handles manufacturer ID framing automatically |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| quicktype | latest (CLI) | Generate TypeScript types from JSON Schema | Build-time script, not a runtime dependency |
| ajv | latest | JSON Schema validation in editor (dev/test) | Round-trip validation tests, config import validation |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| quicktype | json-schema-to-typescript | quicktype handles more languages, but json-schema-to-typescript is TS-focused and lighter. Either works -- quicktype preferred for potential future C++ generation. |
| ajv | zod (from schema) | Zod is runtime-only, ajv validates against the canonical JSON Schema directly. ajv aligns better with D-01 (schema is source of truth). |

**Installation:**
```bash
# Dev dependency for editor-tx (build-time type generation)
cd editor-tx && npm install --save-dev quicktype ajv

# No new firmware dependencies -- ArduinoJson 7.0.3 and LittleFS already present
```

## Architecture Patterns

### Recommended Project Structure (Phase 1 additions)
```
schema/
  t16-config.schema.json    # Canonical config schema (JSON Schema draft 2020-12)
  generate-types.sh         # Build script: quicktype schema -> TypeScript types

src/
  ConfigManager.hpp         # New: RAM-resident config with dirty flag + idle flush
  ConfigManager.cpp         # New: load/save/migrate implementation
  SysExProtocol.hpp         # New: command definitions, framing constants, validation
  Libs/
    DataManager.hpp         # Kept temporarily for CalibrationManager (D-07)

editor-tx/
  src/
    types/
      config.ts             # Generated from schema (DO NOT EDIT manually)
    protocol/
      sysex.ts              # SysEx command constants mirroring firmware definitions
```

### Pattern 1: SysEx Command Framing
**What:** Every SysEx message follows a fixed header structure for reliable parsing.
**When to use:** All firmware-editor communication.

**Message format:**
```
F0              SysEx start (added by MIDI library)
7D              Manufacturer ID (non-commercial)
<cmd>           Command byte (7-bit, 0x00-0x7F)
<sub>           Sub-command byte (7-bit)
[payload...]    Variable-length data (all bytes 7-bit)
F7              SysEx end (added by MIDI library)
```

**Command table:**
```
Cmd  Sub  Direction     Name                    Payload
01   01   editor->fw    VERSION_REQUEST         (none)
01   02   fw->editor    VERSION_RESPONSE        [proto_ver, fw_ver]
02   01   editor->fw    CONFIG_DUMP_REQUEST     (none)
02   02   fw->editor    CONFIG_DUMP_RESPONSE    [JSON bytes...]
02   03   editor->fw    CONFIG_LOAD_FULL        [JSON bytes...]
02   04   fw->editor    CONFIG_LOAD_ACK         [status]
03   01   editor->fw    PARAM_SET               [bank, key_id, value...]
03   02   fw->editor    PARAM_SET_ACK           [status]
04   01   editor->fw    CALIBRATION_RESET       (none)
```

**Why 0x7D:** Reserved by MIDI spec for non-commercial/educational use. The T16 is not a commercial product requiring a registered manufacturer ID. Using 0x7D is correct and free.

**Why not 0x7E/0x7F:** Those are Universal Non-Realtime and Universal Realtime, respectively. They have specific sub-ID assignments managed by the MIDI Association and should not be used for custom device protocols. The current code uses 0x7E, which is technically incorrect.

### Pattern 2: ConfigManager (Hybrid RAM + Flash)
**What:** C++ class holding config structs in RAM, with a dirty flag and idle-timer flush.
**When to use:** All config reads/writes throughout firmware.

```cpp
// ConfigManager.hpp
#pragma once
#include <ArduinoJson.h>
#include "Configuration.hpp"

class ConfigManager {
public:
    void Init();               // Load from flash into structs
    void Save(bool force = false);  // Serialize structs to flash (if dirty or forced)
    bool IsDirty() const;
    void MarkDirty();
    void CheckIdleFlush(unsigned long now);  // Call from loop()

    // Direct struct access for runtime reads
    ConfigurationData& Global();
    KeyModeData& Bank(uint8_t index);
    ControlChangeData& CC(uint8_t index);

    // Per-parameter mutation (marks dirty automatically)
    void SetGlobalParam(const char* key, uint8_t value);
    void SetBankParam(uint8_t bank, const char* key, uint8_t value);

    // Full config as JSON (for SysEx dump)
    size_t SerializeToBuffer(char* buffer, size_t maxSize);
    bool DeserializeFromBuffer(const char* buffer, size_t length);

    // Migration
    bool MigrateIfNeeded();

private:
    ConfigurationData global_;
    KeyModeData banks_[BANK_AMT];
    ControlChangeData cc_[BANK_AMT];

    bool dirty_ = false;
    unsigned long lastChangeTime_ = 0;
    static constexpr unsigned long IDLE_FLUSH_MS = 2000;

    void LoadFromFlash();
    void SaveToFlash();
    bool MigrateV103ToV200(JsonDocument& doc);
};
```

### Pattern 3: Per-Parameter SysEx Addressing
**What:** Addressing scheme that maps `(domain, bank_index, field_id)` to a config parameter.
**When to use:** PROTO-02 per-parameter updates from editor.

```
PARAM_SET payload:  [domain] [bank_index] [field_id] [value_byte(s)]

domain:
  0x00 = global
  0x01 = bank keyboard config
  0x02 = bank CC config

bank_index:  0-3 (ignored for global domain)

field_id:  Sequential ID matching JSON key order in schema
  Global: mode=0, sensitivity=1, brightness=2, midi_trs=3, ...
  Bank KB: channel=0, scale=1, base_octave=2, base_note=3, ...
  Bank CC: (uses array indexing -- field_id = CC index, value = [channel, cc_id])
```

This keeps messages tiny (5-7 bytes total) for instant parameter updates. The editor sends one PARAM_SET per knob/slider change. ConfigManager applies to RAM immediately (no flash write), marks dirty, and the idle timer flushes later.

### Pattern 4: Config Version Migration
**What:** Single migration function from v103 to v200 format.
**When to use:** First boot after firmware update.

```cpp
bool ConfigManager::MigrateV103ToV200(JsonDocument& doc) {
    uint8_t version = doc["version"] | 0;
    if (version == 103) {
        // v103 -> v200 field mapping:
        // - "version" changes from 103 to 200
        // - All existing fields preserved with same keys
        // - New fields get defaults
        // - Structure reorganized to match schema
        doc["version"] = 200;
        // ... field mapping ...
        return true;
    }
    return false;  // Unknown version, leave as-is
}
```

### Anti-Patterns to Avoid
- **Per-field filesystem writes:** The current DataManager opens/reads/parses/writes the JSON file for EVERY field. ConfigManager must never do this -- load once, modify in RAM, save once.
- **Magic byte matching:** The current `if (data[2] == 127 && data[3] == 7)` pattern is fragile and undocumented. Use named constants and a command dispatch table.
- **JSON string in SysEx without length prefix:** Always validate the payload length matches expected format before calling `deserializeJson()`. A corrupted or truncated SysEx could crash ArduinoJson.
- **Using 0x7E for custom commands:** 0x7E is Universal Non-Realtime with registered sub-IDs. Custom device protocols must use 0x7D or a registered manufacturer ID.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| TypeScript type generation | Manual .ts type file matching firmware structs | quicktype from JSON Schema | Structs drift from types over time. Schema is single source of truth (D-01). |
| JSON Schema validation | Custom validation logic in JS | ajv library | Edge cases in type coercion, array bounds, required fields. ajv handles JSON Schema spec fully. |
| SysEx byte framing | Manual F0/F7 wrapping | webmidi.js `sendSysex()` / arduino_midi_library `sendSysEx()` | Both libraries handle start/end bytes automatically. Don't double-wrap. |
| Flash wear batching | Custom timer in loop() | `millis()` comparison in `ConfigManager::CheckIdleFlush()` | Simple timestamp comparison. Don't use FreeRTOS timers or interrupts for this -- loop() runs fast enough. |

**Key insight:** The main complexity in this phase is protocol design (getting the byte layout right), not implementation. The actual code for each piece is small. The risk is in mismatched assumptions between firmware and web.

## Common Pitfalls

### Pitfall 1: SysEx Byte Range Violations
**What goes wrong:** Sending a byte with value >= 128 (0x80) in a SysEx payload terminates the message prematurely. The MIDI spec reserves 0x80-0xFF for status bytes.
**Why it happens:** JSON ASCII is 7-bit safe, but negative int8_t values (like custom_scale entries which are `int8_t`) serialize as negative numbers in JSON (e.g., `-5`), which use the ASCII minus sign (0x2D) -- still safe. The real risk is sending raw binary config data instead of JSON.
**How to avoid:** Always send config as JSON text (ASCII). Never send raw struct bytes over SysEx. Validate all payload bytes < 128 before sending.
**Warning signs:** SysEx messages received shorter than expected. `deserializeJson()` fails with truncated input.

### Pitfall 2: ArduinoJson Document Lifetime
**What goes wrong:** Accessing JsonDocument members after the document goes out of scope yields garbage data. ArduinoJson 7 manages memory internally and frees on destruction.
**Why it happens:** Storing `JsonArray` or `JsonObject` references from a local JsonDocument, then using them after the function returns.
**How to avoid:** In ConfigManager: deserialize into structs immediately during `LoadFromFlash()`. Never store JsonDocument references across function boundaries. The document is ephemeral -- load, copy to structs, discard.
**Warning signs:** Config values appear as 0 or garbage after loading.

### Pitfall 3: webmidi.js sendSysex Manufacturer ID Handling
**What goes wrong:** webmidi.js `sendSysex()` takes the manufacturer ID as the first parameter and automatically prepends it. If you also include it in the data array, the message gets a doubled manufacturer ID.
**Why it happens:** Confusion about what the library handles vs. what you provide.
**How to avoid:** Call `output.sendSysex(0x7D, [cmd, sub, ...payload])`. The library adds `F0 7D` prefix and `F7` suffix automatically. The data array should start with the command byte, not the manufacturer ID.
**Warning signs:** Firmware receives unexpected bytes at positions 0-2. Command dispatch fails silently.

### Pitfall 4: LittleFS Write During SysEx Burst
**What goes wrong:** If the firmware writes to flash while processing a rapid sequence of SysEx messages, the flash write (~10-50ms on ESP32) blocks the main loop and causes MIDI buffer overflows.
**Why it happens:** The current code calls `SaveConfiguration()` immediately on receiving a config load command.
**How to avoid:** D-06 idle flush pattern. Apply changes to RAM immediately, only write to flash after 2 seconds of no SysEx traffic. `CheckIdleFlush()` in `loop()` handles this.
**Warning signs:** Config partially applied after rapid edits. MIDI messages dropped during save.

### Pitfall 5: Config Migration Data Loss
**What goes wrong:** Migration function runs but doesn't preserve a field that exists in v103 but was renamed or restructured in v200.
**Why it happens:** Incomplete field mapping in `migrateV103toV200()`.
**How to avoid:** Document every v103 field and its v200 equivalent. The migration function must read the old JSON, map every field, and write the new JSON. Test with a real v103 config dump.
**Warning signs:** After firmware update, device uses default values instead of user's saved config.

### Pitfall 6: CustomSettings::SysExMaxSize Too Small
**What goes wrong:** Full config dump (~1KB JSON + 3 header bytes) exceeds the MIDI library's SysEx buffer and gets silently truncated.
**Why it happens:** `CustomSettings::SysExMaxSize` is currently 2048, which is adequate for ~1KB payloads. But if the config grows (more banks, longer scales), it could exceed this.
**How to avoid:** Set `SysExMaxSize` to at least 2x the expected max config size. Current 2048 is fine for the ~1KB config. Monitor this if config structure grows.
**Warning signs:** Truncated JSON on the editor side. `JSON.parse()` throws on incomplete input.

## Code Examples

### Firmware: SysEx Command Dispatch
```cpp
// SysExProtocol.hpp
#pragma once
#include <Arduino.h>

namespace SysEx {
    constexpr uint8_t MANUFACTURER_ID = 0x7D;

    // Command bytes
    constexpr uint8_t CMD_VERSION     = 0x01;
    constexpr uint8_t CMD_CONFIG      = 0x02;
    constexpr uint8_t CMD_PARAM       = 0x03;
    constexpr uint8_t CMD_CALIBRATION = 0x04;

    // Sub-command bytes
    constexpr uint8_t SUB_REQUEST  = 0x01;
    constexpr uint8_t SUB_RESPONSE = 0x02;
    constexpr uint8_t SUB_LOAD     = 0x03;
    constexpr uint8_t SUB_ACK      = 0x04;

    // Protocol version
    constexpr uint8_t PROTOCOL_VERSION = 1;

    // Minimum valid message: F0 + mfr_id + cmd + sub + F7 = 5 bytes
    // But MIDI library strips F0/F7, so we see: mfr_id + cmd + sub = 3 bytes minimum
    // Actually arduino_midi_library includes F0 in data[0] and F7 at data[length]
    // So data[0]=F0, data[1]=mfr_id, data[2]=cmd, data[3]=sub, data[length-1] may be F7
    constexpr size_t MIN_MESSAGE_LENGTH = 4;  // F0 + mfr_id + cmd + sub
}
```

### Firmware: Validated SysEx Handler
```cpp
void ProcessSysEx(byte* data, unsigned length) {
    // Validate minimum length
    if (length < SysEx::MIN_MESSAGE_LENGTH) {
        log_d("SysEx too short: %u bytes", length);
        return;
    }

    // Validate manufacturer ID (data[1] after F0)
    if (data[1] != SysEx::MANUFACTURER_ID) {
        return;  // Not for us, silently ignore
    }

    uint8_t cmd = data[2];
    uint8_t sub = data[3];
    const byte* payload = data + 4;
    size_t payloadLen = length - 4;  // Subtract header bytes

    switch (cmd) {
        case SysEx::CMD_VERSION:
            if (sub == SysEx::SUB_REQUEST) {
                HandleVersionRequest();
            }
            break;

        case SysEx::CMD_CONFIG:
            if (sub == SysEx::SUB_REQUEST) {
                HandleConfigDumpRequest();
            } else if (sub == SysEx::SUB_LOAD) {
                HandleConfigLoad(payload, payloadLen);
            }
            break;

        case SysEx::CMD_PARAM:
            if (sub == SysEx::SUB_REQUEST && payloadLen >= 3) {
                HandleParamSet(payload, payloadLen);
            }
            break;

        case SysEx::CMD_CALIBRATION:
            if (sub == SysEx::SUB_REQUEST) {
                HandleCalibrationReset();
            }
            break;

        default:
            log_d("Unknown SysEx command: 0x%02X", cmd);
            break;
    }
}
```

### Web Editor: SysEx Constants (TypeScript)
```typescript
// editor-tx/src/protocol/sysex.ts
export const MANUFACTURER_ID = 0x7D

export const CMD = {
  VERSION: 0x01,
  CONFIG: 0x02,
  PARAM: 0x03,
  CALIBRATION: 0x04,
} as const

export const SUB = {
  REQUEST: 0x01,
  RESPONSE: 0x02,
  LOAD: 0x03,
  ACK: 0x04,
} as const

export function requestVersion(output: any) {
  output.sendSysex(MANUFACTURER_ID, [CMD.VERSION, SUB.REQUEST])
}

export function requestConfigDump(output: any) {
  output.sendSysex(MANUFACTURER_ID, [CMD.CONFIG, SUB.REQUEST])
}

export function sendParamUpdate(
  output: any,
  domain: number,
  bankIndex: number,
  fieldId: number,
  value: number
) {
  output.sendSysex(MANUFACTURER_ID, [
    CMD.PARAM, SUB.REQUEST,
    domain, bankIndex, fieldId, value
  ])
}

export function sendFullConfig(output: any, config: object) {
  const json = JSON.stringify(config)
  const data = Array.from(json).map(c => c.charCodeAt(0))
  output.sendSysex(MANUFACTURER_ID, [CMD.CONFIG, SUB.LOAD, ...data])
}
```

### ConfigManager: Idle Flush Pattern
```cpp
void ConfigManager::CheckIdleFlush(unsigned long now) {
    if (dirty_ && (now - lastChangeTime_ >= IDLE_FLUSH_MS)) {
        SaveToFlash();
        dirty_ = false;
        log_d("Config flushed to flash (idle)");
    }
}

void ConfigManager::MarkDirty() {
    dirty_ = true;
    lastChangeTime_ = millis();
}

// Called from loop():
// configManager.CheckIdleFlush(millis());
```

### JSON Schema Example
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://unwn.dev/t16/config.schema.json",
  "title": "T16 Configuration",
  "type": "object",
  "properties": {
    "version": { "type": "integer", "const": 200 },
    "global": {
      "type": "object",
      "properties": {
        "mode": { "type": "integer", "minimum": 0, "maximum": 4 },
        "sensitivity": { "type": "integer", "minimum": 0, "maximum": 3 },
        "brightness": { "type": "integer", "minimum": 0, "maximum": 8 },
        "midi_trs": { "type": "integer", "minimum": 0, "maximum": 1 },
        "trs_type": { "type": "integer", "minimum": 0, "maximum": 1 },
        "passthrough": { "type": "integer", "minimum": 0, "maximum": 1 },
        "midi_ble": { "type": "integer", "minimum": 0, "maximum": 1 },
        "custom_scale1": {
          "type": "array", "items": { "type": "integer" },
          "minItems": 16, "maxItems": 16
        },
        "custom_scale2": {
          "type": "array", "items": { "type": "integer" },
          "minItems": 16, "maxItems": 16
        }
      },
      "required": ["mode", "sensitivity", "brightness", "midi_trs", "trs_type", "passthrough", "midi_ble", "custom_scale1", "custom_scale2"]
    },
    "banks": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "ch": { "type": "integer", "minimum": 1, "maximum": 16 },
          "scale": { "type": "integer", "minimum": 0 },
          "oct": { "type": "integer", "minimum": 0, "maximum": 10 },
          "note": { "type": "integer", "minimum": 0, "maximum": 127 },
          "vel": { "type": "integer", "minimum": 0 },
          "at": { "type": "integer", "minimum": 0 },
          "flip_x": { "type": "integer", "minimum": 0, "maximum": 1 },
          "flip_y": { "type": "integer", "minimum": 0, "maximum": 1 },
          "koala_mode": { "type": "integer", "minimum": 0, "maximum": 1 },
          "chs": {
            "type": "array", "items": { "type": "integer", "minimum": 1, "maximum": 16 },
            "minItems": 8, "maxItems": 8
          },
          "ids": {
            "type": "array", "items": { "type": "integer", "minimum": 0, "maximum": 127 },
            "minItems": 8, "maxItems": 8
          }
        },
        "required": ["ch", "scale", "oct", "note", "vel", "at", "flip_x", "flip_y", "koala_mode", "chs", "ids"]
      },
      "minItems": 4,
      "maxItems": 4
    }
  },
  "required": ["version", "global", "banks"]
}
```

**Note on schema structure change (D-02):** The v200 schema wraps the top-level scalar fields into a `global` object. The v103 format has them flat at root level. The migration function moves these fields into the `global` key.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `DynamicJsonDocument(size)` | `JsonDocument` (auto-sizing) | ArduinoJson 7.0 (2024) | No manual capacity calculation needed. Already using v7. |
| SPIFFS | LittleFS | ESP32 Arduino Core 2.x | Better wear leveling, directory support. Already using LittleFS. |
| Manual type files | JSON Schema + code generation | Industry standard | Single source of truth for both codebases (D-01). |

**Deprecated/outdated:**
- `StaticJsonDocument` / `DynamicJsonDocument`: Removed in ArduinoJson 7. Use `JsonDocument` directly.
- SPIFFS: Deprecated in ESP32 Arduino Core. LittleFS is the replacement (already in use).

## Open Questions

1. **arduino_midi_library SysEx data format**
   - What we know: The library calls the handler with `(byte* data, unsigned length)`. The current code accesses `data[2]` for the first meaningful byte, suggesting `data[0]` = F0 and `data[1]` = first data byte after F0.
   - What's unclear: Exact byte layout -- does the library include F0 in `data[0]`? Does it include F7? The current code uses `data[2]` = 127 (0x7F), suggesting `data[0]` = F0, `data[1]` = manufacturer/status byte. Need to verify during implementation.
   - Recommendation: Add a debug log printing the first 8 bytes of any received SysEx message at the start of implementation. Adjust header offsets based on actual observed data.

2. **BLE SysEx fragmentation**
   - What we know: BLE MIDI has a ~20-byte MTU by default. Full config dumps (~1KB) will be fragmented by the BLE-MIDI library automatically, but behavior may vary.
   - What's unclear: Whether BLE-MIDI 2.2.0 handles reassembly transparently or if the firmware receives fragments.
   - Recommendation: Noted in STATE.md as a blocker for Phase 4. For Phase 1, focus on USB transport. BLE will work for per-parameter updates (small messages). Full config dump over BLE can be validated later.

3. **quicktype C++ output quality for embedded**
   - What we know: quicktype generates C++ from JSON Schema, but targets desktop C++ (nlohmann/json, not ArduinoJson).
   - What's unclear: Whether generated C++ is directly usable on ESP32 or needs heavy modification.
   - Recommendation: Use quicktype for TypeScript generation only. For C++, use the JSON Schema as documentation -- the C++ structs are hand-written to match, and the round-trip test validates agreement. This is pragmatic and avoids depending on generated C++ fitting ArduinoJson's API.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | PlatformIO Unity (firmware native tests) + Vitest (web, Phase 3) |
| Config file | `platformio.ini` `[env:native]` section (needs creation -- Wave 0) |
| Quick run command | `pio test -e native` |
| Full suite command | `pio test -e native && cd editor-tx && npx vitest run` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROTO-01 | SysEx messages use 0x7D manufacturer ID and command framing | unit | `pio test -e native -f test_sysex_protocol` | No -- Wave 0 |
| PROTO-02 | Per-parameter SysEx sets correct field in config struct | unit | `pio test -e native -f test_param_set` | No -- Wave 0 |
| PROTO-03 | Full config dump serializes all fields, round-trips correctly | unit | `pio test -e native -f test_config_roundtrip` | No -- Wave 0 |
| PROTO-04 | Invalid SysEx (short, wrong mfr ID, bad payload) rejected | unit | `pio test -e native -f test_sysex_validation` | No -- Wave 0 |
| PROTO-05 | Version handshake returns protocol version and firmware version | unit | `pio test -e native -f test_version_handshake` | No -- Wave 0 |
| FWARCH-06 | ConfigManager load-once/save-once (no intermediate flash writes) | unit | `pio test -e native -f test_config_manager` | No -- Wave 0 |
| WEBARCH-05 | Generated TypeScript types match schema | unit | `node schema/validate-types.js` | No -- Wave 0 |
| FWFEAT-03 | v103 config migrates to v200 with all fields preserved | unit | `pio test -e native -f test_migration` | No -- Wave 0 |

### Sampling Rate
- **Per task commit:** `pio test -e native` (firmware unit tests)
- **Per wave merge:** `pio test -e native && node schema/validate-types.js`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `platformio.ini` `[env:native]` section -- PlatformIO native test environment for desktop execution
- [ ] `test/test_sysex_protocol/test_main.cpp` -- SysEx framing and dispatch tests
- [ ] `test/test_config_manager/test_main.cpp` -- ConfigManager load/save/dirty/flush tests
- [ ] `test/test_migration/test_main.cpp` -- v103 to v200 migration tests
- [ ] `test/test_config_roundtrip/test_main.cpp` -- Full config serialization round-trip
- [ ] `schema/validate-types.js` -- Script to validate generated TS types match schema

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Node.js | Type generation, schema validation | Yes | v22.22.1 | -- |
| npm | Package management | Yes | 11.11.0 | -- |
| npx | quicktype CLI execution | Yes | 11.11.0 | -- |
| PlatformIO | Firmware build and test | Yes | 6.1.19 | -- |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:** None.

## Sources

### Primary (HIGH confidence)
- ArduinoJson v7 official docs (https://arduinojson.org/v7/) -- JsonDocument API, memory management, ESP32 considerations
- MIDI 1.0 specification -- SysEx manufacturer ID 0x7D definition, 7-bit data byte requirement
- webmidi.js API docs (https://webmidijs.org/api/classes/Output) -- sendSysex() manufacturer ID parameter format
- Electra One SysEx implementation (https://docs.electra.one/developers/midiimplementation.html) -- reference protocol design pattern (operation + resource + payload)

### Secondary (MEDIUM confidence)
- quicktype (https://quicktype.io/) -- JSON Schema to TypeScript generation capability confirmed via docs
- ESP32 LittleFS flash wear characteristics -- 100K erase cycles per sector, LittleFS provides wear leveling

### Tertiary (LOW confidence)
- arduino_midi_library SysEx handler byte layout -- inferred from current code behavior, needs runtime verification

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries already in use, versions verified
- Architecture: HIGH -- patterns directly informed by locked decisions (D-01 through D-07) and reference implementations
- Pitfalls: HIGH -- derived from reading actual codebase bugs (DataManager round-trips, magic bytes, SysEx framing)
- Protocol design: MEDIUM -- manufacturer ID and framing are well-researched, but exact arduino_midi_library byte offsets need runtime verification

**Research date:** 2026-04-03
**Valid until:** 2026-05-03 (stable domain -- MIDI spec and ArduinoJson 7 are not changing rapidly)
