#include "game/Multibody/collision/CollideSweptSphere.h"

namespace {

// 0x004fe5c0 (PDB: 0x005002c0 / Corner)
cdc::Vector3 Corner(const BBox& box, unsigned int cornerIndex)
{
    return cdc::Vector3::Make(
        (cornerIndex & 1u) ? box.bMax.x : box.bMin.x,
        (cornerIndex & 2u) ? box.bMax.y : box.bMin.y,
        (cornerIndex & 4u) ? box.bMax.z : box.bMin.z);
}

} // namespace

// 0x004fe620 (PDB: 0x00500320)
CollideCode CollideSweptSphereAndTriangle(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphere,
    const cdc::Vector3& dir,
    const MTriangle& tri)
{
    if (cdc::Dot3(dir, dir) == 0.0f) {
        return NO_HIT;
    }

    const cdc::Vector3 e1 = tri.v1 - tri.v0;
    const cdc::Vector3 e2 = tri.v2 - tri.v0;
    cdc::Vector3 triNorm;
    cdc::Cross3(&triNorm, e1, e2);
    triNorm.Normalize3();

    const float planeOffset = cdc::Dot3(triNorm, tri.v0);
    const float d0 = cdc::Dot3(triNorm, sphere.x) - planeOffset;
    const float d1 = cdc::Dot3(triNorm, sphere.x + dir * (*t)) - planeOffset;
    if ((d0 > sphere.radius && d1 > sphere.radius)
        || (d0 < -sphere.radius && d1 < -sphere.radius)) {
        return NO_HIT;
    }

    const float denom = -cdc::Dot3(triNorm, dir);
    if (denom > 0.0f) {
        const cdc::Vector3 p = sphere.x - tri.v0 - triNorm * sphere.radius;
        const float numer = cdc::Dot3(triNorm, p);
        if (numer >= 0.0f && numer <= denom) {
            cdc::Vector3 pCrossDir;
            cdc::Cross3(&pCrossDir, p, dir);
            const float u = cdc::Dot3(e2, pCrossDir);
            const float v = -cdc::Dot3(e1, pCrossDir);
            if (u >= 0.0f && v >= 0.0f && (u + v) <= denom) {
                *t = numer / denom;
                *normal = triNorm;
                return HIT;
            }
        }
    }

    const cdc::Vector3* const edgeVerts[3][2] = {
        { &tri.v0, &tri.v1 },
        { &tri.v1, &tri.v2 },
        { &tri.v2, &tri.v0 },
    };

    CollideCode result = NO_HIT;
    for (int i = 0; i < 3; ++i) {
        const CollideCode edgeCode = CollideSegmentAndCapsule(
            t, normal, sphere.x, dir, *edgeVerts[i][0], *edgeVerts[i][1], sphere.radius);
        if (edgeCode == HIT) {
            result = HIT;
        } else if (edgeCode == HIT_INSIDE) {
            return HIT_INSIDE;
        }
    }
    return result;
}

// 0x004fe9d0 (PDB: 0x005006d0)
CollideCode CollideSweptSphereAndSphere(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphereA,
    const cdc::Vector3& dir,
    const MSphere& sphereB)
{
    MSphere combined = sphereB;
    combined.radius = sphereA.radius + sphereB.radius;
    return CollideSegmentAndSphere(t, normal, sphereA.x, dir, combined);
}

// 0x004fea30 (PDB: 0x00500730)
CollideCode CollideSweptSphereAndAlignedBox(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphere,
    const cdc::Vector3& dir,
    const BBox& box)
{
    const cdc::Vector3 radiusVec = cdc::Vector3::Make(sphere.radius, sphere.radius, sphere.radius);
    BBox expandedBox;
    expandedBox.bMin = box.bMin - radiusVec;
    expandedBox.bMax = box.bMax + radiusVec;

    float bestT = *t;
    cdc::Vector3 hitNormal;
    const CollideCode code = CollideSegmentAndAlignedBox(&bestT, &hitNormal, sphere.x, dir, expandedBox);
    if (code == NO_HIT) {
        return NO_HIT;
    }

    const cdc::Vector3 hitPoint = sphere.x + dir * bestT;
    unsigned int minMask = 0;
    unsigned int maxMask = 0;
    if (hitPoint.x < box.bMin.x) {
        minMask |= 1u;
    }
    if (hitPoint.x > box.bMax.x) {
        maxMask |= 1u;
    }
    if (hitPoint.y < box.bMin.y) {
        minMask |= 2u;
    }
    if (hitPoint.y > box.bMax.y) {
        maxMask |= 2u;
    }
    if (hitPoint.z < box.bMin.z) {
        minMask |= 4u;
    }
    if (hitPoint.z > box.bMax.z) {
        maxMask |= 4u;
    }

    const unsigned int combinedMask = minMask | maxMask;
    if (combinedMask == 0) {
        *t = 0.0f;
        return HIT_INSIDE;
    }

    if (combinedMask == 7u) {
        CollideCode edgeResult = NO_HIT;
        const cdc::Vector3 corner = Corner(box, maxMask);
        for (unsigned int axisBit = 1u; axisBit <= 4u; axisBit <<= 1u) {
            const CollideCode edgeCode = CollideSegmentAndCapsule(
                t, normal, sphere.x, dir, corner, Corner(box, maxMask ^ axisBit), sphere.radius);
            if (edgeCode == HIT) {
                edgeResult = HIT;
            } else if (edgeCode == HIT_INSIDE) {
                *t = 0.0f;
                return HIT_INSIDE;
            }
        }
        return edgeResult;
    }

    // Power-of-two mask corresponds to a box face Voronoi region
    if ((combinedMask & (combinedMask - 1u)) == 0) {
        *t = bestT;
        if (code == HIT) {
            *normal = hitNormal;
        }
        return code;
    }

    // Two bits set: Voronoi edge region
    return CollideSegmentAndCapsule(
        t, normal, sphere.x, dir, Corner(box, minMask ^ 7u), Corner(box, maxMask), sphere.radius);
}

// 0x004fed10 (PDB: 0x00500a10)
CollideCode CollideSweptSphereAndBox(
    float* t,
    cdc::Vector3* normal,
    const MSphere& sphere,
    const cdc::Vector3& dir,
    const OBox& box)
{
    MSphere localSphere;
    localSphere.x = cdc::Mul3x3(box.RT, sphere.x - box.x);
    localSphere.radius = sphere.radius;

    const cdc::Vector3 localDir = cdc::Mul3x3(box.RT, dir);

    BBox localBox;
    localBox.bMin = box.h * -1.0f;
    localBox.bMax = box.h;

    const CollideCode code = CollideSweptSphereAndAlignedBox(t, normal, localSphere, localDir, localBox);
    if (code == HIT) {
        *normal = cdc::Mul3x3(box.R, *normal);
    }
    return code;
}
