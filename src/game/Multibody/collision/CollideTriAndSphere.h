#ifndef GAME_MULTIBODY_COLLISION_COLLIDETRIANDSPHERE_H
#define GAME_MULTIBODY_COLLISION_COLLIDETRIANDSPHERE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/camera/cameracollide.h"

// 0x005012d0 (PDB: 0x00502fd0 / ?CollideTriAndSphere@@YAHPAUCPoint@@ABVVector3@cdc@@MABUMTriangle@@E@Z)
int CollideTriAndSphere(
    CPoint* cp,
    const cdc::Vector3& spherePos,
    float sphereRad,
    const MTriangle& tri,
    unsigned char adj);

#endif // GAME_MULTIBODY_COLLISION_COLLIDETRIANDSPHERE_H
