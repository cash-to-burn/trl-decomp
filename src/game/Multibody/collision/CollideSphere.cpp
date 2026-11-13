#include "game/Multibody/collision/CollideSphere.h"

#include <cmath>

namespace {

void SetLocalAxisNormal(cdc::Vector3* out, int axis, float value)
{
    *out = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    switch (axis) {
    case 0:  out->x = value; break;
    case 1:  out->y = value; break;
    default: out->z = value; break;
    }
}

float AxisValue(const cdc::Vector3& v, int axis)
{
    switch (axis) {
    case 0:  return v.x;
    case 1:  return v.y;
    default: return v.z;
    }
}

} // namespace

// 0x004fe270 (PDB: 0x004fff70)
int CollideSphereAndSphere(
    CPoint* cp,
    const cdc::Vector3& posA,
    float radA,
    const cdc::Vector3& posB,
    float radB)
{
    const cdc::Vector3 diff = posB - posA;
    const float distSq = cdc::Dot3(diff, diff);
    return MultibodyCollision::ResolveSpherePairContact(cp, posA, radA, posB, radB, distSq);
}

// 0x004fe3a0 (PDB: 0x005000a0)
int CollideSphereAndBox(
    CPoint* cp,
    const cdc::Vector3& spherePos,
    float sphereRad,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents)
{
    const cdc::Quat qInv = ~boxRot;
    const cdc::Vector3 localPos = qInv.Rotate(spherePos - boxCenter);
    const cdc::Vector3 clamped = cdc::Vector3::Make(
        MultibodyCollision::Clamp(localPos.x, -boxHalfExtents.x, boxHalfExtents.x),
        MultibodyCollision::Clamp(localPos.y, -boxHalfExtents.y, boxHalfExtents.y),
        MultibodyCollision::Clamp(localPos.z, -boxHalfExtents.z, boxHalfExtents.z),
        0.0f);

    cdc::Vector3 delta = clamped - localPos;
    const float distSq = cdc::Dot3(delta, delta);
    if (distSq > sphereRad * sphereRad) {
        return 0;
    }

    if (MultibodyCollision::IsZeroVector(delta)) {
        const cdc::Vector3 pen = boxHalfExtents - MultibodyCollision::AbsVector3(localPos);
        int bestAxis = 0;
        float minPen = pen.x;
        if (pen.y < minPen) {
            minPen = pen.y;
            bestAxis = 1;
        }
        if (pen.z < minPen) {
            minPen = pen.z;
            bestAxis = 2;
        }

        const float sign = (AxisValue(localPos, bestAxis) >= 0.0f) ? 1.0f : -1.0f;
        cdc::Vector3 localNorm;
        SetLocalAxisNormal(&localNorm, bestAxis, -sign);
        cp->normal = boxRot.Rotate(localNorm);
        cp->position = spherePos;
        cp->separation = -minPen - sphereRad;
        return 1;
    }

    cp->normal = boxRot.Rotate(delta);
    cp->normal.Normalize3();
    cp->separation = std::sqrt(distSq) - sphereRad;
    cp->position = boxCenter + boxRot.Rotate(clamped);
    return 1;
}
