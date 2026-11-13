#include "game/Multibody/src/MassProperties.h"

namespace {

inline void ZeroInertiaMatrix(cdc::Matrix* m)
{
    m->col0 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m->col1 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m->col2 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m->col3 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 1.0f);
}

inline void CopyUpperToLower3x3(cdc::Matrix* m)
{
    m->col0.y = m->col1.x;
    m->col0.z = m->col2.x;
    m->col1.z = m->col2.y;
}

} // namespace

// 0x00515020 (PDB: 0x00516b10 / ?CheckValidity@MassProperties@@QBE_NXZ)
bool MassProperties::CheckValidity() const
{
    return mass > 0.0f &&
           I.col0.x > 0.0f &&
           I.col1.y > 0.0f &&
           I.col2.z > 0.0f;
}

// 0x00515070 (PDB: 0x00516b60 / ?SetZero@MassProperties@@QAEXXZ)
void MassProperties::SetZero()
{
    mass = 0.0f;
    center = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    ZeroInertiaMatrix(&I);
}

// 0x005150b0 (PDB: 0x00516ba0 / ?SetSphere@MassProperties@@QAEXMM@Z)
void MassProperties::SetSphere(float massValue, float radius)
{
    mass = massValue;
    center = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    ZeroInertiaMatrix(&I);
    const float diag = 0.4f * massValue * radius * radius;
    I.col0.x = diag;
    I.col1.y = diag;
    I.col2.z = diag;
}

// 0x00515110 (PDB: 0x00516c00 / ?SetBox@MassProperties@@QAEXMABVVector3@cdc@@@Z)
void MassProperties::SetBox(float massValue, const cdc::Vector3& width)
{
    mass = massValue;
    center = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    float wx = width.x;
    float wy = width.y;
    float wz = width.z;

    float maxDim = wx > wy ? wx : wy;
    if (wz > maxDim) {
        maxDim = wz;
    }
    const float minDim = 0.2f * maxDim;
    if (wx < minDim) {
        wx = minDim;
    }
    if (wy < minDim) {
        wy = minDim;
    }
    if (wz < minDim) {
        wz = minDim;
    }

    ZeroInertiaMatrix(&I);
    const float xx = wx * wx;
    const float yy = wy * wy;
    const float zz = wz * wz;
    constexpr float kOneTwelfth = 1.0f / 12.0f;
    I.col0.x = kOneTwelfth * massValue * (yy + zz);
    I.col1.y = kOneTwelfth * massValue * (xx + zz);
    I.col2.z = kOneTwelfth * massValue * (xx + yy);
}

// 0x00515230 (PDB: 0x00516d20 / ?SetCapsule@MassProperties@@QAEXMMM@Z)
void MassProperties::SetCapsule(float massValue, float radius, float length)
{
    mass = massValue;
    center = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    const float minRadius = 0.2f * length;
    const float r = radius < minRadius ? minRadius : radius;

    constexpr float kPi = 3.1415927410125732f;
    const float r2 = r * r;
    const float cylVol = kPi * r2 * length;
    const float hemiVol = (2.0f / 3.0f) * r2 * r;
    const float invTotalVol = 1.0f / (cylVol + 2.0f * hemiVol);

    const float cylMass = invTotalVol * cylVol * massValue;
    const float hemiMass = invTotalVol * hemiVol * massValue;

    const float hemiOffset = 0.375f * r + 0.5f * length;
    const float cylMR2 = cylMass * r * r;
    const float hemiMR2 = hemiMass * r * r;

    constexpr float kOneTwelfth = 1.0f / 12.0f;
    constexpr float kHemiTransCoeff = 0.2593750059604645f; // 2/5 - (3/8)^2

    const float ixx =
        2.0f * kHemiTransCoeff * hemiMR2 +
        kOneTwelfth * cylMass * length * length +
        2.0f * (hemiOffset * hemiOffset * hemiMass) +
        0.25f * cylMR2;

    ZeroInertiaMatrix(&I);
    I.col0.x = ixx;
    I.col1.y = ixx;
    I.col2.z = 2.0f * 0.4f * hemiMR2 + 0.5f * cylMR2;
}

// 0x00515360 (PDB: 0x00516e50 / ?Translate@MassProperties@@QAEXABVVector3@cdc@@@Z)
void MassProperties::Translate(const cdc::Vector3& offset)
{
    const float x0 = center.x;
    const float y0 = center.y;
    const float z0 = center.z;

    I.col0.x -= mass * (y0 * y0 + z0 * z0);
    I.col1.x -= mass * -(x0 * y0);
    I.col2.x -= mass * -(x0 * z0);
    I.col1.y -= mass * (x0 * x0 + z0 * z0);
    I.col2.y -= mass * -(y0 * z0);
    I.col2.z -= mass * (x0 * x0 + y0 * y0);

    center.x += offset.x;
    center.y += offset.y;
    center.z += offset.z;

    const float x1 = center.x;
    const float y1 = center.y;
    const float z1 = center.z;

    I.col0.x += mass * (y1 * y1 + z1 * z1);
    I.col1.x += mass * -(x1 * y1);
    I.col2.x += mass * -(x1 * z1);
    I.col1.y += mass * (x1 * x1 + z1 * z1);
    I.col2.y += mass * -(y1 * z1);
    I.col2.z += mass * (x1 * x1 + y1 * y1);

    CopyUpperToLower3x3(&I);
}

// 0x00515470 (PDB: 0x00516f60 / ?Rotate@MassProperties@@QAEXABVMatrix@cdc@@@Z)
void MassProperties::Rotate(const cdc::Matrix& R)
{
    const cdc::Vector resCenter = cdc::Mul3x3(R, static_cast<const cdc::Vector&>(center));
    center = cdc::Vector3::Make(resCenter.x, resCenter.y, resCenter.z, resCenter.w);

    cdc::Matrix RT;
    cdc::Transpose(&RT, R);

    cdc::Matrix temp;
    cdc::Mul3x3(&temp, I, RT);
    cdc::Mul3x3(&I, R, temp);

    CopyUpperToLower3x3(&I);
}

// 0x00515510 (PDB: 0x00517000 / ?Add@MassProperties@@QAEXABU1@@Z)
void MassProperties::Add(const MassProperties& other)
{
    const float totalMass = mass + other.mass;
    const float invTotal = 1.0f / totalMass;

    center.x = (center.x * mass + other.center.x * other.mass) * invTotal;
    center.y = (center.y * mass + other.center.y * other.mass) * invTotal;
    center.z = (center.z * mass + other.center.z * other.mass) * invTotal;
    mass = totalMass;

    I.col0 = cdc::Vector3::Make(
        I.col0.x + other.I.col0.x,
        I.col0.y + other.I.col0.y,
        I.col0.z + other.I.col0.z,
        I.col0.w + other.I.col0.w);
    I.col1 = cdc::Vector3::Make(
        I.col1.x + other.I.col1.x,
        I.col1.y + other.I.col1.y,
        I.col1.z + other.I.col1.z,
        I.col1.w + other.I.col1.w);
    I.col2 = cdc::Vector3::Make(
        I.col2.x + other.I.col2.x,
        I.col2.y + other.I.col2.y,
        I.col2.z + other.I.col2.z,
        I.col2.w + other.I.col2.w);
    I.col3 = cdc::Vector3::Make(
        I.col3.x + other.I.col3.x,
        I.col3.y + other.I.col3.y,
        I.col3.z + other.I.col3.z,
        I.col3.w + other.I.col3.w);
}
