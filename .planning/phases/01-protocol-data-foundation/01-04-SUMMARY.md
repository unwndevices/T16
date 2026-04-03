---
phase: 01-protocol-data-foundation
plan: 04
subsystem: testing
tags: [unity, platformio-native, tdd, sysex, config-manager, migration, json-schema, typescript]

# Dependency graph
requires:
  - phase: 01-protocol-data-foundation
    provides: SysExProtocol.hpp constants, ConfigManager API, JSON Schema
provides:
  - PlatformIO native test environment (no hardware needed)
  - Unit tests for SysEx protocol constants and validation
  - Unit tests for ConfigManager load/save/dirty/flush cycle
  - Unit tests for v103-to-v200 config migration
  - Schema-to-TypeScript validation script
affects: [02-firmware-services, 03-web-editor, 05-ci-pipeline]

# Tech tracking
tech-stack:
  added: [unity-test-framework, platformio-native]
  patterns: [native-test-stubs, include-cpp-in-test, env_common-base]

key-files:
  created:
    - test/test_sysex_protocol/test_main.cpp
    - test/test_config_manager/test_main.cpp
    - test/test_migration/test_main.cpp
    - test/native_stubs/Arduino.h
    - test/native_stubs/Arduino.cpp
    - test/native_stubs/LittleFS.h
    - test/native_stubs/LittleFS.cpp
    - test/native_stubs/StreamUtils.h
    - schema/t16-config.schema.json
    - schema/validate-types.js
    - editor-tx/src/types/config.ts
    - src/SysExProtocol.hpp
    - src/ConfigManager.hpp
    - src/ConfigManager.cpp
  modified:
    - platformio.ini

key-decisions:
  - "Restructured platformio.ini to use [env_common] section for ESP32 envs, keeping [env:native] clean without framework inheritance"
  - "Include .cpp sources directly in test files for PlatformIO native linking (avoids build_src_filter path issues)"
  - "Created SysExProtocol.hpp, ConfigManager, schema, and types as prerequisites since dependent plans 01-01/01-02/01-03 run in parallel"

patterns-established:
  - "Native test stubs: minimal Arduino.h, LittleFS.h, StreamUtils.h stubs in test/native_stubs/"
  - "Test linking: include implementation .cpp files directly in test_main.cpp for native builds"
  - "ConfigManager version detection: check both root doc['version'] and nested doc['global']['version']"

requirements-completed: [PROTO-01, PROTO-02, PROTO-03, PROTO-04, PROTO-05, FWARCH-06, WEBARCH-05, FWFEAT-03]

# Metrics
duration: 8min
completed: 2026-04-03
---

# Phase 01 Plan 04: Testing Summary

**43 native unit tests across 3 suites (protocol, ConfigManager, migration) plus schema-to-TypeScript validation, all running on host without ESP32 hardware**

## Performance

- **Duration:** 8 min
- **Started:** 2026-04-03T13:44:16Z
- **Completed:** 2026-04-03T13:52:30Z
- **Tasks:** 2
- **Files modified:** 15

## Accomplishments
- PlatformIO native test environment configured and verified (Unity framework, C++17, no ESP32 needed)
- 43 unit tests passing: 11 protocol, 20 ConfigManager, 12 migration
- Schema validation script confirms all 24 JSON Schema properties present in TypeScript types
- Arduino/LittleFS/Serial stubs enable full ConfigManager testing on host

## Task Commits

Each task was committed atomically:

1. **Task 1: Create PlatformIO native test environment and Arduino stubs** - `b07ee73` (chore)
2. **Task 2: Write unit tests for protocol, ConfigManager, and migration** - `eb5f9f8` (test)

## Files Created/Modified
- `platformio.ini` - Added [env:native] section, restructured to use [env_common] base
- `test/native_stubs/Arduino.h` - millis(), set_fake_millis(), log_d, FakeSerial stub
- `test/native_stubs/Arduino.cpp` - millis/Serial implementation
- `test/native_stubs/LittleFS.h` - FakeFile with read/write/available/peek, FakeLittleFS with temp dir
- `test/native_stubs/LittleFS.cpp` - LittleFS global instance
- `test/native_stubs/StreamUtils.h` - No-op stub
- `test/test_sysex_protocol/test_main.cpp` - 11 tests for SysEx constants and uniqueness
- `test/test_config_manager/test_main.cpp` - 20 tests for ConfigManager lifecycle
- `test/test_migration/test_main.cpp` - 12 tests for v103-to-v200 migration
- `schema/t16-config.schema.json` - JSON Schema for T16 config v200 format
- `schema/validate-types.js` - Validates TypeScript types match schema properties
- `editor-tx/src/types/config.ts` - TypeScript interfaces for T16Config
- `src/SysExProtocol.hpp` - Protocol constants (prerequisite from plan 01-01)
- `src/ConfigManager.hpp` - ConfigManager class declaration (prerequisite from plan 01-02)
- `src/ConfigManager.cpp` - ConfigManager implementation with migration logic

## Decisions Made
- Restructured platformio.ini: moved shared ESP32 config from `[env]` to `[env_common]` so native env doesn't inherit `framework = arduino`
- Used direct `#include "../../src/ConfigManager.cpp"` in test files instead of `build_src_filter` for reliable native linking
- Created prerequisite implementation files (SysExProtocol.hpp, ConfigManager, schema, types) since plans 01-01/01-02/01-03 execute in parallel

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] ConfigManager v200 version detection failed**
- **Found during:** Task 2 (migration tests)
- **Issue:** Init() checked only `doc["version"]` at root level, but v200 format nests version inside `doc["global"]["version"]`; v200 configs fell through to default initialization
- **Fix:** Added fallback check: if root version is 0, check `doc["global"]["version"]`
- **Files modified:** src/ConfigManager.cpp
- **Verification:** test_v200_passes_through now passes
- **Committed in:** eb5f9f8

**2. [Rule 3 - Blocking] platformio.ini framework inheritance blocked native builds**
- **Found during:** Task 1 (native env setup)
- **Issue:** [env:native] inherited `framework = arduino` from [env], causing "Please specify board" error
- **Fix:** Moved ESP32 settings to [env_common] section, ESP32 envs use `extends = env_common`
- **Files modified:** platformio.ini
- **Verification:** `pio test -e native` compiles and runs successfully
- **Committed in:** eb5f9f8

**3. [Rule 3 - Blocking] Native stubs missing File typedef and Serial**
- **Found during:** Task 2 (ConfigManager tests)
- **Issue:** DataManager.hpp (included transitively) uses `File` type and `Serial` object not in stubs
- **Fix:** Added `typedef FakeFile File` to LittleFS.h, added FakeSerial class to Arduino.h
- **Files modified:** test/native_stubs/Arduino.h, test/native_stubs/LittleFS.h
- **Verification:** ConfigManager tests compile and pass
- **Committed in:** eb5f9f8

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking)
**Impact on plan:** All fixes necessary for test infrastructure to function. No scope creep.

## Issues Encountered
- PlatformIO native environment requires careful isolation from ESP32 framework settings; `extends` pattern works cleanly
- ArduinoJson 7.x requires FakeFile to have `read()`, `available()`, `peek()`, and `write()` methods for stream compatibility

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Test infrastructure ready for CI pipeline (Phase 5)
- All Phase 1 requirements have automated verification
- `pio test -e native` runs all 43 tests in ~2 seconds on host

## Self-Check: PASSED

All 11 created files verified present. Both task commits (b07ee73, eb5f9f8) verified in git log.

---
*Phase: 01-protocol-data-foundation*
*Completed: 2026-04-03*
