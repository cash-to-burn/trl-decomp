#pragma once

#ifndef GAME_MULTIBODY_COLLISION_COLLIDESEGMENT_H
#define GAME_MULTIBODY_COLLISION_COLLIDESEGMENT_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/cameracollide.h"

// Size: 0x04 (4 bytes, TI: 0x9809)
enum CollideCode {
    NO_HIT                = 0,
    HIT                   = 1,
    HIT_INSIDE            = 2,
    HIT_CYLINDER_ENDCAP_X = 3,
    HIT_CYLINDER_ENDCAP_Y = 4,
};

// Size: 0x20 (32 bytes, TI: 0x980e)
struct MSphere {
    /* 0x000 */ cdc::Vector3 x;
    /* 0x010 */ float radius;
    /* 0x014 */ float pad014[3];
};
static_assert(offsetof(MSphere, radius) == 0x10, "MSphere::radius offset mismatch");
static_assert(sizeof(MSphere) == 0x20, "MSphere size mismatch");

// Size: 0x30 (48 bytes, TI: 0x986d)
struct MBox {
    /* 0x000 */ cdc::Vector3 x;
    /* 0x010 */ cdc::Quat q;
    /* 0x020 */ cdc::Vector3 h;
};
static_assert(offsetof(MBox, q) == 0x10, "MBox::q offset mismatch");
static_assert(offsetof(MBox, h) == 0x20, "MBox::h offset mismatch");
static_assert(sizeof(MBox) == 0x30, "MBox size mismatch");

// Segment intersection routines (Original: game/Multibody/collision/CollideSegment.cpp)
CollideCode CollideSegmentAndSphere(float* t, cdc::Vector3* normal, const cdc::Vector3& start, const cdc::Vector3& dir, const MSphere& sphere);                                          // 0x004fd850 (PDB: 0x004ff550)
CollideCode CollideSegmentAndTri(float* t, cdc::Vector3* normal, const cdc::Vector3& start, const cdc::Vector3& dir, const MTriangle& tri);                                              // 0x004fd9b0 (PDB: 0x004ff6b0)
CollideCode CollideSegmentAndAlignedBox(float* t, cdc::Vector3* normal, const cdc::Vector3& start, const cdc::Vector3& dir, const BBox& box);                                            // 0x004fdb80 (PDB: 0x004ff880)
CollideCode CollideSegmentAndBox(float* t, cdc::Vector3* normal, const cdc::Vector3& start, const cdc::Vector3& dir, const MBox& box);                                                   // 0x004fdd10 (PDB: 0x004ffa10)
CollideCode CollideSegmentAndCapsule(float* t, cdc::Vector3* normal, const cdc::Vector3& start, const cdc::Vector3& dir, const cdc::Vector3& capA, const cdc::Vector3& capB, float radius); // 0x004fde00 (PDB: 0x004ffb00)

#endif // GAME_MULTIBODY_COLLISION_COLLIDESEGMENT_H
