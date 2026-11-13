#include "game/Multibody/collision/CollideSegment.h"
#include "game/camera/cameracollide.h"
#include "game/collide.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

// Cross-subsystem externs
extern void* getScratchAddr(int size);                                                                                    // 0x004019c0 (PDB: 0x004019f0)
extern void  popScratchAddr(int size);                                                                                    // 0x004019f0 (PDB: 0x00401a20)
extern bool  TestSegmentAndBox(const cdc::Vector3& start, const cdc::Vector3& dir, const cdc::Vector3& segMin, const cdc::Vector3& segMax, const BBox& box); // 0x00501dd0 (PDB: 0x00503a70)
extern void  MULTIBODY_PrintWarning(const char* fmt, ...);                                                                // 0x00552e70 (PDB: 0x00554e70)
extern const char* MB_CurrentEntity;                                                                                      // 0x01116144 (PDB: 0x011172fc)

namespace {

struct KDTraversalEntry {
    const KDNode* node;
    float tMin;
    float tMax;
};

float AxisCoord(const cdc::Vector3& v, uint8_t axis)
{
    switch (axis) {
    case 0: return v.x;
    case 1: return v.y;
    default: return v.z;
    }
}

cdc::Vector3 AbsVector3(const cdc::Vector3& v)
{
    return cdc::Vector3::Make(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z), 0.0f);
}

bool ShouldTestFace(const IndexedFace& face, uint8_t flags)
{
    return (face.collisionFlags & flags) != 0 && (face.adjacencyFlags & 0x40) == 0;
}

// 0x00502ea0 (PDB: 0x00504b40)
bool TestAlignedBoxAndAlignedBox(const BBox& a, const BBox& b)
{
    return a.bMin.x <= b.bMax.x && b.bMin.x <= a.bMax.x &&
           a.bMin.y <= b.bMax.y && b.bMin.y <= a.bMax.y &&
           a.bMin.z <= b.bMax.z && b.bMin.z <= a.bMax.z;
}

} // namespace

// 0x00502f10 (PDB: 0x00504bb0)
uint16_t Mesh::Query(uint16_t* faceIndices, uint16_t maxFaces, const BBox& box, uint8_t flags)
{
    BBox localBox;
    localBox.bMin = box.bMin - m_position;
    localBox.bMax = box.bMax - m_position;

    if (!TestAlignedBoxAndAlignedBox(m_box, localBox)) {
        return 0;
    }

    constexpr int kStackBytes = 0x400;
    const KDNode** stack = static_cast<const KDNode**>(getScratchAddr(kStackBytes));
    stack[0] = m_root;
    int sp = 1;
    uint16_t count = 0;

    while (count < maxFaces && sp != 0) {
        const KDNode* node = stack[--sp];
        if (node->IsLeaf()) {
            const uint8_t numFaces = node->GetNumFaces();
            const uint16_t baseIndex = node->GetFaceIndex();
            for (uint8_t i = 0; i < numFaces && count < maxFaces; ++i) {
                const uint16_t faceIdx = static_cast<uint16_t>(baseIndex + i);
                if (ShouldTestFace(m_faces[faceIdx], flags)) {
                    faceIndices[count++] = faceIdx;
                }
            }
        } else {
            const uint8_t axis = node->GetAxis();
            if (AxisCoord(localBox.bMin, axis) <= node->GetNegOffset()) {
                stack[sp++] = node->GetNegChild();
            }
            if (AxisCoord(localBox.bMax, axis) >= node->GetPosOffset()) {
                stack[sp++] = node->GetPosChild();
            }
        }
    }

    popScratchAddr(kStackBytes);

    if (sp > 0 && count == maxFaces) {
        if (MB_CurrentEntity != nullptr) {
            MULTIBODY_PrintWarning("WARNING: too many collision faces near %s", MB_CurrentEntity);
        } else {
            MULTIBODY_PrintWarning("WARNING: too many collision faces in query");
        }
    }

    return count;
}

// 0x005030a0 (PDB: 0x00504d40)
int Mesh::CollideSegment(
    SegmentResult* results,
    int maxResults,
    const cdc::Vector3& start,
    const cdc::Vector3& end,
    unsigned char flags)
{
    const cdc::Vector3 localStart = start - m_position;
    const cdc::Vector3 localEnd = end - m_position;
    const cdc::Vector3 dir = localEnd - localStart;

    BBox segBox;
    segBox.bMin = cdc::Vector3::Make(
        std::min(localStart.x, localEnd.x),
        std::min(localStart.y, localEnd.y),
        std::min(localStart.z, localEnd.z),
        0.0f);
    segBox.bMax = cdc::Vector3::Make(
        std::max(localStart.x, localEnd.x),
        std::max(localStart.y, localEnd.y),
        std::max(localStart.z, localEnd.z),
        0.0f);

    if (!TestSegmentAndBox(localStart, dir, segBox.bMin, segBox.bMax, m_box)) {
        return 0;
    }

    const cdc::Vector3 absDir = AbsVector3(dir);

    constexpr int kStackBytes = 0xc00;
    const KDNode** stack = static_cast<const KDNode**>(getScratchAddr(kStackBytes));
    stack[0] = m_root;
    int sp = 1;
    int numHits = 0;

    while (numHits < maxResults && sp != 0) {
        const KDNode* node = stack[--sp];
        if (node->IsLeaf()) {
            const uint8_t numFaces = node->GetNumFaces();
            const uint16_t baseIndex = node->GetFaceIndex();
            for (uint8_t i = 0; i < numFaces && numHits < maxResults; ++i) {
                IndexedFace* face = &m_faces[static_cast<uint16_t>(baseIndex + i)];
                if (!ShouldTestFace(*face, flags)) {
                    continue;
                }

                MTriangle tri;
                GetTriangle(&tri, face);
                float hitT = 1.0f;
                cdc::Vector3 hitNormal;
                if (CollideSegmentAndTri(&hitT, &hitNormal, localStart, dir, tri) != NO_HIT) {
                    results[numHits].face = face;
                    results[numHits].lambda = hitT;
                    ++numHits;
                }
            }
        } else {
            const uint8_t axis = node->GetAxis();
            const float startCoord = AxisCoord(localStart, axis);
            const float dirCoord = AxisCoord(dir, axis);
            const float negOffset = node->GetNegOffset();
            const float posOffset = node->GetPosOffset();

            if (AxisCoord(absDir, axis) < FLT_EPSILON) {
                if (startCoord <= negOffset) {
                    stack[sp++] = node->GetNegChild();
                }
                if (startCoord >= posOffset) {
                    stack[sp++] = node->GetPosChild();
                }
            } else {
                const float invD = 1.0f / dirCoord;
                if (dirCoord > 0.0f) {
                    const float tNeg = (negOffset - startCoord) * invD;
                    const float tPos = (posOffset - startCoord) * invD;
                    if (tPos <= 1.0f) {
                        stack[sp++] = node->GetPosChild();
                    }
                    if (tNeg >= 0.0f) {
                        stack[sp++] = node->GetNegChild();
                    }
                } else {
                    const float tPos = (posOffset - startCoord) * invD;
                    const float tNeg = (negOffset - startCoord) * invD;
                    if (tNeg <= 1.0f) {
                        stack[sp++] = node->GetNegChild();
                    }
                    if (tPos >= 0.0f) {
                        stack[sp++] = node->GetPosChild();
                    }
                }
            }
        }
    }

    popScratchAddr(kStackBytes);
    return numHits;
}

// 0x00503350 (PDB: 0x00504ff0)
IndexedFace* Mesh::CollideSegment(
    cdc::Vector3* hitPoint,
    cdc::Vector3* hitNormal,
    const cdc::Vector3& start,
    const cdc::Vector3& end,
    unsigned char flags)
{
    const cdc::Vector3 localStart = start - m_position;
    const cdc::Vector3 localEnd = end - m_position;
    const cdc::Vector3 dir = localEnd - localStart;

    BBox segBox;
    segBox.bMin = cdc::Vector3::Make(
        std::min(localStart.x, localEnd.x),
        std::min(localStart.y, localEnd.y),
        std::min(localStart.z, localEnd.z),
        0.0f);
    segBox.bMax = cdc::Vector3::Make(
        std::max(localStart.x, localEnd.x),
        std::max(localStart.y, localEnd.y),
        std::max(localStart.z, localEnd.z),
        0.0f);

    if (!TestSegmentAndBox(localStart, dir, segBox.bMin, segBox.bMax, m_box)) {
        return nullptr;
    }

    const cdc::Vector3 absDir = AbsVector3(dir);

    constexpr int kStackBytes = 0xc00;
    KDTraversalEntry* stack = static_cast<KDTraversalEntry*>(getScratchAddr(kStackBytes));
    stack[0] = { m_root, 0.0f, 1.0f };
    int sp = 1;

    float bestT = 1.0f;
    IndexedFace* bestFace = nullptr;

    while (sp != 0) {
        const KDTraversalEntry entry = stack[--sp];
        const float limitT = std::min(entry.tMax, bestT);
        if (entry.tMin > limitT) {
            continue;
        }

        const KDNode* node = entry.node;
        if (node->IsLeaf()) {
            const uint8_t numFaces = node->GetNumFaces();
            const uint16_t baseIndex = node->GetFaceIndex();
            for (uint8_t i = 0; i < numFaces; ++i) {
                IndexedFace* face = &m_faces[static_cast<uint16_t>(baseIndex + i)];
                if (!ShouldTestFace(*face, flags)) {
                    continue;
                }

                MTriangle tri;
                GetTriangle(&tri, face);
                float t = bestT;
                cdc::Vector3 triNormal;
                if (CollideSegmentAndTri(&t, &triNormal, localStart, dir, tri) != NO_HIT && t <= bestT) {
                    bestT = t;
                    bestFace = face;
                    if (hitNormal != nullptr) {
                        *hitNormal = triNormal;
                    }
                }
            }
        } else {
            const uint8_t axis = node->GetAxis();
            const float startCoord = AxisCoord(localStart, axis);
            const float dirCoord = AxisCoord(dir, axis);
            const float negOffset = node->GetNegOffset();
            const float posOffset = node->GetPosOffset();

            if (AxisCoord(absDir, axis) < FLT_EPSILON) {
                const float coordAtMin = startCoord + entry.tMin * dirCoord;
                if (coordAtMin <= negOffset) {
                    stack[sp++] = { node->GetNegChild(), entry.tMin, entry.tMax };
                }
                if (coordAtMin >= posOffset) {
                    stack[sp++] = { node->GetPosChild(), entry.tMin, entry.tMax };
                }
            } else {
                const float invD = 1.0f / dirCoord;
                if (dirCoord > 0.0f) {
                    const float tPos = (posOffset - startCoord) * invD;
                    const float tNeg = (negOffset - startCoord) * invD;
                    if (tPos <= entry.tMax) {
                        stack[sp++] = { node->GetPosChild(), std::max(entry.tMin, tPos), entry.tMax };
                    }
                    if (tNeg >= entry.tMin) {
                        stack[sp++] = { node->GetNegChild(), entry.tMin, std::min(entry.tMax, tNeg) };
                    }
                } else {
                    const float tNeg = (negOffset - startCoord) * invD;
                    const float tPos = (posOffset - startCoord) * invD;
                    if (tNeg <= entry.tMax) {
                        stack[sp++] = { node->GetNegChild(), std::max(entry.tMin, tNeg), entry.tMax };
                    }
                    if (tPos >= entry.tMin) {
                        stack[sp++] = { node->GetPosChild(), entry.tMin, std::min(entry.tMax, tPos) };
                    }
                }
            }
        }
    }

    if (bestFace != nullptr && hitPoint != nullptr) {
        *hitPoint = localStart + dir * bestT + m_position;
    }

    popScratchAddr(kStackBytes);
    return bestFace;
}
