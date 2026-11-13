#ifndef GAME_MULTIBODY_COLLISION_COLLIDECAPSULE_H
#define GAME_MULTIBODY_COLLISION_COLLIDECAPSULE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"

// 0x004fcae0 (PDB: 0x004fe7e0 / ?CollideCapsuleAndSphere@@YAHPAUCPoint@@ABVVector3@cdc@@1M1M@Z)
int CollideCapsuleAndSphere(
    CPoint* cp,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    const cdc::Vector3& spherePos,
    float sphereRad);

// 0x004fcc10 (PDB: 0x004fe910 / ?CollideCapsuleAndCapsule@@YAHPAUCPoint@@ABVVector3@cdc@@1M11M@Z)
int CollideCapsuleAndCapsule(
    CPoint* cp,
    const cdc::Vector3& a0,
    const cdc::Vector3& a1,
    float radA,
    const cdc::Vector3& b0,
    const cdc::Vector3& b1,
    float radB);

#endif // GAME_MULTIBODY_COLLISION_COLLIDECAPSULE_H
