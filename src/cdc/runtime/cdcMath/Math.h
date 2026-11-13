#pragma once

#ifndef CDC_MATH_H
#define CDC_MATH_H

#include <cstdint>
#include <cmath>

namespace cdc {

struct Vector3;

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

    // 0x00427650
    inline float Len3Squared() const
    {
        return x * x + y * y + z * z;
    }

    // 0x005deba0 (PDB: 0x005e15d0 / ?Normalize3@Vector@cdc@@QAEXXZ)
    inline void Normalize3()
    {
        const float lenSq = Len3Squared();
        if (lenSq > 0.0f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            x *= invLen;
            y *= invLen;
            z *= invLen;
        }
    }

    // 0x005dec50 (PDB: 0x005e1680 / ?UnitVec3@Vector@cdc@@QBE?AVVector3@2@XZ)
    inline Vector3 UnitVec3() const;

    inline Vector& operator+=(const Vector& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }
};

struct alignas(16) Vector3 : public Vector {
    static inline Vector3 Make(float _x, float _y, float _z, float _w = 0.0f)
    {
        Vector3 v;
        v.x = _x;
        v.y = _y;
        v.z = _z;
        v.w = _w;
        return v;
    }
};

inline Vector3 Vector::UnitVec3() const
{
    Vector3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = 0.0f;
    res.Normalize3();
    return res;
}

// 0x004026b0
inline float Dot3(const Vector& a, const Vector& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 0x00423ff0
inline void Cross3(Vector3* out, const Vector3& a, const Vector3& b)
{
    out->x = a.y * b.z - a.z * b.y;
    out->y = a.z * b.x - a.x * b.z;
    out->z = a.x * b.y - a.y * b.x;
    out->w = 0.0f;
}

inline Vector3 operator+(const Vector& a, const Vector& b)
{
    return Vector3::Make(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline Vector3 operator-(const Vector& a, const Vector& b)
{
    return Vector3::Make(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline Vector3 operator*(const Vector& v, float s)
{
    return Vector3::Make(v.x * s, v.y * s, v.z * s, v.w * s);
}

struct alignas(16) Euler : public Vector {};

struct Matrix;

struct alignas(16) Quat : public Vector {
    static inline Quat Identity()
    {
        Quat q;
        q.x = 0.0f;
        q.y = 0.0f;
        q.z = 0.0f;
        q.w = 1.0f;
        return q;
    }

    // 0x005deaf0 (PDB: 0x005e1520 / ?Normalize@Quat@cdc@@QAEXXZ)
    inline void Normalize()
    {
        const float lenSq = x * x + y * y + z * z + w * w;
        if (lenSq > 0.0f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            x *= invLen;
            y *= invLen;
            z *= invLen;
            w *= invLen;
        }
    }

    // 0x0041aed0
    inline Quat Inverse() const
    {
        Quat q;
        q.x = -x;
        q.y = -y;
        q.z = -z;
        q.w = w;
        return q;
    }

    // 0x005ded10 (PDB: 0x005e1740 / ?Build@Quat@cdc@@QAEXABVMatrix@2@@Z)
    inline void Build(const Matrix& mat);

    // 0x005df360 (PDB: 0x005e1d90 / ?Build@Quat@cdc@@QAEXABVVector3@2@M@Z)
    inline void Build(const Vector3& axis, float angle)
    {
        const float half = angle * 0.5f;
        const float s = std::sin(half);
        const float c = std::cos(half);
        const float lenSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
        const float invLen = 1.0f / std::sqrt(lenSq);
        x = axis.x * invLen * s;
        y = axis.y * invLen * s;
        z = axis.z * invLen * s;
        w = c;
    }

    // 0x005de8d0 (PDB: 0x005e1300 / ?Rotate@Quat@cdc@@QBE?AVVector3@2@ABV32@@Z)
    inline Vector3 Rotate(const Vector3& v) const
    {
        // q * (v, 0) * q^-1
        const float qx = x, qy = y, qz = z, qw = w;
        const float tx = 2.0f * (qy * v.z - qz * v.y);
        const float ty = 2.0f * (qz * v.x - qx * v.z);
        const float tz = 2.0f * (qx * v.y - qy * v.x);
        return Vector3::Make(
            v.x + qw * tx + (qy * tz - qz * ty),
            v.y + qw * ty + (qz * tx - qx * tz),
            v.z + qw * tz + (qx * ty - qy * tx),
            0.0f
        );
    }
};

// 0x005de560 (PDB: 0x005e0f90 / ?QuatMul@cdc@@YA?AVQuat@1@ABV21@0@Z)
inline Quat QuatMul(const Quat& a, const Quat& b)
{
    Quat out;
    out.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    out.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    out.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    out.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return out;
}

// 0x0041aea0
inline Quat operator*(const Quat& a, const Quat& b)
{
    return QuatMul(a, b);
}

// 0x005dee30 (PDB: 0x005e1860 / ?Slerp@cdc@@YA?AVQuat@1@ABV21@0M@Z)
inline Quat Slerp(const Quat& from, const Quat& to, float alpha)
{
    float cosOmega = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
    Quat adj = to;
    if (cosOmega < 0.0f) {
        cosOmega = -cosOmega;
        adj.x = -adj.x;
        adj.y = -adj.y;
        adj.z = -adj.z;
        adj.w = -adj.w;
    }

    float scale0, scale1;
    if (1.0f - cosOmega > 1.0e-4f) {
        const float omega = std::acos(cosOmega);
        const float invSin = 1.0f / std::sin(omega);
        scale0 = std::sin((1.0f - alpha) * omega) * invSin;
        scale1 = std::sin(alpha * omega) * invSin;
    } else {
        scale0 = 1.0f - alpha;
        scale1 = alpha;
    }

    Quat out;
    out.x = from.x * scale0 + adj.x * scale1;
    out.y = from.y * scale0 + adj.y * scale1;
    out.z = from.z * scale0 + adj.z * scale1;
    out.w = from.w * scale0 + adj.w * scale1;
    return out;
}

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

    // 0x00403050
    inline void Identity()
    {
        col0 = Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
        col1 = Vector3::Make(0.0f, 1.0f, 0.0f, 0.0f);
        col2 = Vector3::Make(0.0f, 0.0f, 1.0f, 0.0f);
        col3 = Vector3::Make(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // 0x00407d00 (PDB: 0x004084e0 / ?SetTrans@Matrix@cdc@@QAEXABVVector3@2@@Z)
    inline void SetTrans(const Vector3& t)
    {
        col3.x = t.x;
        col3.y = t.y;
        col3.z = t.z;
    }

    // 0x00419590
    inline void SetCol1(const Vector3& c1)
    {
        col1.x = c1.x;
        col1.y = c1.y;
        col1.z = c1.z;
    }

    // 0x00419600
    inline void SetCol2(const Vector3& c2)
    {
        col2.x = c2.x;
        col2.y = c2.y;
        col2.z = c2.z;
    }

    // 0x00477f00 (PDB: 0x00479870 / ?Build@Matrix@cdc@@QAEXABVVector3@2@00@Z)
    void Build(const Vector3& c0, const Vector3& c1, const Vector3& c2);

    // 0x005de940 (PDB: 0x005e1370 / ?Build@Matrix@cdc@@QAEXABVQuat@2@@Z)
    inline void Build(const Quat& q)
    {
        const float x2 = q.x + q.x;
        const float y2 = q.y + q.y;
        const float z2 = q.z + q.z;
        const float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
        const float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
        const float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;

        col0 = Vector3::Make(1.0f - (yy + zz), xy + wz, xz - wy, 0.0f);
        col1 = Vector3::Make(xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f);
        col2 = Vector3::Make(xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f);
        col3 = Vector3::Make(0.0f, 0.0f, 0.0f, 1.0f);
    }
};

inline void Quat::Build(const Matrix& mat)
{
    const float trace = mat.col0.x + mat.col1.y + mat.col2.z;
    if (trace > 0.0f) {
        x = mat.col1.z - mat.col2.y;
        y = mat.col2.x - mat.col0.z;
        z = mat.col0.y - mat.col1.x;
        w = trace + 1.0f;
    } else {
        int i = 0;
        if (mat.col1.y > mat.col0.x) i = 1;
        if (mat.col2.z > (i == 0 ? mat.col0.x : mat.col1.y)) i = 2;
        if (i == 0) {
            x = mat.col0.x + 1.0f - mat.col1.y - mat.col2.z;
            y = mat.col1.x + mat.col0.y;
            z = mat.col2.x + mat.col0.z;
            w = mat.col1.z - mat.col2.y;
        } else if (i == 1) {
            x = mat.col1.x + mat.col0.y;
            y = 1.0f - mat.col0.x + mat.col1.y - mat.col2.z;
            z = mat.col2.y + mat.col1.z;
            w = mat.col2.x - mat.col0.z;
        } else {
            x = mat.col2.x + mat.col0.z;
            y = mat.col2.y + mat.col1.z;
            z = 1.0f - mat.col0.x - mat.col1.y + mat.col2.z;
            w = mat.col0.y - mat.col1.x;
        }
    }
    Normalize();
}

// 0x005dd7d0 (PDB: 0x005e0200 / ?Mul3x3@cdc@@YA?AVVector@1@ABVMatrix@1@ABV21@@Z)
inline Vector3 Mul3x3(const Matrix& m, const Vector& v)
{
    return Vector3::Make(
        m.col0.x * v.x + m.col1.x * v.y + m.col2.x * v.z,
        m.col0.y * v.x + m.col1.y * v.y + m.col2.y * v.z,
        m.col0.z * v.x + m.col1.z * v.y + m.col2.z * v.z,
        0.0f
    );
}

// 0x005dd820 (PDB: 0x005e0250 / ?Mul3x4@cdc@@YA?AVVector@1@ABVMatrix@1@ABV21@@Z)
inline Vector3 Mul3x4(const Matrix& m, const Vector& v)
{
    return Vector3::Make(
        m.col0.x * v.x + m.col1.x * v.y + m.col2.x * v.z + m.col3.x,
        m.col0.y * v.x + m.col1.y * v.y + m.col2.y * v.z + m.col3.y,
        m.col0.z * v.x + m.col1.z * v.y + m.col2.z * v.z + m.col3.z,
        1.0f
    );
}

// 0x005dd6d0 (PDB: 0x005e0100 / ?Mul3x3@cdc@@YAXPAVMatrix@1@ABV21@1@Z)
inline void Mul3x3(Matrix* out, const Matrix& a, const Matrix& b)
{
    const Matrix tmpA = a;
    const Matrix tmpB = b;
    out->col0 = Mul3x3(tmpA, tmpB.col0);
    out->col1 = Mul3x3(tmpA, tmpB.col1);
    out->col2 = Mul3x3(tmpA, tmpB.col2);
    out->col3 = tmpB.col3;
}

// 0x005dd4c0 (PDB: 0x005dfef0 / ?Mul4x4@cdc@@YAXPAVMatrix@1@ABV21@1@Z)
inline void Mul4x4(Matrix* out, const Matrix& a, const Matrix& b)
{
    const Matrix tmpA = a;
    const Matrix tmpB = b;
    auto mulCol = [&](const Vector& c) {
        return Vector3::Make(
            tmpA.col0.x * c.x + tmpA.col1.x * c.y + tmpA.col2.x * c.z + tmpA.col3.x * c.w,
            tmpA.col0.y * c.x + tmpA.col1.y * c.y + tmpA.col2.y * c.z + tmpA.col3.y * c.w,
            tmpA.col0.z * c.x + tmpA.col1.z * c.y + tmpA.col2.z * c.z + tmpA.col3.z * c.w,
            tmpA.col0.w * c.x + tmpA.col1.w * c.y + tmpA.col2.w * c.z + tmpA.col3.w * c.w
        );
    };
    out->col0 = mulCol(tmpB.col0);
    out->col1 = mulCol(tmpB.col1);
    out->col2 = mulCol(tmpB.col2);
    out->col3 = mulCol(tmpB.col3);
}

// 0x005ddad0 (PDB: 0x005e0500 / ?OrthonormalInverse3x4@cdc@@YAXPAVMatrix@1@ABV21@@Z)
inline void OrthonormalInverse3x4(Matrix* out, const Matrix& m)
{
    out->col0 = Vector3::Make(m.col0.x, m.col1.x, m.col2.x, 0.0f);
    out->col1 = Vector3::Make(m.col0.y, m.col1.y, m.col2.y, 0.0f);
    out->col2 = Vector3::Make(m.col0.z, m.col1.z, m.col2.z, 0.0f);
    out->col3 = Vector3::Make(
        -(m.col0.x * m.col3.x + m.col0.y * m.col3.y + m.col0.z * m.col3.z),
        -(m.col1.x * m.col3.x + m.col1.y * m.col3.y + m.col1.z * m.col3.z),
        -(m.col2.x * m.col3.x + m.col2.y * m.col3.y + m.col2.z * m.col3.z),
        1.0f
    );
}

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
