# Phase 11 — Hardware Abstraction Layer: Context

**Phase goal:** Firmware classes consume `HardwareVariantConfig` constexpr constants instead of fixed macros, with no behavior change on T16.
**Requirements:** HAL-01, HAL-02, HAL-03, HAL-04
**Depends on:** Phase 10 decisions (D10.2 bare `-DT16`/`-DT32`, D10.3 namespaced `pinout::t16`/`pinout::t32`)

---

## Locked Decisions

### D11.1 — Per-variant config files mirror the pinout layout
- `variant_t16.hpp` and `variant_t32.hpp` each define a complete `inline constexpr HardwareVariantConfig kConfig{...}` inside `namespace variant::t16` / `namespace variant::t32`.
- `variant.hpp` aliases the active one via the same `#if defined(T32)` switch used in `pinout.h`:
  ```cpp
  #if defined(T32)
    #include "variant_t32.hpp"
    namespace variant { using CurrentVariant = t32; }
  #else
    #include "variant_t16.hpp"
    namespace variant { using CurrentVariant = t16; }
  #endif
  ```
- Firmware code references `variant::CurrentVariant::kConfig.TOTAL_KEYS`, etc. (or a shorter `using` alias inside each consumer).
- Symmetric with Phase 10 pinout structure — same mental model.

### D11.2 — Legacy macros deleted outright in Phase 11
- `BANK_AMT`, `NUM_LEDS`, and any other variant-related `#define`s are removed in this phase.
- All consumers (`Adc`, `Keyboard`, `LedManager`, `DataManager`, `main.cpp`) are migrated to read `CurrentVariant::kConfig.*` in the same phase.
- No alias-then-remove transition state. Bigger diff but eliminates the "two ways to ask" confusion.
- Success criterion #4 (T16 regression on real hardware) is the gate.

### D11.3 — `MultiplexerConfig` stored as `inline constexpr` in the variant header
- C++17 `inline constexpr MultiplexerConfig kMuxes[] = {...}` lives directly in `variant_t16.hpp` / `variant_t32.hpp`.
- One canonical definition across all translation units; no ODR risk; no separate `.cpp`.
- Explicitly avoids the `static constexpr auto MUX_CONFIGS` pattern called out as out-of-scope in REQUIREMENTS.md.
- `Adc::InitMux` consumes the array via `variant::CurrentVariant::kMuxes`.

### D11.4 — Capability flags travel via the SysEx handshake response
- Firmware handshake response gains a `capabilities` object alongside `variant`:
  ```json
  { "variant": "T32",
    "capabilities": { "touchSlider": false, "koalaMode": false } }
  ```
- Source of truth: `HardwareVariantConfig::HAS_TOUCH_SLIDER`, `SUPPORTS_KOALA_MODE` constexpr fields.
- Editor-tx reads capabilities at connect time and hides unsupported features without needing its own per-variant table.
- Bumps the SysEx handshake schema (coordinate with Phase 13 schema version bump and Phase 14 editor work).

---

## Implementation Notes for Researcher / Planner

- `HardwareVariantConfig` struct definition lives in a shared header (e.g., `variant_config.hpp`) that both `variant_t16.hpp` and `variant_t32.hpp` include. Fields: `TOTAL_KEYS`, `MUX_COUNT`, `LED_COUNT`, `HAS_TOUCH_SLIDER`, `SUPPORTS_KOALA_MODE`. Add fields conservatively — only what consumers actually read.
- `MultiplexerConfig` struct: `int8_t commonPin; int8_t enablePin; std::array<int8_t, 4> selectPins; std::array<uint8_t, N> keyMapping; bool useSharedSelect;`. Decide whether `keyMapping` is sized 16 (per-mux) or `TOTAL_KEYS / MUX_COUNT`.
- T16 must still ship after Phase 11 — every `Adc`/`Keyboard`/`LedManager`/`DataManager` change must compile and run on real T16 hardware before merge. The T32 build only needs to compile; T32 hardware bring-up is Phase 12.
- The SysEx handshake change in D11.4 is a forward-compatible add — older editor-tx versions ignoring unknown JSON fields keep working. Document the new field in the SysEx protocol reference.
- Audit `main.cpp` for hardcoded `16`/`NUM_LEDS`/array sizes; many will need replacement with `CurrentVariant::kConfig.TOTAL_KEYS` / `LED_COUNT`.

## Out of Scope for Phase 11

- T32 hardware bring-up (dual-mux scan, key permutation, real-device calibration) → Phase 12
- Config schema variant discriminator → Phase 13
- Editor-tx consuming the new `capabilities` object → Phase 14
- Removal of `REV_A`/`REV_B` flags (orthogonal axis; revisit only if it blocks variant work)

## Open Items for Researcher

1. Final `HardwareVariantConfig` field list — survey every macro/literal that varies between T16 and T32 today; only those become fields.
2. Whether `keyMapping` in `MultiplexerConfig` is fixed-size `std::array<uint8_t, 16>` or templated. Smaller variants of the future could differ.
3. SysEx handshake schema documentation — locate the current handshake spec and propose the diff for `capabilities`.
4. Whether `CurrentVariant` belongs in a global `using` declaration or stays inside `namespace variant`.
