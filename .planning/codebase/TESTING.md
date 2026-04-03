# Testing Patterns

**Analysis Date:** 2026-04-03

## Test Framework

**Runner:**
- PlatformIO Test Runner (Unity-based) -- configured but unused
- Config: `platformio.ini`
- No test framework dependencies in `lib_deps`

**Assertion Library:**
- None configured (Unity would be available via PlatformIO but no tests exist)

**Run Commands:**
```bash
pio test                   # Would run tests (none exist)
pio test -e esp32s3        # Target-specific
```

## Test File Organization

**Location:**
- `test/` directory exists with only a `README` placeholder
- No test files exist anywhere in the project

**Web editor:**
- No test framework configured in `editor-tx/package.json`
- No test scripts, no Jest/Vitest config, no test files

## Current Test Coverage

**Firmware:** 0% -- no automated tests of any kind

**Web editor:** 0% -- no automated tests of any kind

## Hardware Testing (Manual)

**Built-in hardware test:** `HardwareTest()` in `src/main.cpp`
- Runs at startup if calibration data is missing
- Tests each key's ADC reading (checks for open/short circuit)
- Visual feedback via LEDs -- lights up failing keys
- Blocks indefinitely if test fails (`while (!test_passed)`)

**Calibration routine:** `CalibrationRoutine()` in `src/main.cpp`
- Interactive per-key calibration requiring physical button presses
- Stores min/max ADC values to LittleFS
- Runs automatically on first boot or when calibration data is corrupt

## What Should Be Tested

### Firmware -- Unit Testable (No Hardware)

**Signal class** (`src/Libs/Signal.hpp`):
- Connect/Disconnect/Emit behavior
- Multiple slot management
- ID-based disconnection
- Pure logic, no hardware dependencies
- Priority: High

**Timer class** (`src/Libs/Timer.hpp`):
- Delta time calculation
- Elapsed time tracking
- Start/Stop/Restart lifecycle
- Depends on `millis()` which can be mocked
- Priority: Medium

**Types/Vector2** (`src/Libs/Types.hpp`):
- Lerp (both float and integer variants)
- Operator overloads (+, -, *, /)
- Pure math, zero dependencies
- Priority: Low

**Configuration serialization** (`src/Configuration.cpp`):
- SaveConfiguration / LoadConfiguration round-trip
- Depends on ArduinoJson (can be tested natively)
- Priority: High

**Keyboard LUT generation** (`src/Libs/Keyboard.hpp` -- `GenerateLUTs()`):
- Verify LUT values for LINEAR, EXPONENTIAL, LOGARITHMIC, QUADRATIC curves
- Pure math
- Priority: Medium

**Scale mapping** (`src/Scales.hpp`):
- Note map generation
- Chord mapping
- Priority: High

### Firmware -- Integration Testable (With Hardware Mock)

**Key state machine** (`src/Libs/Keyboard.hpp` -- `Key::Update()`):
- IDLE -> STARTED -> PRESSED -> RELEASED -> IDLE transitions
- Aftertouch threshold behavior
- Requires mocked `Adc`
- Priority: High

**Button state machine** (`src/Libs/Button.hpp`):
- Click vs long press detection
- Debounce behavior
- Requires mocked `digitalRead` / `millis`
- Priority: High

### Web Editor -- Unit Testable

**SysEx serialization/deserialization** (`editor-tx/src/components/MidiProvider.jsx`):
- `deserializeSysex()` function
- Config JSON round-trip
- Priority: High

**Sync status diffing** (`editor-tx/src/components/MidiProvider.jsx`):
- `updateSyncStatus()` -- deep comparison logic
- Priority: Medium

## PlatformIO Native Testing Setup

PlatformIO supports `native` environment for running tests on the host machine (no ESP32 needed). To enable:

**Add to `platformio.ini`:**
```ini
[env:native]
platform = native
test_framework = unity
build_flags = -std=c++17
```

**Test file structure:**
```
test/
├── test_signal/
│   └── test_signal.cpp
├── test_timer/
│   └── test_timer.cpp
├── test_types/
│   └── test_types.cpp
└── test_configuration/
    └── test_configuration.cpp
```

**Example test (Signal):**
```cpp
#include <unity.h>
#include "Signal.hpp"

static int lastValue = 0;
void callback(int val) { lastValue = val; }

void test_signal_connect_and_emit() {
    Signal<int> signal;
    signal.Connect(callback);
    signal.Emit(42);
    TEST_ASSERT_EQUAL(42, lastValue);
}

void test_signal_disconnect_all() {
    Signal<int> signal;
    signal.Connect(callback);
    signal.DisconnectAll();
    lastValue = 0;
    signal.Emit(99);
    TEST_ASSERT_EQUAL(0, lastValue);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_signal_connect_and_emit);
    RUN_TEST(test_signal_disconnect_all);
    return UNITY_END();
}
```

## Web Editor Testing Setup

No testing infrastructure exists. To add:

**Install Vitest:**
```bash
cd editor-tx
npm install -D vitest @testing-library/react @testing-library/jest-dom jsdom
```

**Add to `package.json` scripts:**
```json
"test": "vitest",
"test:run": "vitest run",
"coverage": "vitest run --coverage"
```

**Add `vitest.config.js`:**
```js
import { defineConfig } from 'vitest/config'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  test: {
    environment: 'jsdom',
    globals: true,
  },
})
```

## Test Types

**Unit Tests:**
- None exist
- Should target: Signal, Timer, Types, LUT generation, scale mapping, SysEx serialization

**Integration Tests:**
- None exist
- Should target: Key/Button state machines with mocked hardware, config round-trip with mocked filesystem

**E2E Tests:**
- Not applicable for firmware
- Web editor could use Playwright but low priority given it's a config tool

## Coverage

**Requirements:** None enforced
**Current coverage:** 0% across both firmware and web editor

---

*Testing analysis: 2026-04-03*
