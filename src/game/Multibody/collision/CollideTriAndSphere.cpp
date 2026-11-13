#include "game/Multibody/collision/CollideTriAndSphere.h"

#include <cfloat>
#include <cmath>

namespace {

// PDB: TRIANGLE_GROW :: const float (0x00f05868)
constexpr float TRIANGLE_GROW = 100.0f * FLT_EPSILON;

cdc::Vector3 CrossProduct(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    cdc::Cross3(&out, a, b);
    return out;
}

int EmitSphereContact(
    CPoint* cp,
    const cdc::Vector3& contactPos,
    const cdc::Vector3& delta,
    float sphereRad,
    const cdc::Vector3& fallbackNormal)
{
    const float distSq = cdc::Dot3(delta, delta);
    if (distSq > sphereRad * sphereRad) {
        return 0;
    }

    cp->position = contactPos;
    cp->separation = std::sqrt(distSq) - sphereRad;
    cp->normal = (distSq > FLT_EPSILON) ? delta.UnitVec3() : fallbackNormal;
    return 1;
}

} // namespace

// 0x005012d0 (PDB: 0x00502fd0)
int CollideTriAndSphere(
    CPoint* cp,
    const cdc::Vector3& spherePos,
    float sphereRad,
    const MTriangle& tri,
    unsigned char adj)
{
    const cdc::Vector3 triGrowOffset = (tri.v0 + tri.v1 + tri.v2) * (TRIANGLE_GROW * (1.0f / 3.0f));
    const float growScale = 1.0f + TRIANGLE_GROW;

    const cdc::Vector3 gv[3] = {
        tri.v0 * growScale - triGrowOffset,
        tri.v1 * growScale - triGrowOffset,
        tri.v2 * growScale - triGrowOffset,
    };
    const cdc::Vector3 edges[3] = {
        gv[1] - gv[0],
        gv[2] - gv[1],
        gv[0] - gv[2],
    };

    cdc::Vector3 n = CrossProduct(edges[2], edges[0]);
    n.Normalize3();

    const float planeDist = cdc::Dot3(n, spherePos - gv[0]);
    if (planeDist < 0.0f || planeDist > sphereRad) {
        return 0;
    }

    const cdc::Vector3 diff[3] = {
        spherePos - gv[0],
        spherePos - gv[1],
        spherePos - gv[2],
    };
    const float dStart[3] = {
        cdc::Dot3(diff[0], edges[0]),
        cdc::Dot3(diff[1], edges[1]),
        cdc::Dot3(diff[2], edges[2]),
    };
    const float dEnd[3] = {
        cdc::Dot3(gv[1] - spherePos, edges[0]),
        cdc::Dot3(gv[2] - spherePos, edges[1]),
        cdc::Dot3(gv[0] - spherePos, edges[2]),
    };

    // Vertex Voronoi regions tested in retail order: v1, v2, v0
    const int vertexOrder[3] = { 1, 2, 0 };
    const unsigned char vertexAdjMask[3] = { 0x18, 0x30, 0x28 };
    for (int i = 0; i < 3; ++i) {
        const int vi = vertexOrder[i];
        const int prevEdge = (vi + 2) % 3;
        if (dEnd[prevEdge] <= 0.0f && dStart[vi] <= 0.0f) {
            if ((adj & vertexAdjMask[i]) == 0) {
                return 0;
            }
            return EmitSphereContact(cp, gv[vi], diff[vi], sphereRad, n);
        }
    }

    const float crossProj[3] = {
        cdc::Dot3(n, CrossProduct(edges[0], diff[0])),
        cdc::Dot3(n, CrossProduct(edges[1], diff[1])),
        cdc::Dot3(n, CrossProduct(edges[2], diff[2])),
    };

    // Edge Voronoi regions
    const unsigned char edgeAdjMask[3] = { 0x08, 0x10, 0x20 };
    for (int i = 0; i < 3; ++i) {
        if (crossProj[i] <= 0.0f && dStart[i] >= 0.0f && dEnd[i] >= 0.0f) {
            if ((adj & edgeAdjMask[i]) == 0) {
                return 0;
            }
            const float t = dStart[i] / (dEnd[i] + dStart[i]);
            const cdc::Vector3 closest = gv[i] + edges[i] * t;
            return EmitSphereContact(cp, closest, spherePos - closest, sphereRad, n);
        }
    }

    // Face interior contact using barycentric weights
    const float invSum = 1.0f / (crossProj[0] + crossProj[1] + crossProj[2]);
    const float u0 = crossProj[1] * invSum;
    const float u1 = crossProj[2] * invSum;
    const float u2 = 1.0f - u0 - u1;

    cp->position = gv[0] * u0 + gv[1] * u1 + gv[2] * u2;
    cp->normal = n;
    cp->separation = planeDist - sphereRad;
    return 1;
}
