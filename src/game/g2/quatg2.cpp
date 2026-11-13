#include "game/g2/quatg2.h"
#include <cmath>

namespace {

constexpr float kHalfPi = 1.57079637f; // 0x00effa28

// 0x00425770
inline float ClampedAsin(float value)
{
    if (value < -1.0f) {
        value = -1.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return std::asin(value);
}

} // namespace

// 0x004e00f0 (PDB: 0x004e1c20)
void G2Quat_ToMatrixf(cdc::Quat* quat, cdc::Matrix* matrix)
{
    cdc::Matrix temp;
    temp.Build(*quat);
    *matrix = temp;
}

// 0x004e0120 (PDB: 0x004e1c50)
void G2EulerAngles_FromMatrixf(_G2EulerAngles_Type* euler, cdc::Matrix* matrix, int order)
{
    if (order == 0) {
        cdc::Euler cdcEuler(*matrix);
        euler->x = cdcEuler.x;
        euler->y = cdcEuler.y;
        euler->z = cdcEuler.z;
    } else if (order == 0x15) {
        const float y = ClampedAsin(matrix->col2.x);
        float x;
        float z;
        if (y >= kHalfPi) {
            x = std::atan2(matrix->col0.y, matrix->col1.y);
            z = 0.0f;
        } else if (y <= -kHalfPi) {
            x = -std::atan2(matrix->col0.y, matrix->col1.y);
            z = 0.0f;
        } else {
            x = std::atan2(-matrix->col2.y, matrix->col2.z);
            z = std::atan2(-matrix->col1.x, matrix->col0.x);
        }
        euler->x = x;
        euler->y = y;
        euler->z = z;
    }
    euler->order = order;
}

// 0x004e01e0 (PDB: 0x004e1d10)
void G2Quat_ToEuler(cdc::Quat* quat, _G2EulerAngles_Type* euler, int order)
{
    cdc::Matrix mat;
    mat.Build(*quat);
    G2EulerAngles_FromMatrixf(euler, &mat, order);
}

// 0x004e0220 (PDB: 0x004e1d50)
void G2Quat_FromEulerf(cdc::Quat* quat, _G2EulerAngles_Type* euler)
{
    cdc::Euler cdcEuler;
    cdcEuler.x = euler->x;
    cdcEuler.y = euler->y;
    cdcEuler.z = euler->z;
    cdcEuler.w = 0.0f;
    *quat = cdc::Quat(cdcEuler);
}

// 0x004e0280 (PDB: 0x004e1db0)
void G2Quat_FromMatrixf(cdc::Quat* quat, cdc::Matrix* matrix)
{
    cdc::Quat temp;
    temp.Build(*matrix);
    *quat = temp;
}

// 0x004e02b0 (PDB: 0x004e1de0)
void G2Quat_Slerp(float ratio, cdc::Quat* quatA, cdc::Quat* quatB, cdc::Quat* quatOut, int spin)
{
    (void)spin;
    *quatOut = cdc::Slerp(*quatA, *quatB, ratio);
}
