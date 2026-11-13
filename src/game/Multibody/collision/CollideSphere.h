#ifndef GAME_MULTIBODY_COLLISION_COLLIDESPHERE_H
#define GAME_MULTIBODY_COLLISION_COLLIDESPHERE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"

// 0x004fe270 (PDB: 0x004fff70 / ?CollideSphereAndSphere@@YAHPAUCPoint@@ABVVector3@cdc@@M1M@Z)
int CollideSphereAndSphere(
    CPoint* cp,
    const cdc::Vector3& posA,
    float radA,
    const cdc::Vector3& posB,
    float radB);

// 0x004fe3a0 (PDB: 0x005000a0 / ?CollideSphereAndBox@@YAHPAUCPoint@@ABVVector3@cdc@@M1ABVQuat@3@1@Z)
int CollideSphereAndBox(
    CPoint* cp,
    const cdc::Vector3& spherePos,
    float sphereRad,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents);

#endif // GAME_MULTIBODY_COLLISION_COLLIDESPHERE_H
