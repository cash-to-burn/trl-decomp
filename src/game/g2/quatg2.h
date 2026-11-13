#pragma once

#ifndef GAME_G2_QUATG2_H
#define GAME_G2_QUATG2_H

#include "cdc/runtime/cdcMath/Math.h"

// Size: 0x10 (16 bytes, TI: 0x59e4)
struct _G2EulerAngles_Type {
    /* 0x00 */ float x;
    /* 0x04 */ float y;
    /* 0x08 */ float z;
    /* 0x0c */ int order;
};
static_assert(sizeof(_G2EulerAngles_Type) == 0x10, "_G2EulerAngles_Type size mismatch");

void G2Quat_ToMatrixf(cdc::Quat* quat, cdc::Matrix* matrix);                                   // 0x004e00f0 (PDB: 0x004e1c20)
void G2EulerAngles_FromMatrixf(_G2EulerAngles_Type* euler, cdc::Matrix* matrix, int order);    // 0x004e0120 (PDB: 0x004e1c50)
void G2Quat_ToEuler(cdc::Quat* quat, _G2EulerAngles_Type* euler, int order);                   // 0x004e01e0 (PDB: 0x004e1d10)
void G2Quat_FromEulerf(cdc::Quat* quat, _G2EulerAngles_Type* euler);                           // 0x004e0220 (PDB: 0x004e1d50)
void G2Quat_FromMatrixf(cdc::Quat* quat, cdc::Matrix* matrix);                                 // 0x004e0280 (PDB: 0x004e1db0)
void G2Quat_Slerp(float ratio, cdc::Quat* quatA, cdc::Quat* quatB, cdc::Quat* quatOut, int spin); // 0x004e02b0 (PDB: 0x004e1de0)

#endif // GAME_G2_QUATG2_H
