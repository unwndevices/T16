#pragma once

// Variant selector. Build flags must define exactly one of -DT16 / -DT32
// (set in platformio.ini per env). Mirrors pinout.h selection: the matching
// variant_<variant>.hpp declares `namespace variant::<variant>` with the
// active HardwareVariantConfig and MultiplexerConfig kMuxes; this header
// aliases it as `variant::CurrentVariant` so consumers can write
// `variant::CurrentVariant::kConfig.TOTAL_KEYS` regardless of variant.

#if defined(T32)
  #include "variant_t32.hpp"
  namespace variant { namespace CurrentVariant = t32; }
#elif defined(T16)
  #include "variant_t16.hpp"
  namespace variant { namespace CurrentVariant = t16; }
#else
  #error "No variant defined: build must set -DT16 or -DT32 (see platformio.ini envs)"
#endif
