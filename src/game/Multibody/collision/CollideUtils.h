#ifndef GAME_MULTIBODY_COLLISION_COLLIDEUTILS_H
#define GAME_MULTIBODY_COLLISION_COLLIDEUTILS_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/cameracollide.h"

// PDB: ClipPoint (size: 0x20)
struct alignas(16) ClipPoint {
    cdc::Vector3 p;      // 0x00
    int16_t edge0;       // 0x10
    int16_t edge1;       // 0x12
    int16_t clipEdge0;   // 0x14
    int16_t clipEdge1;   // 0x16
    uint32_t pad18[2];   // 0x18..0x1f
};
static_assert(sizeof(ClipPoint) == 0x20, "ClipPoint size mismatch");

// PDB: CPoint (size: 0x30)
struct alignas(16) CPoint {
    cdc::Vector3 position;  // 0x00
    cdc::Vector3 normal;    // 0x10
    float separation;       // 0x20
    uint16_t faceIndex;     // 0x24
    uint16_t pad26;         // 0x26
    uint32_t pad28[2];      // 0x28..0x2f
};
static_assert(sizeof(CPoint) == 0x30, "CPoint size mismatch");

// PDB: OBox (size: 0xc0)
struct alignas(16) OBox {
    cdc::Vector3 x;         // 0x00
    cdc::Matrix R;          // 0x10
    cdc::Matrix RT;         // 0x50
    cdc::Vector3 h;         // 0x90
    BBox aabb;              // 0xa0
};
static_assert(sizeof(OBox) == 0xc0, "OBox size mismatch");

// 0x00501ca0 (PDB: 0x00503940 / ?LineLineMidpoint@@YA?AVVector3@cdc@@ABV12@000@Z)
cdc::Vector3 LineLineMidpoint(
    const cdc::Vector3& p1,
    const cdc::Vector3& d1,
    const cdc::Vector3& p2,
    const cdc::Vector3& d2);

// 0x00501dd0 (PDB: 0x00503a70 / ?TestSegmentAndBox@@YA_NABVVector3@cdc@@000ABUBBox@@@Z)
bool TestSegmentAndBox(
    const cdc::Vector3& segMid,
    const cdc::Vector3& segHalf,
    const cdc::Vector3& segMin,
    const cdc::Vector3& segMax,
    const BBox& box);

// 0x00501f10 (PDB: 0x00503bb0 / ?ClipPolyToPlane@@YAHPAVVector3@cdc@@0HABV12@M@Z)
int ClipPolyToPlane(
    cdc::Vector3* outVerts,
    cdc::Vector3* inVerts,
    int numVerts,
    const cdc::Vector3& planeNormal,
    float planeDist);

// 0x005021d0 (PDB: 0x00503e70 / ?ClipPolyToPlane@@YAHPAUClipPoint@@PBU1@HABVVector3@cdc@@MF@Z)
int ClipPolyToPlane(
    ClipPoint* outVerts,
    const ClipPoint* inVerts,
    int numVerts,
    const cdc::Vector3& planeNormal,
    float planeDist,
    short clipEdge);

// 0x00502530 (PDB: 0x005041d0 / ?PointToSegmentDistance@@YAMPAMPAVVector3@cdc@@ABV12@22@Z)
float PointToSegmentDistance(
    float* outT,
    cdc::Vector3* outClosest,
    const cdc::Vector3& point,
    const cdc::Vector3& segA,
    const cdc::Vector3& segB);

// 0x005026d0 (PDB: 0x00504370 / ?PointToAlignedBoxDistance@@YAMPAVVector3@cdc@@ABV12@ABUBBox@@@Z)
float PointToAlignedBoxDistance(
    cdc::Vector3* outClosest,
    const cdc::Vector3& point,
    const BBox& box);

// 0x00502800 (PDB: 0x005044a0 / ?PointToBoxDistance@@YAMPAVVector3@cdc@@0ABV12@1ABVQuat@2@1@Z)
float PointToBoxDistance(
    cdc::Vector3* outClosest,
    cdc::Vector3* outNormal,
    const cdc::Vector3& point,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents);

// 0x00502a50 (PDB: 0x005046f0 / ?TestPointAndBox@@YA_NABVVector3@cdc@@0ABVQuat@2@0@Z)
bool TestPointAndBox(
    const cdc::Vector3& point,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents);

// 0x00502b00 (PDB: 0x005047a0 / ?SegmentToSegmentDistance@@YAMPAVVector3@cdc@@0ABV12@111@Z)
float SegmentToSegmentDistance(
    cdc::Vector3* outP1,
    cdc::Vector3* outP2,
    const cdc::Vector3& a0,
    const cdc::Vector3& a1,
    const cdc::Vector3& b0,
    const cdc::Vector3& b1);

// Shared Multibody collision helpers (matching retail inline helpers)
namespace MultibodyCollision {

inline float Clamp(float value, float minVal, float maxVal)
{
    if (value < minVal) {
        return minVal;
    }
    return value > maxVal ? maxVal : value;
}

inline bool IsZeroVector(const cdc::Vector3& v)
{
    constexpr float kZeroEps = 1.0e-6f;
    return v.x <= kZeroEps && v.x >= -kZeroEps &&
           v.y <= kZeroEps && v.y >= -kZeroEps &&
           v.z <= kZeroEps && v.z >= -kZeroEps;
}

inline cdc::Vector3 AbsVector3(const cdc::Vector3& v)
{
    return cdc::Vector3::Make(
        v.x < 0.0f ? -v.x : v.x,
        v.y < 0.0f ? -v.y : v.y,
        v.z < 0.0f ? -v.z : v.z,
        0.0f);
}

inline int ResolveSpherePairContact(
    CPoint* cp,
    const cdc::Vector3& posA,
    float radA,
    const cdc::Vector3& posB,
    float radB,
    float distSq)
{
    const float radSum = radA + radB;
    if (distSq > radSum * radSum) {
        return 0;
    }

    const float dist = std::sqrt(distSq);
    const cdc::Vector3 diff = posB - posA;
    if (IsZeroVector(diff)) {
        cp->normal = cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
    } else {
        cp->normal = diff;
        cp->normal.Normalize3();
    }

    cp->separation = dist - radSum;
    cp->position = posA + cp->normal * (0.5f * (dist + radA - radB));
    return 1;
}

} // namespace MultibodyCollision

#endif // GAME_MULTIBODY_COLLISION_COLLIDEUTILS_H
