#include "game/Multibody/collision/CollideTriAndCapsule.h"
#include "game/Multibody/collision/CollideTriAndSphere.h"

#include <cfloat>
#include <cmath>

namespace {

// PDB: TRIANGLE_GROW :: const float (0x00f05848)
constexpr float TRIANGLE_GROW = 100.0f * FLT_EPSILON;

// File-scope diagnostic globals present in retail Mod 269
unsigned char gadj;       // 0x01116150 (PDB: 0x01117300)
cdc::Vector3 gv0;         // 0x01116160 (PDB: 0x01117310)
cdc::Vector3 gs;          // 0x01116170 (PDB: 0x01117320)
cdc::Vector3 gv2;         // 0x01116180 (PDB: 0x01117330)
cdc::Vector3 gt;          // 0x01116190 (PDB: 0x01117340)
cdc::Vector3 gv1;         // 0x011161a0 (PDB: 0x01117350)

cdc::Vector3 CrossProduct(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    cdc::Cross3(&out, a, b);
    return out;
}

cdc::Vector3 MinVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.x < b.x ? a.x : b.x,
        a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z);
}

cdc::Vector3 MaxVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z);
}

bool IsPointInsideTriangle(
    const cdc::Vector3& p,
    const cdc::Vector3* gv,
    const cdc::Vector3* edges,
    const cdc::Vector3& normal)
{
    return cdc::Dot3(normal, CrossProduct(edges[0], p - gv[0])) > 0.0f
        && cdc::Dot3(normal, CrossProduct(edges[1], p - gv[1])) > 0.0f
        && cdc::Dot3(normal, CrossProduct(edges[2], p - gv[2])) > 0.0f;
}

void EmitCapsuleFaceContact(
    CPoint* cp,
    const cdc::Vector3& capEnd,
    const cdc::Vector3& normal,
    float capRad,
    float separation)
{
    cp->separation = separation;
    cp->normal = normal;
    cp->position = capEnd - normal * capRad;
}

} // namespace

// 0x005009b0 (PDB: 0x005026b0)
int ComputeEdgeContact(
    CPoint* cp,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    const cdc::Vector3& edgeA,
    const cdc::Vector3& edgeB,
    const cdc::Vector3& oppVert,
    const cdc::Vector3& triNormal)
{
    (void)oppVert;

    cdc::Vector3 pEdge;
    cdc::Vector3 pCap;
    const float distSq = SegmentToSegmentDistance(&pEdge, &pCap, edgeA, edgeB, capA, capB);
    if (distSq > capRad * capRad) {
        return 0;
    }

    cdc::Vector3 normal = pCap - pEdge;
    if (cdc::Dot3(normal, normal) < FLT_EPSILON) {
        normal = triNormal;
    }
    normal.Normalize3();

    cp->normal = normal;
    cp->position = pCap - normal * capRad;
    cp->separation = std::sqrt(distSq) - capRad;
    return 1;
}

// 0x00500ac0 (PDB: 0x005027c0)
int CollideTriAndCapsule(
    CPoint* cp,
    int maxContacts,
    const MTriangle& tri,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    unsigned char adj)
{
    cdc::Vector3 capAxis = capB - capA;
    if (cdc::Dot3(capAxis, capAxis) <= FLT_EPSILON) {
        return CollideTriAndSphere(cp, capA, capRad, tri, adj);
    }
    capAxis.Normalize3();

    gadj = adj;
    gs = capA;
    gt = capB;
    gv0 = tri.v0;
    gv1 = tri.v1;
    gv2 = tri.v2;

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
    if (MultibodyCollision::IsZeroVector(n)) {
        return 0;
    }
    n.Normalize3();

    const cdc::Vector3 capMid = (capA + capB) * 0.5f;
    if (cdc::Dot3(n, capMid - gv[0]) < 0.0f) {
        return 0;
    }

    // Expanded AABB overlap test between capsule and grown triangle
    const cdc::Vector3 rVec = cdc::Vector3::Make(capRad, capRad, capRad);
    const cdc::Vector3 triMin = MinVec3(gv[0], MinVec3(gv[1], gv[2]));
    const cdc::Vector3 triMax = MaxVec3(gv[0], MaxVec3(gv[1], gv[2]));
    const cdc::Vector3 capMin = MinVec3(capA, capB);
    const cdc::Vector3 capMax = MaxVec3(capA, capB);
    const cdc::Vector3 axisSep = MaxVec3(triMin - (capMax + rVec), (capMin - rVec) - triMax);
    if (axisSep.x > 0.0f || axisSep.y > 0.0f || axisSep.z > 0.0f) {
        return 0;
    }

    const float sepA = cdc::Dot3(n, capA - gv[0]) - capRad;
    const float sepB = cdc::Dot3(n, capB - gv[0]) - capRad;
    const float minSep = sepA < sepB ? sepA : sepB;
    if (minSep > 0.0f) {
        return 0;
    }

    const bool insideA = IsPointInsideTriangle(capA, gv, edges, n);
    const bool insideB = IsPointInsideTriangle(capB, gv, edges, n);

    int numContacts = 0;
    if (insideA && sepA <= 0.0f) {
        EmitCapsuleFaceContact(&cp[numContacts], capA, n, capRad, sepA);
        ++numContacts;
    }
    if (numContacts == maxContacts) {
        return numContacts;
    }

    if (insideB && sepB <= 0.0f) {
        EmitCapsuleFaceContact(&cp[numContacts], capB, n, capRad, sepB);
        ++numContacts;
    }
    if ((insideA && insideB) || numContacts == maxContacts) {
        return numContacts;
    }

    const unsigned char edgeAdjMask[3] = { 0x08, 0x10, 0x20 };
    for (int i = 0; i < 3; ++i) {
        if ((adj & edgeAdjMask[i]) != 0) {
            numContacts += ComputeEdgeContact(
                &cp[numContacts],
                capA,
                capB,
                capRad,
                gv[i],
                gv[(i + 1) % 3],
                gv[(i + 2) % 3],
                n);
        }
        if (numContacts == 2 || numContacts == maxContacts) {
            return numContacts;
        }
    }

    return numContacts;
}
