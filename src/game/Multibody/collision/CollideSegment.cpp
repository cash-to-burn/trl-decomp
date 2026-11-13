#include "game/Multibody/collision/CollideSegment.h"
#include "game/collide.h"

#include <cfloat>
#include <cmath>

namespace {

constexpr float kTriangleGrow = 100.0f * FLT_EPSILON; // 0x37480000 (~1.1920929e-5f)

float VectorComponent(const cdc::Vector3& v, int axis)
{
    switch (axis) {
    case 0: return v.x;
    case 1: return v.y;
    default: return v.z;
    }
}

void SetVectorComponent(cdc::Vector3* v, int axis, float value)
{
    switch (axis) {
    case 0: v->x = value; break;
    case 1: v->y = value; break;
    default: v->z = value; break;
    }
}

} // namespace

// 0x004fd850 (PDB: 0x004ff550)
CollideCode CollideSegmentAndSphere(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    const MSphere& sphere)
{
    const cdc::Vector3 m = start - sphere.x;
    const float c = cdc::Dot3(m, m) - sphere.radius * sphere.radius;
    if (c <= 0.0f) {
        return HIT_INSIDE;
    }

    const float b = cdc::Dot3(m, dir);
    const float a = cdc::Dot3(dir, dir);
    const float discr = b * b - a * c;
    if (discr < 0.0f || a <= 0.0f) {
        return NO_HIT;
    }

    const float lambda = (-b - std::sqrt(discr)) / a;
    if (lambda < 0.0f || lambda > *t) {
        return NO_HIT;
    }

    *t = lambda;
    *normal = m + dir * lambda;
    normal->Normalize3();
    return HIT;
}

// 0x004fd9b0 (PDB: 0x004ff6b0)
CollideCode CollideSegmentAndTri(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    const MTriangle& tri)
{
    const cdc::Vector3 e1 = tri.v1 - tri.v0;
    const cdc::Vector3 e2 = tri.v2 - tri.v0;
    const cdc::Vector3 n = e1 % e2;
    const float denom = -cdc::Dot3(dir, n);
    if (denom <= kTriangleGrow) {
        return NO_HIT;
    }

    const cdc::Vector3 ap = start - tri.v0;
    const float numer = cdc::Dot3(ap, n);
    if (numer < 0.0f || numer > denom * (*t)) {
        return NO_HIT;
    }

    const cdc::Vector3 q = ap % dir;
    const float u = cdc::Dot3(e2, q);
    if (u < -kTriangleGrow * denom) {
        return NO_HIT;
    }

    const float v = -cdc::Dot3(e1, q);
    if (v < -kTriangleGrow * denom || (u + v) > denom * (1.0f + kTriangleGrow)) {
        return NO_HIT;
    }

    *t = numer / denom;
    *normal = n;
    normal->Normalize3();
    return HIT;
}

// 0x004fdb80 (PDB: 0x004ff880)
CollideCode CollideSegmentAndAlignedBox(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    const BBox& box)
{
    float tmin = 0.0f;
    float tmax = *t;
    cdc::Vector3 hitNormal = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    for (int axis = 0; axis < 3; ++axis) {
        const float d = VectorComponent(dir, axis);
        const float s = VectorComponent(start, axis);
        const float bmin = VectorComponent(box.bMin, axis);
        const float bmax = VectorComponent(box.bMax, axis);

        if (std::fabs(d) == 0.0f) {
            if (s < bmin || s > bmax) {
                return NO_HIT;
            }
            continue;
        }

        const float invD = 1.0f / d;
        float t1 = (bmin - s) * invD;
        float t2 = (bmax - s) * invD;
        float sign = -1.0f;
        if (t1 > t2) {
            const float tmp = t1;
            t1 = t2;
            t2 = tmp;
            sign = 1.0f;
        }

        if (t1 > tmin) {
            tmin = t1;
            hitNormal = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            SetVectorComponent(&hitNormal, axis, sign);
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax || tmax < 0.0f) {
            return NO_HIT;
        }
    }

    *t = tmin;
    if (tmin == 0.0f) {
        return HIT_INSIDE;
    }

    *normal = hitNormal;
    return HIT;
}

// 0x004fdd10 (PDB: 0x004ffa10)
CollideCode CollideSegmentAndBox(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    const MBox& box)
{
    const cdc::Quat qInv = ~box.q;
    const cdc::Vector3 localStart = qInv.Rotate(start - box.x);
    const cdc::Vector3 localDir = qInv.Rotate(dir);

    BBox localBox;
    localBox.bMin = box.h * -1.0f;
    localBox.bMax = box.h;

    const CollideCode result = CollideSegmentAndAlignedBox(t, normal, localStart, localDir, localBox);
    if (result == HIT) {
        *normal = box.q.Rotate(*normal);
    }
    return result;
}

// 0x004fde00 (PDB: 0x004ffb00)
CollideCode CollideSegmentAndCapsule(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float radius)
{
    cdc::Vector3 axis = capB - capA;
    const float len = std::sqrt(cdc::Dot3(axis, axis));
    if (len == 0.0f) {
        axis = cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
    } else {
        axis.Normalize3();
    }

    const cdc::Vector3 ao = start - capA;
    const float aoDotAxis = cdc::Dot3(ao, axis);
    const float dDotAxis = cdc::Dot3(dir, axis);
    const float endProj = aoDotAxis + (*t) * dDotAxis;

    if ((aoDotAxis < -radius && endProj < -radius) ||
        (aoDotAxis > len + radius && endProj > len + radius)) {
        return NO_HIT;
    }

    const float radSq = radius * radius;
    const float c = cdc::Dot3(ao, ao) - aoDotAxis * aoDotAxis - radSq;
    const float a = cdc::Dot3(dir, dir) - dDotAxis * dDotAxis;

    if (c <= 0.0f) {
        float clampedProj = aoDotAxis;
        if (clampedProj > len) {
            clampedProj = len;
        }
        if (clampedProj < 0.0f) {
            clampedProj = 0.0f;
        }

        const cdc::Vector3 delta = ao - axis * clampedProj;
        if (cdc::Dot3(delta, delta) <= radSq) {
            *t = 0.0f;
            return HIT_INSIDE;
        }

        MSphere capSphere;
        capSphere.x = capA + axis * clampedProj;
        capSphere.radius = radius;
        return CollideSegmentAndSphere(t, normal, start, dir, capSphere);
    }

    if (std::fabs(a) == 0.0f) {
        return NO_HIT;
    }

    const float b = cdc::Dot3(ao, dir) - aoDotAxis * dDotAxis;
    const float discr = b * b - a * c;
    if (discr < 0.0f) {
        return NO_HIT;
    }

    const float lambda = (-b - std::sqrt(discr)) / a;
    const float hitProj = aoDotAxis + lambda * dDotAxis;

    if (hitProj < 0.0f || hitProj > len) {
        MSphere capSphere;
        capSphere.x = (hitProj < 0.0f) ? capA : capB;
        capSphere.radius = radius;
        return CollideSegmentAndSphere(t, normal, start, dir, capSphere);
    }

    if (lambda < 0.0f || lambda > *t) {
        return NO_HIT;
    }

    *t = lambda;
    const cdc::Vector3 hitOffset = ao + dir * lambda;
    *normal = hitOffset - axis * hitProj;
    normal->Normalize3();
    return HIT;
}
