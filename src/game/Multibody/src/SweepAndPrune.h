#ifndef GAME_MULTIBODY_SRC_SWEEPANDPRUNE_H
#define GAME_MULTIBODY_SRC_SWEEPANDPRUNE_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"

struct SAPBox;

// PDB: BoxPair (size: 0x08)
struct BoxPair {
    SAPBox* otherBox; // 0x00
    BoxPair* next;    // 0x04
};
static_assert(sizeof(BoxPair) == 0x08, "BoxPair size mismatch");

// PDB: SAPBox (size: 0x50)
struct alignas(16) SAPBox {
    enum Flags {
        FLAG_STATIC = 0x1,
        FLAG_DEACTIVATED = 0x2
    };

    cdc::Vector3 lo;        // 0x00
    cdc::Vector3 hi;        // 0x10
    unsigned int flags;     // 0x20
    void* clientData;       // 0x24
    SAPBox* next;           // 0x28
    BoxPair* pairList;      // 0x2c
    int loIndex[3];         // 0x30
    int hiIndex[3];         // 0x3c
    int hitCount;           // 0x48
    unsigned int timeStamp; // 0x4c
};
static_assert(sizeof(SAPBox) == 0x50, "SAPBox size mismatch");

// PDB: EndPoint (size: 0x14)
struct EndPoint {
    enum Type {
        LO = 0,
        HI = 1
    };

    int type;       // 0x00
    float value;    // 0x04
    int stabbing;   // 0x08
    SAPBox* box;    // 0x0c
    int* index;     // 0x10
};
static_assert(sizeof(EndPoint) == 0x14, "EndPoint size mismatch");

struct SweepAndPrune {
    enum TestAxes {
        TEST_X = 0x1,
        TEST_Y = 0x2,
        TEST_Z = 0x4,
        TEST_XY = TEST_X | TEST_Y,
        TEST_XZ = TEST_X | TEST_Z,
        TEST_YZ = TEST_Y | TEST_Z,
        TEST_ALL = TEST_X | TEST_Y | TEST_Z
    };

    typedef bool (*LineProbeCallback)(
        float* t,
        cdc::Vector3* normal,
        const cdc::Vector3& start,
        const cdc::Vector3& dir,
        void* clientData,
        void* userData);

    // 0x00519020 (PDB: 0x0051ab50 / ?Create@SweepAndPrune@@SAPAU1@HH@Z)
    static SweepAndPrune* Create(int maxBoxes, int maxPairs);

    // 0x00519060 (PDB: 0x0051ab90 / ?Destroy@SweepAndPrune@@SAXPAU1@@Z)
    static void Destroy(SweepAndPrune* sap);

    // 0x005186a0 (PDB: 0x0051a1d0 / ?CreateBox@SweepAndPrune@@QAEPAUSAPBox@@ABVVector3@cdc@@0I@Z)
    SAPBox* CreateBox(const cdc::Vector3& lo, const cdc::Vector3& hi, unsigned int flags);

    // 0x005186b0 (PDB: 0x0051a1e0 / ?DestroyBox@SweepAndPrune@@QAEXPAUSAPBox@@@Z)
    void DestroyBox(SAPBox* box);

    // 0x00519090 (PDB: 0x0051abc0 / ?MoveBox@SweepAndPrune@@QAEXPAUSAPBox@@ABVVector3@cdc@@1@Z)
    void MoveBox(SAPBox* box, const cdc::Vector3& lo, const cdc::Vector3& hi);

    // 0x005186c0 (PDB: 0x0051a1f0 / ?QueryBox@SweepAndPrune@@QAEHABVVector3@cdc@@0PAPAXH@Z)
    int QueryBox(const cdc::Vector3& lo, const cdc::Vector3& hi, void** results, int maxResults);

    // 0x005190a0 (PDB: 0x0051abd0 / ?Relocate@SweepAndPrune@@QAEXABVVector3@cdc@@@Z)
    void Relocate(const cdc::Vector3& offset);

    // 0x005186d0 (PDB: 0x0051a200 / ?LineProbe@SweepAndPrune@@QAEPAXPAMAAVVector3@cdc@@ABV34@2IP6A_N0PAV34@22PAX4@Z4@Z)
    void* LineProbe(
        float* t,
        cdc::Vector3* normal,
        const cdc::Vector3& start,
        const cdc::Vector3& dir,
        unsigned int axisMask,
        LineProbeCallback cb,
        void* userData);

    // PDB: 0x00611e20 / ?GetMemoryUsed@SweepAndPrune@@QBEHXZ
    int GetMemoryUsed() const;
};

// PDB: SweepAndPruneImpl (size: 0x40)
struct SweepAndPruneImpl : public SweepAndPrune {
    SAPBox* boxPool;            // 0x00
    SAPBox* firstBox;           // 0x04
    BoxPair* pairPool;          // 0x08
    BoxPair* firstPair;         // 0x0c
    SAPBox** hitList;           // 0x10
    EndPoint* pointList[3];     // 0x14
    int memUsed;                // 0x20
    int maxb;                   // 0x24
    int nb;                     // 0x28
    int maxp;                   // 0x2c
    int np;                     // 0x30
    int n;                      // 0x34
    int nhit;                   // 0x38
    unsigned int timeStamp;     // 0x3c

    // 0x005186e0 (PDB: 0x0051a210 / ??0SweepAndPruneImpl@@QAE@HH@Z)
    SweepAndPruneImpl(int maxBoxes, int maxPairs);

    // 0x00517610 (PDB: 0x00519140 / ??1SweepAndPruneImpl@@QAE@XZ)
    ~SweepAndPruneImpl();

    // 0x00517520 (PDB: 0x00519050 / ?Overlap@SweepAndPruneImpl@@KA_NPBUSAPBox@@0@Z)
    static bool Overlap(const SAPBox* a, const SAPBox* b);

    // 0x00517590 (PDB: 0x005190c0 / ?LowerBound@SweepAndPruneImpl@@KAHPBUEndPoint@@HM@Z)
    static int LowerBound(const EndPoint* points, int count, float value);

    // 0x005175d0 (PDB: 0x00519100 / ?UpperBound@SweepAndPruneImpl@@KAHPBUEndPoint@@HM@Z)
    static int UpperBound(const EndPoint* points, int count, float value);

    // 0x005176d0 (PDB: 0x00519200 / ?BumpTimeStamp@SweepAndPruneImpl@@IAEXXZ)
    void BumpTimeStamp();

    // 0x00517710 / 0x00517780 (pair list helpers)
    void InsertOne(SAPBox* box, SAPBox* other);
    void DeleteOne(SAPBox* box, SAPBox* other);
    void InsertPair(SAPBox* a, SAPBox* b);
    void RemovePair(SAPBox* a, SAPBox* b);

    // 0x00517810 (PDB: 0x00519340 / ?FindHits@SweepAndPruneImpl@@IAEXABVVector3@cdc@@0@Z)
    void FindHits(const cdc::Vector3& lo, const cdc::Vector3& hi);

    // 0x00517920 (PDB: 0x00519450 / ?CreateBoxImpl@SweepAndPruneImpl@@IAEPAUSAPBox@@ABVVector3@cdc@@0I@Z)
    SAPBox* CreateBoxImpl(const cdc::Vector3& lo, const cdc::Vector3& hi, unsigned int flags);

    // 0x00517e60 (PDB: 0x00519990 / ?DestroyBoxImpl@SweepAndPruneImpl@@IAEXPAUSAPBox@@@Z)
    void DestroyBoxImpl(SAPBox* box);

    // 0x00518060 (PDB: 0x00519b90 / ?QueryBoxImpl@SweepAndPruneImpl@@IAEHABVVector3@cdc@@0PAPAXH@Z)
    int QueryBoxImpl(const cdc::Vector3& lo, const cdc::Vector3& hi, void** results, int maxResults);

    // 0x00518110 (PDB: 0x00519c40 / ?LineProbeImpl@SweepAndPruneImpl@@IAEPAXPAMAAVVector3@cdc@@ABV34@2IP6A_N0PAV34@22PAX4@Z4@Z)
    void* LineProbeImpl(
        float* t,
        cdc::Vector3* normal,
        const cdc::Vector3& start,
        const cdc::Vector3& dir,
        unsigned int axisMask,
        LineProbeCallback cb,
        void* userData);

    // 0x00518870 (PDB: 0x0051a3a0 / ?MoveBoxImpl@SweepAndPruneImpl@@IAEXPAUSAPBox@@ABVVector3@cdc@@1@Z)
    void MoveBoxImpl(SAPBox* box, const cdc::Vector3& lo, const cdc::Vector3& hi);

    // 0x00518f80 (PDB: 0x0051aab0 / ?RelocateImpl@SweepAndPruneImpl@@IAEXABVVector3@cdc@@@Z)
    void RelocateImpl(const cdc::Vector3& offset);
};
static_assert(sizeof(SweepAndPruneImpl) == 0x40, "SweepAndPruneImpl size mismatch");

#endif // GAME_MULTIBODY_SRC_SWEEPANDPRUNE_H
