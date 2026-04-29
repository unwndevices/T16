#pragma once

// =============================================================================
// PHASE 11 PLACEHOLDER (matches pinout_t32.h placeholder posture)
// Values let -DT32 builds compile cleanly. Phase 12 (T32 Hardware Bring-Up)
// will validate the dual-mux key permutation and finalize LED layout.
// DO NOT flash a -DT32 binary onto T32 hardware until Phase 12 lands.
// =============================================================================

#include "variant_config.hpp"
#include "pinout_t32.h"

namespace variant::t32 {

inline constexpr HardwareVariantConfig kConfig{
    /* TOTAL_KEYS          */ 32,
    /* MUX_COUNT           */ 2,
    /* LED_COUNT           */ 33,   // 32 matrix + 0 slider + 1 state pixel (Phase 12 confirms layout)
    /* MATRIX_WIDTH        */ 8,
    /* MATRIX_HEIGHT       */ 4,
    /* SLIDER_LENGTH       */ 0,
    /* BANK_AMT            */ 4,
    /* HAS_TOUCH_SLIDER    */ false,
    /* SUPPORTS_KOALA_MODE */ false,
    /* NAME                */ "T32",
};

inline constexpr MultiplexerConfig kMuxes[] = {
    {
        /* commonPin       */ pinout::t32::COM,
        /* enablePin       */ -1,
        /* selectPins      */ { pinout::t32::S0, pinout::t32::S1,
                                pinout::t32::S2, pinout::t32::S3 },
        /* keyMapping      */ { 0, 1, 2, 3, 4, 5, 6, 7,
                                8, 9, 10, 11, 12, 13, 14, 15 },
        /* useSharedSelect */ false,
    },
    {
        // Phase 12 placeholder — second mux mirrors first; real T32 has
        // a separate commonPin and shared select pins.
        /* commonPin       */ pinout::t32::COM,
        /* enablePin       */ -1,
        /* selectPins      */ { pinout::t32::S0, pinout::t32::S1,
                                pinout::t32::S2, pinout::t32::S3 },
        /* keyMapping      */ { 16, 17, 18, 19, 20, 21, 22, 23,
                                24, 25, 26, 27, 28, 29, 30, 31 },
        /* useSharedSelect */ true,
    },
};

}  // namespace variant::t32
