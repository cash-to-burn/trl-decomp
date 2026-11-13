#include "game/Multibody/collision/CollideBoxAndCapsule.h"

#include <cfloat>
#include <cmath>

#include "game/Multibody/collision/CollideSphere.h"

namespace {

enum AxisType
{
    FACE_X = 0,
    FACE_Y = 1,
    FACE_Z = 2,
    EDGE_X = 3,
    EDGE_Y = 4,
    EDGE_Z = 5,
    ARVO_1 = 6,
    ARVO_2 = 7,
    VERT_1 = 8,
    VERT_2 = 9,
    VERT_3 = 10,
    VERT_4 = 11,
    VERT_5 = 12,
    VERT_6 = 13,
    VERT_7 = 14,
    VERT_8 = 15,
};

inline float SignFloat(float v)
{
    return (v < 0.0f) ? -1.0f : 1.0f;
}

inline cdc::Vector3 CrossVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    cdc::Cross3(&out, a, b);
    return out;
}

// 0x00501c40 (PDB: 0x004fd260 / LPROC CrossTrick)
inline cdc::Vector3 CrossTrick(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.y * b.z + a.z * b.y,
        a.z * b.x + a.x * b.z,
        a.x * b.y + a.y * b.x,
        0.0f);
}

inline cdc::Vector3 AbsVec3(const cdc::Vector3& v)
{
    return cdc::Vector3::Make(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z), 0.0f);
}

inline cdc::Vector3 NegVec3(const cdc::Vector3& v)
{
    return cdc::Vector3::Make(-v.x, -v.y, -v.z, 0.0f);
}

inline cdc::Vector3 MinVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.x < b.x ? a.x : b.x,
        a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z,
        0.0f);
}

inline cdc::Vector3 MaxVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return cdc::Vector3::Make(
        a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z,
        0.0f);
}

inline cdc::Vector3 ClampVec3(const cdc::Vector3& v, const cdc::Vector3& lo, const cdc::Vector3& hi)
{
    return MaxVec3(lo, MinVec3(v, hi));
}

inline float GetAxis(const cdc::Vector3& v, int axis)
{
    switch (axis) {
    case 0:
        return v.x;
    case 1:
        return v.y;
    default:
        return v.z;
    }
}

inline void SetAxis(cdc::Vector3& v, int axis, float val)
{
    switch (axis) {
    case 0:
        v.x = val;
        break;
    case 1:
        v.y = val;
        break;
    default:
        v.z = val;
        break;
    }
}

inline cdc::Vector3 UnitAxis(int axis, float scale)
{
    cdc::Vector3 v = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    SetAxis(v, axis, scale);
    return v;
}

// 0x004fb5c0 (PDB: 0x004fd2c0 / LPROC ComputeArvoContact)
static int ComputeArvoContact(
    CPoint* cp,
    const cdc::Vector3& ext,
    const cdc::Vector3& s0,
    const cdc::Vector3& s1,
    float r)
{
    const cdc::Vector3 negExt = NegVec3(ext);
    const cdc::Vector3 d0 = s0 - ClampVec3(s0, negExt, ext);
    const cdc::Vector3 d1 = s1 - ClampVec3(s1, negExt, ext);
    const float d0Sq = cdc::Dot3(d0, d0);
    const float d1Sq = cdc::Dot3(d1, d1);

    const cdc::Vector3 zero = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    cdc::Quat identity;
    identity.x = 0.0f;
    identity.y = 0.0f;
    identity.z = 0.0f;
    identity.w = 1.0f;
    const cdc::Vector3& closestEndpoint = (d0Sq < d1Sq) ? s0 : s1;

    const int numContacts = CollideSphereAndBox(cp, closestEndpoint, r, zero, identity, ext);
    for (int i = 0; i < numContacts; ++i) {
        cp[i].normal = NegVec3(cp[i].normal);
    }
    return numContacts;
}

// 0x004fb7a0 (PDB: 0x004fd4a0 / LPROC ComputeFaceContact)
static int ComputeFaceContact(
    CPoint* cp,
    const cdc::Vector3& ext,
    const cdc::Vector3& s0,
    const cdc::Vector3& s1,
    float r,
    const cdc::Vector3& n,
    AxisType axis)
{
    if (axis < FACE_X || axis > FACE_Z) {
        return 0;
    }

    static const int kTangentAxes[3][2] = {
        { 1, 2 },
        { 0, 2 },
        { 0, 1 },
    };

    const int axisAIdx = kTangentAxes[axis][0];
    const int axisBIdx = kTangentAxes[axis][1];
    const cdc::Vector3 axisA = UnitAxis(axisAIdx, 1.0f);
    const cdc::Vector3 axisB = UnitAxis(axisBIdx, 1.0f);
    const float extN = GetAxis(ext, axis);
    const float extA = GetAxis(ext, axisAIdx);
    const float extB = GetAxis(ext, axisBIdx);

    cdc::Vector3 seg[2] = { s0, s1 };
    cdc::Vector3 buf1[8];
    cdc::Vector3 buf2[8];
    int numVerts = ClipPolyToPlane(buf1, seg, 2, NegVec3(axisA), extA);
    numVerts = ClipPolyToPlane(buf2, buf1, numVerts, axisA, extA);
    numVerts = ClipPolyToPlane(buf1, buf2, numVerts, NegVec3(axisB), extB);
    numVerts = ClipPolyToPlane(buf2, buf1, numVerts, axisB, extB);

    int numContacts = 0;
    for (int i = 0; i < numVerts; ++i) {
        const float sep = cdc::Dot3(n, buf2[i]) - extN - r;
        if (sep <= 0.0f) {
            cp[numContacts].separation = sep;
            cp[numContacts].normal = n;
            cp[numContacts].position = buf2[i];
            SetAxis(cp[numContacts].position, axis, GetAxis(ext, axis) * GetAxis(n, axis));
            ++numContacts;
        }
    }

    if (numContacts == 0) {
        return ComputeArvoContact(cp, ext, s0, s1, r);
    }
    return numContacts;
}

// 0x004fbbb0 (PDB: 0x004fd8b0 / LPROC ComputeEdgeContact)
static int ComputeEdgeContact(
    CPoint* cp,
    const cdc::Vector3& ext,
    const cdc::Vector3& s0,
    const cdc::Vector3& s1,
    float r,
    const cdc::Vector3& n,
    float sep,
    AxisType axis)
{
    const cdc::Vector3 edgePoint = cdc::Vector3::Make(
        SignFloat(n.x) * ext.x,
        SignFloat(n.y) * ext.y,
        SignFloat(n.z) * ext.z,
        0.0f);
    const cdc::Vector3 edgeDir = UnitAxis(axis - EDGE_X, 1.0f);

    cdc::Vector3 segDir = s1 - s0;
    segDir.Normalize3();

    cp->separation = sep;
    cp->normal = n;
    const cdc::Vector3 mid = LineLineMidpoint(edgePoint, edgeDir, s0, segDir);
    cp->position = mid - n * (r * 0.5f);
    return 1;
}

struct BestAxisState
{
    float sep;
    float faceBestSep;
    cdc::Vector3 normal;
    AxisType axis;
};

static void TryCandidateAxis(
    BestAxisState& state,
    const cdc::Vector3& axisVec,
    float len,
    float rawSep,
    AxisType axisId)
{
    if (len > FLT_EPSILON &&
        (state.sep * len) < rawSep &&
        ((state.faceBestSep + 1.0f) * len) < (rawSep * 1.05f))
    {
        const float invLen = 1.0f / len;
        state.axis = axisId;
        state.sep = rawSep * invLen;
        state.normal = axisVec * invLen;
    }
}

} // namespace

// 0x004fbd40 (PDB: 0x004fda40 / ?CollideBoxAndCapsule@@YAHPAUCPoint@@ABVVector3@cdc@@ABVQuat@3@111M@Z)
int CollideBoxAndCapsule(
    CPoint* cp,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents,
    const cdc::Vector3& capStart,
    const cdc::Vector3& capEnd,
    float capRad)
{
    const cdc::Vector3 extPlusR = boxHalfExtents + cdc::Vector3::Make(capRad, capRad, capRad, 0.0f);
    const cdc::Quat invRot = ~boxRot;
    const cdc::Vector3 s0 = invRot.Rotate(capStart - boxCenter);
    const cdc::Vector3 s1 = invRot.Rotate(capEnd - boxCenter);

    const cdc::Vector3 segVec = s1 - s0;
    if (cdc::Dot3(segVec, segVec) <= FLT_EPSILON) {
        return CollideSphereAndBox(cp, capStart, capRad, boxCenter, boxRot, boxHalfExtents);
    }

    cdc::Vector3 segDir = segVec;
    segDir.Normalize3();

    const cdc::Vector3 segMax = MaxVec3(s0, s1);
    const cdc::Vector3 segMin = MinVec3(s0, s1);
    const cdc::Vector3 dNeg = NegVec3(extPlusR) - segMax;
    const cdc::Vector3 dPos = segMin - extPlusR;
    const cdc::Vector3 sepFace = MaxVec3(dNeg, dPos);
    if (sepFace.x > 0.0f || sepFace.y > 0.0f || sepFace.z > 0.0f) {
        return 0;
    }

    const cdc::Vector3 segCrossLen = cdc::Vector3::Make(
        std::sqrt(segDir.y * segDir.y + segDir.z * segDir.z),
        std::sqrt(segDir.x * segDir.x + segDir.z * segDir.z),
        std::sqrt(segDir.x * segDir.x + segDir.y * segDir.y),
        0.0f);
    const cdc::Vector3 boxProjEdge = CrossTrick(boxHalfExtents, AbsVec3(segDir));
    const cdc::Vector3 segDistEdge = AbsVec3(CrossVec3(segDir, s0));
    const cdc::Vector3 sepEdge = segDistEdge - boxProjEdge - (segCrossLen * capRad);
    if (sepEdge.x > 0.0f || sepEdge.y > 0.0f || sepEdge.z > 0.0f) {
        return 0;
    }

    const cdc::Vector3 negExt = NegVec3(boxHalfExtents);
    const cdc::Vector3 arvoVec[2] = {
        s0 - ClampVec3(s0, negExt, boxHalfExtents),
        s1 - ClampVec3(s1, negExt, boxHalfExtents),
    };
    float arvoLen[2];
    float sepArvo[2];
    for (int i = 0; i < 2; ++i) {
        arvoLen[i] = arvoVec[i].Len3();
        const float dot0 = cdc::Dot3(arvoVec[i], s0);
        const float dot1 = cdc::Dot3(arvoVec[i], s1);
        const float minDot = (dot0 < dot1) ? dot0 : dot1;
        sepArvo[i] = minDot - cdc::Dot3(boxHalfExtents, AbsVec3(arvoVec[i])) - arvoLen[i] * capRad;
        if (sepArvo[i] > 0.0f) {
            return 0;
        }
    }

    cdc::Vector3 verts[8];
    cdc::Vector3 vertNormals[8];
    float vertNormLens[8];
    float vertSeps[8];
    for (int i = 0; i < 8; ++i) {
        verts[i] = cdc::Vector3::Make(
            (i & 1) ? boxHalfExtents.x : -boxHalfExtents.x,
            (i & 2) ? boxHalfExtents.y : -boxHalfExtents.y,
            (i & 4) ? boxHalfExtents.z : -boxHalfExtents.z,
            0.0f);
        vertNormals[i] = CrossVec3(segDir, CrossVec3(s0 - verts[i], segDir));
        vertNormLens[i] = vertNormals[i].Len3();
        vertSeps[i] = cdc::Dot3(vertNormals[i], s0)
            - cdc::Dot3(boxHalfExtents, AbsVec3(vertNormals[i]))
            - vertNormLens[i] * capRad;
        if (vertSeps[i] > 0.0f) {
            return 0;
        }
    }

    BestAxisState best;
    best.axis = FACE_X;
    best.sep = sepFace.x;
    best.normal = UnitAxis(0, (dPos.x <= dNeg.x) ? -1.0f : 1.0f);
    for (int axisIdx = 1; axisIdx < 3; ++axisIdx) {
        const float fSep = GetAxis(sepFace, axisIdx);
        if (fSep > best.sep) {
            best.sep = fSep;
            best.axis = static_cast<AxisType>(FACE_X + axisIdx);
            best.normal = UnitAxis(
                axisIdx,
                (GetAxis(dPos, axisIdx) <= GetAxis(dNeg, axisIdx)) ? -1.0f : 1.0f);
        }
    }
    best.faceBestSep = best.sep;

    const cdc::Vector3 edgeAxes[3] = {
        cdc::Vector3::Make(0.0f, -segDir.z, segDir.y, 0.0f),
        cdc::Vector3::Make(segDir.z, 0.0f, -segDir.x, 0.0f),
        cdc::Vector3::Make(-segDir.y, segDir.x, 0.0f, 0.0f),
    };
    for (int i = 0; i < 3; ++i) {
        const float len = edgeAxes[i].Len3();
        const float rawSep = GetAxis(sepEdge, i);
        const cdc::Vector3 signedVec = edgeAxes[i] * SignFloat(cdc::Dot3(edgeAxes[i], s0));
        TryCandidateAxis(best, signedVec, len, rawSep, static_cast<AxisType>(EDGE_X + i));
    }

    for (int i = 0; i < 2; ++i) {
        TryCandidateAxis(best, arvoVec[i], arvoLen[i], sepArvo[i], static_cast<AxisType>(ARVO_1 + i));
    }

    for (int i = 0; i < 8; ++i) {
        TryCandidateAxis(best, vertNormals[i], vertNormLens[i], vertSeps[i], static_cast<AxisType>(VERT_1 + i));
    }

    int numContacts = 0;
    if (best.axis >= FACE_X && best.axis <= FACE_Z) {
        numContacts = ComputeFaceContact(cp, boxHalfExtents, s0, s1, capRad, best.normal, best.axis);
    } else if (best.axis >= EDGE_X && best.axis <= EDGE_Z) {
        numContacts = ComputeEdgeContact(cp, boxHalfExtents, s0, s1, capRad, best.normal, best.sep, best.axis);
    } else if (best.axis == ARVO_1 || best.axis == ARVO_2) {
        numContacts = ComputeArvoContact(cp, boxHalfExtents, s0, s1, capRad);
    } else if (best.axis >= VERT_1 && best.axis <= VERT_8) {
        cp[0].normal = best.normal;
        cp[0].separation = best.sep;
        cp[0].position = verts[best.axis - VERT_1];
        numContacts = 1;
    }

    for (int i = 0; i < numContacts; ++i) {
        cp[i].position = boxCenter + boxRot.Rotate(cp[i].position);
        cp[i].normal = boxRot.Rotate(cp[i].normal);
    }

    return numContacts;
}
