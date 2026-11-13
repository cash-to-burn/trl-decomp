#ifndef GAME_MULTIBODY_COLLISION_COLLIDEPLANE_H
#define GAME_MULTIBODY_COLLISION_COLLIDEPLANE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"

// 0x004fd100 (PDB: 0x004fee00 / ?CollideSphereAndPlane@@YAHPAUCPoint@@ABVVector3@cdc@@M1M@Z)
int CollideSphereAndPlane(
    CPoint* cp,
    const cdc::Vector3& pos,
    float rad,
    const cdc::Vector3& planeNormal,
    float planeDist);

// 0x004fd1d0 (PDB: 0x004feed0 / ?CollideCapsuleAndPlane@@YAHPAUCPoint@@ABVVector3@cdc@@1M1M@Z)
int CollideCapsuleAndPlane(
    CPoint* cp,
    const cdc::Vector3& p0,
    const cdc::Vector3& p1,
    float rad,
    const cdc::Vector3& planeNormal,
    float planeDist);

// 0x004fd350 (PDB: 0x004ff050 / ?CollideBoxAndPlane@@YAHPAUCPoint@@ABVVector3@cdc@@ABVQuat@3@11M@Z)
int CollideBoxAndPlane(
    CPoint* cp,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents,
    const cdc::Vector3& planeNormal,
    float planeDist);

#endif // GAME_MULTIBODY_COLLISION_COLLIDEPLANE_H
