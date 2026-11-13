#include "game/Multibody/collision/CollideTriAndBox.h"

#include <cfloat>
#include <cstdint>

namespace {

// 0x004fee40 (PDB: 0x00500b40 / CrossTrick)
cdc::Vector3 CrossTrick(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.y * b.z + a.z * b.y,
        a.z * b.x + a.x * b.z,
        a.x * b.y + a.y * b.x);
}

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

bool PassesTriAdjacencyFilter(uint8_t adj, int16_t e0, int16_t e1)
{
    if (e0 > 0) {
        if (e1 > 0) {
            return (adj & static_cast<uint8_t>(e0 | e1)) != 0;
        }
        return (adj & static_cast<uint8_t>(e0)) != 0;
    }
    if (e1 > 0) {
        return (adj & static_cast<uint8_t>(e1)) != 0;
    }
    return true;
}

struct BoxFaceVertexSpec {
    float sx;
    float sy;
    float sz;
    int16_t edge0;
    int16_t edge1;
};

} // namespace

// 0x004feea0 (PDB: 0x00500ba0)
int TriAndBox::ComputeBoxFaceContact(CPoint* cp, int maxContacts, AxisType faceAxis)
{
    cdc::Vector3 uAxis;
    cdc::Vector3 vAxis;
    float faceExtent = 0.0f;
    float uExtent = 0.0f;
    float vExtent = 0.0f;
    short clipEdges[4] = {};

    if (faceAxis == BoxFaceX) {
        uAxis = cdc::Vector3::Make(0.0f, 1.0f, 0.0f);
        vAxis = cdc::Vector3::Make(0.0f, 0.0f, 1.0f);
        faceExtent = h.x;
        uExtent = h.y;
        vExtent = h.z;
        const short posSet[4] = { BOX_EDGE_NX_PY, BOX_EDGE_NX_NY, BOX_EDGE_NX_PZ, BOX_EDGE_NX_NZ };
        const short negSet[4] = { BOX_EDGE_PX_PY, BOX_EDGE_PX_NY, BOX_EDGE_PX_PZ, BOX_EDGE_PX_NZ };
        const short* src = (normal.x > 0.0f) ? posSet : negSet;
        for (int i = 0; i < 4; ++i) {
            clipEdges[i] = src[i];
        }
    } else if (faceAxis == BoxFaceY) {
        uAxis = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
        vAxis = cdc::Vector3::Make(0.0f, 0.0f, 1.0f);
        faceExtent = h.y;
        uExtent = h.x;
        vExtent = h.z;
        const short posSet[4] = { BOX_EDGE_PX_NY, BOX_EDGE_NX_NY, BOX_EDGE_NY_PZ, BOX_EDGE_NY_NZ };
        const short negSet[4] = { BOX_EDGE_PX_PY, BOX_EDGE_NX_PY, BOX_EDGE_PY_PZ, BOX_EDGE_PY_NZ };
        const short* src = (normal.y > 0.0f) ? posSet : negSet;
        for (int i = 0; i < 4; ++i) {
            clipEdges[i] = src[i];
        }
    } else {
        uAxis = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
        vAxis = cdc::Vector3::Make(0.0f, 1.0f, 0.0f);
        faceExtent = h.z;
        uExtent = h.x;
        vExtent = h.y;
        const short posSet[4] = { BOX_EDGE_PY_NZ, BOX_EDGE_NY_NZ, BOX_EDGE_PX_NZ, BOX_EDGE_NX_NZ };
        const short negSet[4] = { BOX_EDGE_PY_PZ, BOX_EDGE_NY_PZ, BOX_EDGE_PX_PZ, BOX_EDGE_NX_PZ };
        const short* src = (normal.z > 0.0f) ? posSet : negSet;
        for (int i = 0; i < 4; ++i) {
            clipEdges[i] = src[i];
        }
    }

    ClipPoint triPoly[3];
    const cdc::Vector3* verts[3] = { &v1, &v2, &v3 };
    const int16_t edgePairs[3][2] = {
        { TRI_EDGE2, TRI_EDGE0 },
        { TRI_EDGE0, TRI_EDGE1 },
        { TRI_EDGE1, TRI_EDGE2 },
    };
    for (int i = 0; i < 3; ++i) {
        triPoly[i].p = *verts[i];
        triPoly[i].edge0 = edgePairs[i][0];
        triPoly[i].edge1 = edgePairs[i][1];
        triPoly[i].clipEdge0 = -1;
        triPoly[i].clipEdge1 = -1;
    }

    ClipPoint polyA[10];
    ClipPoint polyB[10];
    int count = ClipPolyToPlane(polyA, triPoly, 3, uAxis, uExtent, clipEdges[0]);
    count = ClipPolyToPlane(polyB, polyA, count, uAxis * -1.0f, uExtent, clipEdges[1]);
    count = ClipPolyToPlane(polyA, polyB, count, vAxis, vExtent, clipEdges[2]);
    count = ClipPolyToPlane(polyB, polyA, count, vAxis * -1.0f, vExtent, clipEdges[3]);

    int numContacts = 0;
    for (int i = 0; i < count && numContacts < maxContacts; ++i) {
        const float sep = -cdc::Dot3(normal, polyB[i].p) - faceExtent;
        if (sep > 0.0f) {
            continue;
        }
        if (!PassesTriAdjacencyFilter(adjacencyFlags, polyB[i].edge0, polyB[i].edge1)) {
            continue;
        }

        cp[numContacts].position = polyB[i].p;
        cp[numContacts].normal = normal;
        cp[numContacts].separation = sep;
        ++numContacts;
    }
    return numContacts;
}

// 0x004ff620 (PDB: 0x00501320)
void TriAndBox::BuildBoxPolygon(ClipPoint* poly)
{
    static const BoxFaceVertexSpec kFaceSpecs[6][4] = {
        // 0: -X face (normal.x >= 0)
        {
            { -1.0f, -1.0f, -1.0f, BOX_EDGE_NX_NZ, BOX_EDGE_NX_NY },
            { -1.0f, -1.0f,  1.0f, BOX_EDGE_NX_NY, BOX_EDGE_NX_PZ },
            { -1.0f,  1.0f,  1.0f, BOX_EDGE_NX_PZ, BOX_EDGE_NX_PY },
            { -1.0f,  1.0f, -1.0f, BOX_EDGE_NX_PY, BOX_EDGE_NX_NZ },
        },
        // 1: +X face (normal.x < 0)
        {
            {  1.0f, -1.0f, -1.0f, BOX_EDGE_PX_NY, BOX_EDGE_PX_NZ },
            {  1.0f,  1.0f, -1.0f, BOX_EDGE_PX_NZ, BOX_EDGE_PX_PY },
            {  1.0f,  1.0f,  1.0f, BOX_EDGE_PX_PY, BOX_EDGE_PX_PZ },
            {  1.0f, -1.0f,  1.0f, BOX_EDGE_PX_PZ, BOX_EDGE_PX_NY },
        },
        // 2: -Y face (normal.y >= 0)
        {
            { -1.0f, -1.0f, -1.0f, BOX_EDGE_NX_NY, BOX_EDGE_NY_NZ },
            {  1.0f, -1.0f, -1.0f, BOX_EDGE_NY_NZ, BOX_EDGE_PX_NY },
            {  1.0f, -1.0f,  1.0f, BOX_EDGE_PX_NY, BOX_EDGE_NY_PZ },
            { -1.0f, -1.0f,  1.0f, BOX_EDGE_NY_PZ, BOX_EDGE_NX_NY },
        },
        // 3: +Y face (normal.y < 0)
        {
            { -1.0f,  1.0f, -1.0f, BOX_EDGE_PY_NZ, BOX_EDGE_NX_PY },
            { -1.0f,  1.0f,  1.0f, BOX_EDGE_NX_PY, BOX_EDGE_PY_PZ },
            {  1.0f,  1.0f,  1.0f, BOX_EDGE_PY_PZ, BOX_EDGE_PX_PY },
            {  1.0f,  1.0f, -1.0f, BOX_EDGE_PX_PY, BOX_EDGE_PY_NZ },
        },
        // 4: -Z face (normal.z >= 0)
        {
            { -1.0f, -1.0f, -1.0f, BOX_EDGE_NY_NZ, BOX_EDGE_NX_NZ },
            { -1.0f,  1.0f, -1.0f, BOX_EDGE_NX_NZ, BOX_EDGE_PY_NZ },
            {  1.0f,  1.0f, -1.0f, BOX_EDGE_PY_NZ, BOX_EDGE_PX_NZ },
            {  1.0f, -1.0f, -1.0f, BOX_EDGE_PX_NZ, BOX_EDGE_NY_NZ },
        },
        // 5: +Z face (normal.z < 0)
        {
            { -1.0f, -1.0f,  1.0f, BOX_EDGE_NX_PZ, BOX_EDGE_NY_PZ },
            {  1.0f, -1.0f,  1.0f, BOX_EDGE_NY_PZ, BOX_EDGE_PX_PZ },
            {  1.0f,  1.0f,  1.0f, BOX_EDGE_PX_PZ, BOX_EDGE_PY_PZ },
            { -1.0f,  1.0f,  1.0f, BOX_EDGE_PY_PZ, BOX_EDGE_NX_PZ },
        },
    };

    const cdc::Vector3 absN = MultibodyCollision::AbsVector3(normal);
    int specIdx = 0;
    if (absN.x > absN.y && absN.x > absN.z) {
        specIdx = (normal.x >= 0.0f) ? 0 : 1;
    } else if (absN.y > absN.z) {
        specIdx = (normal.y >= 0.0f) ? 2 : 3;
    } else {
        specIdx = (normal.z >= 0.0f) ? 4 : 5;
    }

    for (int i = 0; i < 4; ++i) {
        const BoxFaceVertexSpec& s = kFaceSpecs[specIdx][i];
        poly[i].p = cdc::Vector3::Make(s.sx * h.x, s.sy * h.y, s.sz * h.z);
        poly[i].edge0 = s.edge0;
        poly[i].edge1 = s.edge1;
    }
}

// 0x004ffad0 (PDB: 0x005017d0)
int TriAndBox::ComputeTriFaceContact(CPoint* cp, int maxContacts)
{
    ClipPoint boxPoly[4];
    for (int i = 0; i < 4; ++i) {
        boxPoly[i].edge0 = -1;
        boxPoly[i].edge1 = -1;
        boxPoly[i].clipEdge0 = -1;
        boxPoly[i].clipEdge1 = -1;
    }
    BuildBoxPolygon(boxPoly);

    ClipPoint polyA[10];
    ClipPoint polyB[10];
    for (int i = 0; i < 10; ++i) {
        polyA[i].edge0 = -1;
        polyA[i].edge1 = -1;
        polyA[i].clipEdge0 = -1;
        polyA[i].clipEdge1 = -1;
        polyB[i].edge0 = -1;
        polyB[i].edge1 = -1;
        polyB[i].clipEdge0 = -1;
        polyB[i].clipEdge1 = -1;
    }

    const cdc::Vector3* triVerts[3] = { &v1, &v2, &v3 };
    const cdc::Vector3* triEdges[3] = { &d1, &d2, &d3 };
    const short triEdgeIds[3] = { TRI_EDGE0, TRI_EDGE1, TRI_EDGE2 };

    const ClipPoint* src = boxPoly;
    int count = 4;
    for (int i = 0; i < 3; ++i) {
        cdc::Vector3 edgePlaneNorm = CrossProduct(*triEdges[i], normal);
        edgePlaneNorm.Normalize3();
        const float edgePlaneDist = cdc::Dot3(edgePlaneNorm, *triVerts[i]);
        ClipPoint* dst = (i % 2 == 0) ? polyA : polyB;
        count = ClipPolyToPlane(dst, src, count, edgePlaneNorm, edgePlaneDist, triEdgeIds[i]);
        src = dst;
    }

    int numContacts = 0;
    for (int i = 0; i < count && numContacts < maxContacts; ++i) {
        const float d = cdc::Dot3(normal, src[i].p - v1);
        if (d > 0.0f) {
            continue;
        }
        if (!PassesTriAdjacencyFilter(adjacencyFlags, src[i].clipEdge0, src[i].clipEdge1)) {
            continue;
        }

        cp[numContacts].position = src[i].p - normal * separation;
        cp[numContacts].normal = normal;
        cp[numContacts].separation = d;
        ++numContacts;
    }
    return numContacts;
}

// 0x00500000 (PDB: 0x00501d00)
int TriAndBox::Collide(CPoint* cp, int maxContacts, const OBox* obox, const MTriangle* tri, unsigned char adj)
{
    box = obox;
    v1 = cdc::Mul3x3(obox->RT, tri->v0 - obox->x);
    v2 = cdc::Mul3x3(obox->RT, tri->v1 - obox->x);
    v3 = cdc::Mul3x3(obox->RT, tri->v2 - obox->x);

    d1 = v2 - v1;
    d2 = v3 - v2;
    d3 = v1 - v3;

    triNormal = CrossProduct(d3, d1);
    if (MultibodyCollision::IsZeroVector(triNormal)) {
        return 0;
    }
    triNormal.Normalize3();

    const float planeD = -cdc::Dot3(triNormal, v1);
    if (planeD < 0.0f) {
        return 0;
    }

    h = obox->h;
    const float absD = planeD < 0.0f ? -planeD : planeD;
    const float boxProj = cdc::Dot3(MultibodyCollision::AbsVector3(triNormal), h);
    const float triFaceSep = absD - boxProj;
    if (triFaceSep > 0.0f) {
        return 0;
    }

    const cdc::Vector3 triMin = MinVec3(v1, MinVec3(v2, v3));
    const cdc::Vector3 triMax = MaxVec3(v1, MaxVec3(v2, v3));
    const cdc::Vector3 negDiff = (h * -1.0f) - triMax;
    const cdc::Vector3 posDiff = triMin - h;
    const cdc::Vector3 boxFaceSep = MaxVec3(negDiff, posDiff);
    if (boxFaceSep.x > 0.0f || boxFaceSep.y > 0.0f || boxFaceSep.z > 0.0f) {
        return 0;
    }

    // Cross-axis SAT rejection for triangle edges vs box cardinal axes
    const cdc::Vector3 epsH = h * FLT_EPSILON;
    const cdc::Vector3 triHalfVec = MultibodyCollision::AbsVector3(CrossProduct(v1 - v3, d1)) * 0.5f;

    const cdc::Vector3 edgeSum[3] = { v1 + v3, v1 + v2, v2 + v3 };
    const cdc::Vector3* edgeVecs[3] = { &d1, &d2, &d3 };
    for (int i = 0; i < 3; ++i) {
        const cdc::Vector3 absMid = MultibodyCollision::AbsVector3(CrossProduct(edgeSum[i], *edgeVecs[i])) * 0.5f;
        const cdc::Vector3 boxRad = CrossTrick(h, MultibodyCollision::AbsVector3(*edgeVecs[i]));
        const cdc::Vector3 edgeSep = absMid - triHalfVec - boxRad;
        if (edgeSep.x > epsH.x || edgeSep.y > epsH.y || edgeSep.z > epsH.z) {
            return 0;
        }
    }

    separation = triFaceSep;
    normal = triNormal;
    axis = TriFace;
    adjacencyFlags = adj;

    const float boxFaceVals[3] = { boxFaceSep.x, boxFaceSep.y, boxFaceSep.z };
    const float posDiffVals[3] = { posDiff.x, posDiff.y, posDiff.z };
    const float negDiffVals[3] = { negDiff.x, negDiff.y, negDiff.z };
    const AxisType boxAxes[3] = { BoxFaceX, BoxFaceY, BoxFaceZ };

    for (int i = 0; i < 3; ++i) {
        if (boxFaceVals[i] > separation && (boxFaceVals[i] * 1.2f > triFaceSep + 1.0f)) {
            const float sign = (posDiffVals[i] > negDiffVals[i]) ? -1.0f : 1.0f;
            const cdc::Vector3 candidate = cdc::Vector3::Make(
                i == 0 ? sign : 0.0f,
                i == 1 ? sign : 0.0f,
                i == 2 ? sign : 0.0f);
            if (cdc::Dot3(candidate, triNormal) >= -0.1f) {
                normal = candidate;
                axis = boxAxes[i];
                separation = boxFaceVals[i];
            }
        }
    }

    int numContacts = 0;
    if (axis != TriFace) {
        numContacts = ComputeBoxFaceContact(cp, maxContacts, axis);
        if (numContacts == 0) {
            normal = triNormal;
            separation = triFaceSep;
            axis = TriFace;
            numContacts = ComputeTriFaceContact(cp, maxContacts);
        }
    } else {
        numContacts = ComputeTriFaceContact(cp, maxContacts);
    }

    for (int i = 0; i < numContacts; ++i) {
        cp[i].position = cdc::Mul3x3(box->R, cp[i].position) + box->x;
        cp[i].normal = cdc::Mul3x3(box->R, cp[i].normal);
    }
    return numContacts;
}

// 0x00500980 (PDB: 0x00502680)
int CollideTriAndBox(
    CPoint* cp,
    int maxContacts,
    const OBox& box,
    const MTriangle& tri,
    unsigned char adj)
{
    TriAndBox triAndBox;
    return triAndBox.Collide(cp, maxContacts, &box, &tri, adj);
}
