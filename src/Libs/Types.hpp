#ifndef TYPES_HPP
#define TYPES_HPP
#include <stdint.h>
#include <math.h>
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

#endif // !TYPES_HPP
