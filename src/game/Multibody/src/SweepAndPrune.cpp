#include "game/Multibody/src/SweepAndPrune.h"

#include <cfloat>
#include <cstring>

// External diagnostics handler (0x00552e70)
extern void MB_Warning(const char* fmt, ...);

namespace {

inline void* AllocAligned16(size_t size)
{
    uint8_t* raw = new uint8_t[size + 16];
    uintptr_t aligned = (reinterpret_cast<uintptr_t>(raw) + 16) & ~static_cast<uintptr_t>(15);
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}

inline void FreeAligned16(void* ptr)
{
    if (ptr != nullptr) {
        delete[] reinterpret_cast<uint8_t*>(reinterpret_cast<void**>(ptr)[-1]);
    }
}

inline float AxisCoord(const cdc::Vector3& v, int axis)
{
    return v.m[axis];
}

inline void UpdateEndpointIndex(EndPoint& ep, int newIndex)
{
    *ep.index = newIndex;
}

} // namespace

// 0x00519020 (PDB: 0x0051ab50 / ?Create@SweepAndPrune@@SAPAU1@HH@Z)
SweepAndPrune* SweepAndPrune::Create(int maxBoxes, int maxPairs)
{
    return new SweepAndPruneImpl(maxBoxes, maxPairs);
}

// 0x00519060 (PDB: 0x0051ab90 / ?Destroy@SweepAndPrune@@SAXPAU1@@Z)
void SweepAndPrune::Destroy(SweepAndPrune* sap)
{
    if (sap != nullptr) {
        delete static_cast<SweepAndPruneImpl*>(sap);
    }
}

// 0x005186a0 (PDB: 0x0051a1d0 / ?CreateBox@SweepAndPrune@@QAEPAUSAPBox@@ABVVector3@cdc@@0I@Z)
SAPBox* SweepAndPrune::CreateBox(const cdc::Vector3& lo, const cdc::Vector3& hi, unsigned int flags)
{
    return static_cast<SweepAndPruneImpl*>(this)->CreateBoxImpl(lo, hi, flags);
}

// 0x005186b0 (PDB: 0x0051a1e0 / ?DestroyBox@SweepAndPrune@@QAEXPAUSAPBox@@@Z)
void SweepAndPrune::DestroyBox(SAPBox* box)
{
    static_cast<SweepAndPruneImpl*>(this)->DestroyBoxImpl(box);
}

// 0x00519090 (PDB: 0x0051abc0 / ?MoveBox@SweepAndPrune@@QAEXPAUSAPBox@@ABVVector3@cdc@@1@Z)
void SweepAndPrune::MoveBox(SAPBox* box, const cdc::Vector3& lo, const cdc::Vector3& hi)
{
    static_cast<SweepAndPruneImpl*>(this)->MoveBoxImpl(box, lo, hi);
}

// 0x005186c0 (PDB: 0x0051a1f0 / ?QueryBox@SweepAndPrune@@QAEHABVVector3@cdc@@0PAPAXH@Z)
int SweepAndPrune::QueryBox(const cdc::Vector3& lo, const cdc::Vector3& hi, void** results, int maxResults)
{
    return static_cast<SweepAndPruneImpl*>(this)->QueryBoxImpl(lo, hi, results, maxResults);
}

// 0x005190a0 (PDB: 0x0051abd0 / ?Relocate@SweepAndPrune@@QAEXABVVector3@cdc@@@Z)
void SweepAndPrune::Relocate(const cdc::Vector3& offset)
{
    static_cast<SweepAndPruneImpl*>(this)->RelocateImpl(offset);
}

// 0x005186d0 (PDB: 0x0051a200 / ?LineProbe@SweepAndPrune@@QAEPAXPAMAAVVector3@cdc@@ABV34@2IP6A_N0PAV34@22PAX4@Z4@Z)
void* SweepAndPrune::LineProbe(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    unsigned int axisMask,
    LineProbeCallback cb,
    void* userData)
{
    return static_cast<SweepAndPruneImpl*>(this)->LineProbeImpl(t, normal, start, dir, axisMask, cb, userData);
}

// PDB: 0x00611e20 / ?GetMemoryUsed@SweepAndPrune@@QBEHXZ
int SweepAndPrune::GetMemoryUsed() const
{
    return static_cast<const SweepAndPruneImpl*>(this)->memUsed;
}

// 0x005186e0 (PDB: 0x0051a210 / ??0SweepAndPruneImpl@@QAE@HH@Z)
SweepAndPruneImpl::SweepAndPruneImpl(int maxBoxes, int maxPairs)
    : boxPool(nullptr)
    , firstBox(nullptr)
    , pairPool(nullptr)
    , firstPair(nullptr)
    , hitList(nullptr)
    , memUsed(1)
    , maxb(maxBoxes)
    , nb(0)
    , maxp(maxPairs)
    , np(0)
    , n(0)
    , nhit(0)
    , timeStamp(0)
{
    const size_t boxPoolBytes = static_cast<size_t>(maxb) * sizeof(SAPBox);
    boxPool = static_cast<SAPBox*>(AllocAligned16(boxPoolBytes));
    memUsed += static_cast<int>(boxPoolBytes);
    for (int i = 0; i < maxb - 1; ++i) {
        boxPool[i].next = &boxPool[i + 1];
    }
    if (maxb > 0) {
        boxPool[maxb - 1].next = nullptr;
    }
    firstBox = boxPool;

    if (maxp > 0) {
        const size_t pairPoolBytes = static_cast<size_t>(maxp * 2) * sizeof(BoxPair);
        pairPool = new BoxPair[maxp * 2];
        memUsed += static_cast<int>(pairPoolBytes);
        for (int i = 0; i < maxp * 2 - 1; ++i) {
            pairPool[i].next = &pairPool[i + 1];
        }
        pairPool[maxp * 2 - 1].next = nullptr;
    } else {
        pairPool = nullptr;
    }
    firstPair = pairPool;

    const size_t hitListBytes = static_cast<size_t>(maxb) * sizeof(SAPBox*);
    hitList = new SAPBox*[maxb];
    memUsed += static_cast<int>(hitListBytes);

    const size_t axisBytes = static_cast<size_t>(maxb * 2) * sizeof(EndPoint);
    for (int axis = 0; axis < 3; ++axis) {
        pointList[axis] = new EndPoint[maxb * 2];
    }
    memUsed += static_cast<int>(axisBytes * 3);
}

// 0x00517610 (PDB: 0x00519140 / ??1SweepAndPruneImpl@@QAE@XZ)
SweepAndPruneImpl::~SweepAndPruneImpl()
{
    for (int axis = 0; axis < 3; ++axis) {
        delete[] pointList[axis];
    }
    delete[] hitList;
    delete[] pairPool;
    FreeAligned16(boxPool);
}

// 0x00517520 (PDB: 0x00519050 / ?Overlap@SweepAndPruneImpl@@KA_NPBUSAPBox@@0@Z)
bool SweepAndPruneImpl::Overlap(const SAPBox* a, const SAPBox* b)
{
    return a->lo.x <= b->hi.x && a->hi.x >= b->lo.x &&
           a->lo.y <= b->hi.y && a->hi.y >= b->lo.y &&
           a->lo.z <= b->hi.z && a->hi.z >= b->lo.z;
}

// 0x00517590 (PDB: 0x005190c0 / ?LowerBound@SweepAndPruneImpl@@KAHPBUEndPoint@@HM@Z)
int SweepAndPruneImpl::LowerBound(const EndPoint* points, int count, float value)
{
    int lo = 0;
    int hi = count;
    while (lo < hi) {
        const int mid = (lo + hi) >> 1;
        if (points[mid].value < value) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

// 0x005175d0 (PDB: 0x00519100 / ?UpperBound@SweepAndPruneImpl@@KAHPBUEndPoint@@HM@Z)
int SweepAndPruneImpl::UpperBound(const EndPoint* points, int count, float value)
{
    int lo = 0;
    int hi = count;
    while (lo < hi) {
        const int mid = (lo + hi) >> 1;
        if (points[mid].value > value) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return lo;
}

// 0x005176d0 (PDB: 0x00519200 / ?BumpTimeStamp@SweepAndPruneImpl@@IAEXXZ)
void SweepAndPruneImpl::BumpTimeStamp()
{
    ++timeStamp;
    if (timeStamp == 0) {
        timeStamp = 1;
        for (int i = 0; i < maxb; ++i) {
            boxPool[i].timeStamp = 0;
        }
    }
}

void SweepAndPruneImpl::InsertOne(SAPBox* box, SAPBox* other)
{
    if (firstPair == nullptr) {
        MB_Warning("WARNING: [SweepAndPrune] Out of space for pairs!\n");
        return;
    }

    BoxPair* pair = firstPair;
    firstPair = pair->next;
    ++np;

    pair->otherBox = other;
    pair->next = box->pairList;
    box->pairList = pair;
}

void SweepAndPruneImpl::DeleteOne(SAPBox* box, SAPBox* other)
{
    BoxPair** prev = &box->pairList;
    while (*prev != nullptr) {
        BoxPair* cur = *prev;
        if (cur->otherBox == other) {
            *prev = cur->next;
            cur->next = firstPair;
            firstPair = cur;
            --np;
            return;
        }
        prev = &cur->next;
    }
}

void SweepAndPruneImpl::InsertPair(SAPBox* a, SAPBox* b)
{
    if ((a->flags & SAPBox::FLAG_STATIC) == 0) {
        InsertOne(a, b);
    }
    if ((b->flags & SAPBox::FLAG_STATIC) == 0) {
        InsertOne(b, a);
    }
}

void SweepAndPruneImpl::RemovePair(SAPBox* a, SAPBox* b)
{
    if ((a->flags & SAPBox::FLAG_STATIC) == 0) {
        DeleteOne(a, b);
    }
    if ((b->flags & SAPBox::FLAG_STATIC) == 0) {
        DeleteOne(b, a);
    }
}

// 0x00517810 (PDB: 0x00519340 / ?FindHits@SweepAndPruneImpl@@IAEXABVVector3@cdc@@0@Z)
void SweepAndPruneImpl::FindHits(const cdc::Vector3& lo, const cdc::Vector3& hi)
{
    BumpTimeStamp();
    nhit = 0;

    for (int axis = 0; axis < 3; ++axis) {
        const EndPoint* points = pointList[axis];
        const int iLo = LowerBound(points, n, AxisCoord(lo, axis));
        const int iHi = UpperBound(points, n, AxisCoord(hi, axis));

        if (iLo > 0) {
            int stab = points[iLo - 1].stabbing;
            for (int i = iLo - 1; stab > 0 && i >= 0; --i) {
                if (points[i].type == EndPoint::LO) {
                    SAPBox* box = points[i].box;
                    if (points[box->hiIndex[axis]].value >= AxisCoord(lo, axis)) {
                        if (box->timeStamp == timeStamp) {
                            ++box->hitCount;
                        } else {
                            box->timeStamp = timeStamp;
                            box->hitCount = 1;
                        }
                        if (box->hitCount == 3) {
                            hitList[nhit++] = box;
                        }
                        --stab;
                    }
                }
            }
        }

        for (int i = iLo; i < iHi; ++i) {
            if (points[i].type == EndPoint::LO) {
                SAPBox* box = points[i].box;
                if (box->timeStamp == timeStamp) {
                    ++box->hitCount;
                } else {
                    box->timeStamp = timeStamp;
                    box->hitCount = 1;
                }
                if (box->hitCount == 3) {
                    hitList[nhit++] = box;
                }
            }
        }
    }
}

// 0x00517920 (PDB: 0x00519450 / ?CreateBoxImpl@SweepAndPruneImpl@@IAEPAUSAPBox@@ABVVector3@cdc@@0I@Z)
SAPBox* SweepAndPruneImpl::CreateBoxImpl(const cdc::Vector3& lo, const cdc::Vector3& hi, unsigned int flags)
{
    if (firstBox == nullptr) {
        MB_Warning("WARNING: [SweepAndPrune] Out of space for boxes!\n");
        return nullptr;
    }

    cdc::Vector3 clampedLo = lo;
    cdc::Vector3 clampedHi = hi;
    if (!lo.IsValid() || !hi.IsValid() || lo.x > hi.x || lo.y > hi.y || lo.z > hi.z) {
        clampedLo = cdc::Vector3::Make(-1.0f, -1.0f, -1.0f, 0.0f);
        clampedHi = cdc::Vector3::Make(1.0f, 1.0f, 1.0f, 0.0f);
    }

    SAPBox* box = firstBox;
    firstBox = box->next;
    ++nb;

    box->lo = clampedLo;
    box->hi = clampedHi;
    box->flags = flags;
    box->clientData = nullptr;
    box->next = nullptr;
    box->pairList = nullptr;
    box->hitCount = 0;
    box->timeStamp = 0;

    FindHits(clampedLo, clampedHi);
    for (int i = 0; i < nhit; ++i) {
        InsertPair(box, hitList[i]);
    }
    nhit = 0;

    for (int axis = 0; axis < 3; ++axis) {
        EndPoint* points = pointList[axis];
        const float loVal = AxisCoord(clampedLo, axis);
        const float hiVal = AxisCoord(clampedHi, axis);
        const int iLo = LowerBound(points, n, loVal);
        const int iHi = LowerBound(points, n, hiVal);

        if (iHi < n) {
            std::memmove(
                &points[iHi + 2],
                &points[iHi],
                static_cast<size_t>(n - iHi) * sizeof(EndPoint));
        }
        if (iLo < iHi) {
            std::memmove(
                &points[iLo + 1],
                &points[iLo],
                static_cast<size_t>(iHi - iLo) * sizeof(EndPoint));
        }

        const int newLoIdx = iLo;
        const int newHiIdx = iHi + 1;

        for (int i = newLoIdx + 1; i < newHiIdx; ++i) {
            ++points[i].stabbing;
        }

        points[newLoIdx].type = EndPoint::LO;
        points[newLoIdx].value = loVal;
        points[newLoIdx].stabbing = (newLoIdx > 0) ? (points[newLoIdx - 1].stabbing + 1) : 1;
        points[newLoIdx].box = box;
        points[newLoIdx].index = &box->loIndex[axis];

        points[newHiIdx].type = EndPoint::HI;
        points[newHiIdx].value = hiVal;
        points[newHiIdx].stabbing = points[newHiIdx - 1].stabbing - 1;
        points[newHiIdx].box = box;
        points[newHiIdx].index = &box->hiIndex[axis];

        for (int i = newLoIdx; i < n + 2; ++i) {
            UpdateEndpointIndex(points[i], i);
        }
    }

    n += 2;
    return box;
}

// 0x00517e60 (PDB: 0x00519990 / ?DestroyBoxImpl@SweepAndPruneImpl@@IAEXPAUSAPBox@@@Z)
void SweepAndPruneImpl::DestroyBoxImpl(SAPBox* box)
{
    if (box == nullptr) {
        return;
    }

    FindHits(box->lo, box->hi);
    for (int i = 0; i < nhit; ++i) {
        if (hitList[i] != box) {
            RemovePair(box, hitList[i]);
        }
    }
    nhit = 0;

    for (int axis = 0; axis < 3; ++axis) {
        EndPoint* points = pointList[axis];
        const int iLo = box->loIndex[axis];
        const int iHi = box->hiIndex[axis];

        if (iHi - iLo - 1 > 0) {
            std::memmove(
                &points[iLo],
                &points[iLo + 1],
                static_cast<size_t>(iHi - iLo - 1) * sizeof(EndPoint));
        }
        if (n - iHi - 1 > 0) {
            std::memmove(
                &points[iHi - 1],
                &points[iHi + 1],
                static_cast<size_t>(n - iHi - 1) * sizeof(EndPoint));
        }

        for (int i = iLo; i < n - 2; ++i) {
            UpdateEndpointIndex(points[i], i);
        }
        for (int i = iLo; i < iHi - 1; ++i) {
            --points[i].stabbing;
        }
    }

    n -= 2;

    while (box->pairList != nullptr) {
        BoxPair* p = box->pairList;
        box->pairList = p->next;
        p->next = firstPair;
        firstPair = p;
        --np;
    }

    box->next = firstBox;
    firstBox = box;
    --nb;
}

// 0x00518060 (PDB: 0x00519b90 / ?QueryBoxImpl@SweepAndPruneImpl@@IAEHABVVector3@cdc@@0PAPAXH@Z)
int SweepAndPruneImpl::QueryBoxImpl(const cdc::Vector3& lo, const cdc::Vector3& hi, void** results, int maxResults)
{
    if (!lo.IsValid() || !hi.IsValid()) {
        return 0;
    }

    FindHits(lo, hi);
    const int count = nhit < maxResults ? nhit : maxResults;
    for (int i = 0; i < count; ++i) {
        results[i] = hitList[i]->clientData;
    }
    nhit = 0;
    return count;
}

// 0x00518110 (PDB: 0x00519c40 / ?LineProbeImpl@SweepAndPruneImpl@@IAEPAXPAMAAVVector3@cdc@@ABV34@2IP6A_N0PAV34@22PAX4@Z4@Z)
void* SweepAndPruneImpl::LineProbeImpl(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    unsigned int axisMask,
    LineProbeCallback cb,
    void* userData)
{
    if (!start.IsValid() || !dir.IsValid()) {
        return nullptr;
    }

    BumpTimeStamp();

    int axes[3];
    int numAxes = 0;
    if ((axisMask & 1u) != 0) {
        axes[numAxes++] = 0;
    }
    if ((axisMask & 2u) != 0) {
        axes[numAxes++] = 1;
    }
    if ((axisMask & 4u) != 0) {
        axes[numAxes++] = 2;
    }
    if (numAxes == 0) {
        return nullptr;
    }

    void* result = nullptr;
    int index[3] = { 0, 0, 0 };
    nhit = 0;

    for (int k = 0; k < numAxes; ++k) {
        const int axis = axes[k];
        const EndPoint* points = pointList[axis];
        const float startCoord = AxisCoord(start, axis);
        const float dirCoord = AxisCoord(dir, axis);

        index[axis] = (dirCoord >= 0.0f)
            ? LowerBound(points, n, startCoord)
            : UpperBound(points, n, startCoord);

        if (index[axis] > 0) {
            int stab = points[index[axis] - 1].stabbing;
            for (int i = index[axis] - 1; stab > 0 && i >= 0; --i) {
                if (points[i].type == EndPoint::LO) {
                    SAPBox* box = points[i].box;
                    const float boxHiVal = points[box->hiIndex[axis]].value;
                    const bool containsStart = (dirCoord >= 0.0f)
                        ? (boxHiVal >= startCoord)
                        : (boxHiVal > startCoord);
                    if (containsStart) {
                        if (box->timeStamp == timeStamp) {
                            ++box->hitCount;
                        } else {
                            box->timeStamp = timeStamp;
                            box->hitCount = 1;
                        }
                        if (box->hitCount == numAxes) {
                            hitList[nhit++] = box;
                        }
                        --stab;
                    }
                }
            }
        }
    }

    for (int i = 0; i < nhit; ++i) {
        SAPBox* box = hitList[i];
        if (cb(t, normal, start, dir, box->clientData, userData)) {
            result = box->clientData;
        }
    }

    float invDir[3] = { 0.0f, 0.0f, 0.0f };
    float nextT[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
    int bestAxis = axes[0];

    for (int k = 0; k < numAxes; ++k) {
        const int axis = axes[k];
        const EndPoint* points = pointList[axis];
        const float startCoord = AxisCoord(start, axis);
        const float dirCoord = AxisCoord(dir, axis);
        invDir[axis] = (dirCoord != 0.0f) ? (1.0f / dirCoord) : 0.0f;

        if (dirCoord < 0.0f) {
            if (index[axis] > 0) {
                --index[axis];
                nextT[axis] = (points[index[axis]].value - startCoord) * invDir[axis];
            }
        } else if (dirCoord > 0.0f) {
            if (index[axis] < n) {
                nextT[axis] = (points[index[axis]].value - startCoord) * invDir[axis];
                ++index[axis];
            }
        }

        if (nextT[axis] < nextT[bestAxis]) {
            bestAxis = axis;
        }
    }

    while (nextT[bestAxis] <= *t) {
        nextT[bestAxis] = FLT_MAX;
        const EndPoint* points = pointList[bestAxis];
        const float startCoord = AxisCoord(start, bestAxis);
        const float dirCoord = AxisCoord(dir, bestAxis);

        if (dirCoord < 0.0f) {
            const EndPoint& ep = points[index[bestAxis]];
            if (ep.type == EndPoint::HI) {
                SAPBox* box = ep.box;
                if (box->timeStamp == timeStamp) {
                    ++box->hitCount;
                } else {
                    box->timeStamp = timeStamp;
                    box->hitCount = 1;
                }
                if (box->hitCount == numAxes) {
                    if (cb(t, normal, start, dir, box->clientData, userData)) {
                        result = box->clientData;
                    }
                }
            } else {
                --ep.box->hitCount;
            }

            if (index[bestAxis] > 0) {
                --index[bestAxis];
                nextT[bestAxis] = (points[index[bestAxis]].value - startCoord) * invDir[bestAxis];
            }
        } else {
            const EndPoint& ep = points[index[bestAxis] - 1];
            if (ep.type == EndPoint::LO) {
                SAPBox* box = ep.box;
                if (box->timeStamp == timeStamp) {
                    ++box->hitCount;
                } else {
                    box->timeStamp = timeStamp;
                    box->hitCount = 1;
                }
                if (box->hitCount == numAxes) {
                    if (cb(t, normal, start, dir, box->clientData, userData)) {
                        result = box->clientData;
                    }
                }
            } else {
                --ep.box->hitCount;
            }

            if (index[bestAxis] < n) {
                nextT[bestAxis] = (points[index[bestAxis]].value - startCoord) * invDir[bestAxis];
                ++index[bestAxis];
            }
        }

        for (int k = 0; k < numAxes; ++k) {
            const int axis = axes[k];
            if (nextT[axis] < nextT[bestAxis]) {
                bestAxis = axis;
            }
        }
    }

    nhit = 0;
    return result;
}

// 0x00518870 (PDB: 0x0051a3a0 / ?MoveBoxImpl@SweepAndPruneImpl@@IAEXPAUSAPBox@@ABVVector3@cdc@@1@Z)
void SweepAndPruneImpl::MoveBoxImpl(SAPBox* box, const cdc::Vector3& lo, const cdc::Vector3& hi)
{
    if (!lo.IsValid() || !hi.IsValid() || lo.x > hi.x || lo.y > hi.y || lo.z > hi.z) {
        return;
    }

    const cdc::Vector3 oldLo = box->lo;
    const cdc::Vector3 oldHi = box->hi;
    box->lo = lo;
    box->hi = hi;

    for (int axis = 0; axis < 3; ++axis) {
        EndPoint* points = pointList[axis];
        const float newLo = AxisCoord(lo, axis);
        const float newHi = AxisCoord(hi, axis);

        auto shiftEndpoint = [&](int idx, float targetVal, int epType) {
            EndPoint moving = points[idx];
            moving.value = targetVal;

            if ((epType == EndPoint::LO && targetVal < AxisCoord(oldLo, axis)) ||
                (epType == EndPoint::HI && targetVal < AxisCoord(oldHi, axis))) {
                while (idx > 0 && points[idx - 1].value > targetVal) {
                    EndPoint other = points[idx - 1];
                    points[idx] = other;
                    UpdateEndpointIndex(points[idx], idx);

                    if (epType == EndPoint::LO) {
                        ++points[idx].stabbing;
                        if (other.type == EndPoint::HI) {
                            if (Overlap(box, other.box)) {
                                InsertPair(box, other.box);
                            }
                            ++moving.stabbing;
                        } else {
                            --moving.stabbing;
                        }
                    } else {
                        --points[idx].stabbing;
                        if (other.type == EndPoint::HI) {
                            ++moving.stabbing;
                        } else {
                            RemovePair(box, other.box);
                            --moving.stabbing;
                        }
                    }
                    --idx;
                }
            } else {
                while (idx < n - 1 && points[idx + 1].value < targetVal) {
                    EndPoint other = points[idx + 1];
                    points[idx] = other;
                    UpdateEndpointIndex(points[idx], idx);

                    if (epType == EndPoint::HI) {
                        ++points[idx].stabbing;
                        if (other.type == EndPoint::LO) {
                            if (Overlap(box, other.box)) {
                                InsertPair(box, other.box);
                            }
                            ++moving.stabbing;
                        } else {
                            --moving.stabbing;
                        }
                    } else {
                        --points[idx].stabbing;
                        if (other.type == EndPoint::LO) {
                            ++moving.stabbing;
                        } else {
                            RemovePair(box, other.box);
                            --moving.stabbing;
                        }
                    }
                    ++idx;
                }
            }

            points[idx] = moving;
            UpdateEndpointIndex(points[idx], idx);
        };

        const float prevLo = AxisCoord(oldLo, axis);
        const float prevHi = AxisCoord(oldHi, axis);

        if (newLo < prevLo) {
            shiftEndpoint(box->loIndex[axis], newLo, EndPoint::LO);
            shiftEndpoint(box->hiIndex[axis], newHi, EndPoint::HI);
        } else if (newHi > prevHi) {
            shiftEndpoint(box->hiIndex[axis], newHi, EndPoint::HI);
            shiftEndpoint(box->loIndex[axis], newLo, EndPoint::LO);
        } else {
            if (newLo > prevLo) {
                shiftEndpoint(box->loIndex[axis], newLo, EndPoint::LO);
            }
            if (newHi < prevHi) {
                shiftEndpoint(box->hiIndex[axis], newHi, EndPoint::HI);
            }
        }
    }
}

// 0x00518f80 (PDB: 0x0051aab0 / ?RelocateImpl@SweepAndPruneImpl@@IAEXABVVector3@cdc@@@Z)
void SweepAndPruneImpl::RelocateImpl(const cdc::Vector3& offset)
{
    for (int i = 0; i < n; ++i) {
        EndPoint& ep = pointList[0][i];
        if (ep.type == EndPoint::LO) {
            ep.box->lo += offset;
            ep.box->hi += offset;
        }
    }

    for (int axis = 0; axis < 3; ++axis) {
        const float delta = AxisCoord(offset, axis);
        EndPoint* points = pointList[axis];
        for (int i = 0; i < n; ++i) {
            points[i].value += delta;
        }
    }
}
