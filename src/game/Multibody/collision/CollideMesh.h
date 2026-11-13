#ifndef GAME_MULTIBODY_COLLISION_COLLIDEMESH_H
#define GAME_MULTIBODY_COLLISION_COLLIDEMESH_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideSegment.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/camera/cameracollide.h"

// 0x004fcd40 (PDB: 0x004fea40 / ?CollideMeshAndSphere@@YAHPAUCPoint@@HABUMSphere@@PBVMesh@@E@Z)
int CollideMeshAndSphere(
    CPoint* cp,
    int maxContacts,
    const MSphere& sphere,
    const Mesh* mesh,
    unsigned char flags);

// 0x004fcea0 (PDB: 0x004feba0 / ?CollideMeshAndBox@@YAHPAUCPoint@@HABUOBox@@PBVMesh@@E@Z)
int CollideMeshAndBox(
    CPoint* cp,
    int maxContacts,
    const OBox& box,
    const Mesh* mesh,
    unsigned char flags);

// 0x004fcfd0 (PDB: 0x004fecd0 / ?CollideMeshAndCapsule@@YAHPAUCPoint@@HPBVMesh@@ABVVector3@cdc@@2MABUBBox@@E@Z)
int CollideMeshAndCapsule(
    CPoint* cp,
    int maxContacts,
    const Mesh* mesh,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    const BBox& aabb,
    unsigned char flags);

#endif // GAME_MULTIBODY_COLLISION_COLLIDEMESH_H
