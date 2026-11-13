#ifndef GAME_MULTIBODY_COLLISION_COLLIDESWEPTSPHERE_H
#define GAME_MULTIBODY_COLLISION_COLLIDESWEPTSPHERE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideSegment.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/camera/cameracollide.h"

// 0x004fe620 (PDB: 0x00500320 / ?CollideSweptSphereAndTriangle@@YA?AW4CollideCode@@PAMAAVVector3@cdc@@ABUMSphere@@ABV23@ABUMTriangle@@@Z)
CollideCode CollideSweptSphereAndTriangle(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphere,
    const cdc::Vector3& dir,
    const MTriangle& tri);

// 0x004fe9d0 (PDB: 0x005006d0 / ?CollideSweptSphereAndSphere@@YA?AW4CollideCode@@PAMAAVVector3@cdc@@ABUMSphere@@ABV23@2@Z)
CollideCode CollideSweptSphereAndSphere(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphereA,
    const cdc::Vector3& dir,
    const MSphere& sphereB);

// 0x004fea30 (PDB: 0x00500730 / ?CollideSweptSphereAndAlignedBox@@YA?AW4CollideCode@@PAMAAVVector3@cdc@@ABUMSphere@@ABV23@ABUBBox@@@Z)
CollideCode CollideSweptSphereAndAlignedBox(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphere,
    const cdc::Vector3& dir,
    const BBox& box);

// 0x004fed10 (PDB: 0x00500a10 / ?CollideSweptSphereAndBox@@YA?AW4CollideCode@@PAMAAVVector3@cdc@@ABUMSphere@@ABV23@ABUOBox@@@Z)
CollideCode CollideSweptSphereAndBox(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphere,
    const cdc::Vector3& dir,
    const OBox& box);

#endif // GAME_MULTIBODY_COLLISION_COLLIDESWEPTSPHERE_H
