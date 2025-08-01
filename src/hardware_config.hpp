#ifndef HARDWARE_CONFIG_HPP
#define HARDWARE_CONFIG_HPP

#include <cstdint>

enum class HardwareVariant {
    T16 = 16,
    T32 = 32,
    T64 = 64
};

template<HardwareVariant variant>
struct HardwareConfig {
    static constexpr uint8_t NUM_KEYS = static_cast<uint8_t>(variant);
    static constexpr uint8_t MATRIX_ROWS = (variant == HardwareVariant::T16) ? 4 : 
                                          (variant == HardwareVariant::T32) ? 4 : 8;
    static constexpr uint8_t MATRIX_COLS = (variant == HardwareVariant::T16) ? 4 : 
                                          (variant == HardwareVariant::T32) ? 8 : 8;
    static constexpr uint8_t NUM_LEDS = NUM_KEYS;
    static constexpr uint8_t NUM_MULTIPLEXERS = (variant == HardwareVariant::T16) ? 1 : 
                                               (variant == HardwareVariant::T32) ? 2 : 4;
    static constexpr uint8_t MUX_CHANNELS = 8;
    static constexpr uint8_t TOUCH_SEGMENTS = 7;
};

template<HardwareVariant variant>
struct MultiplexerConfig {
    static constexpr uint8_t NUM_SELECT_PINS = 3;
    static constexpr bool SHARED_SELECT_PINS = (variant != HardwareVariant::T16);
    static constexpr uint8_t CHANNELS_PER_MUX = 8;
    static constexpr uint8_t TOTAL_CHANNELS = HardwareConfig<variant>::NUM_MULTIPLEXERS * CHANNELS_PER_MUX;
};

template<HardwareVariant variant>
struct HardwarePinout;

#if defined(T16_VARIANT)
    using CurrentHardwareConfig = HardwareConfig<HardwareVariant::T16>;
    using CurrentMuxConfig = MultiplexerConfig<HardwareVariant::T16>;
    constexpr HardwareVariant CURRENT_VARIANT = HardwareVariant::T16;
#elif defined(T32_VARIANT)
    using CurrentHardwareConfig = HardwareConfig<HardwareVariant::T32>;
    using CurrentMuxConfig = MultiplexerConfig<HardwareVariant::T32>;
    constexpr HardwareVariant CURRENT_VARIANT = HardwareVariant::T32;
#elif defined(T64_VARIANT)
    using CurrentHardwareConfig = HardwareConfig<HardwareVariant::T64>;
    using CurrentMuxConfig = MultiplexerConfig<HardwareVariant::T64>;
    constexpr HardwareVariant CURRENT_VARIANT = HardwareVariant::T64;
#else
    using CurrentHardwareConfig = HardwareConfig<HardwareVariant::T16>;
    using CurrentMuxConfig = MultiplexerConfig<HardwareVariant::T16>;
    constexpr HardwareVariant CURRENT_VARIANT = HardwareVariant::T16;
#endif

#endif // HARDWARE_CONFIG_HPP