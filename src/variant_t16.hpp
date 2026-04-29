#pragma once

#include "variant_config.hpp"
#include "pinout_t16.h"

namespace variant::t16 {

inline constexpr HardwareVariantConfig kConfig{
    /* TOTAL_KEYS          */ 16,
    /* MUX_COUNT           */ 1,
    /* LED_COUNT           */ 24,   // 4*4 matrix + 7 slider + 1 state pixel
    /* MATRIX_WIDTH        */ 4,
    /* MATRIX_HEIGHT       */ 4,
    /* SLIDER_LENGTH       */ 7,
    /* BANK_AMT            */ 4,
    /* HAS_TOUCH_SLIDER    */ true,
    /* SUPPORTS_KOALA_MODE */ true,
    /* NAME                */ "T16",
};

inline constexpr MultiplexerConfig kMuxes[] = {
    {
        /* commonPin       */ pinout::t16::COM,
        /* enablePin       */ -1,
        /* selectPins      */ { pinout::t16::S0, pinout::t16::S1,
                                pinout::t16::S2, pinout::t16::S3 },
        /* keyMapping      */ { 0, 1, 2, 3, 4, 5, 6, 7,
                                8, 9, 10, 11, 12, 13, 14, 15 },
        /* useSharedSelect */ false,
    },
};

}  // namespace variant::t16
