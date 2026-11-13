#pragma once

#ifndef CDC_MATH_H
#define CDC_MATH_H

#include <cstdint>
#include <cmath>

namespace cdc {

struct alignas(16) Vector {
    union {
        struct { float x, y, z, w; };
        float m[4];
    };

    // 0x0045f5f0 (PDB: 0x00460da0 / ?Len4@Vector@cdc@@QBEMXZ)
    inline float Len4() const
    {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }
};

struct alignas(16) Vector3 : public Vector {};
struct alignas(16) Euler : public Vector {};
struct alignas(16) Quat : public Vector {};

// 0x004880e0 (PDB: 0x00489b20 / ?Dist2@cdc@@YAMABVVector@1@0@Z)
inline float Dist2(const Vector& a, const Vector& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

struct alignas(16) Matrix {
    Vector col0;
    Vector col1;
    Vector col2;
    Vector col3;
};

} // namespace cdc

struct Vector3f {
    float x;
    float y;
    float z;

    Vector3f() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct PlaneConstants {
    short a;
    short b;
    short c;
    short flags;
    int d;
};

struct _CVECTOR {
    union {
        uint32_t c;
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
    };
};

static_assert(sizeof(cdc::Vector) == 0x10, "cdc::Vector size mismatch");
static_assert(sizeof(cdc::Vector3) == 0x10, "cdc::Vector3 size mismatch");
static_assert(sizeof(cdc::Euler) == 0x10, "cdc::Euler size mismatch");
static_assert(sizeof(cdc::Quat) == 0x10, "cdc::Quat size mismatch");
static_assert(sizeof(cdc::Matrix) == 0x40, "cdc::Matrix size mismatch");
static_assert(sizeof(Vector3f) == 0x0c, "Vector3f size mismatch");
static_assert(sizeof(PlaneConstants) == 0x0c, "PlaneConstants size mismatch");
static_assert(sizeof(_CVECTOR) == 0x04, "_CVECTOR size mismatch");

#endif // CDC_MATH_H
