# Phase 11 — Plan 03 Summary: Adc Multiplexer Migration

## What Shipped

- `src/Libs/Adc.hpp` — added `#include "../variant_config.hpp"`, declared new `void InitMuxes(const MultiplexerConfig*, uint8_t)` method, added private `_total_channels` member. Existing `Init(AdcChannelConfig*, uint8_t)` kept intact.
- `src/Libs/Adc.cpp` — implemented `Adc::InitMuxes` to drive select pins from first mux, configure common pin, and allocate `keyMapping.size()` × mux_count channels.
- `src/AppEngine.cpp` — replaced `AdcChannelConfig adc_config; adc_config.InitMux(...); adc_.Init(&adc_config, 16);` block with `adc_.InitMuxes(variant::CurrentVariant::kMuxes, sizeof(...)/sizeof(...))`. Added `#include "variant.hpp"`.

## Verification

- `pio run -e t16_release` → SUCCESS (27s).
- `pio run -e t32_release` → SUCCESS (27s).
- T16: same 16 channels, same select/common pins, single mux scan path. T32: builds clean with 32 channels allocated, only first mux scanned (Phase 12 to extend ReadValues for second mux).

## Requirements Touched

- HAL-02 (Adc slice).
- HAL-03 (`MultiplexerConfig` consumed by `Adc::InitMuxes`).
