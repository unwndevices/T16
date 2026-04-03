# Phase 2: Firmware Service Extraction - Research

**Researched:** 2026-04-03
**Domain:** ESP32-S3 C++ firmware architecture refactoring (PlatformIO/Arduino)
**Confidence:** HIGH

## Summary

This phase extracts the 887-line monolithic `main.cpp` into service classes following the eisei `EiseiEngine` pattern, fixes 4 known firmware bugs, adds a serial command interface, and introduces host-side unit tests. The codebase has clear extraction seams: ProcessKey/ProcessStrum/ProcessSlider/ProcessButton/ProcessModeButton are free functions operating on global state, making them straightforward candidates for encapsulation into service classes.

Phase 1 already established the patterns: `ConfigManager` (load-once/save-once with dirty tracking), `SysExHandler` (dependency injection via constructor refs), `#pragma once`, trailing underscore naming, and the native test environment with Unity framework + Arduino stubs. Phase 2 extends this to the remaining application logic.

**Primary recommendation:** Extract services in dependency order (ModeManager first, then InputProcessor, then SliderProcessor, then the AppEngine orchestrator), fixing each bug at the point where its subsystem is extracted. The transport abstraction in MidiProvider is a contained refactor that can happen early and independently.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** main.cpp orchestration uses an **AppEngine class** -- single orchestrator object owns all services, main.cpp just creates it and calls `init()`/`update()`. Matches eisei's EiseiEngine pattern.
- **D-02:** Services communicate via **dependency injection through constructor refs** -- services receive references to their dependencies at construction (e.g., `InputProcessor(ConfigManager&, MidiProvider&)`). Explicit, testable, no hidden coupling.
- **D-03:** Task scheduling uses **timed update intervals** -- each service has an `Update()` called from the main loop, services that need timing track their own intervals. No FreeRTOS tasks, no external scheduler library.
- **D-04:** Mode switching via a **ModeManager service** -- owns the current mode enum, exposes `SetMode()`/`GetMode()`, services query it. Replaces scattered `if (mode == ...)` blocks.
- **D-05:** Bugs are fixed **during extraction** -- fix each bug as its subsystem is extracted into a service class.
- **D-06:** LedManager memory leak fixed with **std::unique_ptr** -- `currentPattern_` and `nextPattern_` as `std::unique_ptr<Pattern>`. Auto-cleanup on transition.
- **D-07:** HardwareTest uses **3-second timeout per key** -- if key doesn't respond within 3s, mark as failed, flash red LED, continue.
- **D-08:** Serial commands follow **eisei pattern** -- categorized command files, registered with a `SerialCommandManager`. Text-based commands over USB serial.
- **D-09:** Serial commands expose **core diagnostics** -- config dump, key states, slider values, MIDI monitor, version info.
- **D-10:** Test coverage targets **service interfaces + state machines** -- test ModeManager transitions, InputProcessor key-to-note mapping, transport abstraction dispatch. Mock hardware dependencies.

### Claude's Discretion
- Exact service class boundaries and file organization
- Header guard style (#pragma once vs #ifndef -- eisei uses #pragma once)
- Namespace usage (eisei uses `namespace eisei {}`)
- Private member naming convention (eisei uses trailing underscore)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FWARCH-01 | Application logic extracted into service classes | Service boundary analysis below; AppEngine + 5 services identified |
| FWARCH-02 | main.cpp under 150 lines, init+loop calling services | AppEngine pattern documented; main.cpp becomes ~30 lines |
| FWARCH-03 | Header-only implementations moved to .cpp files | Audit of 9 header-only files with extraction plan |
| FWARCH-04 | Global mutable state encapsulated into services | 8 global variables identified, ownership mapping documented |
| FWARCH-05 | MIDI transport abstracted into interface loop | MidiTransport interface pattern documented with code example |
| FWARCH-07 | Task scheduler replaces tight loop() | Timed update intervals pattern documented (per D-03) |
| FWBUG-01 | LedManager memory leak fixed | std::unique_ptr pattern documented; leak sites identified |
| FWBUG-02 | TouchSlider::SetPosition no-op fixed | Bug at TouchSlider.cpp:126-130 confirmed; fix documented |
| FWBUG-03 | Unreachable XY_PAD branch removed | Dead code at main.cpp:870-873 confirmed |
| FWBUG-04 | HardwareTest infinite loop fixed | Timeout + per-key error indication pattern documented |
| FWFEAT-01 | Serial command interface | SerialCommandManager + categorized command files pattern |
| TEST-01 | Firmware unit tests on native env | Existing test infra validated; new test targets identified |
</phase_requirements>

## Architecture Patterns

### Recommended Service Structure

```
src/
  main.cpp                      # ~30 lines: create AppEngine, call init()/update()
  AppEngine.hpp                  # Class declaration
  AppEngine.cpp                  # Owns all services, orchestrates init/update
  services/
    ModeManager.hpp/.cpp         # Mode enum, SetMode/GetMode, mode-specific allowed slider modes
    InputProcessor.hpp/.cpp      # Key processing (ProcessKey, ProcessStrum, ProcessQuickSettings)
    SliderProcessor.hpp/.cpp     # Slider processing (ProcessSlider, ProcessSliderButton logic)
    ButtonHandler.hpp/.cpp       # Button event dispatch (ProcessButton logic)
    CalibrationService.hpp/.cpp  # CalibrationRoutine + HardwareTest
    SerialCommandManager.hpp/.cpp # Command registration + dispatch
    serial_commands/
      DiagnosticCommands.hpp/.cpp # Config dump, version, key states
      MidiCommands.hpp/.cpp       # MIDI monitor, panic
  ConfigManager.hpp/.cpp         # Already exists from Phase 1 (move to top level or keep)
  SysExHandler.hpp/.cpp          # Already exists from Phase 1
  SysExProtocol.hpp              # Already exists from Phase 1
  Configuration.hpp/.cpp         # Struct definitions (kept, but globals removed)
  Scales.hpp/.cpp                # Scale data + note mapping functions
  pinout.h                       # Hardware pin definitions (unchanged)
  Libs/
    Adc.hpp/.cpp                 # Already has .cpp -- keep as-is
    Keyboard.hpp -> .hpp/.cpp    # Extract implementation to .cpp
    Button.hpp -> .hpp/.cpp      # Extract implementation to .cpp
    TouchSlider.hpp/.cpp         # Already has .cpp -- fix SetPosition bug
    Signal.hpp                   # Template -- stays header-only (correct for templates)
    Timer.hpp                    # Small utility -- can stay header-only
    Types.hpp                    # Type definitions -- stays header-only
    MidiProvider.hpp/.cpp        # Refactor for transport abstraction
    Leds/
      LedManager.hpp/.cpp        # Extract implementation, add unique_ptr
      patterns/
        Pattern.hpp/.cpp         # Extract static members to .cpp
        [all pattern files]      # Extract RunPattern implementations to .cpp
```

### Pattern 1: AppEngine Orchestrator

**What:** Single class that owns all services, wires dependencies, and provides `init()`/`update()`.
**When to use:** Always -- this is the main entry point after extraction.

```cpp
// AppEngine.hpp
#pragma once

#include "ConfigManager.hpp"
#include "Libs/MidiProvider.hpp"
#include "Libs/Keyboard.hpp"
#include "Libs/TouchSlider.hpp"
#include "Libs/Button.hpp"
#include "Libs/Leds/LedManager.hpp"
#include "Libs/Adc.hpp"
#include "services/ModeManager.hpp"
#include "services/InputProcessor.hpp"
#include "services/SliderProcessor.hpp"
#include "services/ButtonHandler.hpp"
#include "services/CalibrationService.hpp"
#include "services/SerialCommandManager.hpp"
#include "SysExHandler.hpp"

namespace t16 {

class AppEngine {
public:
    void init();
    void update();

private:
    // Hardware
    Adc adc_;
    Keyboard keyboard_;
    TouchSlider slider_;
    Button touchBtn_;
    Button modeBtn_;
    LedManager ledManager_;
    MidiProvider midiProvider_;

    // Services
    ConfigManager configManager_;
    ModeManager modeManager_;
    SysExHandler sysExHandler_{configManager_, midiProvider_};
    InputProcessor inputProcessor_{configManager_, midiProvider_, ledManager_, keyboard_};
    SliderProcessor sliderProcessor_{configManager_, midiProvider_, ledManager_, slider_};
    ButtonHandler buttonHandler_{modeManager_, sliderProcessor_, configManager_, midiProvider_};
    CalibrationService calibrationService_{adc_, ledManager_};
    SerialCommandManager serialCommands_;
};

} // namespace t16
```

### Pattern 2: Transport Abstraction for MidiProvider (FWARCH-05)

**What:** Replace the if/if/if pattern with a vector of active transport pointers.
**When to use:** MidiProvider refactor.

```cpp
// Inside MidiProvider
class MidiTransport {
public:
    virtual ~MidiTransport() = default;
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel) = 0;
    virtual void sendPitchBend(int bend, uint8_t channel) = 0;
    virtual void sendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel) = 0;
    virtual void sendSysEx(size_t size, const byte* data) = 0;
    virtual void read() = 0;
};

// MidiProvider holds:
std::array<MidiTransport*, 3> transports_;  // USB, BLE, TRS
uint8_t activeCount_ = 0;

// All send methods become loops:
void MidiProvider::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    for (uint8_t i = 0; i < activeCount_; i++) {
        transports_[i]->sendNoteOn(note, velocity, channel);
    }
}
```

**Note on virtual dispatch cost:** On ESP32-S3 at 240MHz, virtual call overhead is ~5ns per call. MIDI events happen at most ~1000/sec across all keys. This is negligible. The readability and testability gain far outweighs the cost.

**Note on `std::vector` vs `std::array`:** Use `std::array<MidiTransport*, 3>` with a count -- avoids heap allocation, maximum 3 transports is fixed and known.

### Pattern 3: Timed Update Intervals (FWARCH-07)

**What:** Each service tracks its own update interval, AppEngine calls all updates from loop.
**When to use:** Services that don't need to run every loop iteration.

```cpp
// In a service that needs timing control
class SliderProcessor {
public:
    void update() {
        unsigned long now = millis();
        if (now - lastUpdate_ < UPDATE_INTERVAL_MS) return;
        lastUpdate_ = now;
        // actual processing...
    }
private:
    unsigned long lastUpdate_ = 0;
    static constexpr unsigned long UPDATE_INTERVAL_MS = 2;  // ~500Hz
};
```

### Pattern 4: ModeManager Service (D-04)

**What:** Centralizes mode state and provides mode-specific queries.
**When to use:** Replaces scattered `if (cfg.mode == ...)` blocks.

```cpp
class ModeManager {
public:
    void setMode(Mode mode);
    Mode getMode() const { return currentMode_; }

    // Encapsulates the allowed-slider-modes logic from ProcessButton
    const SliderMode* getAllowedSliderModes(size_t& count) const;
    SliderMode getDefaultSliderMode() const;

private:
    Mode currentMode_ = Mode::KEYBOARD;
};
```

### Pattern 5: SerialCommandManager (D-08)

**What:** Text-based command registration and dispatch over USB Serial.
**When to use:** Diagnostic output following eisei pattern.

```cpp
class SerialCommandManager {
public:
    using CommandHandler = std::function<void(const char* args)>;

    void registerCommand(const char* name, const char* help, CommandHandler handler);
    void processInput();  // Called from update loop, reads Serial

private:
    struct Command {
        const char* name;
        const char* help;
        CommandHandler handler;
    };
    std::array<Command, 16> commands_;  // Fixed capacity, no heap
    uint8_t commandCount_ = 0;
    char inputBuffer_[64] = {0};
    uint8_t inputPos_ = 0;
};
```

### Anti-Patterns to Avoid

- **Global constructors interleaved with includes:** The current pattern of `#include "Foo.hpp"` immediately followed by `Foo foo;` must not be replicated. All construction happens inside AppEngine.
- **Free functions operating on global state:** ProcessKey, ProcessSlider, etc. become methods of service classes that operate on member state.
- **Header-defined static members:** `Key::press_threshold`, `Pattern::currentPalette` etc. defined in headers will cause linker errors with multiple translation units. Move to .cpp files.
- **Raw `new` for patterns:** Current `currentPattern = new WaveTransition()` leaks. Use `std::make_unique<WaveTransition>()`.

## Bug Analysis

### FWBUG-01: LedManager Memory Leak

**Location:** `LedManager.hpp` lines 233-246
**Root cause:** `TransitionToPattern()` does `currentPattern = new WaveTransition()` without deleting the previous `currentPattern`. Similarly, `UpdateTransition()` on line 245 creates another `new WaveTransition(Direction::DOWN)` without cleanup.
**Leak frequency:** Every mode switch triggers `TransitionToPattern()`, so memory leaks on every mode button press.
**Fix:** Replace raw `Pattern*` with `std::unique_ptr<Pattern>`. When assigning a new pattern, the old one is automatically deleted.

```cpp
std::unique_ptr<Pattern> currentPattern_;
std::unique_ptr<Pattern> nextPattern_;

void transitionToPattern(std::unique_ptr<Pattern> pattern) {
    nextPattern_ = std::move(pattern);
    currentPattern_ = std::make_unique<WaveTransition>();
}
```

**Complication:** Currently, patterns like `touch_blur`, `no_blur`, `strips` are stack-allocated globals in main.cpp and passed as raw pointers to `TransitionToPattern()`. With `unique_ptr`, these must become heap-allocated (owned by LedManager or AppEngine). The stack-allocated pattern globals must be eliminated -- each transition creates a fresh pattern via `make_unique`.

### FWBUG-02: TouchSlider::SetPosition No-Op

**Location:** `TouchSlider.cpp` lines 126-130
**Root cause:** `SetPosition(uint8_t, uint8_t)` calculates a `quantized_position` but never assigns it to `lastPosition`. The single-arg `SetPosition(float)` works correctly (line 49).
**Fix:**
```cpp
void TouchSlider::SetPosition(uint8_t intPosition, uint8_t numPositions) {
    if (numPositions == 0) return;
    lastPosition = static_cast<float>(intPosition) / static_cast<float>(numPositions);
}
```

### FWBUG-03: Unreachable XY_PAD Branch

**Location:** `main.cpp` lines 870-873
**Root cause:** `if (cfg.mode == Mode::XY_PAD)` is checked on line 822 with the full XY_PAD processing block. The duplicate check on line 870 is unreachable because it's in an `else if` chain after the first XY_PAD block already handled it.
**Fix:** Delete lines 870-873 during extraction.

### FWBUG-04: HardwareTest Infinite Loop

**Location:** `main.cpp` lines 585-608
**Root cause:** `while (!test_passed) { delay(100); }` on line 604-607 loops forever if any key fails. `test_passed` is never modified inside the loop.
**Fix per D-07:** Add 3-second timeout per key. Mark failed keys with red LED, continue testing remaining keys, then show summary and allow proceeding.

## Header-Only Extraction Audit (FWARCH-03)

Files that need `.hpp/.cpp` splitting:

| File | Lines | Static Members | Free Functions | Priority |
|------|-------|----------------|----------------|----------|
| `Libs/Keyboard.hpp` | 513 | 4 (`press_threshold`, `rel_threshold`, `at_threshold`, `instances`) | `fmap()` | HIGH -- static definitions break multi-TU |
| `Libs/Button.hpp` | 191 | None | None | MEDIUM |
| `Libs/Leds/LedManager.hpp` | 312 | None in class, but defines globals (`leds_plus_safety_pixel`, `matrixleds`, etc.) | `XY()` | HIGH -- globals defined in header |
| `Libs/Leds/patterns/Pattern.hpp` | 121 | 4 (`currentPalette`, `targetPalette`, `selectedOption`, etc.) | `wu_pixel()`, `wu_pixel_1d()` | HIGH -- static definitions break multi-TU |
| `Libs/Leds/patterns/*.hpp` (8 files) | ~30-80 each | None | Pattern implementations | MEDIUM -- inline in header works for single-include but messy |
| `Scales.hpp` | ~80 | None | `SetNoteMap()`, `SetChordMapping()` + `scales[]` array | HIGH -- array defined in header |
| `Libs/Signal.hpp` | 49 | None | None | SKIP -- template, must stay header-only |
| `Libs/Timer.hpp` | ~20 | None | None | SKIP -- trivial utility |
| `Libs/Types.hpp` | ~10 | None | None | SKIP -- type definitions only |

**Critical issue:** `Key::press_threshold` and friends are defined in `Keyboard.hpp` at file scope. If `Keyboard.hpp` is included in more than one .cpp file (which will happen after extraction), the linker will fail with multiple definition errors. Same for `Pattern::currentPalette`, `scales[]` array, `wu_pixel()`, `XY()`, LED globals, and `fmap()`.

## Global State Inventory (FWARCH-04)

Current globals in main.cpp that must be encapsulated:

| Variable | Type | New Owner |
|----------|------|-----------|
| `midi_provider` | MidiProvider | AppEngine |
| `led_manager` | LedManager | AppEngine |
| `adc` | Adc | AppEngine |
| `keyboard` | Keyboard | AppEngine |
| `slider` | TouchSlider | AppEngine |
| `t_btn`, `m_btn` | Button | AppEngine |
| `slider_mode` | SliderMode enum | SliderProcessor |
| `marker` | uint8_t | Unused -- remove |
| `current_chord`, `current_base_note`, `current_key_idx` | uint8_t | InputProcessor (strum state) |
| `current_qs_option`, `current_value_length` | uint8_t | InputProcessor (quick settings state) |
| `palette[]` | CRGBPalette16[] | LedManager or AppEngine (config-driven) |
| `touch_blur`, `no_blur`, `strips`, `strum`, `quick` | Pattern instances | Eliminated -- LedManager creates patterns via make_unique |
| `keys[]` | Key[] | AppEngine (passed to Keyboard) |
| `slider_sensor[]` | uint8_t[] | AppEngine (passed to TouchSlider) |
| `cfg`, `kb_cfg[]`, `cc_cfg[]`, `parameters`, `qs`, `calibration_data` | Configuration structs | ConfigManager (already owns `global_`, `banks_[]`, `cc_[]`); Parameters moves to a RuntimeState struct in AppEngine |

Globals in Configuration.hpp (extern declarations):
- `cfg`, `kb_cfg`, `cc_cfg`, `parameters`, `qs`, `calibration_data` -- these are currently extern-declared and defined in `Configuration.cpp`. After extraction, ConfigManager owns config data (already does), and `Parameters` becomes a member of AppEngine or a dedicated RuntimeState.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Smart pointers | Custom ref-counting | `std::unique_ptr` | Already in C++17 standard, zero overhead |
| Container iteration | Manual index loops for transports | Range-based for over `std::array` | Safer, same performance |
| String command parsing | Manual char-by-char parser | `strcmp` with registered command table | Simple, proven, tiny footprint |
| Time tracking | Custom timer class per service | `millis()` comparison in each `update()` | Arduino idiom, no overhead |

## Common Pitfalls

### Pitfall 1: Static Member Multiple Definition
**What goes wrong:** Moving `Keyboard.hpp` to be included from multiple .cpp files causes linker errors for `Key::press_threshold` etc.
**Why it happens:** Static member definitions at file scope in headers are emitted into every translation unit.
**How to avoid:** Move all `Type::member = value;` definitions to corresponding .cpp files.
**Warning signs:** Linker errors like "multiple definition of `Key::press_threshold`".

### Pitfall 2: LED Global Variables in Header
**What goes wrong:** `leds_plus_safety_pixel`, `matrixleds`, `sliderleds`, `patternleds` are defined in `LedManager.hpp`. Including this header from multiple .cpp files causes multiple definition errors.
**Why it happens:** These are non-inline variable definitions at namespace scope in a header.
**How to avoid:** Move to `LedManager.cpp` and expose via accessor methods or extern declarations in the .cpp only.
**Warning signs:** Linker errors on LED array names.

### Pitfall 3: Pattern Ownership During Transition
**What goes wrong:** During a transition, both `currentPattern_` (the WaveTransition) and `nextPattern_` must be alive. If `nextPattern_` is prematurely destroyed, `RunPattern()` will crash when the transition completes and tries to swap.
**Why it happens:** `unique_ptr` move semantics can be confusing during the transition handoff.
**How to avoid:** The transition completion in `RunPattern()` should do `currentPattern_ = std::move(nextPattern_)`. After this, `nextPattern_` is null, which is correct (no pending transition).
**Warning signs:** Null pointer dereference after pattern transition completes.

### Pitfall 4: Signal Thread Safety
**What goes wrong:** `TouchSlider::Update()` runs on a FreeRTOS task on core 0 (line 103 of TouchSlider.hpp), while the main loop runs on core 1. `Signal::Emit()` during `Update()` calls handlers that may access state from the main loop.
**Why it happens:** `Signal` uses `std::map` which is not thread-safe.
**How to avoid:** Per D-03, timed update intervals from the main loop replace FreeRTOS tasks. Remove `TouchSlider::Start()` and `Button::Start()` task creation. Call `slider.Update()` and button updates directly from AppEngine::update() on the main loop.
**Warning signs:** Sporadic crashes, corrupted state.

### Pitfall 5: Keyboard Key Array Ownership
**What goes wrong:** `Key keys[]` is currently a global array. `Keyboard` stores a pointer to it via `KeyboardConfig`. If AppEngine owns the Key array and it moves in memory (e.g., AppEngine is relocated), the pointer becomes dangling.
**Why it happens:** `KeyboardConfig` stores a raw pointer `Key* _keys`.
**How to avoid:** Either make the Key array a member of Keyboard directly, or ensure AppEngine is never moved (construct it once in main.cpp, never copy/move).
**Warning signs:** Garbage key readings, crashes in Key::Update().

### Pitfall 6: ConfigManager Already Exists
**What goes wrong:** Phase 1 created ConfigManager with its own data storage (`global_`, `banks_[]`, `cc_[]`). But main.cpp still uses the old globals (`cfg`, `kb_cfg`, `cc_cfg`). Services must use ConfigManager accessors, not the old globals.
**Why it happens:** Incomplete migration from Phase 1 globals to Phase 1 service.
**How to avoid:** During extraction, remove the old extern globals from `Configuration.hpp`. Services take `ConfigManager&` and call `.Global()`, `.Bank(i)`, `.CC(i)`.

## Code Examples

### AppEngine main.cpp (target state)

```cpp
// main.cpp -- target: ~30 lines
#include <Arduino.h>
#include "AppEngine.hpp"

static t16::AppEngine engine;

void setup() {
    engine.init();
}

void loop() {
    engine.update();
}
```

### ModeManager with Allowed Slider Modes

```cpp
// services/ModeManager.cpp
#include "ModeManager.hpp"

namespace t16 {

static constexpr SliderMode kKeyboardSliderModes[] = {BEND, MOD, OCTAVE, BANK};
static constexpr SliderMode kStrumSliderModes[] = {STRUMMING, OCTAVE, BANK};
static constexpr SliderMode kXYStripSliderModes[] = {SLEW, BANK};
static constexpr SliderMode kQuickSliderModes[] = {QUICK};

const SliderMode* ModeManager::getAllowedSliderModes(size_t& count) const {
    switch (currentMode_) {
    case Mode::KEYBOARD:
        count = sizeof(kKeyboardSliderModes) / sizeof(kKeyboardSliderModes[0]);
        return kKeyboardSliderModes;
    case Mode::STRUM:
        count = sizeof(kStrumSliderModes) / sizeof(kStrumSliderModes[0]);
        return kStrumSliderModes;
    case Mode::XY_PAD:
    case Mode::STRIPS:
        count = sizeof(kXYStripSliderModes) / sizeof(kXYStripSliderModes[0]);
        return kXYStripSliderModes;
    case Mode::QUICK_SETTINGS:
        count = sizeof(kQuickSliderModes) / sizeof(kQuickSliderModes[0]);
        return kQuickSliderModes;
    default:
        count = 0;
        return nullptr;
    }
}

} // namespace t16
```

### HardwareTest with Timeout (FWBUG-04 Fix)

```cpp
void CalibrationService::hardwareTest() {
    ledManager_.TestAll(HUE_YELLOW);
    delay(1500);
    ledManager_.OffAll();

    bool allPassed = true;
    for (int i = 0; i < 16; i++) {
        adc_.SetMuxChannel(keys_[i].mux_idx);
        unsigned long start = millis();
        bool keyOk = false;

        while (millis() - start < 3000) {  // 3-second timeout per key
            uint16_t value = adc_.GetRaw();
            if (value <= 4000 && value >= 50) {
                keyOk = true;
                break;
            }
            delay(10);
        }

        if (!keyOk) {
            ledManager_.SetLed(i, true);  // Red LED for failed key
            allPassed = false;
        }
        FastLED.show();
    }

    if (!allPassed) {
        delay(3000);  // Show failed keys for 3 seconds, then continue
    }
}
```

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Unity (PlatformIO built-in) |
| Config file | `platformio.ini` [env:native] section |
| Quick run command | `pio test -e native -f test_<name> --no-testing` then `pio test -e native -f test_<name>` |
| Full suite command | `pio test -e native` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FWARCH-01 | Service classes exist and compile | unit | `pio test -e native -f test_mode_manager` | No -- Wave 0 |
| FWARCH-05 | Transport abstraction dispatches to all active transports | unit | `pio test -e native -f test_transport` | No -- Wave 0 |
| FWBUG-02 | TouchSlider::SetPosition assigns lastPosition | unit | `pio test -e native -f test_slider` | No -- Wave 0 |
| TEST-01 | ModeManager transitions | unit | `pio test -e native -f test_mode_manager` | No -- Wave 0 |
| TEST-01 | InputProcessor key-to-note mapping | unit | `pio test -e native -f test_input_processor` | No -- Wave 0 |
| TEST-01 | Config serialization round-trip | unit | `pio test -e native -f test_config_manager` | Yes |
| TEST-01 | SysEx encoding | unit | `pio test -e native -f test_sysex_protocol` | Yes |

### Sampling Rate
- **Per task commit:** `pio test -e native -f test_<relevant>` (specific test suite)
- **Per wave merge:** `pio test -e native` (full native suite)
- **Phase gate:** Full suite green + `pio run -e esp32s3` compiles clean

### Wave 0 Gaps
- [ ] `test/test_mode_manager/test_main.cpp` -- covers ModeManager transitions, allowed slider modes
- [ ] `test/test_transport/test_main.cpp` -- covers transport abstraction dispatch (mock transports)
- [ ] `test/test_slider/test_main.cpp` -- covers SetPosition bug fix
- [ ] `test/test_input_processor/test_main.cpp` -- covers key-to-note mapping
- [ ] Stubs may need extension for Mode/SliderMode enums in native test environment

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Raw `new`/`delete` for patterns | `std::unique_ptr` | C++14/17 | Eliminates memory leaks automatically |
| `#ifndef` include guards | `#pragma once` | Widespread by 2015 | Simpler, faster compilation, already used in Phase 1 files |
| Leading underscore privates | Trailing underscore privates | Style convention | Consistent with eisei reference |
| Global free functions | Namespaced service methods | Architecture pattern | Testable, encapsulated |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| PlatformIO CLI | Build + test | Yes | 6.1.19 | -- |
| g++ (native tests) | `pio test -e native` | Yes | 15.2.1 | -- |
| Unity test framework | TEST-01 | Yes (PlatformIO built-in) | -- | -- |
| ArduinoJson | ConfigManager | Yes (lib_dep) | ^7.0.3 | -- |

**Missing dependencies with no fallback:** None.

## Open Questions

1. **Signal thread safety during extraction**
   - What we know: TouchSlider and Button have `Start()` methods that create FreeRTOS tasks on core 0. STATE.md flags this as a concern.
   - What's unclear: Whether the current firmware actually calls `Start()` (it's not called in main.cpp's setup/loop -- the main loop polls directly).
   - Recommendation: Verify `Start()` is not called. If not, the FreeRTOS tasks are dead code and can be removed during extraction. If called from somewhere not visible, must be handled.

2. **Pattern static members and FastLED globals**
   - What we know: `Pattern::currentPalette` is a static member used by all pattern instances. LED arrays are globals.
   - What's unclear: Whether patterns need to share palette state or could each hold their own.
   - Recommendation: Keep `currentPalette` as a static for now (it's a shared visual property), but move the definition to `Pattern.cpp`. LED arrays become LedManager private members with controlled access.

3. **CalibrationService scope**
   - What we know: CalibrationRoutine is ~75 lines of interactive hardware calibration that blocks the main loop.
   - What's unclear: Whether this should be a service (persistent) or a one-shot procedure.
   - Recommendation: Make it a service class with `runCalibration()` and `runHardwareTest()` methods. It only runs during startup or on SysEx trigger, not in the regular update loop.

## Sources

### Primary (HIGH confidence)
- Direct codebase analysis of all source files listed above
- Phase 1 artifacts (ConfigManager.hpp, SysExHandler.hpp, test infrastructure)
- CONTEXT.md locked decisions (D-01 through D-10)

### Secondary (MEDIUM confidence)
- eisei codebase patterns referenced in CLAUDE.md (not directly inspected but described in project docs)
- C++17 `std::unique_ptr` and `std::array` are well-established standard library features

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- PlatformIO/Arduino/C++17, no new dependencies needed
- Architecture: HIGH -- service boundaries clearly visible from existing code structure, eisei reference well-documented
- Pitfalls: HIGH -- all bugs verified by direct code inspection, thread safety concern flagged in STATE.md
- Test infrastructure: HIGH -- validated existing native test env, Unity framework working

**Research date:** 2026-04-03
**Valid until:** 2026-05-03 (stable -- no external dependency changes expected)
