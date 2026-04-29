# Phase 10 — Build System & Variant Selection: Context

**Phase goal:** Compile-time variant selection picks T16 or T32 binaries via PlatformIO build flags.
**Requirements:** BUILD-01, BUILD-02, BUILD-03

---

## Locked Decisions

### D10.1 — Default build env: `t16_release`
- `default_envs = t16_release` in `platformio.ini`.
- Bare `pio run` continues to build the T16 release binary, matching success criterion #2 and preserving existing developer muscle memory.

### D10.2 — Variant flag: bare `-DT16` / `-DT32` defines
- Build envs set `build_flags = -DT16` or `build_flags = -DT32` (mutually exclusive).
- Code branches use `#if defined(T32)` / `#else` blocks.
- No `-DVARIANT=...` token; no integer-mapped enum at the preprocessor level.

### D10.3 — Pinout headers: namespaced (`namespace pinout::t16` / `pinout::t32`)
- `pinout_t16.h` declares `namespace pinout::t16 { constexpr uint8_t S0 = ...; ... }`.
- `pinout_t32.h` declares `namespace pinout::t32 { ... }`.
- Single `pinout.h` aliases the active variant:
  ```cpp
  #if defined(T32)
    #include "pinout_t32.h"
    using CurrentPinout = pinout::t32;
  #else
    #include "pinout_t16.h"
    using CurrentPinout = pinout::t16;
  #endif
  ```
- All firmware code references `CurrentPinout::S0`, `CurrentPinout::LED_PIN`, etc. — no bare `#define`s leak.
- Aligns with eisei target style; eliminates the C-header `#define` pattern called out in CLAUDE.md.

### D10.4 — CI matrix: all 4 envs from Phase 10
- GitHub Actions runs `t16_debug`, `t16_release`, `t32_debug`, `t32_release` as required jobs.
- Pinout headers must compile cleanly for both variants in Phase 10, even though `Adc`/`Keyboard`/etc. still reference legacy macros — those break in T32 only after Phase 11 starts touching them.
- Catches T32 regressions immediately when Phase 11 work begins.

---

## Implementation Notes for Researcher / Planner

- The four envs share most of `[env:esp32s3]`; use PlatformIO `[env]` inheritance or `extends` rather than copy-paste. Validate the `extends` syntax against current PlatformIO version.
- `pinout.h` today contains a single set of `#define`s — Phase 10 deletes them and re-introduces the constants inside `namespace pinout::t16` (and the new `pinout::t32` file). All `#include "pinout.h"` consumers should keep working because `CurrentPinout::FOO` resolves to the same value.
- `REV_A` / `REV_B` flags currently live alongside variant-related work. Decide whether `REV_*` becomes a per-variant property of `pinout::t16` or stays as an orthogonal build flag — flag for researcher to investigate; do not couple unless necessary.
- Smoke test (success criterion #4) runs on physical T16 hardware; planner must include this as a manual verification step.

## Out of Scope for Phase 10

- `HardwareVariantConfig` constexpr struct → Phase 11 (HAL-01)
- Macro removal from `Adc`/`Keyboard`/`LedManager`/`DataManager` → Phase 11 (HAL-02)
- Any T32 hardware behavior → Phase 12

## Open Items for Researcher

1. Confirm PlatformIO `extends`/`[env]` inheritance syntax and that all four envs can share `lib_deps`.
2. Identify every `#include "pinout.h"` consumer to confirm none rely on the `#define` form (e.g., `#ifdef PIN_*`).
3. Decide how `REV_A`/`REV_B` interacts with variant flags (orthogonal vs. folded into pinout namespace).
