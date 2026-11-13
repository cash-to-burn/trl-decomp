#ifndef GAME_MULTIBODY_COLLISION_COLLIDETRIANDCAPSULE_H
#define GAME_MULTIBODY_COLLISION_COLLIDETRIANDCAPSULE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/camera/cameracollide.h"

// 0x005009b0 (PDB: 0x005026b0 / ?ComputeEdgeContact@@YAHPAUCPoint@@ABVVector3@cdc@@1M1111@Z)
int ComputeEdgeContact(
    CPoint* cp,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    const cdc::Vector3& edgeA,
    const cdc::Vector3& edgeB,
    const cdc::Vector3& oppVert,
    const cdc::Vector3& triNormal);

// 0x00500ac0 (PDB: 0x005027c0 / ?CollideTriAndCapsule@@YAHPAUCPoint@@HABUMTriangle@@ABVVector3@cdc@@2ME@Z)
int CollideTriAndCapsule(
    CPoint* cp,
    int maxContacts,
    const MTriangle& tri,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    unsigned char adj);

#endif // GAME_MULTIBODY_COLLISION_COLLIDETRIANDCAPSULE_H
