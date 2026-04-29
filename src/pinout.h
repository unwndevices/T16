#pragma once

// Variant selector. Build flags must define exactly one of -DT16 / -DT32
// (set in platformio.ini per env). The matching pinout_<variant>.h declares
// `namespace pinout::<variant>` with all GPIO / serial / button constants;
// this header aliases the active variant's namespace as `CurrentPinout`
// so consumers can write `CurrentPinout::S0` regardless of variant.

#if defined(T32)
  #include "pinout_t32.h"
  namespace CurrentPinout = pinout::t32;
#elif defined(T16)
  #include "pinout_t16.h"
  namespace CurrentPinout = pinout::t16;
#else
  #error "No variant defined: build must set -DT16 or -DT32 (see platformio.ini envs)"
#endif
