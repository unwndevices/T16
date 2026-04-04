# Phase 4: Integration & CI - Research

**Researched:** 2026-04-04
**Domain:** End-to-end integration (per-param sync, firmware update flow, config import validation, CI pipeline)
**Confidence:** HIGH

## Summary

Phase 4 wires together the Phase 1 protocol, Phase 2 firmware services, and Phase 3 web rewrite into a working end-to-end system. Five requirements are addressed: per-parameter sync performance (FWFEAT-02 overlap), auto-bootloader firmware update (WEBFEAT-03), config import validation (WEBFEAT-04), CI pipeline (TEST-03), and code formatting enforcement (TEST-04).

The existing codebase already has substantial infrastructure: SysExHandler with HandleParamSet() and ACK responses, ConfigContext with updateParam() that sends immediately, midi.ts service layer with sendParamUpdate(), and an Upload page with esptool-js integration. The work is primarily: (1) adding round-trip timing measurement, (2) implementing the SysEx bootloader command on firmware + wiring auto-bootloader UX in Upload page, (3) building config validation against the JSON schema, and (4) expanding the deploy.yml into a full CI workflow.

**Primary recommendation:** Structure work as four independent streams (sync measurement, bootloader flow, config validation, CI pipeline) since they have minimal code overlap. The bootloader command is the highest-risk item due to ESP32-S3 hardware behavior variability.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Measure round-trip time via timestamps in the web editor -- log send time in ConnectionContext, measure ACK return. Display in dev console, assert in integration tests.
- Skip BLE for full config dumps -- BLE used for note/CC only, full config sync requires USB connection.
- No debounce on parameter sends -- each param change sends immediately. Firmware ConfigManager already batches flash writes via 2s idle flush.
- On sync failure: retry once after 500ms timeout, then show toast notification and mark SyncIndicator red.
- ESP32-S3 bootloader entry via SysEx command -- firmware receives BOOTLOADER SysEx, calls `ESP.restart()` into download mode via USB strapping pins. esptool-js handles the rest.
- Two-step UX: user clicks "Update Firmware" on Upload page, SysEx triggers bootloader, then esptool-js connects via Web Serial automatically.
- Keep manual boot button instructions as fallback.
- Firmware reports version via VERSION command. Upload page compares with selected binary version. Warn (not block) if downgrading.
- TypeScript runtime type validation against config.ts types. Reject if structure doesn't match, show specific field-level error.
- Specific error messaging -- "Missing field: banks[0].scale" or "Invalid value: velocity_curve must be 0-4".
- Migrate then validate -- if imported config has version < 200, run migration logic. If migration succeeds, import. If not, reject with version error.
- Keep .topo file extension. Add JSON schema version field to exported files.
- Trigger on push to any branch + PR to main. Deploy only on main merge.
- PlatformIO in GitHub Actions -- `pio run` for build verification, `pio test -e native` for unit tests.
- ESLint + Prettier for web, clang-format for firmware -- fail CI on lint errors.
- No coverage gate -- run tests, fail on test failures, don't enforce coverage percentage.

### Claude's Discretion
- Exact ESP32-S3 bootloader entry sequence (GPIO strapping vs USB-CDC reset)
- CI action versions and caching strategy
- Prettier configuration details
- Test expansion scope (which additional tests to add)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FWFEAT-02 | SysEx command triggers ESP32 bootloader mode for firmware update (no physical button hold) | ESP32-S3 bootloader entry via RTC_CNTL register write + esp_restart(), new CMD_BOOTLOADER SysEx command in protocol |
| WEBFEAT-03 | Firmware update without bootloader button -- click "Update" triggers auto-bootloader via SysEx | Upload page state machine, SysEx bootloader command, esptool-js 0.6.0 Web Serial auto-connect |
| WEBFEAT-04 | Config backup import validates against schema before applying | JSON Schema validation using existing schema/t16-config.schema.json, TypeScript validation function, migration-then-validate flow |
| TEST-03 | CI pipeline via GitHub Actions (build firmware, build web, run tests, lint) | GitHub Actions workflow with PlatformIO + Node.js matrix, caching strategy, existing deploy.yml as base |
| TEST-04 | clang-format for firmware code, ESLint 9 + Prettier for web code | .clang-format with Allman/4-space matching project conventions, Prettier integration with existing ESLint flat config |
</phase_requirements>

## Standard Stack

### Core (already installed)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| esptool-js | 0.6.0 | Browser-based firmware flashing via Web Serial | Already in package.json, official Espressif package |
| webmidi | 3.1.16 | WebMIDI API wrapper for SysEx communication | Already in package.json, primary device communication |
| vitest | 4.1.2 | Web test runner | Already configured with jsdom, RTL, path aliases |
| Unity (PlatformIO) | built-in | Firmware unit test framework | Already in platformio.ini native env |
| ESLint | 9.39.4 | Web linting | Already configured with flat config + typescript-eslint |

### Supporting (to add)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| prettier | 3.8.x | Web code formatting | CI enforcement + developer formatting |
| ajv | 8.x | JSON Schema validation at runtime | Config import validation against t16-config.schema.json |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| ajv (JSON Schema) | Hand-rolled TypeScript validation | Schema already exists (schema/t16-config.schema.json); ajv validates against it directly with field-level errors. Hand-rolling duplicates the schema and misses edge cases. |
| ajv | zod / valibot | Would require rewriting schema in a different format. JSON Schema is the source of truth and already generates the TypeScript types. ajv validates the existing schema directly. |

**Installation:**
```bash
cd editor-tx && npm install ajv
cd editor-tx && npm install -D prettier
```

## Architecture Patterns

### Recommended Project Structure (Phase 4 additions)
```
src/
  SysExProtocol.hpp          # Add CMD_BOOTLOADER constant
  SysExHandler.hpp/cpp       # Add HandleBootloaderRequest()
editor-tx/src/
  services/
    configValidator.ts        # NEW: ajv-based schema validation + migration
    midi.ts                   # Existing (no changes expected)
  pages/Upload/
    Upload.tsx                # Modify: auto-bootloader state machine
    useBootloader.ts          # NEW: hook for bootloader SysEx + Web Serial flow
  contexts/
    ConfigContext.tsx          # Modify: add round-trip timing, import/export
  protocol/
    sysex.ts                  # Add CMD.BOOTLOADER, requestBootloader()
.github/workflows/
  ci.yml                      # NEW: full CI pipeline
  deploy.yml                  # Existing: keep as-is for FTP deploy on main
.clang-format                 # NEW: firmware formatting config
.prettierrc                   # NEW: web formatting config
```

### Pattern 1: SysEx Bootloader Command (Firmware)
**What:** New SysEx command byte that triggers ESP32-S3 download mode entry
**When to use:** When the web editor sends a BOOTLOADER SysEx to initiate firmware update
**Example:**
```cpp
// In SysExProtocol.hpp -- add new command
constexpr uint8_t CMD_BOOTLOADER = 0x05;

// In SysExHandler.cpp -- add case to ProcessSysEx switch
case SysEx::CMD_BOOTLOADER:
    if (sub == SysEx::SUB_REQUEST)
    {
        HandleBootloaderRequest();
    }
    break;

// Implementation -- enter download mode via RTC register
void SysExHandler::HandleBootloaderRequest()
{
    // Send ACK first so editor knows we received the command
    SendAck(SysEx::CMD_BOOTLOADER, SysEx::STATUS_OK);
    delay(100); // Allow USB to flush the ACK

    // ESP32-S3: Force download boot on next reset via RTC control register
    #include "soc/rtc_cntl_reg.h"
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
}
```

### Pattern 2: Round-Trip Timing Measurement
**What:** Timestamp before sending param update, measure when ACK arrives
**When to use:** Dev console performance monitoring + integration test assertions
**Example:**
```typescript
// In ConfigContext.tsx -- add timing to updateParam
const updateParam = useCallback(
  (domain: number, bank: number, field: number, value: number) => {
    dispatch({ type: 'UPDATE_PARAM', domain, bank, field, value })
    if (output) {
      const sendTime = performance.now()
      midiSendParamUpdate(output, domain, bank, field, value)
      // ACK listener measures round-trip: performance.now() - sendTime
    }
  },
  [output],
)
```

### Pattern 3: Config Validation with ajv
**What:** Validate imported .topo files against the existing JSON schema
**When to use:** Config import flow -- validate structure and field ranges before applying
**Example:**
```typescript
// services/configValidator.ts
import Ajv from 'ajv'
import schema from '@/../schema/t16-config.schema.json'

const ajv = new Ajv({ allErrors: true })
const validate = ajv.compile(schema)

export interface ValidationResult {
  valid: boolean
  errors: { field: string; message: string }[]
}

export function validateConfig(data: unknown): ValidationResult {
  const valid = validate(data)
  if (valid) return { valid: true, errors: [] }

  const errors = (validate.errors ?? []).map(err => ({
    field: err.instancePath.replace(/^\//, '').replace(/\//g, '.') || err.params?.missingProperty as string || 'unknown',
    message: err.message ?? 'validation failed',
  }))
  return { valid: false, errors }
}
```

### Pattern 4: Auto-Bootloader State Machine (Upload Page)
**What:** Multi-step flow: send SysEx bootloader command, wait for device disconnect, connect via Web Serial, flash
**When to use:** Upload page "Update Firmware" button flow
**States:** idle -> entering_bootloader -> bootloader_ready -> uploading -> success/error

### Anti-Patterns to Avoid
- **Polling for bootloader mode:** Do not poll the USB port. Instead, listen for the MIDI device disconnect event (which fires when ESP32 resets), then use `navigator.serial.requestPort()` or auto-connect to the USB Serial/JTAG debug port.
- **Blocking on ACK for param sync:** The existing pattern is correct -- fire-and-forget with timing measurement. Do not add request-response blocking; it would kill <100ms performance.
- **Inline schema validation:** Do not hand-write field-by-field validation in the import handler. Use ajv against the existing JSON schema to keep validation in sync with the schema source of truth.
- **Combining CI and deploy workflows:** Keep `deploy.yml` (FTP deploy on main push) separate from the new `ci.yml` (build/test/lint on all pushes). Merging them creates conditional complexity.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Config schema validation | Custom field-by-field validator | ajv + schema/t16-config.schema.json | Schema already exists, hand-rolling duplicates it and drifts; ajv gives field-path errors for free |
| Web code formatting | Custom ESLint rules for formatting | Prettier | Industry standard, ESLint team officially recommends separate formatter |
| C++ code formatting | Manual review / ad-hoc scripts | clang-format with .clang-format config | Deterministic, runs in CI, matches existing Allman/4-space conventions automatically |
| ESP32 bootloader entry | Custom GPIO manipulation | `REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT) + esp_restart()` | Official ESP-IDF mechanism for software-triggered download mode |

## Common Pitfalls

### Pitfall 1: ESP32-S3 Bootloader Entry Unreliability
**What goes wrong:** `REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT)` + `esp_restart()` may not work on all USB hub configurations. Some USB hubs/hosts don't properly re-enumerate the device after reset.
**Why it happens:** The ESP32-S3's USB Serial/JTAG controller behavior during reset depends on the host USB stack and hub topology. The RTC register approach works for the internal USB PHY but the JTAG/serial debug interface may not always present as a COM port after reset.
**How to avoid:** (1) Send ACK before restarting so editor confirms receipt. (2) Add a short delay (100-200ms) after ACK before calling esp_restart() to let USB buffers flush. (3) Always provide the manual "hold BOOT button" fallback as specified in CONTEXT.md. (4) Test with the actual T16 hardware, not just dev boards.
**Warning signs:** esptool-js fails to detect the device after bootloader entry; browser Web Serial port picker shows no devices.

### Pitfall 2: WebMIDI SysEx and Web Serial Port Conflict
**What goes wrong:** The browser may hold the USB port open via WebMIDI when the editor tries to open it via Web Serial for firmware flashing.
**Why it happens:** WebMIDI and Web Serial both access the same USB device. When the ESP32 resets into bootloader, the MIDI device disappears (good), but the browser may not fully release the USB interface before Web Serial tries to claim it.
**How to avoid:** After sending the bootloader SysEx, wait for the MIDI device `disconnected` event. Add a small delay (500-1000ms) after disconnect before attempting Web Serial connection. The ConnectionContext already handles disconnect events.
**Warning signs:** Web Serial requestPort() fails with "port in use" or shows no matching ports.

### Pitfall 3: ajv Bundle Size
**What goes wrong:** ajv adds ~50KB to the web bundle if fully imported.
**Why it happens:** ajv includes support for all JSON Schema drafts and formats by default.
**How to avoid:** Import only the core: `import Ajv from 'ajv'`. Do not import ajv-formats or other plugins unless needed. The T16 schema uses basic types only (integer, object, array) -- no formats, no $ref.
**Warning signs:** Bundle size increase >100KB after adding ajv.

### Pitfall 4: PlatformIO CI Caching
**What goes wrong:** PlatformIO downloads ~500MB of ESP32 toolchain + framework on every CI run, taking 3-5 minutes.
**Why it happens:** No caching of `~/.platformio` directory between CI runs.
**How to avoid:** Cache `~/.platformio` in GitHub Actions. Use a hash of `platformio.ini` as the cache key so the cache invalidates when dependencies change.
**Warning signs:** CI build step takes >3 minutes consistently.

### Pitfall 5: Config Migration in Import Flow
**What goes wrong:** Imported .topo files with version < 200 fail validation because fields have different names/structure.
**Why it happens:** Validation runs against v200 schema, but imported file is v103 format.
**How to avoid:** Follow the locked decision: migrate THEN validate. Run the same migration logic that firmware uses (MigrateV103ToV200 equivalent in TypeScript) before validating against the v200 schema. If migration itself fails, reject with a version-specific error.
**Warning signs:** All v103 imports rejected even though migration code exists.

## Code Examples

### ESP32-S3 Software Bootloader Entry
```cpp
// Source: ESP32 forum + ESP-IDF documentation
// Required headers for ESP32-S3 download mode entry
#include "esp_system.h"
#include "soc/rtc_cntl_reg.h"

void HandleBootloaderRequest()
{
    // Send acknowledgement before restarting
    SendAck(SysEx::CMD_BOOTLOADER, SysEx::STATUS_OK);

    // Allow USB to flush the ACK response
    delay(100);

    // Set RTC register to force download boot on next reset
    // This is the ESP-IDF official mechanism for software-triggered bootloader entry
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);

    // Restart into download mode
    esp_restart();
}
```

### GitHub Actions CI Workflow Structure
```yaml
# .github/workflows/ci.yml
name: CI

on:
  push:
    branches: ['**']
  pull_request:
    branches: [main]

jobs:
  firmware-build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/cache@v4
        with:
          path: ~/.platformio
          key: pio-${{ hashFiles('platformio.ini') }}
          restore-keys: pio-
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - run: pip install platformio
      - run: pio run -e esp32s3        # Build firmware
      - run: pio test -e native         # Run native unit tests

  firmware-format:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: |
          find src/ -name '*.hpp' -o -name '*.cpp' -o -name '*.h' | \
            xargs clang-format --dry-run --Werror

  web-build:
    runs-on: ubuntu-latest
    defaults:
      run:
        working-directory: editor-tx
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '22'
          cache: 'npm'
          cache-dependency-path: editor-tx/package-lock.json
      - run: npm ci
      - run: npm run typecheck          # tsc --noEmit
      - run: npm run lint               # eslint
      - run: npx prettier --check .     # formatting check
      - run: npm run test               # vitest run
      - run: npm run build              # vite build
```

### .clang-format Configuration
```yaml
# Matches T16 firmware conventions: Allman braces, 4-space indent
BasedOnStyle: Microsoft
IndentWidth: 4
UseTab: Never
BreakBeforeBraces: Allman
ColumnLimit: 120
AllowShortIfStatementsOnASingleLine: Never
AllowShortFunctionsOnASingleLine: None
AllowShortLoopsOnASingleLine: false
IndentCaseLabels: true
PointerAlignment: Left
SpaceAfterCStyleCast: false
SpaceBeforeParens: ControlStatements
AccessModifierOffset: -4
NamespaceIndentation: All
```

### Config Import Validation Flow
```typescript
// services/configValidator.ts
import Ajv from 'ajv'
import schema from '@/../schema/t16-config.schema.json'

const ajv = new Ajv({ allErrors: true, verbose: true })
const validate = ajv.compile(schema)

export interface ValidationError {
  field: string
  message: string
}

export interface ValidationResult {
  valid: boolean
  errors: ValidationError[]
}

export function validateConfig(data: unknown): ValidationResult {
  const valid = validate(data)
  if (valid) return { valid: true, errors: [] }

  return {
    valid: false,
    errors: (validate.errors ?? []).map(err => ({
      field: err.instancePath
        ? err.instancePath.slice(1).replace(/\//g, '.')
        : (err.params?.missingProperty as string) ?? 'root',
      message: err.message ?? 'validation failed',
    })),
  }
}
```

### Prettier Configuration
```json
{
  "semi": false,
  "singleQuote": true,
  "trailingComma": "all",
  "tabWidth": 2,
  "printWidth": 100
}
```
Note: Matches the existing code style observed in Phase 3 output (no semicolons, single quotes). Tab width 2 matches the TypeScript files.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual bootloader button hold | Software-triggered via RTC register | ESP-IDF 4.x+ | Users never need to hold BOOT button |
| Hand-rolled config validation | JSON Schema + ajv | Standard practice | Schema is single source of truth for types AND validation |
| No CI | GitHub Actions with PlatformIO | Industry standard | Every push validated automatically |
| No code formatting | clang-format + Prettier | Modern C++ / JS practice | Consistent code style without manual review |

## Open Questions

1. **ESP32-S3 Bootloader Reliability on T16 Hardware**
   - What we know: `REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT) + esp_restart()` is the documented approach. Some forum reports suggest unreliability with certain USB host stacks.
   - What's unclear: Whether the T16's specific USB connection (USB Serial/JTAG vs USB-OTG via TinyUSB) affects behavior. The T16 uses TinyUSB for USB MIDI, which may interact differently with the bootloader.
   - Recommendation: Implement the standard approach, test on actual T16 hardware. The manual fallback (hold BOOT button) is already specified as required. Mark the auto-bootloader as "best effort" -- it should work for most configurations but the fallback must always be available.

2. **esptool-js Auto-Connect After Bootloader Entry**
   - What we know: After ESP32 resets into download mode, it re-enumerates as USB Serial/JTAG debug unit. esptool-js 0.6.0 uses `navigator.serial.requestPort()` which requires user gesture.
   - What's unclear: Whether the browser can auto-reconnect to the serial port without a new user gesture, or if a second click is needed. The `serial.getPorts()` API may allow reconnecting to a previously-granted port.
   - Recommendation: Try `navigator.serial.getPorts()` first to find the already-authorized port. If that fails, prompt the user with requestPort(). Test the exact flow in Chrome.

3. **Config V103 Migration in Web Editor**
   - What we know: Firmware has `MigrateV103ToV200()` in ConfigManager. The web editor needs equivalent logic.
   - What's unclear: Whether to duplicate the migration logic in TypeScript or to simplify (since most v103 users would have already migrated via firmware).
   - Recommendation: Implement a lightweight TypeScript migration for the common case. Since .topo exports from v103 firmware are the main use case, the migration should handle the known v103 structure.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| PlatformIO | Firmware build/test | Yes | 6.1.19 | -- |
| clang-format | Firmware formatting | Yes | 21.1.8 | -- |
| Node.js | Web build/test | Yes | 22.22.1 | -- |
| npm | Web package management | Yes | 11.11.0 | -- |
| Prettier | Web formatting | Yes (npx) | 3.8.1 | -- |
| gh CLI | GitHub Actions management | Yes | 2.87.3 | -- |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:** None.

Note: CI runs on GitHub Actions `ubuntu-latest`, not this development machine. PlatformIO and clang-format need to be installed in CI via pip and apt respectively. Node.js available via `actions/setup-node@v4`.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework (firmware) | Unity via PlatformIO native env |
| Framework (web) | Vitest 4.1.2 + React Testing Library |
| Config file (firmware) | platformio.ini [env:native] |
| Config file (web) | editor-tx/vitest.config.ts |
| Quick run command (firmware) | `pio test -e native` |
| Quick run command (web) | `cd editor-tx && npm run test` |
| Full suite command | `pio test -e native && cd editor-tx && npm run test` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FWFEAT-02 | Bootloader SysEx triggers download mode | manual-only (requires hardware) | N/A -- hardware test | N/A |
| FWFEAT-02 | SysEx handler dispatches BOOTLOADER command | unit | `pio test -e native -f test_sysex_protocol` | Needs extension |
| WEBFEAT-03 | Upload page auto-bootloader state machine | unit | `cd editor-tx && npx vitest run src/pages/Upload` | Wave 0 |
| WEBFEAT-04 | Config import validates against schema | unit | `cd editor-tx && npx vitest run src/services/configValidator.test.ts` | Wave 0 |
| WEBFEAT-04 | Config import rejects malformed files | unit | `cd editor-tx && npx vitest run src/services/configValidator.test.ts` | Wave 0 |
| WEBFEAT-04 | Config import migrates v103 format | unit | `cd editor-tx && npx vitest run src/services/configValidator.test.ts` | Wave 0 |
| TEST-03 | CI builds firmware | smoke | `gh run list --workflow=ci.yml` (verify after push) | Wave 0 |
| TEST-03 | CI builds web | smoke | `gh run list --workflow=ci.yml` | Wave 0 |
| TEST-04 | clang-format check passes | smoke | `find src/ -name '*.hpp' -o -name '*.cpp' -o -name '*.h' \| xargs clang-format --dry-run --Werror` | Wave 0 |
| TEST-04 | Prettier check passes | smoke | `cd editor-tx && npx prettier --check .` | Wave 0 |

### Sampling Rate
- **Per task commit:** `pio test -e native && cd editor-tx && npm run test`
- **Per wave merge:** Full suite + lint checks
- **Phase gate:** Full suite green + CI workflow green on GitHub before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `editor-tx/src/services/configValidator.test.ts` -- covers WEBFEAT-04 (schema validation, field errors, migration)
- [ ] `editor-tx/src/pages/Upload/Upload.test.tsx` -- covers WEBFEAT-03 (bootloader state machine)
- [ ] `.github/workflows/ci.yml` -- covers TEST-03 (full CI pipeline)
- [ ] `.clang-format` -- covers TEST-04 (firmware formatting)
- [ ] `.prettierrc` -- covers TEST-04 (web formatting)
- [ ] Firmware `SysExHandler` test extension for CMD_BOOTLOADER -- covers FWFEAT-02

## Sources

### Primary (HIGH confidence)
- Existing codebase: `src/SysExHandler.cpp`, `src/SysExProtocol.hpp`, `editor-tx/src/services/midi.ts`, `editor-tx/src/protocol/sysex.ts`, `editor-tx/src/contexts/ConfigContext.tsx`, `editor-tx/src/pages/Upload/Upload.tsx`
- JSON Schema: `schema/t16-config.schema.json` -- complete config validation schema
- Existing tests: `test/` (7 firmware suites), `editor-tx/src/services/midi.test.ts`, `editor-tx/src/contexts/ConfigContext.test.tsx`, `editor-tx/src/contexts/ConnectionContext.test.tsx`
- Existing CI: `.github/workflows/deploy.yml`

### Secondary (MEDIUM confidence)
- [ESP32 Forum: bootloader entry via RTC register](https://esp32.com/viewtopic.php?t=36467) -- software bootloader entry pattern
- [Espressif esptool docs: boot mode selection](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html) -- ESP32-S3 strapping pin behavior
- [Arduino-ESP32 USB CDC/DFU docs](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html) -- USB CDC flashing reference
- [Clang-Format Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) -- clang-format configuration reference
- [PlatformIO GitHub Actions examples](https://github.com/rgl/platformio-esp32-arduino-hello-world/blob/master/.github/workflows/build.yml) -- CI workflow patterns

### Tertiary (LOW confidence)
- [ESP32 Forum: chip_usb_set_persist_flags](https://esp32.com/viewtopic.php?t=34660) -- alternative USB-based bootloader entry (may not apply to TinyUSB config)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries already in use or well-established (ajv, prettier)
- Architecture: HIGH -- existing code patterns are clear, additions are well-scoped
- Bootloader entry: MEDIUM -- documented approach but hardware-dependent behavior, needs testing on actual T16
- CI pipeline: HIGH -- standard GitHub Actions patterns, all tools verified available
- Pitfalls: MEDIUM -- based on forum reports and common integration issues

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable domain, 30 days)
