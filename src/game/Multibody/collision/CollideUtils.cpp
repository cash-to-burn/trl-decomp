#include "game/Multibody/collision/CollideUtils.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace {

constexpr float kCollideEpsilon = FLT_EPSILON;

// 0x00501c40 (PDB: 0x004fd260 / LPROC CrossTrick)
cdc::Vector3 CrossTrick(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.z * b.y + a.y * b.z,
        a.z * b.x + a.x * b.z,
        a.x * b.y + a.y * b.x,
        0.0f);
}

float VectorAxis(const cdc::Vector3& v, int axis)
{
    switch (axis) {
    case 0:  return v.x;
    case 1:  return v.y;
    default: return v.z;
    }
}

void AddAxisValue(cdc::Vector3* v, int axis, float delta)
{
    switch (axis) {
    case 0:  v->x += delta; break;
    case 1:  v->y += delta; break;
    default: v->z += delta; break;
    }
}

template <typename TVertex>
float PlaneSignedDist(const TVertex& v, const cdc::Vector3& planeNormal, float planeDist)
{
    return cdc::Dot3(planeNormal, v.p) - planeDist;
}

template <>
float PlaneSignedDist<cdc::Vector3>(const cdc::Vector3& v, const cdc::Vector3& planeNormal, float planeDist)
{
    return cdc::Dot3(planeNormal, v) - planeDist;
}

cdc::Vector3 InterpolateClipPos(const cdc::Vector3& prev, const cdc::Vector3& cur, float t)
{
    return prev + (cur - prev) * t;
}

ClipPoint MakeClippedEdgePoint(
    const ClipPoint& prev,
    const ClipPoint& cur,
    float dPrev,
    float dCur,
    short clipEdge,
    bool twoVertMode)
{
    ClipPoint out;
    const float t = dPrev / (dPrev - dCur);
    out.p = prev.p + (cur.p - prev.p) * t;
    out.pad18[0] = 0;
    out.pad18[1] = 0;

    if (dPrev < 0.0f) {
        out.clipEdge0 = prev.clipEdge1;
        out.clipEdge1 = clipEdge;
        out.edge0 = prev.edge1;
        out.edge1 = -1;
    } else {
        out.clipEdge0 = clipEdge;
        out.clipEdge1 = twoVertMode ? static_cast<int16_t>(-1) : cur.clipEdge0;
        out.edge0 = -1;
        out.edge1 = cur.edge0;
    }
    return out;
}

} // namespace

// 0x00501ca0 (PDB: 0x00503940)
cdc::Vector3 LineLineMidpoint(
    const cdc::Vector3& p1,
    const cdc::Vector3& d1,
    const cdc::Vector3& p2,
    const cdc::Vector3& d2)
{
    const float e = cdc::Dot3(d1, d2);
    const cdc::Vector3 r = p1 - p2;
    const float a = cdc::Dot3(r, d1);
    const float b = cdc::Dot3(r, d2);
    const float denom = 1.0f - e * e;

    float t1 = 0.0f;
    float t2 = b;
    if (denom > kCollideEpsilon) {
        const float invDenom = 1.0f / denom;
        t1 = (b * e - a) * invDenom;
        t2 = (b - a * e) * invDenom;
    }

    const cdc::Vector3 q1 = p1 + d1 * t1;
    const cdc::Vector3 q2 = p2 + d2 * t2;
    return (q1 + q2) * 0.5f;
}

// 0x00501dd0 (PDB: 0x00503a70)
bool TestSegmentAndBox(
    const cdc::Vector3& segMid,
    const cdc::Vector3& segHalf,
    const cdc::Vector3& segMin,
    const cdc::Vector3& segMax,
    const BBox& box)
{
    if (box.bMin.x > segMax.x || segMin.x > box.bMax.x ||
        box.bMin.y > segMax.y || segMin.y > box.bMax.y ||
        box.bMin.z > segMax.z || segMin.z > box.bMax.z) {
        return false;
    }

    const cdc::Vector3 boxCenter = (box.bMin + box.bMax) * 0.5f;
    const cdc::Vector3 boxHalf = (box.bMax - box.bMin) * 0.5f;
    const cdc::Vector3 absHalf = MultibodyCollision::AbsVector3(segHalf);
    const cdc::Vector3 crossBounds = CrossTrick(boxHalf, absHalf);

    cdc::Vector3 crossVec;
    cdc::Cross3(&crossVec, segMid - boxCenter, segHalf);
    const cdc::Vector3 absCross = MultibodyCollision::AbsVector3(crossVec);

    return absCross.x <= crossBounds.x &&
           absCross.y <= crossBounds.y &&
           absCross.z <= crossBounds.z;
}

// 0x00501f10 (PDB: 0x00503bb0)
int ClipPolyToPlane(
    cdc::Vector3* outVerts,
    cdc::Vector3* inVerts,
    int numVerts,
    const cdc::Vector3& planeNormal,
    float planeDist)
{
    if (numVerts <= 0) {
        return 0;
    }

    if (numVerts == 1) {
        if (PlaneSignedDist(inVerts[0], planeNormal, planeDist) <= 0.0f) {
            outVerts[0] = inVerts[0];
            return 1;
        }
        return 0;
    }

    int outCount = 0;
    if (numVerts == 2) {
        const float d0 = PlaneSignedDist(inVerts[0], planeNormal, planeDist);
        const float d1 = PlaneSignedDist(inVerts[1], planeNormal, planeDist);
        if (d0 <= 0.0f) {
            outVerts[outCount++] = inVerts[0];
        }
        if (d1 <= 0.0f) {
            outVerts[outCount++] = inVerts[1];
        }
        if (d0 * d1 < 0.0f) {
            outVerts[outCount++] = InterpolateClipPos(inVerts[0], inVerts[1], d0 / (d0 - d1));
        }
        return outCount;
    }

    int prevIdx = numVerts - 1;
    for (int i = 0; i < numVerts; ++i) {
        const float dPrev = PlaneSignedDist(inVerts[prevIdx], planeNormal, planeDist);
        const float dCur = PlaneSignedDist(inVerts[i], planeNormal, planeDist);
        if (dPrev <= 0.0f) {
            outVerts[outCount++] = inVerts[prevIdx];
        }
        if (dPrev * dCur < 0.0f) {
            outVerts[outCount++] = InterpolateClipPos(inVerts[prevIdx], inVerts[i], dPrev / (dPrev - dCur));
        }
        prevIdx = i;
    }

    return outCount;
}

// 0x005021d0 (PDB: 0x00503e70)
int ClipPolyToPlane(
    ClipPoint* outVerts,
    const ClipPoint* inVerts,
    int numVerts,
    const cdc::Vector3& planeNormal,
    float planeDist,
    short clipEdge)
{
    if (numVerts <= 0) {
        return 0;
    }

    if (numVerts == 1) {
        if (PlaneSignedDist(inVerts[0], planeNormal, planeDist) <= 0.0f) {
            outVerts[0] = inVerts[0];
            return 1;
        }
        return 0;
    }

    int outCount = 0;
    if (numVerts == 2) {
        const float d0 = PlaneSignedDist(inVerts[0], planeNormal, planeDist);
        const float d1 = PlaneSignedDist(inVerts[1], planeNormal, planeDist);
        if (d0 <= 0.0f) {
            outVerts[outCount++] = inVerts[0];
        }
        if (d1 <= 0.0f) {
            outVerts[outCount++] = inVerts[1];
        }
        if (d0 * d1 < 0.0f) {
            outVerts[outCount++] = MakeClippedEdgePoint(inVerts[0], inVerts[1], d0, d1, clipEdge, true);
        }
        return outCount;
    }

    int prevIdx = numVerts - 1;
    for (int i = 0; i < numVerts; ++i) {
        const float dPrev = PlaneSignedDist(inVerts[prevIdx], planeNormal, planeDist);
        const float dCur = PlaneSignedDist(inVerts[i], planeNormal, planeDist);
        if (dPrev <= 0.0f) {
            outVerts[outCount++] = inVerts[prevIdx];
        }
        if (dPrev * dCur < 0.0f) {
            outVerts[outCount++] = MakeClippedEdgePoint(inVerts[prevIdx], inVerts[i], dPrev, dCur, clipEdge, false);
        }
        prevIdx = i;
    }

    return outCount;
}

// 0x00502530 (PDB: 0x005041d0)
float PointToSegmentDistance(
    float* outT,
    cdc::Vector3* outClosest,
    const cdc::Vector3& point,
    const cdc::Vector3& segA,
    const cdc::Vector3& segB)
{
    const cdc::Vector3 d = segB - segA;
    const float lenSq = cdc::Dot3(d, d);
    if (lenSq == 0.0f) {
        if (outT != nullptr) {
            *outT = 0.0f;
        }
        if (outClosest != nullptr) {
            *outClosest = segA;
        }
        const cdc::Vector3 diff = point - segA;
        return cdc::Dot3(diff, diff);
    }

    const float t = MultibodyCollision::Clamp(cdc::Dot3(point - segA, d) / lenSq, 0.0f, 1.0f);
    const cdc::Vector3 closest = segA + d * t;
    if (outT != nullptr) {
        *outT = t;
    }
    if (outClosest != nullptr) {
        *outClosest = closest;
    }

    const cdc::Vector3 diff = closest - point;
    return cdc::Dot3(diff, diff);
}

// 0x005026d0 (PDB: 0x00504370)
float PointToAlignedBoxDistance(
    cdc::Vector3* outClosest,
    const cdc::Vector3& point,
    const BBox& box)
{
    const cdc::Vector3 closest = cdc::Vector3::Make(
        MultibodyCollision::Clamp(point.x, box.bMin.x, box.bMax.x),
        MultibodyCollision::Clamp(point.y, box.bMin.y, box.bMax.y),
        MultibodyCollision::Clamp(point.z, box.bMin.z, box.bMax.z),
        0.0f);
    *outClosest = closest;

    const cdc::Vector3 diff = closest - point;
    return cdc::Dot3(diff, diff);
}

// 0x00502800 (PDB: 0x005044a0)
float PointToBoxDistance(
    cdc::Vector3* outClosest,
    cdc::Vector3* outNormal,
    const cdc::Vector3& point,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents)
{
    const cdc::Quat qInv = ~boxRot;
    const cdc::Vector3 localPt = qInv.Rotate(point - boxCenter);

    BBox localBox;
    localBox.bMin = boxHalfExtents * -1.0f;
    localBox.bMax = boxHalfExtents;

    cdc::Vector3 localClosest;
    const float distSq = PointToAlignedBoxDistance(&localClosest, localPt, localBox);
    *outClosest = boxCenter + boxRot.Rotate(localClosest);

    cdc::Vector3 localNormal = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    for (int axis = 0; axis < 3; ++axis) {
        const float c = VectorAxis(localClosest, axis);
        const float h = VectorAxis(boxHalfExtents, axis);
        if (c == h) {
            AddAxisValue(&localNormal, axis, 1.0f);
        } else if (c == -h) {
            AddAxisValue(&localNormal, axis, -1.0f);
        }
    }

    if (MultibodyCollision::IsZeroVector(localNormal)) {
        *outNormal = cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
    } else {
        localNormal.Normalize3();
        *outNormal = boxRot.Rotate(localNormal);
    }

    return distSq;
}

// 0x00502a50 (PDB: 0x005046f0)
bool TestPointAndBox(
    const cdc::Vector3& point,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents)
{
    const cdc::Quat qInv = ~boxRot;
    const cdc::Vector3 localPt = qInv.Rotate(point - boxCenter);

    return localPt.x <= boxHalfExtents.x && localPt.x >= -boxHalfExtents.x &&
           localPt.y <= boxHalfExtents.y && localPt.y >= -boxHalfExtents.y &&
           localPt.z <= boxHalfExtents.z && localPt.z >= -boxHalfExtents.z;
}

// 0x00502b00 (PDB: 0x005047a0)
float SegmentToSegmentDistance(
    cdc::Vector3* outP1,
    cdc::Vector3* outP2,
    const cdc::Vector3& a0,
    const cdc::Vector3& a1,
    const cdc::Vector3& b0,
    const cdc::Vector3& b1)
{
    const cdc::Vector3 d1 = a1 - a0;
    const cdc::Vector3 d2 = b1 - b0;
    const cdc::Vector3 r = a0 - b0;
    const float a = cdc::Dot3(d1, d1);
    const float e = cdc::Dot3(d2, d2);

    if (a <= kCollideEpsilon && e <= kCollideEpsilon) {
        *outP1 = a0;
        *outP2 = b0;
        return cdc::Dot3(r, r);
    }

    const float f = cdc::Dot3(d2, r);
    float s = 0.0f;
    float t = 0.0f;

    if (a <= kCollideEpsilon) {
        t = MultibodyCollision::Clamp(f / e, 0.0f, 1.0f);
    } else {
        const float c = cdc::Dot3(d1, r);
        if (e <= kCollideEpsilon) {
            s = MultibodyCollision::Clamp(-c / a, 0.0f, 1.0f);
        } else {
            const float b = cdc::Dot3(d1, d2);
            const float denom = a * e - b * b;
            if (denom > kCollideEpsilon) {
                s = MultibodyCollision::Clamp((b * f - c * e) / denom, 0.0f, 1.0f);
            }

            t = (b * s + f) / e;
            if (t < 0.0f) {
                t = 0.0f;
                s = MultibodyCollision::Clamp(-c / a, 0.0f, 1.0f);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = MultibodyCollision::Clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }

    const cdc::Vector3 c1 = a0 + d1 * s;
    const cdc::Vector3 c2 = b0 + d2 * t;
    *outP1 = c1;
    *outP2 = c2;

    const cdc::Vector3 diff = c2 - c1;
    return cdc::Dot3(diff, diff);
}
