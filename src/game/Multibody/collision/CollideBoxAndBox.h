#ifndef GAME_MULTIBODY_COLLISION_COLLIDEBOXANDBOX_H
#define GAME_MULTIBODY_COLLISION_COLLIDEBOXANDBOX_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"

// 0x004fa270 (PDB: 0x004fbf10 / ?CollideBoxAndBox@@YAHPAUCPoint@@ABVVector3@cdc@@111ABVQuat@3@2@Z)
int CollideBoxAndBox(
    CPoint* cp,
    const cdc::Vector3& hA,
    const cdc::Vector3& hB,
    const cdc::Vector3& posA,
    const cdc::Vector3& posB,
    const cdc::Quat& qA,
    const cdc::Quat& qB);

// 0x005060a0 (PDB: 0x004fad10 / LPROC ComputeSkewedBasis)
void ComputeSkewedBasis(cdc::Vector3* u, cdc::Vector3* v, const cdc::Vector3& n);

#endif // GAME_MULTIBODY_COLLISION_COLLIDEBOXANDBOX_H
