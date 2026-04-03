#pragma once

#include <stdint.h>
#include <math.h>

namespace t16
{

enum Mode
{
    KEYBOARD,
    STRUM,
    XY_PAD,
    STRIPS,
    QUICK_SETTINGS,
    MODE_AMOUNT
};

enum SliderMode
{
    BEND,
    OCTAVE,
    MOD,
    BANK,
    SLEW,
    STRUMMING,
    QUICK,
    SLIDER_MODE_AMOUNT
};

} // namespace t16

// Bring enums into global scope for backward compatibility
using t16::Mode;
using t16::SliderMode;

// Re-export enum values into global scope so existing unscoped usage
// (e.g. `KEYBOARD`, `Mode::KEYBOARD`) continues to work.
using t16::KEYBOARD;
using t16::STRUM;
using t16::XY_PAD;
using t16::STRIPS;
using t16::QUICK_SETTINGS;
using t16::MODE_AMOUNT;

using t16::BEND;
using t16::OCTAVE;
using t16::MOD;
using t16::BANK;
using t16::SLEW;
using t16::STRUMMING;
using t16::QUICK;
using t16::SLIDER_MODE_AMOUNT;

typedef struct Vector2
{
    int8_t x;
    int8_t y;

    Vector2(int8_t x = 0, int8_t y = 0) : x(x), y(y) {}

    static inline Vector2 Lerp(const Vector2 &v1, const Vector2 &v2, float t)
    {
        return Vector2((int8_t)(v1.x + (int8_t)(ceil((v2.x - v1.x) * t))), (int8_t)(v1.y + (int8_t)(ceil((v2.y - v1.y) * t))));
    }

    static inline Vector2 Lerp(const Vector2 &v1, const Vector2 &v2, uint8_t t)
    {
        // Linear interpolation in integer space.
        return Vector2((v1.x * (254 - t) + v2.x * t) / 254,
                       (v1.y * (254 - t) + v2.y * t) / 254);
    }
    Vector2 operator+(const Vector2 &other) const
    {
        return Vector2(x + other.x, y + other.y);
    }
    Vector2 &operator+=(const Vector2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vector2 &operator-=(const Vector2 &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2 operator-(const Vector2 &other) const
    {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(const int8_t &scalar) const
    {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 operator/(const int8_t &scalar) const
    {
        return Vector2(x / scalar, y / scalar);
    }
} Vector2;
