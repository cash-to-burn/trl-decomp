#ifndef GAME_MULTIBODY_COLLISION_COLLIDEBOXANDCAPSULE_H
#define GAME_MULTIBODY_COLLISION_COLLIDEBOXANDCAPSULE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"

// 0x004fbd40 (PDB: 0x004fda40 / ?CollideBoxAndCapsule@@YAHPAUCPoint@@ABVVector3@cdc@@ABVQuat@3@111M@Z)
int CollideBoxAndCapsule(
    CPoint* cp,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents,
    const cdc::Vector3& capStart,
    const cdc::Vector3& capEnd,
    float capRad);

#endif // GAME_MULTIBODY_COLLISION_COLLIDEBOXANDCAPSULE_H
