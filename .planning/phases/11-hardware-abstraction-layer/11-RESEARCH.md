# Phase 11 — Hardware Abstraction Layer: Research

**Phase:** 11 — Hardware Abstraction Layer
**Phase goal:** Firmware classes consume `HardwareVariantConfig` constexpr constants instead of fixed macros, with no behavior change on T16.
**Requirements:** HAL-01, HAL-02, HAL-03, HAL-04
**Status:** Research complete — ready for planning

---

## 1. Existing State (codebase audit)

### 1.1 Pinout layer (Phase 10 — done)

Two per-variant headers + a selector header alias the active namespace as `CurrentPinout`:

- `src/pinout_t16.h` — `namespace pinout::t16 { ... }` with all GPIO/serial/button constants
- `src/pinout_t32.h` — `namespace pinout::t32 { ... }` (placeholder values mirroring T16; Phase 12 fills in real values)
- `src/pinout.h` — selects via `#if defined(T32)` / `#elif defined(T16)`, errors otherwise; aliases via `namespace CurrentPinout = pinout::<variant>`

**Phase 11 should mirror this exact structure** for `HardwareVariantConfig` (locked in CONTEXT D11.1).

### 1.2 Macros / literals that vary between T16 and T32

Survey of every macro / literal that needs to be replaced:

| Symbol | Current value (T16) | Defined in | Used in |
|---|---|---|---|
| `kMatrixWidth` | `4` | `Libs/Leds/LedManager.hpp:9` (`#define`) | `LedManager.cpp`, `patterns/Sea.hpp`, `patterns/Sea2.hpp`, `patterns/WaveTransition.hpp` |
| `kMatrixHeight` | `4` | `Libs/Leds/LedManager.hpp:10` (`#define`) | same as above |
| `sliderLength` | `7` | `Libs/Leds/LedManager.hpp:11` (`#define`) | `LedManager.cpp` |
| `NUM_LEDS` | `kMatrixWidth*kMatrixHeight + sliderLength + 1 = 24` | `Libs/Leds/LedManager.hpp:12` (`#define`) | `LedManager.cpp` (FastLED registration, `leds_plus_safety_pixel`, fills) |
| `patternleds[16]` | hardcoded `16` | `Libs/Leds/LedManager.cpp:22,35`, `patterns/Strum.hpp:37`, `patterns/TouchBlur.hpp:68` | LED matrix-keys array |
| `matrixleds(leds_set(1, 16))` | hardcoded `16` | `Libs/Leds/LedManager.cpp:21,34` | matrix CRGBSet view |
| Adc internal `for (uint8_t i = 0; i < 16; i++)` (channels) | hardcoded `16` | `Libs/Adc.cpp:64` | ADC channel allocation |
| `_mux_pin[4]` | hardcoded `4` | `Libs/Adc.hpp:105`, `Libs/Adc.cpp:60` | Adc mux select pins |

**Note on `BANK_AMT`:** `BANK_AMT = 4` lives in `Configuration.hpp:9` and is the **memory bank count** (4 user-configurable mappings), NOT key count. It does **not** vary between T16 and T32 — both have 4 banks. CONTEXT.md D11.2 names `BANK_AMT` as a legacy macro to remove, but the success criterion in ROADMAP.md is the authoritative list (HAL-02 lists `BANK_AMT` as an example, but the only macros actually variant-dependent are the LED/key counts above). Plans must handle this carefully — see §4 Open Question A.

### 1.3 Where `BANK_AMT` is referenced

- `Configuration.hpp:9` — definition (`const uint8_t BANK_AMT = 4`)
- `Configuration.hpp:104,105` — `kb_cfg[BANK_AMT]`, `cc_cfg[BANK_AMT]` array decls
- `Configuration.cpp:5,6,23,103` — array defs and loops
- `ConfigManager.hpp:40,41` — `banks_[BANK_AMT]`, `cc_[BANK_AMT]` member arrays
- `SysExHandler.cpp:170,179` — bounds checks on bank index
- `services/serial_commands/DiagnosticCommands.cpp:38` — diag iteration

### 1.4 Multiplexer init in `Adc`

- `AdcChannelConfig::InitMux(pin, mux_pin_0, mux_pin_1, mux_pin_2, mux_pin_3)` is the current entry point — single-mux only, four select pins as positional args.
- `Adc::Init` allocates exactly 16 channels when `_mux_pin[0] != 0` (single mux assumed).
- `SetMuxChannel(chn)` writes the 4-bit channel address to `_mux_pin[0..3]` — single mux.

For T32 this needs to scale to two mux instances sharing the four select pins but with separate common pins. CONTEXT D11.3 / HAL-03 require a `MultiplexerConfig` struct describing each mux.

### 1.5 SysEx handshake (current state)

`SysExHandler::HandleVersionRequest` (lines 97–108) sends a fixed **5-byte raw response**:
```
{ MANUFACTURER_ID, CMD_VERSION, SUB_RESPONSE, PROTOCOL_VERSION, firmwareVersion_ }
```
**Not JSON.** CONTEXT D11.4 specifies a JSON capabilities payload. The cleanest move is to add a new SysEx command (`CMD_CAPABILITIES`) that emits a small JSON document `{ "variant": "...", "capabilities": { ... } }`, leaving the existing 5-byte version handshake untouched. This is forward-compatible — older editor-tx versions never send the new command.

`SysExProtocol.hpp` defines the command enum — needs one new value.

---

## 2. `HardwareVariantConfig` field set (HAL-01)

Conservative field set, only what consumers actually read:

```cpp
// src/variant_config.hpp (shared header)
#pragma once
#include <cstdint>

struct HardwareVariantConfig {
    uint8_t  TOTAL_KEYS;          // 16 (T16) / 32 (T32)
    uint8_t  MUX_COUNT;           // 1  (T16) / 2  (T32)
    uint8_t  LED_COUNT;           // 24 (T16, = 16 matrix + 7 slider + 1 state) / 40 (T32)
    uint8_t  MATRIX_WIDTH;        // 4 / 8 — replaces kMatrixWidth
    uint8_t  MATRIX_HEIGHT;       // 4 / 4 — replaces kMatrixHeight
    uint8_t  SLIDER_LENGTH;       // 7 / 0
    bool     HAS_TOUCH_SLIDER;    // true / false
    bool     SUPPORTS_KOALA_MODE; // true / false
    const char* NAME;             // "T16" / "T32" — used in capabilities JSON
};
```

**LED_COUNT for T32:** Phase 12 will validate the matrix layout. For Phase 11 the T32 variant uses a placeholder of `32 + 0 + 1 = 33` (no slider, +1 for the state LED safety pixel). Final layout pinned in Phase 12.

**Why these fields and not more:** every other "variant-looking" symbol (e.g. `BANK_AMT`, `_windowSize`, debounce times) is either invariant across variants or owned by a different concern (config schema, calibration). Adding fields speculatively contradicts D11.2 ("eliminate the 'two ways to ask' confusion").

---

## 3. `MultiplexerConfig` shape (HAL-03)

Per CONTEXT D11.3 — `inline constexpr MultiplexerConfig kMuxes[]` lives in each variant header.

```cpp
// src/variant_config.hpp
#include <array>

struct MultiplexerConfig {
    int8_t  commonPin;                  // ADC input GPIO (one per mux)
    int8_t  enablePin;                  // -1 if always enabled (T16)
    std::array<int8_t, 4> selectPins;   // S0..S3 — shared across muxes for T32
    std::array<uint8_t, 16> keyMapping; // logical key index for each of the 16 channels
    bool    useSharedSelect;            // true → don't re-set S0..S3 for this mux
};
```

**Decision on `keyMapping` size:** **fixed `std::array<uint8_t, 16>`** — every CD4067-class mux is 16-channel by definition; this is not a future-proofing axis. (Open Question B in CONTEXT — closed: fixed 16.) Smaller mux variants (8-channel) are not on the roadmap; if they ever appear, the array can grow with a templated wrapper without forcing today's code into a templated mess.

T16 array: one entry, identity mapping `{0,1,...,15}`.
T32 array: two entries, identity mapping for first mux, second mux uses identity here too (the actual T32 permutation lands in Phase 12 per CONTEXT note).

---

## 4. Open questions resolved

**A. `BANK_AMT` removal scope**
ROADMAP success criterion #2 says "no longer reference `BANK_AMT` or `NUM_LEDS` macros". CONTEXT D11.2 says delete the macro outright. But `BANK_AMT` is not actually variant-dependent — both T16 and T32 have 4 banks. **Resolution:** Move `BANK_AMT` from `#define`/`const` in `Configuration.hpp` into `HardwareVariantConfig` as `BANK_AMT` field with the same value `4` for both variants. This satisfies the literal text of HAL-02 (consumers stop reading the global macro) without inventing a phantom variant axis. Plan-04 and Plan-05 cover this migration.

**B. `keyMapping` sizing** — closed (fixed `std::array<uint8_t, 16>`, see §3).

**C. Capability emission mechanism** — closed: new SysEx command `CMD_CAPABILITIES` (sub `SUB_REQUEST` → JSON response). Existing 5-byte version handshake unchanged.

**D. `CurrentVariant` location** — closed per CONTEXT D11.1: alias inside `namespace variant` (`namespace variant { using CurrentVariant = t16; }`).

---

## 5. Migration mechanics (avoid behavior change on T16)

For every `#define`/`const` removal, the replacement constexpr **must produce the same value**:

- `kMatrixWidth` (4) → `variant::CurrentVariant::kConfig.MATRIX_WIDTH` (4 on T16)
- `NUM_LEDS` (24) → `variant::CurrentVariant::kConfig.LED_COUNT` (24 on T16)
- `BANK_AMT` (4) → `variant::CurrentVariant::kConfig.BANK_AMT` (4 on T16)
- `Adc::Init` literal `16` (channel allocation) → `variant::CurrentVariant::kConfig.TOTAL_KEYS / MUX_COUNT` (16 on T16)

**Type compatibility:** All count fields are `uint8_t`. Existing `for (uint8_t i = 0; i < 16; i++)` loops compile unchanged when `16` is replaced with a `uint8_t` constexpr.

**Array sizing:** `extern CRGB leds_plus_safety_pixel[NUM_LEDS + 1];` must become `extern CRGB leds_plus_safety_pixel[variant::CurrentVariant::kConfig.LED_COUNT + 1];`. C++17 allows constexpr in array bounds, and namespace-qualified constexpr access works in array bounds since C++11.

**Pattern files:** `patterns/Strum.hpp` and `patterns/TouchBlur.hpp` use `fill_solid(patternleds, 16, ...)`. Replace with `MATRIX_WIDTH * MATRIX_HEIGHT`. The `patternleds[16]` definition itself becomes `patternleds[MATRIX_WIDTH*MATRIX_HEIGHT]` — but per LedManager `patternleds` is a fixed buffer for matrix keys only, so size = `MATRIX_WIDTH*MATRIX_HEIGHT`.

---

## 6. T16 regression check (success criterion #4)

Per the orchestrator note and CONTEXT, **hardware verification is deferred to end-of-milestone batch testing**. Phase 11 plans rely on:

- Build success on both `t16_release` and `t32_release` envs (PIO produces firmware.bin/firmware.elf, links cleanly).
- Grep audit: no occurrence of removed macros (`BANK_AMT`, `NUM_LEDS`, `kMatrixWidth`, `kMatrixHeight`, `sliderLength`) in `src/` outside the variant headers.
- One **`checkpoint:human-verify`** task at the very end of the phase: a manual T16 hardware regression (calibration + MIDI capture diff) flagged for human execution. This task is `autonomous: false` and produces no automated verdict — it is the Phase 11 → Phase 12 gate.

---

## 7. Files to be created

- `src/variant_config.hpp` — `HardwareVariantConfig` + `MultiplexerConfig` struct definitions (shared)
- `src/variant_t16.hpp` — `namespace variant::t16 { inline constexpr HardwareVariantConfig kConfig{...}; inline constexpr MultiplexerConfig kMuxes[] = {...}; }`
- `src/variant_t32.hpp` — same shape, T32 values (placeholder for Phase 12)
- `src/variant.hpp` — `#if defined(T32)` selector, aliases `namespace variant { using CurrentVariant = t32; }` (or t16)

## 8. Files to be modified

- `src/Libs/Leds/LedManager.hpp` — remove `#define kMatrixWidth/kMatrixHeight/sliderLength/NUM_LEDS`, switch arrays to constexpr-sized
- `src/Libs/Leds/LedManager.cpp` — replace LED count literals
- `src/Libs/Leds/patterns/{NoBlur,Sea,Sea2,Strum,TouchBlur,WaveTransition}.hpp` — replace `kMatrixWidth/Height` and the literal `16` in `fill_solid(patternleds, 16, ...)`
- `src/Libs/Adc.hpp/.cpp` — accept `MultiplexerConfig`; replace literal `16` in channel allocation
- `src/Libs/Keyboard.cpp` (if uses `BANK_AMT` indirectly via key arrays) — verify, no change expected
- `src/Configuration.hpp/.cpp` — remove `const uint8_t BANK_AMT`, replace consumers with `variant::CurrentVariant::kConfig.BANK_AMT`
- `src/ConfigManager.hpp` — `banks_[BANK_AMT]` → `banks_[variant::CurrentVariant::kConfig.BANK_AMT]`
- `src/SysExHandler.hpp/.cpp` — add `HandleCapabilitiesRequest()` emitting JSON
- `src/SysExProtocol.hpp` — add `CMD_CAPABILITIES`
- `src/services/serial_commands/DiagnosticCommands.cpp` — replace `BANK_AMT` reference
- `src/main.cpp` — replace any literal `16` used as key count; pass `MultiplexerConfig` to `Adc::Init` (T16 = single entry of `kMuxes`)

## 9. Validation Architecture

Per CONTEXT and the orchestrator note, **acceptance criteria use grep and build checks, not hardware**, except for the final regression checkpoint.

- **Build verification:** `pio run -e t16_release` and `pio run -e t32_release` both produce `firmware.bin` and exit 0.
- **Grep guard:** `! grep -rn "BANK_AMT\b\|NUM_LEDS\b\|kMatrixWidth\b\|kMatrixHeight\b\|sliderLength\b" src/ --include='*.cpp' --include='*.hpp' --include='*.h' | grep -v "^src/variant_"` returns no matches.
- **Capability JSON spot check:** firmware string `"variant"` and `"capabilities"` appear in the compiled binary (verifiable via `grep` of `.elf` strings or by reading source).
- **Hardware regression:** single `checkpoint:human-verify` task at end of phase (T16 calibration + MIDI smoke test). Marked `autonomous: false`. Does not block automated plan completion but is the gate to Phase 12.

---

## RESEARCH COMPLETE
