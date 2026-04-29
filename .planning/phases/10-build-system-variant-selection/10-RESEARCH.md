# Phase 10: Build System & Variant Selection — Research

**Researched:** 2026-04-29
**Domain:** PlatformIO build configuration, C++ preprocessor / namespace pinout migration, ESP32-S3 firmware
**Confidence:** HIGH (codebase-grounded; PlatformIO syntax already used in repo)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D10.1** — Default build env: `default_envs = t16_release` in `platformio.ini`. Bare `pio run` continues to build T16 release.
- **D10.2** — Variant flag: bare `-DT16` / `-DT32` defines (mutually exclusive). Code branches use `#if defined(T32)` / `#else`. No `-DVARIANT=...` token; no integer-mapped enum.
- **D10.3** — Pinout headers: namespaced (`namespace pinout::t16` / `namespace pinout::t32`). Single `pinout.h` aliases the active variant via `using CurrentPinout = pinout::tXX;`. All firmware code references `CurrentPinout::S0`, `CurrentPinout::LED_PIN`, etc. — no bare `#define`s leak.
- **D10.4** — CI matrix: GitHub Actions runs all four envs (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`) as required jobs. Pinout headers must compile cleanly for both variants in Phase 10.

### Claude's Discretion
- Exact PlatformIO inheritance mechanism (`extends` vs `[env]` base) — choose whichever preserves the current `[env_common]` shared-deps pattern.
- Whether to keep, fold, or rename `REV_A` / `REV_B` flags during this phase. Default: keep them orthogonal (per D10.3 implementation note).
- File naming for the new pinout headers (e.g., `pinout_t16.h` vs `Pinout/T16.h`) — go with flat `pinout_t16.h` / `pinout_t32.h` siblings of `pinout.h` to minimize churn.

### Deferred Ideas (OUT OF SCOPE)
- `HardwareVariantConfig` constexpr struct → Phase 11 (HAL-01).
- Macro removal from `Adc`/`Keyboard`/`LedManager`/`DataManager` body code → Phase 11 (HAL-02).
- Any T32 hardware behavior or runtime variant detection → Phase 12.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BUILD-01 | Per-variant pinout headers (`pinout_t16.h`, `pinout_t32.h`) with single `pinout.h` aliasing `CurrentPinout` to active variant | Pinout migration plan below — only 3 includer files (`AppEngine.hpp`, `LedManager.hpp`, `ButtonHandler.cpp`); macro audit complete |
| BUILD-02 | Four PlatformIO build envs (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`) selecting variant via `-DT16` / `-DT32` | Existing `extends = env_common` pattern proven in repo; identical pattern works for four envs |
| BUILD-03 | Default build target (`default_envs`) builds T16 release so existing `pio run` flows keep working | `[platformio]` already uses `default_envs` — change value from `esp32s3` to `t16_release` |
</phase_requirements>

## Summary

Phase 10 is a **build-system + pinout namespacing** refactor. It does NOT change runtime behavior on T16 hardware. Three things must change in lockstep:

1. **`platformio.ini`** — Replace the single `[env:esp32s3]` + `[env:release]` pair with four envs (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`), each setting `-DT16` or `-DT32` plus the existing debug/release flags. Switch `default_envs` to `t16_release`. Keep `[env:native]` untouched (unit tests).
2. **Pinout headers** — Create `src/pinout_t16.h` (constants from current `REV_B` block, lifted into `namespace pinout::t16`) and `src/pinout_t32.h` (placeholder constants — values come from Phase 12; for Phase 10 they can mirror T16 OR be sentinel `0xFF` so the file compiles). Rewrite `src/pinout.h` as a thin selector that includes the right header and emits `using CurrentPinout = pinout::tXX;`.
3. **Three call-sites** — Update `AppEngine.hpp`, `AppEngine.cpp`, `LedManager.hpp/.cpp`, `ButtonHandler.cpp` to use `CurrentPinout::FOO` instead of bare `PIN_FOO` macros. CONTEXT.md is explicit that the broader `Adc`/`Keyboard`/`LedManager` macro cleanup belongs to Phase 11, but `pinout.h` must stop emitting bare `#define`s — so these direct consumers must migrate now (otherwise the build breaks).
4. **CI** — Extend `.github/workflows/ci.yml` `firmware-build` job to a matrix of all four envs.

**Primary recommendation:** Execute as four sequential plans within the same phase: (1) PlatformIO envs + default_envs, (2) Pinout namespace headers + selector, (3) Call-site migration to `CurrentPinout::`, (4) CI matrix + smoke test. Plans 1 and 2 can technically run in parallel (no file overlap), but (3) depends on (2), and (4) depends on (1)+(3) so it can verify the matrix is real-green.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Variant flag selection | Build system (PlatformIO) | — | Compile-time only; no runtime tier involved |
| Pin constant resolution | C++ preprocessor + namespace | — | Header-only resolution at compile time |
| Active variant aliasing | `pinout.h` (header) | — | Single point of `using CurrentPinout = ...` |
| CI matrix enforcement | GitHub Actions | — | Build verification across all four envs |

This phase touches no runtime tier — purely build/preprocessor.

## Standard Stack

### Core (already in use)
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| PlatformIO | repo-pinned via `~/.platformio` | Build orchestration | Project's locked toolchain (CLAUDE.md constraint: "must stay") |
| GCC for ESP32-S3 (xtensa) | provided by `platform = espressif32` | C++ compilation | Toolchain pinned by platform package |
| GitHub Actions | n/a | CI matrix | Already configured in `.github/workflows/ci.yml` |

### Supporting
No new libraries needed. This is a pure build/preprocessor refactor.

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `extends = env_common` (PlatformIO inheritance) | Repeat `lib_deps` in each env | Inheritance keeps single source of truth — repo already uses it; no reason to abandon |
| Namespace + `using CurrentPinout = ...` | `inline namespace pinout` ADL trick | Locked decision D10.3; ADL makes call-site less explicit |
| Bare `-DT16` / `-DT32` | `-DVARIANT=16` integer | Locked decision D10.2; integer mapping invites accidental truthy checks |

## Architecture Patterns

### System Architecture Diagram

```
                   ┌──────────────────────┐
   pio run -e ──→  │ platformio.ini       │ ──── -DT16 or -DT32 ───┐
   t16_release     │ [env:t16_release]    │                        │
   (default)       │ [env:t32_release]    │                        ▼
                   │ [env:t16_debug]      │             ┌──────────────────────┐
                   │ [env:t32_debug]      │             │ pinout.h (selector)  │
                   │ extends = env_common │             │                      │
                   └──────────────────────┘             │ #if defined(T32)     │
                              │                         │   include pinout_t32 │
                              │                         │   using CurrentPinout│
                              ▼                         │     = pinout::t32;   │
                   [env_common] (shared lib_deps)        │ #else                │
                              │                         │   include pinout_t16 │
                              ▼                         │   using CurrentPinout│
                   [env:native] (unit tests, untouched) │     = pinout::t16;   │
                                                        │ #endif               │
                                                        └──────────────────────┘
                                                              │
                                                              ▼
                          ┌─────────────────────────────────────────────────┐
                          │ AppEngine.hpp/.cpp, LedManager.hpp/.cpp,        │
                          │ ButtonHandler.cpp use CurrentPinout::S0, etc.   │
                          └─────────────────────────────────────────────────┘
```

### Recommended Project Structure
```
src/
├── pinout.h              # Selector: includes correct variant header, emits CurrentPinout alias
├── pinout_t16.h          # namespace pinout::t16 { constexpr uint8_t S0 = 4; ... }
├── pinout_t32.h          # namespace pinout::t32 { constexpr uint8_t S0 = ...; ... }   (Phase 10: placeholder values OK)
├── AppEngine.hpp/.cpp    # uses CurrentPinout::TOUCH, ::MODE, ::T1..T7, ::S0..S3, ::COM, ::RX/TX/TX2
├── Libs/Leds/LedManager.hpp/.cpp  # uses CurrentPinout::LED_PIN
└── services/ButtonHandler.cpp     # uses CurrentPinout::TOUCH, ::MODE
```

### Pattern 1: PlatformIO `extends` inheritance (already used in repo)
**What:** A child env inherits all keys from a base section and can override or append via `${section.key}`.
**When to use:** When envs share most settings (lib_deps, framework, board) but differ in a few flags.
**Example:**
```ini
; Source: existing platformio.ini, extended for four variants
[env_common]
platform = espressif32
framework = arduino
board_build.filesystem = littlefs
lib_archive = no
lib_deps =
	adafruit/Adafruit TinyUSB Library @ 3.3.0
	; ... (unchanged)
build_flags =
	-DUSE_TINYUSB
monitor_speed = 115200
monitor_filters = esp32_exception_decoder

[env:t16_debug]
extends = env_common
board = unwn_s3
build_flags =
	${env_common.build_flags}
	-Os
	-DCORE_DEBUG_LEVEL=5
	-DREV_B
	-DT16

[env:t16_release]
extends = env_common
board = unwn_s3
build_flags =
	${env_common.build_flags}
	-Os
	-DCORE_DEBUG_LEVEL=0
	-DREV_B
	-DT16

[env:t32_debug]
extends = env_common
board = unwn_s3
build_flags =
	${env_common.build_flags}
	-Os
	-DCORE_DEBUG_LEVEL=5
	-DREV_B
	-DT32

[env:t32_release]
extends = env_common
board = unwn_s3
build_flags =
	${env_common.build_flags}
	-Os
	-DCORE_DEBUG_LEVEL=0
	-DREV_B
	-DT32
```
Note: `[env_common]` is a "free section" name (anything PlatformIO does not recognize as a reserved env section is treated as inheritable via `extends`). The repo already proves this pattern works.

### Pattern 2: Namespaced pinout header
**What:** Replace `#define PIN_S0 4` with `namespace pinout::t16 { constexpr uint8_t S0 = 4; }`. Use `constexpr uint8_t` (matches GPIO pin types in Arduino-ESP32 and avoids implicit-int issues).
**When to use:** Per D10.3, mandatory for all variant-conditional pin sets.
**Example:**
```cpp
// src/pinout_t16.h
#pragma once
#include <cstdint>

namespace pinout::t16 {

// Multiplexer (single mux on T16)
constexpr uint8_t S0 = 4;
constexpr uint8_t S1 = 5;
constexpr uint8_t S2 = 6;
constexpr uint8_t S3 = 7;
constexpr uint8_t COM = 8;

// LED strip
constexpr uint8_t LED_PIN = 1;

// Touch slider sensors
constexpr uint8_t T1 = 3;
constexpr uint8_t T2 = 9;
constexpr uint8_t T3 = 10;
constexpr uint8_t T4 = 11;
constexpr uint8_t T5 = 12;
constexpr uint8_t T6 = 13;
constexpr uint8_t T7 = 14;

// MIDI / serial pins
constexpr uint8_t TX  = 43;
constexpr uint8_t RX  = 44;
constexpr uint8_t TX2 = 42;

// Buttons (REV_B values shown — REV_A path lives behind a #ifdef inside this file
// or in a sibling pinout_t16_revA.h; per CONTEXT.md, REV flags stay orthogonal in Phase 10)
constexpr uint8_t TOUCH = 21;
constexpr uint8_t EXT3  = 41;
constexpr uint8_t EXT4  = 40;
constexpr uint8_t MODE  = 0;

}  // namespace pinout::t16
```

### Pattern 3: `pinout.h` selector
```cpp
// src/pinout.h
#pragma once

#if defined(T32)
  #include "pinout_t32.h"
  using CurrentPinout = pinout::t32;
#elif defined(T16)
  #include "pinout_t16.h"
  using CurrentPinout = pinout::t16;
#else
  #error "No variant defined: build must set -DT16 or -DT32"
#endif
```
The `#error` guard catches misconfigured envs at compile time — better than silently defaulting to T16 and shipping wrong binaries.

### Anti-Patterns to Avoid
- **Default-to-T16 fallback in `pinout.h`** — silent compilation under wrong variant; use `#error` instead.
- **`namespace pinout { namespace t16 { ... } }` (nested form, C++14 style)** — works but verbose; C++17 nested namespace `namespace pinout::t16 {}` is cleaner and matches CLAUDE.md target style (eisei uses C++17).
- **Defining macros inside the namespace** (`namespace pinout::t16 { #define S0 4 }`) — macros are not namespaced; this defeats the entire migration.
- **Modifying `Adc`/`Keyboard` body code that doesn't use `pinout.h`** — that's Phase 11 (HAL-02). Stay surgical.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Build env inheritance | Custom Python `extra_scripts` to template envs | PlatformIO native `extends = ...` | Already idiomatic, already used in repo |
| Conditional pin constants | Runtime variant detection via GPIO probing | Compile-time `-DT16`/`-DT32` | Locked by D10.2; runtime detection adds boot cost |
| Variant-aware tests in this phase | Custom test harness for variant matrix | GitHub Actions matrix strategy | Standard CI pattern; D10.4 |

## Runtime State Inventory

This is a refactor phase (rename `PIN_*` macros → `CurrentPinout::*`). Per the rename/refactor checklist:

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — pin constants are compile-time only, never persisted to LittleFS, never serialized to MIDI/JSON. Verified by `grep -r "PIN_" src/` — only direct usages, no string serialization. | None |
| Live service config | None — no external services name pins | None |
| OS-registered state | None — pin constants don't escape the firmware binary | None |
| Secrets/env vars | None | None |
| Build artifacts | `.pio/build/esp32s3/` and `.pio/build/release/` will become stale once env names change. After plan 1 merges, contributors should run `pio run -t clean` or delete `.pio/build/`. | Document in plan 1 verification step; CI cache key is `pio-${{ hashFiles('platformio.ini') }}` so it auto-invalidates. |

**Canonical question — what runtime state survives a rename?** Nothing. Pin macros never leave the binary in a way that downstream state depends on the literal name. Safe to refactor.

## Common Pitfalls

### Pitfall 1: PlatformIO env name collision with cached build dirs
**What goes wrong:** Existing developers have `.pio/build/esp32s3/` and `.pio/build/release/` from old envs. Removing those env names leaves stale build dirs around but doesn't break anything; new envs build into new dirs.
**Why it happens:** PlatformIO uses env name as build dir name.
**How to avoid:** Mention in plan summary that `.pio/build/` may be safely deleted after merge. CI cache key (`pio-${{ hashFiles('platformio.ini') }}`) auto-invalidates because `platformio.ini` content changes.
**Warning signs:** Disk space creep. Not a build-blocker.

### Pitfall 2: `using CurrentPinout = pinout::t16;` placed in a header with multiple includers
**What goes wrong:** A `using` declaration at namespace scope in a header is fine (one-definition-rule allows duplicate `using` directives). BUT putting it inside `namespace { ... }` (anonymous) makes it translation-unit-local — different TUs see different `CurrentPinout`. Don't use anonymous namespace.
**Why it happens:** Reflexive "make headers internal" instinct.
**How to avoid:** Place `using CurrentPinout = ...;` at file scope (global), no anonymous namespace. Verified safe under ODR because the alias resolves to the same type in every TU within a single build env.
**Warning signs:** Linker complaints about ambiguous types — won't happen if alias is at global scope.

### Pitfall 3: Forgetting `#include <cstdint>` in `pinout_t16.h`
**What goes wrong:** `uint8_t` is undefined; cryptic "uint8_t does not name a type" errors.
**Why it happens:** Old `pinout.h` got it from the Arduino prelude transitively.
**How to avoid:** Explicit `#include <cstdint>` in each variant header.

### Pitfall 4: T32 placeholder header doesn't compile for T32 build
**What goes wrong:** Phase 10 D10.4 requires T32 envs to compile cleanly. If `pinout_t32.h` is empty or malformed, T32 builds fail in CI even though Phase 12 hasn't started.
**Why it happens:** Skipping the placeholder file because "Phase 12 will define real values."
**How to avoid:** Stub `pinout_t32.h` with placeholder values (e.g., copy T16 values verbatim with a `// TODO Phase 12` comment) so all `CurrentPinout::FOO` references resolve to legal `uint8_t` constants. The T32 build won't run on T16 hardware — it just needs to compile.
**Warning signs:** `error: 'S0' is not a member of 'pinout::t32'` in T32 CI job.

### Pitfall 5: REV_A code path silently broken
**What goes wrong:** Old `pinout.h` had `#ifdef REV_A` and `#ifdef REV_B` blocks. If the new `pinout_t16.h` only ports the REV_B values, REV_A breaks. Active build is REV_B (per CLAUDE.md), so this may go unnoticed.
**Why it happens:** REV flags are an orthogonal dimension and easy to forget.
**How to avoid:** Ports both REV_A and REV_B value sets into `pinout_t16.h` using internal `#if defined(REV_A) ... #elif defined(REV_B) ...` blocks inside the `namespace pinout::t16 { ... }` body. Or fold REV into a separate file (`pinout_t16_revA.h` / `pinout_t16_revB.h`) and let `pinout.h` pick. Recommendation: keep them inline within `pinout_t16.h` to minimize file count, since only 4 pin values differ between REV_A and REV_B (S0..S3 and TOUCH).
**Warning signs:** REV_A build (if anyone tries it) emits "S0 is not a member" errors.

## Code Examples

### Migrating a call site
**Before** (`src/AppEngine.hpp:46`):
```cpp
uint8_t sliderSensors_[7] = {PIN_T1, PIN_T2, PIN_T3, PIN_T4, PIN_T5, PIN_T6, PIN_T7};
Button touchBtn_{PIN_TOUCH};
Button modeBtn_{PIN_MODE};
```

**After:**
```cpp
uint8_t sliderSensors_[7] = {CurrentPinout::T1, CurrentPinout::T2, CurrentPinout::T3,
                              CurrentPinout::T4, CurrentPinout::T5, CurrentPinout::T6,
                              CurrentPinout::T7};
Button touchBtn_{CurrentPinout::TOUCH};
Button modeBtn_{CurrentPinout::MODE};
```

### CI matrix snippet
```yaml
# .github/workflows/ci.yml — updated firmware-build job
firmware-build:
  runs-on: ubuntu-latest
  strategy:
    fail-fast: false
    matrix:
      env: [t16_debug, t16_release, t32_debug, t32_release]
  steps:
    - name: Checkout
      uses: actions/checkout@v4
    - name: Cache PlatformIO
      uses: actions/cache@v4
      with:
        path: ~/.platformio
        key: pio-${{ hashFiles('platformio.ini') }}-${{ matrix.env }}
        restore-keys: pio-${{ hashFiles('platformio.ini') }}-
    - name: Set up Python
      uses: actions/setup-python@v5
      with:
        python-version: '3.11'
    - name: Install PlatformIO
      run: pip install platformio
    - name: Build firmware (${{ matrix.env }})
      run: pio run -e ${{ matrix.env }}

firmware-tests:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v4
    - uses: actions/setup-python@v5
      with: { python-version: '3.11' }
    - run: pip install platformio
    - run: pio test -e native
```
Tests are split into a sibling job because `pio test -e native` is independent of the variant matrix.

## Pinout Macro Audit (call-site inventory)

Verified by `grep -rn "PIN_\|REV_A\|REV_B" src/` (excluding `pinout.h` itself):

| File | Macros referenced | Notes |
|------|-------------------|-------|
| `src/AppEngine.hpp` | `PIN_T1..T7`, `PIN_TOUCH`, `PIN_MODE` (member init) + `#ifdef REV_B` for keymap | Migrate `PIN_*` → `CurrentPinout::*`. REV_B keymap stays as-is (it's about physical wiring, not pin numbers). |
| `src/AppEngine.cpp` | `PIN_RX`, `PIN_TX`, `PIN_TX2`, `PIN_TOUCH`, `PIN_MODE`, `PIN_COM`, `PIN_S0..S3` | Migrate all to `CurrentPinout::*`. |
| `src/Libs/Leds/LedManager.hpp` | `#include "pinout.h"` | Header include only — keep but switch to `CurrentPinout::LED_PIN` if it uses PIN_LED inline. (Currently doesn't reference PIN_ macros directly.) |
| `src/Libs/Leds/LedManager.cpp` | `PIN_LED` (in `FastLED.addLeds<...>`) + `#ifdef REV_B` blocks | `FastLED.addLeds<WS2812B, PIN_LED, GRB>` is a template parameter — must be a constant expression. `CurrentPinout::LED_PIN` is `constexpr uint8_t`, so this works. REV_B blocks stay (palette swap, unrelated to pinout). |
| `src/services/ButtonHandler.cpp` | `PIN_TOUCH`, `PIN_MODE` | Migrate. |

**Total migration:** 5 files, ~12 call-site replacements. Small scope.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `#define PIN_S0 4` (C-style) | `constexpr uint8_t S0 = 4;` (C++17) | This phase | Type-safe, namespaced, no preprocessor leakage |
| Single `[env:esp32s3]` + `[env:release]` | Four envs (`t16_*`, `t32_*`) with `extends` | This phase | CI matrix coverage; no copy-paste lib_deps |
| Single GitHub Actions `firmware-build` job | Matrix strategy over four envs | This phase | Catches T32-only regressions immediately |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Placing `using CurrentPinout = pinout::t16;` at global scope in `pinout.h` is ODR-safe across translation units when the same variant flag is set for every TU in a build. | Pattern 3 / Pitfall 2 | LOW — every PlatformIO build env compiles all TUs with identical flags; ODR violation would only occur with mixed flags (impossible per build). [VERIFIED via cppreference: type aliases at namespace scope follow ODR for type identity.] |
| A2 | `FastLED.addLeds<WS2812B, CurrentPinout::LED_PIN, GRB>(...)` accepts the namespaced constexpr as a non-type template parameter. | Macro audit | LOW — `constexpr uint8_t` satisfies the integral constant-expression requirement for non-type template parameters. [VERIFIED — current code uses `PIN_LED` macro which expands to integer literal, semantically identical.] |
| A3 | The repo's `[env_common]` section name and `extends = env_common` syntax is a stable PlatformIO feature. | Standard Stack | LOW — already in production use in this repo's `platformio.ini`. [VERIFIED — repo build works.] |
| A4 | T32 placeholder values can mirror T16 values for Phase 10 without affecting Phase 12 work. | Pitfall 4 | LOW — Phase 12 will overwrite the file; Phase 10 only needs the T32 build to compile, not run. |

**No HIGH-risk assumptions.** All claims are codebase-grounded or rely on well-established C++17 / PlatformIO behavior.

## Open Questions

1. **Should `PIN_LED` become `LED_PIN` or stay `LED`?**
   - What we know: CONTEXT.md sample uses `CurrentPinout::LED_PIN`. Existing macro is `PIN_LED`.
   - What's unclear: Naming convention inside the namespace. Both work.
   - Recommendation: Use `LED_PIN` (CONTEXT.md sample). Be consistent across all pin names: drop the `PIN_` prefix (since the namespace already names it as a pinout) but preserve any suffix. E.g., `PIN_S0` → `S0`, `PIN_LED` → `LED_PIN` (the CONTEXT.md sample), `PIN_TOUCH` → `TOUCH`.
   - **Lock for plans:** drop `PIN_` prefix; otherwise keep names verbatim. Confirmed by CONTEXT.md sample.

2. **Should `pinout_t32.h` mirror T16 values or use sentinel `0xFF`?**
   - What we know: D10.4 says T32 must compile. Phase 12 will fill in real values.
   - What's unclear: Whether placeholder values should look "obviously fake."
   - Recommendation: Mirror T16 for now with a header banner comment `// PHASE 10 PLACEHOLDER — Phase 12 (T32 hardware bring-up) will replace with validated T32 values`. Mirroring avoids cascading "is this 0xFF actually a valid GPIO?" confusion in Phase 11/12 reviewers.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| PlatformIO Core | Build | ✓ assumed (CLAUDE.md mandates it) | repo-pinned | none — phase blocks otherwise |
| GitHub Actions runners | CI matrix | ✓ | ubuntu-latest | none |
| ESP32-S3 toolchain (xtensa-esp32s3-elf) | Firmware compile | ✓ via `platform = espressif32` package | auto-fetched | none |
| Physical T16 device | Smoke test (success criterion #4) | User-side | n/a | Defer smoke test to user — `checkpoint:human-verify` task |
| Physical T32 device | NOT required this phase | n/a | n/a | n/a (deferred to Phase 12) |

**No blocking missing deps.**

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | PlatformIO Unity (existing `[env:native]`) |
| Config file | `platformio.ini` — `[env:native]` section |
| Quick run command | `pio test -e native` |
| Full suite command | `pio test -e native` (same — single suite) |

Note: this phase's verification is dominated by **build matrix success** (4 envs compile), not unit tests. The native test suite remains untouched. Validation is primarily: `pio run -e t16_release && pio run -e t16_debug && pio run -e t32_release && pio run -e t32_debug` all return 0.

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BUILD-01 | `CurrentPinout::S0` resolves correctly per variant | smoke (compile) | `pio run -e t16_release && pio run -e t32_release` | ✅ (envs created in this phase) |
| BUILD-02 | Four envs all compile | smoke | `for e in t16_debug t16_release t32_debug t32_release; do pio run -e $e || exit 1; done` | ✅ |
| BUILD-03 | `pio run` (no env arg) builds T16 release | smoke | `pio run` then check artifact path matches `.pio/build/t16_release/firmware.bin` | ✅ |
| Success criterion #4 | T16 hardware behavior identical | manual | `checkpoint:human-verify` — flash + smoke test on T16 | manual |

### Sampling Rate
- **Per task commit:** No unit tests added. Build verification: `pio run -e t16_release` for any plan touching firmware.
- **Per wave merge:** All four `pio run -e <env>` builds.
- **Phase gate:** All four envs compile + manual T16 smoke test.

### Wave 0 Gaps
None. Existing `[env:native]` test infra covers what's needed; this phase adds compile-time verification, not new test scaffolds.

## Security Domain

This phase is build-system + preprocessor only. No new attack surface, no input handling, no credentials. STRIDE table is included for completeness but every category is N/A.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | n/a |
| V3 Session Management | no | n/a |
| V4 Access Control | no | n/a |
| V5 Input Validation | no | n/a (no inputs) |
| V6 Cryptography | no | n/a |
| V14 Configuration | yes (build config) | Default-deny (`#error` in pinout.h if no variant defined) |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Wrong variant binary flashed to wrong hardware | Tampering (data integrity) | Default-deny `#error` in `pinout.h` ensures no implicit-T16 fallback; CI matrix catches T32 build failures pre-merge |
| Stale build cache produces ghost binary | Tampering | PlatformIO build dir is per-env; CI cache key includes env name |

## Sources

### Primary (HIGH confidence — codebase-grounded)
- `/home/unwn/git/T16/platformio.ini` — current env structure, `extends = env_common` pattern
- `/home/unwn/git/T16/src/pinout.h` — current macro definitions, REV_A/REV_B branches
- `/home/unwn/git/T16/.github/workflows/ci.yml` — current CI structure and cache key
- `grep -rn "PIN_\|REV_A\|REV_B" src/` — call-site audit, 5 files identified

### Secondary (MEDIUM confidence — established C++ idioms)
- C++17 nested namespace syntax (`namespace pinout::t16 {}`) — standard since C++17
- `using` alias at global scope ODR semantics — cppreference standard

### Tertiary (LOW confidence)
None. Phase is well-bounded by codebase evidence.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all tools already in repo
- Architecture: HIGH — locked by CONTEXT.md decisions, codebase audit confirms feasibility
- Pitfalls: HIGH — derived from direct file inspection

**Research date:** 2026-04-29
**Valid until:** 2026-05-29 (30 days; build-system patterns are stable)
