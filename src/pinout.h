#ifndef PINOUT_H
#define PINOUT_H

#include "hardware_config.hpp"

#if defined(T16_VARIANT)
    #include "pinout_t16.h"
    using CurrentPinout = HardwarePinout<HardwareVariant::T16>;
#elif defined(T32_VARIANT)
    #include "pinout_t32.h"
    using CurrentPinout = HardwarePinout<HardwareVariant::T32>;
#elif defined(T64_VARIANT)
    #include "pinout_t64.h"
    using CurrentPinout = HardwarePinout<HardwareVariant::T64>;
#else
    #include "pinout_t16.h"
    using CurrentPinout = HardwarePinout<HardwareVariant::T16>;
#endif

#ifdef REV_A

#define PIN_S0 5
#define PIN_S1 4
#define PIN_S2 7
#define PIN_S3 6

#define PIN_COM 8

#define PIN_LED 1

#define PIN_T1 3
#define PIN_T2 9
#define PIN_T3 10
#define PIN_T4 11
#define PIN_T5 12
#define PIN_T6 13
#define PIN_T7 14

#define PIN_TX 43
#define PIN_RX 44
#define PIN_TX2 42

#define PIN_TOUCH 2
#define PIN_EXT3 41
#define PIN_EXT4 40
#define PIN_MODE 0

#endif // REV_A

#ifdef REV_B

#define PIN_S0 4
#define PIN_S1 5
#define PIN_S2 6
#define PIN_S3 7

#define PIN_COM 8

#define PIN_LED 1

#define PIN_T1 3
#define PIN_T2 9
#define PIN_T3 10
#define PIN_T4 11
#define PIN_T5 12
#define PIN_T6 13
#define PIN_T7 14

#define PIN_TX 43
#define PIN_RX 44
#define PIN_TX2 42

#define PIN_TOUCH 21
#define PIN_EXT3 41
#define PIN_EXT4 40
#define PIN_MODE 0

#endif // REV_B

#endif // PINOUT_H
