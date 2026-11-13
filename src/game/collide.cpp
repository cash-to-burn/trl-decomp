#include "game/collide.h"
#include "game/anim/anim.h"
#include "game/camera/camera.h"
#include "game/main.h"

#include <cstring>

// Cross-subsystem externs
extern void INSTANCE_Post(Instance* instance, int message, int data);                                                              // 0x00455180 (PDB: 0x004567c0)
extern void SIGNAL_HandleOutSignal(Instance* instance, Level* level, Signal* signal, int signalID);                                // 0x00467490 (PDB: 0x00468e50)
extern void SIGNAL_HandleSignal(Instance* instance, Level* level, Signal* signal, int signalID);                                   // 0x004675a0 (PDB: 0x00468f60)
extern int SIGNAL_IsStreamSignal(Signal* signal);                                                                                  // 0x004679d0 (PDB: 0x00469390)
extern void SIGNAL_AddSignalToReset(Signal* signal);                                                                               // 0x00467ba0 (PDB: 0x00469560)
extern int RBUTIL_GetMovingMarker(BaseInstance** outInst, BaseInstance* instance, int markerID, cdc::Matrix* mat, bool useOldMat); // 0x0053bf20 (PDB: 0x0053dd60)

namespace cdc {

struct Allocator {
    virtual ~Allocator() = default;
    void* Alloc(int size, int align); // vtable +0x34 (cdc::Allocator::Alloc)
    void Free(void* ptr);             // vtable +0x38 (cdc::Allocator::Free)
};

struct HeapAllocator : public Allocator {
    void* Realloc(void* ptr, int size); // vtable +0x60 (cdc::HeapAllocator::Realloc)
};

struct Alloc {
    static bool s_initialized;             // 0x01159cf4
    static Allocator* s_defAllocator;      // 0x01159cfc
    static HeapAllocator* s_heapAllocator; // 0x01159d00
    static void Init();                    // 0x005d9a00
};

// 0x00423ff0 (PDB: 0x00423060 / ??Lcdc@@YA?AVVector3@0@ABV10@0@Z)
Vector3 operator%(const Vector3& a, const Vector3& b)
{
    Vector3 out;
    Cross3(&out, a, b);
    return out;
}

} // namespace cdc

// 0x00424040 (PDB: 0x004230b0)
cdc::Vector3 MTriangle::GetNormal()
{
    cdc::Vector3 normal = (v1 - v0) % (v2 - v0);
    normal.Normalize3();
    return normal;
}

// 0x004240b0 (PDB: 0x00423120)
VertexInt16::operator cdc::Vector3()
{
    return cdc::Vector3::Make(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z),
        0.0f);
}

// 0x00424130 (PDB: 0x004231a0)
void Mesh::GetTriangle(MTriangle* out, const IndexedFace* face)
{
    if (m_vertexType == 0) {
        VertexInt16* verts = static_cast<VertexInt16*>(m_vertices);
        out->v0 = verts[face->i0];
        out->v1 = verts[face->i1];
        out->v2 = verts[face->i2];
    } else {
        const cdc::Vector3* verts = static_cast<const cdc::Vector3*>(m_vertices);
        out->v0 = verts[face->i0];
        out->v1 = verts[face->i1];
        out->v2 = verts[face->i2];
    }
}

namespace {

constexpr int kMaxSignalHits = 8;
constexpr int kHPrimTypeMarker = 8;

inline cdc::Vector3 ToVec3(const cdc::Vector& v)
{
    return cdc::Vector3::Make(v.x, v.y, v.z, v.w);
}

inline void EnsureAllocInitialized()
{
    if (!cdc::Alloc::s_initialized) {
        cdc::Alloc::Init();
    }
}

inline HModel* GetBaseHModel(BaseInstance* instance)
{
    return (instance && instance->hModelList)
        ? &instance->hModelList[instance->currentBaseModel]
        : nullptr;
}

HMarker* ResizeDynamicMarkers(HModel* hmodel, int newCount)
{
    EnsureAllocInitialized();
    const int bytes = newCount * static_cast<int>(sizeof(HMarker));
    if (hmodel->dynamicHMarkerList != nullptr) {
        return static_cast<HMarker*>(cdc::Alloc::s_heapAllocator->Realloc(hmodel->dynamicHMarkerList, bytes));
    }
    return static_cast<HMarker*>(cdc::Alloc::s_defAllocator->Alloc(bytes, 0));
}

bool EnsureHModelListAllocated(BaseInstance* instance)
{
    if (instance->hModelList != nullptr) {
        return true;
    }

    EnsureAllocInitialized();
    const int size = static_cast<int>(instance->object->numModels) * static_cast<int>(sizeof(HModel));
    void* mem = cdc::Alloc::s_defAllocator->Alloc(size, 0);
    if (mem != nullptr) {
        std::memset(mem, 0, static_cast<size_t>(size));
    }
    instance->hModelList = static_cast<HModel*>(mem);
    return instance->hModelList != nullptr;
}

int AllocateDynamicMarkerRange(BaseInstance* instance, int count, int* outStartIndex)
{
    if (!EnsureHModelListAllocated(instance) || instance->matrix == nullptr || instance->hModelList == nullptr) {
        return 0;
    }

    HModel* hmodel = &instance->hModelList[instance->currentBaseModel];
    int maxIndex = 0;

    if (hmodel->dynamicHMarkerList == nullptr && hmodel->hPrimList == nullptr) {
        maxIndex = 0;
    } else if (hmodel->numDynamicHMarkers == 0) {
        for (int i = 0; i < hmodel->numHPrims; ++i) {
            const HPrim& prim = hmodel->hPrimList[i];
            if (prim.type == kHPrimTypeMarker && prim.data.hmarker->index > maxIndex) {
                maxIndex = prim.data.hmarker->index;
            }
        }
    } else {
        maxIndex = hmodel->dynamicHMarkerList[hmodel->numDynamicHMarkers - 1].index;
    }

    const int startIndex = hmodel->numDynamicHMarkers;
    const int newCount = startIndex + count;
    HMarker* newList = ResizeDynamicMarkers(hmodel, newCount);
    hmodel->dynamicHMarkerList = newList;
    if (newList == nullptr) {
        hmodel->numDynamicHMarkers = 0;
        return 0;
    }

    *outStartIndex = startIndex;
    hmodel->numDynamicHMarkers = newCount;
    return maxIndex + 1;
}

void ComputeMarkerTransform(cdc::Matrix* out, const cdc::Matrix* instMats, int segment, const HMarker* marker)
{
    if (segment == -1) {
        out->Identity();
        out->SetTrans(cdc::Vector3::Make(marker->px, marker->py, marker->pz, 0.0f));
        return;
    }

    cdc::Euler rot;
    rot.x = marker->rx;
    rot.y = marker->ry;
    rot.z = marker->rz;
    rot.w = 0.0f;

    cdc::Matrix markerMat;
    markerMat.Build_XYZOrder(rot);
    markerMat.col3 = cdc::Vector3::Make(marker->px, marker->py, marker->pz, 1.0f);
    cdc::Mul4x4(out, instMats[segment], markerMat);
}

void DispatchLevelSignals(
    Instance* inst,
    const SignalResults* results,
    int count,
    bool isOutSignal,
    bool markTriggered)
{
    for (int i = 0; i < count; ++i) {
        Level* hitLevel = results[i].level;
        if (hitLevel != STREAM_GetLevelWithID(inst->currentStreamUnitID)) {
            continue;
        }

        Signal* sig = results[i].signal;
        const int sigIdx = static_cast<int>(sig - hitLevel->SignalListStart);
        const int16_t* sigIDs = static_cast<const int16_t*>(hitLevel->SignalIDList);
        const int signalID = sigIDs[sigIdx];

        if (isOutSignal) {
            SIGNAL_HandleOutSignal(inst, hitLevel, sig, signalID);
        } else {
            if (markTriggered) {
                sig->flags |= 0x8000u;
            }
            SIGNAL_HandleSignal(inst, hitLevel, sig, signalID);
            SIGNAL_AddSignalToReset(sig);
        }
    }
}

void OffsetMatrixList(cdc::Matrix* baseMat, const BaseInstance* instance, const cdc::Vector3& offset)
{
    const Object* obj = instance->object;
    const Model* model = obj->modelList[instance->currentRenderModel];
    cdc::Matrix* matrices = baseMat;
    int count = model->numSegments;

    if ((obj->oflags2 & 0x80) == 0 && (instance->iflags & 0x400000) == 0) {
        count = model->numSegments + model->numVirtSegments + 1;
        matrices = baseMat - 1;
    }

    for (int i = 0; i < count; ++i) {
        matrices[i].col3.x += offset.x;
        matrices[i].col3.y += offset.y;
        matrices[i].col3.z += offset.z;
    }
}

} // namespace

// 0x00424280 (PDB: 0x004232f0)
void COLLIDE_SegmentCollisionOn(Instance* instance, int segment)
{
    HModel* hmodel = GetBaseHModel(instance);
    if (hmodel == nullptr) {
        return;
    }

    for (int i = 0; i < hmodel->numHPrims; ++i) {
        HPrim& prim = hmodel->hPrimList[i];
        if (segment == -1 || static_cast<int>(prim.segment) == segment) {
            prim.hpFlags |= 1u;
        }
    }

    instance->iflags2 &= ~0x40000;
}

// 0x004242f0 (PDB: 0x00423360)
void COLLIDE_SegmentCollisionOff(Instance* instance, int segment)
{
    HModel* hmodel = GetBaseHModel(instance);
    if (hmodel == nullptr) {
        return;
    }

    if (segment == -1) {
        for (int i = 0; i < hmodel->numHPrims; ++i) {
            hmodel->hPrimList[i].hpFlags &= ~1u;
        }
        instance->iflags2 |= 0x40000;
        return;
    }

    bool anyActive = false;
    for (int i = 0; i < hmodel->numHPrims; ++i) {
        HPrim& prim = hmodel->hPrimList[i];
        if (static_cast<int>(prim.segment) == segment) {
            prim.hpFlags &= ~1u;
        }
        if ((prim.hpFlags & 1u) != 0) {
            anyActive = true;
        }
    }

    if (!anyActive) {
        instance->iflags2 |= 0x40000;
    }
}

// 0x00424370 (PDB: 0x004233e0)
void COLLIDE_ResetCollisionFlag(Instance* instance, int hpMask, int withMask)
{
    HModel* hmodel = GetBaseHModel(instance);
    if (hmodel == nullptr) {
        return;
    }

    const uint8_t hpClear = static_cast<uint8_t>(~hpMask);
    const uint8_t withClear = static_cast<uint8_t>(~withMask);
    for (int i = 0; i < hmodel->numHPrims; ++i) {
        hmodel->hPrimList[i].hpFlags &= hpClear;
        hmodel->hPrimList[i].withFlags &= withClear;
    }
}

// 0x004243b0 (PDB: 0x00423420)
void COLLIDE_SetTerrainGroupFlag(CollideInfo* collideInfo, int flag)
{
    collideInfo->terrainGroup->flags |= flag;
}

// 0x004243d0 (PDB: 0x00423440)
int COLLIDE_GetNextDynamicMarker(int* outIndex, BaseInstance* instance)
{
    return AllocateDynamicMarkerRange(instance, 1, outIndex);
}

// 0x00424520 (PDB: 0x00423590)
int COLLIDE_GetNextDynamicMarkers(int* outStartIndex, int* outIDs, int count, BaseInstance* instance)
{
    const int firstID = AllocateDynamicMarkerRange(instance, count, outStartIndex);
    if (firstID == 0) {
        return 0;
    }

    for (int i = 0; i < count; ++i) {
        outIDs[i] = firstID + i;
    }
    return count;
}

// 0x00424680 (PDB: 0x004236f0)
int COLLIDE_InsertDynamicMarker(BaseInstance* instance, HMarker* marker)
{
    int slotIndex = 0;
    const int id = COLLIDE_GetNextDynamicMarker(&slotIndex, instance);
    if (id == 0) {
        return 0;
    }

    HModel* hmodel = GetBaseHModel(instance);
    HMarker& dst = hmodel->dynamicHMarkerList[slotIndex];
    dst.segment = marker->segment;
    dst.index = id;
    dst.px = marker->px;
    dst.py = marker->py;
    dst.pz = marker->pz;
    dst.rx = marker->rx;
    dst.ry = marker->ry;
    dst.rz = marker->rz;
    return id;
}

// 0x00424710 (PDB: 0x00423780)
int COLLIDE_InsertDynamicWorldPosMarker(BaseInstance* instance, const cdc::Vector3& pos)
{
    int slotIndex = 0;
    const int id = COLLIDE_GetNextDynamicMarker(&slotIndex, instance);
    if (id == 0) {
        return 0;
    }

    HModel* hmodel = GetBaseHModel(instance);
    HMarker& dst = hmodel->dynamicHMarkerList[slotIndex];
    dst.segment = -1;
    dst.index = id;
    dst.px = pos.x;
    dst.py = pos.y;
    dst.pz = pos.z;
    dst.rx = 0.0f;
    dst.ry = 0.0f;
    dst.rz = 0.0f;
    return id;
}

// 0x004247a0 (PDB: 0x00423810)
int COLLIDE_InsertDynamicWorldPosMarkers(int* outIDs, int count, BaseInstance* instance, const cdc::Vector3& pos)
{
    int startIndex = 0;
    const int actualCount = COLLIDE_GetNextDynamicMarkers(&startIndex, outIDs, count, instance);
    if (actualCount <= 0) {
        return actualCount;
    }

    HModel* hmodel = GetBaseHModel(instance);
    for (int i = 0; i < actualCount; ++i) {
        HMarker& dst = hmodel->dynamicHMarkerList[startIndex + i];
        dst.segment = -1;
        dst.index = outIDs[i];
        dst.px = pos.x;
        dst.py = pos.y;
        dst.pz = pos.z;
        dst.rx = 0.0f;
        dst.ry = 0.0f;
        dst.rz = 0.0f;
    }
    return actualCount;
}

// 0x004249c0 (PDB: 0x00423a30)
bool COLLIDE_SetDynamicWorldPosMarker(BaseInstance* instance, int markerID, const cdc::Vector3& pos)
{
    if (markerID <= 0 || instance->oldMatrix == nullptr || instance->hModelList == nullptr) {
        return false;
    }

    HModel* hmodel = GetBaseHModel(instance);
    for (int i = 0; i < hmodel->numDynamicHMarkers; ++i) {
        HMarker& marker = hmodel->dynamicHMarkerList[i];
        if (marker.index == markerID && marker.segment == -1) {
            marker.px = pos.x;
            marker.py = pos.y;
            marker.pz = pos.z;
            return true;
        }
    }

    return false;
}

// 0x00424a30 (PDB: 0x00423aa0)
void COLLIDE_RelocateDynamicWorldPosMarkers(BaseInstance* instance, const cdc::Vector3& delta)
{
    if (instance->oldMatrix == nullptr || instance->hModelList == nullptr) {
        return;
    }

    HModel* hmodel = GetBaseHModel(instance);
    for (int i = 0; i < hmodel->numDynamicHMarkers; ++i) {
        HMarker& marker = hmodel->dynamicHMarkerList[i];
        if (marker.segment == -1) {
            marker.px += delta.x;
            marker.py += delta.y;
            marker.pz += delta.z;
        }
    }
}

// 0x00424ab0 (PDB: 0x00423b20)
int COLLIDE_DeleteDynamicMarker(BaseInstance* instance, int markerID)
{
    if (instance->matrix == nullptr || instance->hModelList == nullptr) {
        return 0;
    }

    HModel* hmodel = GetBaseHModel(instance);
    if (hmodel->numDynamicHMarkers == 0) {
        return 1;
    }

    int foundIndex = 0;
    while (foundIndex < hmodel->numDynamicHMarkers) {
        if (hmodel->dynamicHMarkerList[foundIndex].index == markerID) {
            break;
        }
        ++foundIndex;
    }

    if (foundIndex == hmodel->numDynamicHMarkers) {
        return 0;
    }

    const int trailingCount = hmodel->numDynamicHMarkers - foundIndex - 1;
    if (trailingCount > 0) {
        std::memmove(
            &hmodel->dynamicHMarkerList[foundIndex],
            &hmodel->dynamicHMarkerList[foundIndex + 1],
            static_cast<size_t>(trailingCount) * sizeof(HMarker));
    }

    --hmodel->numDynamicHMarkers;
    hmodel->dynamicHMarkerList = ResizeDynamicMarkers(hmodel, hmodel->numDynamicHMarkers);
    return 1;
}

// 0x00424bb0 (PDB: 0x00423c20)
void COLLIDE_DeleteDynamicMarkerList(BaseInstance* instance)
{
    if (instance->hModelList == nullptr) {
        return;
    }

    HModel* hmodel = GetBaseHModel(instance);
    if (hmodel->dynamicHMarkerList == nullptr) {
        return;
    }

    EnsureAllocInitialized();
    cdc::Alloc::s_defAllocator->Free(hmodel->dynamicHMarkerList);
    hmodel->dynamicHMarkerList = nullptr;
    hmodel->numDynamicHMarkers = 0;
}

// 0x00424c20 (PDB: 0x00423c90)
int COLLIDE_Signals(SignalResults* results, const cdc::Vector3& start, const cdc::Vector3& end)
{
    int totalHits = 0;
    const cdc::Vector3 startCopy = start;
    const cdc::Vector3 endCopy = end;

    for (int unitIdx = 0; unitIdx < MAX_STREAM_UNITS && totalHits < kMaxSignalHits; ++unitIdx) {
        const StreamUnit& unit = StreamUnitList[unitIdx];
        if (unit.used != 2) {
            continue;
        }

        Level* level = unit.level;
        Terrain* terrain = level->terrain;
        TerrainGroup* sigGroup = terrain->signalTerrainGroup;
        if (sigGroup == nullptr || sigGroup->mesh == nullptr) {
            continue;
        }

        SegmentResult segHits[kMaxSignalHits];
        const int numSegHits = sigGroup->mesh->CollideSegment(segHits, kMaxSignalHits, startCopy, endCopy, 0xff);
        for (int hitIdx = 0; hitIdx < numSegHits && totalHits < kMaxSignalHits; ++hitIdx) {
            const uint16_t sigIdx = segHits[hitIdx].face->GetSignalIndex();
            if (sigIdx != 0xffffu) {
                results[totalHits].lambda = segHits[hitIdx].lambda;
                results[totalHits].signal = &terrain->signals[sigIdx];
                results[totalHits].level = level;
                ++totalHits;
            }
        }
    }

    return totalHits;
}

// 0x00424e30 (PDB: 0x00423ea0)
int COLLIDE_FindCollisionFaceNormal(CollideInfo* collideInfo, cdc::Vector3* normal)
{
    if (collideInfo->type1 == 3) {
        MTriangle tri;
        collideInfo->terrainGroup->mesh->GetTriangle(&tri, static_cast<const IndexedFace*>(collideInfo->prim1));
        *normal = tri.GetNormal();
        return 1;
    }

    if (collideInfo->type0 == 1 && collideInfo->type1 == 5) {
        *normal = cdc::Mul3x3(*collideInfo->inst1->matrix, collideInfo->point0f);
        return 1;
    }

    return 0;
}

// 0x00424ec0 (PDB: 0x00423f30)
StreamUnit* COLLIDE_CameraWithStreamSignals(Camera* camera)
{
    Instance* focusInst = camera->focusInstance;
    const int focusUnitID = focusInst->currentStreamUnitID;
    StreamUnit* focusUnit = STREAM_GetStreamUnitWithID(focusUnitID);

    cdc::Vector3 start;
    if (focusInst == g_gameTracker.playerInstance) {
        start = focusInst->LinkParent ? focusInst->LinkParent->position : focusInst->position;
        start.z += 200.0f;
    } else {
        start = ToVec3(focusInst->matrix[focusInst->rootMatrix].col3);
    }

    Level* focusLevel = focusUnit->level;
    if (focusLevel == nullptr) {
        return nullptr;
    }

    const cdc::Vector3 end = camera->core.position;
    SignalResults sigRes[kMaxSignalHits];
    const int numHits = COLLIDE_Signals(sigRes, start, end);
    if (numHits <= 0) {
        return nullptr;
    }

    StreamUnit* matches[kMaxSignalHits];
    int numMatches = 0;

    for (int i = 0; i < numHits; ++i) {
        if (sigRes[i].level != focusLevel || SIGNAL_IsStreamSignal(sigRes[i].signal) == 0) {
            continue;
        }

        const int portalIndex = sigRes[i].signal->streamPortalIndex;
        const int16_t streamID = focusLevel->terrain->streamUnitPortals[portalIndex].streamID;
        if (streamID == 0) {
            continue;
        }

        StreamUnit* targetUnit = STREAM_GetStreamUnitWithID(streamID);
        if (targetUnit != nullptr) {
            matches[numMatches++] = targetUnit;
        }
    }

    if (numMatches == 0) {
        return nullptr;
    }
    if (numMatches == 1) {
        return matches[0];
    }

    for (int i = 0; i < numMatches; ++i) {
        if (matches[i]->StreamUnitID != focusUnitID) {
            return matches[i];
        }
    }

    return nullptr;
}

// 0x00425060 (PDB: 0x004240d0)
void COLLIDE_InstanceListWithSignals(void)
{
    for (Instance* inst = gInstanceList.first; inst != nullptr; inst = inst->next) {
        if ((inst->iflags2 & 0x24040000) != 0 || inst->matrix == nullptr || inst->oldMatrix == nullptr) {
            continue;
        }

        Instance* player = static_cast<Instance*>(g_gameTracker.playerInstance);
        cdc::Vector3 oldPos;
        cdc::Vector3 newPos;

        if (inst == player) {
            oldPos = inst->oldPos;
            oldPos.z += 200.0f;
            newPos = inst->position;
            newPos.z += 200.0f;
        } else {
            const int rootIdx = inst->rootMatrix;
            oldPos = ToVec3(inst->oldMatrix[rootIdx].col3);
            newPos = ToVec3(inst->matrix[rootIdx].col3);
            if (rootIdx == 0 && (inst->object->oflags2 & 0x100000) != 0) {
                oldPos.z += 100.0f;
                newPos.z += 100.0f;
            }
        }

        const bool isPlayerOrParent = (inst == player) || (player != nullptr && player->LinkParent == inst);
        SignalResults results[kMaxSignalHits];

        if (isPlayerOrParent) {
            const int outCount = COLLIDE_Signals(results, newPos, oldPos);
            DispatchLevelSignals(inst, results, outCount, true, false);
        }

        const int inCount = COLLIDE_Signals(results, oldPos, newPos);
        if (inCount > 0) {
            DispatchLevelSignals(inst, results, inCount, false, isPlayerOrParent);
        }
    }
}

// 0x004252b0 (PDB: 0x00424320)
void COLLIDE_UpdateAllTransformsf(BaseInstance* instance, const cdc::Vector3& offset)
{
    OffsetMatrixList(instance->matrix, instance, offset);

    if ((instance->iflags & 0x400000) == 0) {
        INSTANCE_Post(
            static_cast<Instance*>(instance),
            0x100020,
            static_cast<int>(reinterpret_cast<std::intptr_t>(&offset)));
    }
}

// 0x00425390 (PDB: 0x00424400)
void COLLIDE_MoveAllTransformsf(BaseInstance* instance, const cdc::Vector3& offset)
{
    if (instance->oldMatrix != nullptr) {
        OffsetMatrixList(instance->oldMatrix, instance, offset);
    }
}

// 0x00425450 (PDB: 0x004244c0)
int COLLIDE_GetHMarkerTransformFromID(
    BaseInstance** outInst,
    BaseInstance* instance,
    int markerID,
    cdc::Matrix* mat,
    bool useOldMatrix)
{
    if ((instance->object->oflags & 0x40000000) != 0) {
        return RBUTIL_GetMovingMarker(outInst, instance, markerID, mat, useOldMatrix);
    }

    *outInst = nullptr;
    const cdc::Matrix* instMats = useOldMatrix ? instance->oldMatrix : instance->matrix;
    if (instMats == nullptr || instance->hModelList == nullptr) {
        mat->Identity();
        return 0;
    }

    const int numModels = instance->object->numModels;
    for (int i = 0; i < numModels; ++i) {
        int modelIdx = i;
        if (i == 0) {
            modelIdx = instance->currentBaseModel;
        } else if (i == instance->currentBaseModel) {
            modelIdx = 0;
        }

        const HModel& hmodel = instance->hModelList[modelIdx];
        if (markerID >= 100) {
            int remaining = markerID - 100;
            for (int d = 0; d < hmodel.numDynamicHMarkers; ++d) {
                const HMarker& m = hmodel.dynamicHMarkerList[d];
                if (m.segment == -1) {
                    if (remaining == 0) {
                        ComputeMarkerTransform(mat, instMats, -1, &m);
                        return 1;
                    }
                    --remaining;
                }
            }
            continue;
        }

        for (int p = 0; p < hmodel.numHPrims; ++p) {
            const HPrim& prim = hmodel.hPrimList[p];
            if (prim.type == kHPrimTypeMarker && prim.data.hmarker->index == markerID) {
                ComputeMarkerTransform(mat, instMats, prim.segment, prim.data.hmarker);
                return 1;
            }
        }

        for (int d = 0; d < hmodel.numDynamicHMarkers; ++d) {
            const HMarker& m = hmodel.dynamicHMarkerList[d];
            if (m.index == markerID) {
                ComputeMarkerTransform(mat, instMats, m.segment, &m);
                return 1;
            }
        }
    }

    mat->Identity();
    return 0;
}

// 0x00425720 (PDB: 0x00424790)
int COLLIDE_GetHMarkerTransformFromID(BaseInstance* instance, int markerID, cdc::Matrix* mat, bool useOldMatrix)
{
    BaseInstance* unusedInst = nullptr;
    return COLLIDE_GetHMarkerTransformFromID(&unusedInst, instance, markerID, mat, useOldMatrix);
}
