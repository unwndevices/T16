#pragma once

// Permutation source: origin/3dot0:src/main.cpp:10 (T32 keys[] array).
// Inversion logic and compile-time validation: see static_assert block below.

#include "variant_config.hpp"
#include "pinout_t32.h"

namespace variant::t32 {

inline constexpr HardwareVariantConfig kConfig{
    /* TOTAL_KEYS          */ 32,
    /* MUX_COUNT           */ 2,
    /* LED_COUNT           */ 40,   // 1 state + 7 slider + 32 key LEDs; confirmed on hardware 2026-05-01
    /* MATRIX_WIDTH        */ 8,
    /* MATRIX_HEIGHT       */ 4,
    /* SLIDER_LENGTH       */ 7,
    /* BANK_AMT            */ 4,
    /* HAS_TOUCH_SLIDER    */ true,
    /* SUPPORTS_KOALA_MODE */ false,
    /* NAME                */ "T32",
};

inline constexpr MultiplexerConfig kMuxes[] = {
    {
        /* commonPin       */ pinout::t32::COM,
        /* enablePin       */ -1,
        /* selectPins      */ { pinout::t32::S0, pinout::t32::S1,
                                pinout::t32::S2, pinout::t32::S3 },
        /* keyMapping      */ { 17, 16, 19, 18,  25, 24, 27, 26,
                                10, 11,  8,  9,   3,  2,  0,  1 },
        /* useSharedSelect */ false,
    },
    {
        // Mux 1 — separate commonPin (COM2), shared S0..S3 (useSharedSelect=true).
        /* commonPin       */ pinout::t32::COM2,
        /* enablePin       */ -1,
        /* selectPins      */ { pinout::t32::S0, pinout::t32::S1,
                                pinout::t32::S2, pinout::t32::S3 },
        /* keyMapping      */ { 21, 20, 23, 22,  29, 28, 31, 30,
                                14, 15, 12, 13,   7,  6,  4,  5 },
        /* useSharedSelect */ true,
    },
};

namespace detail {
    // Verbatim from origin/3dot0:src/main.cpp:10 — DO NOT EDIT.
    inline constexpr uint8_t k3dot0Keys[32] = {
        14, 15, 13, 12, 30, 31, 29, 28,
        10, 11,  8,  9, 26, 27, 24, 25,
         1,  0,  3,  2, 17, 16, 19, 18,
         5,  4,  7,  6, 21, 20, 23, 22,
    };

    // Compile-time inversion: for each channel c, find pos such that
    // k3dot0Keys[pos] == c. Mux 0 owns c in [0,16); Mux 1 owns c in [16,32).
    constexpr uint8_t InvertedKeyMapping(uint8_t channel) {
        for (uint8_t pos = 0; pos < 32; ++pos) {
            if (k3dot0Keys[pos] == channel) return pos;
        }
        return 0xFF;
    }

    // Verify Mux 0 keyMapping matches the inversion.
    static_assert([] {
        for (uint8_t ch = 0; ch < 16; ++ch) {
            if (kMuxes[0].keyMapping[ch] != InvertedKeyMapping(ch)) return false;
        }
        return true;
    }(), "T32 mux 0 keyMapping does not match origin/3dot0 inversion");

    // Verify Mux 1 keyMapping matches the inversion (channels 16..31).
    static_assert([] {
        for (uint8_t ch = 0; ch < 16; ++ch) {
            if (kMuxes[1].keyMapping[ch] != InvertedKeyMapping(ch + 16)) return false;
        }
        return true;
    }(), "T32 mux 1 keyMapping does not match origin/3dot0 inversion");
} // namespace detail

}  // namespace variant::t32
