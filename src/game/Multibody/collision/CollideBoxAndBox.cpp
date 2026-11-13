#include "game/Multibody/collision/CollideBoxAndBox.h"

#include <cfloat>
#include <cmath>
#include <cstring>

namespace {

// PDB: BoxAndBoxData (size 0xc0)
struct alignas(16) BoxAndBoxData
{
    cdc::Vector3 hA;    // +0x00
    cdc::Vector3 hB;    // +0x10
    cdc::Vector3 posA;  // +0x20
    cdc::Vector3 posB;  // +0x30
    cdc::Matrix RotA;   // +0x40
    cdc::Matrix RotB;   // +0x80
};

// PDB: CollideBoxAndBox.obj AxisType
enum AxisType
{
    NoAxis = 0,
    Face_A_X = 1,
    Face_A_Y = 2,
    Face_A_Z = 3,
    Face_B_X = 4,
    Face_B_Y = 5,
    Face_B_Z = 6,
    Edge11 = 7,
    Edge12 = 8,
    Edge13 = 9,
    Edge21 = 10,
    Edge22 = 11,
    Edge23 = 12,
    Edge31 = 13,
    Edge32 = 14,
    Edge33 = 15,
};

static const int kTangentAxes[3][2] = {
    { 1, 2 },
    { 0, 2 },
    { 0, 1 },
};

// 0x004f91b0 (PDB: 0x004face0 / ??$Sign@M@cdc@@YA?BMABM@Z)
inline float SignFloat(float v)
{
    return (v < 0.0f) ? -1.0f : 1.0f;
}

inline cdc::Vector3 ToVec3(const cdc::Vector& v)
{
    return cdc::Vector3::Make(v.x, v.y, v.z, v.w);
}

inline cdc::Vector3 NegVec3(const cdc::Vector& v)
{
    return cdc::Vector3::Make(-v.x, -v.y, -v.z, 0.0f);
}

inline cdc::Vector3 AbsVec3(const cdc::Vector& v)
{
    return cdc::Vector3::Make(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z), 0.0f);
}

inline cdc::Vector3 CrossVec3(const cdc::Vector& a, const cdc::Vector& b)
{
    cdc::Vector3 out;
    cdc::Cross3(&out, ToVec3(a), ToVec3(b));
    return out;
}

// 0x00501c40 (PDB: 0x004fd260 / LPROC CrossTrick)
inline cdc::Vector3 CrossTrick(const cdc::Vector& a, const cdc::Vector& b)
{
    return cdc::Vector3::Make(
        a.y * b.z + a.z * b.y,
        a.z * b.x + a.x * b.z,
        a.x * b.y + a.y * b.x,
        0.0f);
}

inline float GetAxis(const cdc::Vector& v, int axis)
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

inline void SetAxis(cdc::Vector& v, int axis, float val)
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

inline const cdc::Vector& MatrixCol(const cdc::Matrix& m, int col)
{
    switch (col) {
    case 0:
        return m.col0;
    case 1:
        return m.col1;
    default:
        return m.col2;
    }
}

// 0x005060a0 (PDB: 0x004fad10 / LPROC ComputeSkewedBasis)
void ComputeSkewedBasis(cdc::Vector3* u, cdc::Vector3* v, const cdc::Vector3& n)
{
    if (std::fabs(n.z) > 0.5f) {
        *u = cdc::Vector3::Make(2.0f * n.z, n.z, -(2.0f * n.x + n.y), 0.0f);
    } else if (std::fabs(n.y) > 0.5f) {
        *u = cdc::Vector3::Make(n.y, -(2.0f * n.z + n.x), 2.0f * n.y, 0.0f);
    } else {
        *u = cdc::Vector3::Make(-(2.0f * n.y + n.z), 2.0f * n.x, n.x, 0.0f);
    }
    u->Normalize3();
    *v = CrossVec3(n, *u);
}

// 0x004f91e0 (PDB: 0x004fae80 / LPROC ComputeIncidentFace)
static void ComputeIncidentFace(
    cdc::Vector3* verts,
    const cdc::Vector3& h,
    const cdc::Vector3& pos,
    const cdc::Matrix& rot,
    const cdc::Vector3& n)
{
    cdc::Matrix rotT;
    cdc::Transpose(&rotT, rot);
    const cdc::Vector3 nLocal = cdc::Mul3x3(rotT, n);
    const cdc::Vector3 absN = AbsVec3(nLocal);

    int domAxis = 2;
    if (absN.x > absN.y && absN.x > absN.z) {
        domAxis = 0;
    } else if (absN.y > absN.z) {
        domAxis = 1;
    }

    const int tA = kTangentAxes[domAxis][0];
    const int tB = kTangentAxes[domAxis][1];
    const float domCoord = -SignFloat(GetAxis(nLocal, domAxis)) * GetAxis(h, domAxis);
    const float extA = GetAxis(h, tA);
    const float extB = GetAxis(h, tB);

    static const float kQuadSigns[4][2] = {
        { -1.0f,  1.0f },
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
    };

    for (int i = 0; i < 4; ++i) {
        cdc::Vector3 localPt = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        SetAxis(localPt, domAxis, domCoord);
        SetAxis(localPt, tA, kQuadSigns[i][0] * extA);
        SetAxis(localPt, tB, kQuadSigns[i][1] * extB);
        verts[i] = pos + cdc::Mul3x3(rot, localPt);
    }
}

// 0x004f9730 (PDB: 0x004fb3d0 / LPROC ComputeFaceContact)
static int ComputeFaceContact(
    CPoint* cp,
    const BoxAndBoxData& data,
    AxisType axis,
    const cdc::Vector3& n)
{
    if (axis < Face_A_X || axis > Face_B_Z) {
        return 0;
    }

    const bool refIsA = (axis <= Face_A_Z);
    const int localAxis = refIsA ? (axis - Face_A_X) : (axis - Face_B_X);
    const cdc::Vector3& refH = refIsA ? data.hA : data.hB;
    const cdc::Vector3& refPos = refIsA ? data.posA : data.posB;
    const cdc::Matrix& refRot = refIsA ? data.RotA : data.RotB;
    const cdc::Vector3& incH = refIsA ? data.hB : data.hA;
    const cdc::Vector3& incPos = refIsA ? data.posB : data.posA;
    const cdc::Matrix& incRot = refIsA ? data.RotB : data.RotA;
    const cdc::Vector3 refN = refIsA ? n : NegVec3(n);

    const float planeDist = cdc::Dot3(refPos, refN) + GetAxis(refH, localAxis);
    const int tA = kTangentAxes[localAxis][0];
    const int tB = kTangentAxes[localAxis][1];
    const cdc::Vector3 sideA = ToVec3(MatrixCol(refRot, tA));
    const cdc::Vector3 sideB = ToVec3(MatrixCol(refRot, tB));
    const float dotA = cdc::Dot3(refPos, sideA);
    const float dotB = cdc::Dot3(refPos, sideB);
    const float negBoundA = GetAxis(refH, tA) - dotA;
    const float posBoundA = dotA + GetAxis(refH, tA);
    const float negBoundB = GetAxis(refH, tB) - dotB;
    const float posBoundB = dotB + GetAxis(refH, tB);

    cdc::Vector3 buf1[8];
    cdc::Vector3 buf2[8];
    ComputeIncidentFace(buf1, incH, incPos, incRot, refN);

    int numVerts = ClipPolyToPlane(buf2, buf1, 4, NegVec3(sideA), negBoundA);
    numVerts = ClipPolyToPlane(buf1, buf2, numVerts, sideA, posBoundA);
    numVerts = ClipPolyToPlane(buf2, buf1, numVerts, NegVec3(sideB), negBoundB);
    numVerts = ClipPolyToPlane(buf1, buf2, numVerts, sideB, posBoundB);

    CPoint rawContacts[8];
    int rawCount = 0;
    for (int i = 0; i < numVerts; ++i) {
        const float sep = cdc::Dot3(refN, buf1[i]) - planeDist;
        if (sep <= 0.0f) {
            rawContacts[rawCount].separation = sep;
            rawContacts[rawCount].normal = n;
            rawContacts[rawCount].position = buf1[i] - refN * sep;
            ++rawCount;
        }
    }

    if (rawCount <= 4) {
        for (int i = 0; i < rawCount; ++i) {
            cp[i] = rawContacts[i];
        }
        return rawCount;
    }

    // Reduce more than 4 manifold vertices down to up to 4 extreme points along a skewed basis.
    cdc::Vector3 u;
    cdc::Vector3 v;
    ComputeSkewedBasis(&u, &v, refN);

    float minU = cdc::Dot3(rawContacts[0].position, u);
    float maxU = minU;
    float minV = cdc::Dot3(rawContacts[0].position, v);
    float maxV = minV;
    int idxMinU = 0;
    int idxMaxU = 0;
    int idxMinV = 0;
    int idxMaxV = 0;

    bool keep[8];
    std::memset(keep, 0, static_cast<std::size_t>(rawCount));

    for (int i = 1; i < rawCount; ++i) {
        const float du = cdc::Dot3(rawContacts[i].position, u);
        if (du < minU) {
            minU = du;
            idxMinU = i;
        }
        if (du > maxU) {
            maxU = du;
            idxMaxU = i;
        }
        const float dv = cdc::Dot3(rawContacts[i].position, v);
        if (dv < minV) {
            minV = dv;
            idxMinV = i;
        }
        if (dv > maxV) {
            maxV = dv;
            idxMaxV = i;
        }
    }

    keep[idxMinU] = true;
    keep[idxMaxU] = true;
    keep[idxMinV] = true;
    keep[idxMaxV] = true;

    int keptCount = 0;
    for (int i = 0; i < rawCount; ++i) {
        if (keep[i]) {
            cp[keptCount++] = rawContacts[i];
        }
    }
    return keptCount;
}

// 0x004f9ed0 (PDB: 0x004fbb70 / LPROC ComputeEdgeEdgeContact)
static int ComputeEdgeEdgeContact(
    CPoint* cp,
    const BoxAndBoxData& data,
    AxisType axis,
    const cdc::Vector3& n,
    float sep)
{
    const cdc::Vector3 pALocal = cdc::Vector3::Make(
        SignFloat(cdc::Dot3(n, data.RotA.col0)) * data.hA.x,
        SignFloat(cdc::Dot3(n, data.RotA.col1)) * data.hA.y,
        SignFloat(cdc::Dot3(n, data.RotA.col2)) * data.hA.z,
        0.0f);
    const cdc::Vector3 pBLocal = NegVec3(cdc::Vector3::Make(
        SignFloat(cdc::Dot3(n, data.RotB.col0)) * data.hB.x,
        SignFloat(cdc::Dot3(n, data.RotB.col1)) * data.hB.y,
        SignFloat(cdc::Dot3(n, data.RotB.col2)) * data.hB.z,
        0.0f));

    const cdc::Vector3 pA = data.posA + cdc::Mul3x3(data.RotA, pALocal);
    const cdc::Vector3 pB = data.posB + cdc::Mul3x3(data.RotB, pBLocal);

    const int idxA = (axis - Edge11) / 3;
    const int idxB = (axis - Edge11) % 3;
    const cdc::Vector3 dirA = ToVec3(MatrixCol(data.RotA, idxA));
    const cdc::Vector3 dirB = ToVec3(MatrixCol(data.RotB, idxB));

    cp->separation = sep;
    cp->normal = n;
    cp->position = LineLineMidpoint(pA, dirA, pB, dirB);
    return 1;
}

} // namespace

// 0x004fa270 (PDB: 0x004fbf10 / ?CollideBoxAndBox@@YAHPAUCPoint@@ABVVector3@cdc@@111ABVQuat@3@2@Z)
int CollideBoxAndBox(
    CPoint* cp,
    const cdc::Vector3& hA,
    const cdc::Vector3& hB,
    const cdc::Vector3& posA,
    const cdc::Vector3& posB,
    const cdc::Quat& qA,
    const cdc::Quat& qB)
{
    cdc::Matrix RotA;
    cdc::Matrix RotB;
    RotA.Build(qA);
    RotB.Build(qB);

    cdc::Matrix RotAT;
    cdc::Matrix RotBT;
    cdc::Transpose(&RotAT, RotA);
    cdc::Transpose(&RotBT, RotB);

    const cdc::Vector3 D = posB - posA;
    const cdc::Vector3 dA = cdc::Mul3x3(RotAT, D);
    const cdc::Vector3 dB = cdc::Mul3x3(RotBT, D);

    cdc::Matrix C;
    cdc::Mul3x3(&C, RotAT, RotB);

    cdc::Matrix absC;
    absC.col0 = AbsVec3(C.col0);
    absC.col1 = AbsVec3(C.col1);
    absC.col2 = AbsVec3(C.col2);
    absC.col3 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    cdc::Matrix absCT;
    cdc::Transpose(&absCT, absC);

    const cdc::Vector3 sepFaceA = AbsVec3(dA) - hA - cdc::Mul3x3(absC, hB);
    if (sepFaceA.x > 0.0f || sepFaceA.y > 0.0f || sepFaceA.z > 0.0f) {
        return 0;
    }

    const cdc::Vector3 sepFaceB = AbsVec3(dB) - cdc::Mul3x3(absCT, hA) - hB;
    if (sepFaceB.x > 0.0f || sepFaceB.y > 0.0f || sepFaceB.z > 0.0f) {
        return 0;
    }

    const cdc::Vector3 epsVec = (hA + hB) * FLT_EPSILON;
    cdc::Vector3 sepEdgeByB[3];
    for (int j = 0; j < 3; ++j) {
        const int j1 = kTangentAxes[j][0];
        const int j2 = kTangentAxes[j][1];
        const cdc::Vector3 sep = AbsVec3(CrossVec3(MatrixCol(C, j), dA))
            - CrossTrick(hA, MatrixCol(absC, j))
            - MatrixCol(absC, j2) * GetAxis(hB, j1)
            - MatrixCol(absC, j1) * GetAxis(hB, j2);
        if (sep.x > epsVec.x || sep.y > epsVec.y || sep.z > epsVec.z) {
            return 0;
        }
        sepEdgeByB[j] = sep;
    }

    AxisType bestAxis = Face_A_X;
    float bestSep = sepFaceA.x;
    cdc::Vector3 bestNormal = (dA.x > 0.0f) ? ToVec3(RotA.col0) : NegVec3(RotA.col0);

    for (int i = 1; i < 3; ++i) {
        const float sep = GetAxis(sepFaceA, i);
        if (sep > bestSep) {
            bestSep = sep;
            bestAxis = static_cast<AxisType>(Face_A_X + i);
            const cdc::Vector& col = MatrixCol(RotA, i);
            bestNormal = (GetAxis(dA, i) > 0.0f) ? ToVec3(col) : NegVec3(col);
        }
    }

    for (int j = 0; j < 3; ++j) {
        const float sep = GetAxis(sepFaceB, j);
        if (sep > bestSep) {
            bestSep = sep;
            bestAxis = static_cast<AxisType>(Face_B_X + j);
            const cdc::Vector& col = MatrixCol(RotB, j);
            bestNormal = (GetAxis(dB, j) > 0.0f) ? ToVec3(col) : NegVec3(col);
        }
    }

    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            const cdc::Vector3 edgeAxis = CrossVec3(MatrixCol(RotA, i), MatrixCol(RotB, j));
            const float len = edgeAxis.Len3();
            const float rawSep = GetAxis(sepEdgeByB[j], i);
            if (len > FLT_EPSILON && (bestSep * len) < (rawSep * 1.5f)) {
                const float invLen = 1.0f / len;
                bestAxis = static_cast<AxisType>(Edge11 + i * 3 + j);
                bestSep = rawSep * invLen;
                bestNormal = edgeAxis * (SignFloat(cdc::Dot3(edgeAxis, D)) * invLen);
            }
        }
    }

    if (bestSep > 0.0f) {
        return 0;
    }

    BoxAndBoxData data;
    data.hA = hA;
    data.hB = hB;
    data.posA = posA;
    data.posB = posB;
    data.RotA = RotA;
    data.RotB = RotB;

    if (bestAxis >= Face_A_X && bestAxis <= Face_B_Z) {
        return ComputeFaceContact(cp, data, bestAxis, bestNormal);
    }
    if (bestAxis >= Edge11 && bestAxis <= Edge33) {
        return ComputeEdgeEdgeContact(cp, data, bestAxis, bestNormal, bestSep);
    }
    return 0;
}
