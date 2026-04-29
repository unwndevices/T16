#pragma once

#include <array>
#include <cstdint>

// Hardware variant capability + sizing config. One inline constexpr instance
// per variant lives in variant_t16.hpp / variant_t32.hpp. variant.hpp selects
// the active one via -DT16 / -DT32 build flags (same pattern as pinout.h).
struct HardwareVariantConfig {
    uint8_t     TOTAL_KEYS;          // 16 (T16) / 32 (T32)
    uint8_t     MUX_COUNT;           // 1  (T16) / 2  (T32)
    uint8_t     LED_COUNT;           // matrix + slider + state pixel
    uint8_t     MATRIX_WIDTH;        // replaces former kMatrixWidth
    uint8_t     MATRIX_HEIGHT;       // replaces former kMatrixHeight
    uint8_t     SLIDER_LENGTH;       // 7 (T16) / 0 (T32)
    uint8_t     BANK_AMT;            // 4 — invariant today, modeled here so
                                     // Configuration / ConfigManager stop
                                     // depending on a global #define
    bool        HAS_TOUCH_SLIDER;
    bool        SUPPORTS_KOALA_MODE;
    const char* NAME;                // "T16" / "T32" — emitted in capabilities JSON
};

// Per-multiplexer description. Each variant exposes an inline constexpr
// MultiplexerConfig kMuxes[MUX_COUNT] in its variant header.
struct MultiplexerConfig {
    int8_t                  commonPin;      // ADC GPIO, one per mux
    int8_t                  enablePin;      // -1 if always enabled (T16)
    std::array<int8_t, 4>   selectPins;     // S0..S3 (shared across muxes on T32)
    std::array<uint8_t, 16> keyMapping;     // logical key index for each of 16 channels
    bool                    useSharedSelect;// true → don't re-drive S0..S3 for this mux
};
