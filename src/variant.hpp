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

namespace variant {
  // Per-variant calibration filename. Format: "/calibration_<lowercase_name>.json".
  // Defined as a function (not a string literal) so we can derive it from
  // CurrentVariant::kConfig.NAME at compile time without runtime allocation.
  // Phase 12.01 (D12.2): T16 -> /calibration_t16.json, T32 -> /calibration_t32.json.
  inline const char* CalibrationFilePath() {
      if constexpr (variant::CurrentVariant::kConfig.NAME[0] == 'T' &&
                    variant::CurrentVariant::kConfig.NAME[1] == '3') {
          return "/calibration_t32.json";
      } else {
          return "/calibration_t16.json";
      }
  }
  inline constexpr const char* kLegacyCalibrationPath = "/calibration_data.json";
} // namespace variant
