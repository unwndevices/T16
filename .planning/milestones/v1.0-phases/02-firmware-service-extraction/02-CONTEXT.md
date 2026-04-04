# Phase 2: Firmware Service Extraction - Context

**Gathered:** 2026-04-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Extract all application logic from the 887-line monolithic main.cpp into testable service classes. Fix all known firmware bugs (FWBUG-01 through FWBUG-04) during extraction. Add serial command interface for diagnostics. main.cpp becomes a slim orchestrator under 150 lines.

</domain>

<decisions>
## Implementation Decisions

### Service Architecture
- **D-01:** main.cpp orchestration uses an **AppEngine class** — single orchestrator object owns all services, main.cpp just creates it and calls `init()`/`update()`. Matches eisei's EiseiEngine pattern.
- **D-02:** Services communicate via **dependency injection through constructor refs** — services receive references to their dependencies at construction (e.g., `InputProcessor(ConfigManager&, MidiProvider&)`). Explicit, testable, no hidden coupling.
- **D-03:** Task scheduling uses **timed update intervals** — each service has an `Update()` called from the main loop, services that need timing track their own intervals. No FreeRTOS tasks, no external scheduler library.
- **D-04:** Mode switching via a **ModeManager service** — owns the current mode enum, exposes `SetMode()`/`GetMode()`, services query it. Replaces scattered `if (mode == ...)` blocks.

### Bug Fix Strategy
- **D-05:** Bugs are fixed **during extraction** — fix each bug as its subsystem is extracted into a service class. Natural cleanup point.
- **D-06:** LedManager memory leak fixed with **std::unique_ptr** — `currentPattern_` and `nextPattern_` as `std::unique_ptr<Pattern>`. Auto-cleanup on transition.
- **D-07:** HardwareTest uses **3-second timeout per key** — if key doesn't respond within 3s, mark as failed, flash red LED for that position, continue testing remaining keys.

### Serial Commands & Testing
- **D-08:** Serial commands follow **eisei pattern** — categorized command files (`DiagnosticCommands.cpp`, `ConfigCommands.cpp`), registered with a `SerialCommandManager`. Text-based commands over USB serial.
- **D-09:** Serial commands expose **core diagnostics** — config dump, key states, slider values, MIDI monitor, version info.
- **D-10:** Test coverage targets **service interfaces + state machines** — test ModeManager transitions, InputProcessor key-to-note mapping, transport abstraction dispatch. Mock hardware dependencies.

### Claude's Discretion
- Exact service class boundaries and file organization
- Header guard style (#pragma once vs #ifndef — eisei uses #pragma once)
- Namespace usage (eisei uses `namespace eisei {}`)
- Private member naming convention (eisei uses trailing underscore)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/Libs/Signal.hpp` — Template observer pattern, used for key/button/slider events
- `src/ConfigManager.hpp/cpp` — Already extracted in Phase 1 (load-once/save-once)
- `src/SysExHandler.hpp/cpp` — Already extracted in Phase 1 (validated dispatch)
- `src/SysExProtocol.hpp` — Protocol constants
- `src/Libs/MidiProvider.hpp/cpp` — MIDI transport (needs refactoring for transport abstraction)

### Established Patterns
- Header-only implementations in `src/Libs/` — need splitting to .hpp/.cpp pairs
- Global constructors interleaved with includes in main.cpp
- Signal/slot for hardware events (Key::onStateChanged, Button::onStateChanged)
- LED patterns use Strategy pattern (Pattern base class)

### Integration Points
- main.cpp line 726: setup() — hardware init, calibration, config load
- main.cpp line 811: loop() — polling MIDI, buttons, slider, keyboard, mode processing, LED render
- All Process*() callbacks defined as free functions in main.cpp
- Configuration.hpp — global structs (will be replaced by ConfigManager)

</code_context>

<specifics>
## Specific Ideas

- Follow eisei's `EiseiEngine` as the reference for AppEngine structure
- Use eisei's `SerialCommandManager` pattern for serial commands
- Use eisei's trailing underscore convention for private members
- Use `#pragma once` instead of include guards (eisei convention)
- Use namespaces to scope service classes

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
