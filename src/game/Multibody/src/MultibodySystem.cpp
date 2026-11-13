#include "game/Multibody/src/MultibodyTypes.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/CollisionManager.h"
#include "game/Multibody/src/ContactCache.h"
#include "game/Multibody/src/ContactJoint.h"
#include "game/Multibody/src/Geometry.h"
#include "game/Multibody/src/SweepAndPrune.h"
#include "game/Multibody/src/SweptSphere.h"

// External diagnostics handler (0x00552e70)
extern void MB_Warning(const char* fmt, ...);

namespace {

inline void* AllocAlignedBytes(size_t size, size_t alignment)
{
    const size_t align = alignment < sizeof(void*) ? sizeof(void*) : alignment;
    uint8_t* raw = new uint8_t[size + align];
    const uintptr_t aligned = (reinterpret_cast<uintptr_t>(raw) + align) & ~static_cast<uintptr_t>(align - 1);
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}

inline void FreeAligned(void* ptr)
{
    if (ptr != nullptr) {
        delete[] reinterpret_cast<uint8_t*>(reinterpret_cast<void**>(ptr)[-1]);
    }
}

template <typename T>
inline T* AllocPoolArray(int count, int& memUsed, size_t alignment = alignof(T))
{
    const size_t bytes = static_cast<size_t>(count) * sizeof(T);
    memUsed += static_cast<int>(bytes);
    return static_cast<T*>(AllocAlignedBytes(bytes, alignment));
}

template <typename T>
inline void LinkFreeList(T* pool, int count)
{
    for (int i = 0; i < count - 1; ++i) {
        pool[i].next = &pool[i + 1];
    }
    pool[count - 1].next = nullptr;
}

template <typename T>
inline void PushActiveNode(T* node, T*& activeHead, int& activeCount)
{
    node->prev = nullptr;
    node->next = activeHead;
    if (activeHead != nullptr) {
        activeHead->prev = node;
    }
    activeHead = node;
    ++activeCount;
}

template <typename T>
inline void ReturnFreeNode(T* node, T*& freeHead, int& activeCount)
{
    node->next = freeHead;
    freeHead = node;
    --activeCount;
}

inline void ZeroContactForces(Contact* contacts, int numContacts)
{
    for (int i = 0; i < numContacts; ++i) {
        contacts[i].force = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

inline void InvalidateGeometryContactCache(MultibodySystemImpl* system, const GeometryImpl* geom)
{
    const int numPairs = system->size.numPairsPerGeometry;
    if (numPairs <= 0 || system->contactPairs == nullptr) {
        return;
    }

    const unsigned int staleStamp = system->timeStamp - 1;
    ContactPair* row = &system->contactPairs[geom->uniqueIndex * static_cast<unsigned int>(numPairs)];
    for (int i = 0; i < numPairs; ++i) {
        const GeometryImpl* other = row[i].other;
        row[i].timeStamp = staleStamp;
        if (other != nullptr) {
            ContactPair* otherRow = &system->contactPairs[other->uniqueIndex * static_cast<unsigned int>(numPairs)];
            for (int j = 0; j < numPairs; ++j) {
                if (otherRow[j].other == geom) {
                    otherRow[j].timeStamp = staleStamp;
                }
            }
        }
    }
}

inline ContactPair* FindOrAllocContactPair(
    MultibodySystemImpl* system,
    const GeometryImpl* geom,
    GeometryImpl* other)
{
    const int numPairs = system->size.numPairsPerGeometry;
    ContactPair* row = &system->contactPairs[geom->uniqueIndex * static_cast<unsigned int>(numPairs)];
    for (int i = 0; i < numPairs; ++i) {
        if (row[i].timeStamp == system->timeStamp) {
            if (row[i].other == other) {
                return &row[i];
            }
        } else {
            row[i].entries = nullptr;
            row[i].other = other;
            return &row[i];
        }
    }
    return nullptr;
}

} // namespace

// 0x00515e10 (PDB: 0x00517900 / ??$RemoveFromList@VJointImpl@@@@YAXPAVJointImpl@@PAPAV0@@Z)
template <typename T>
void RemoveFromList(T* item, T** head)
{
    if (item == *head) {
        *head = item->next;
        if (*head != nullptr) {
            (*head)->prev = nullptr;
        }
    } else {
        item->prev->next = item->next;
        if (item->next != nullptr) {
            item->next->prev = item->prev;
        }
    }
    item->next = nullptr;
    item->prev = nullptr;
}

template void RemoveFromList<JointImpl>(JointImpl* item, JointImpl** head);

// 0x005155c0 (PDB: 0x005170b0 / ?Disconnect@JointImpl@@QAEXXZ)
void JointImpl::Disconnect()
{
    if (body1 != nullptr) {
        body1->RemoveJoint(&node1);
    }
    if (body2 != nullptr) {
        body2->RemoveJoint(&node2);
    }
    body1 = nullptr;
    body2 = nullptr;
    node1.otherBody = nullptr;
    node1.next = nullptr;
    node2.otherBody = nullptr;
    node2.next = nullptr;
}

// 0x00517400 (PDB: 0x00518f30 / ?Create@MultibodySystem@@SAPAV1@ABUMultibodySize@@@Z)
MultibodySystem* MultibodySystem::Create(const MultibodySize& size)
{
    void* mem = AllocAlignedBytes(sizeof(MultibodySystemImpl), alignof(MultibodySystemImpl));
    if (mem == nullptr) {
        return nullptr;
    }
    return new (mem) MultibodySystemImpl(size);
}

// 0x00517440 (PDB: 0x00518f70 / ?Destroy@MultibodySystem@@SAXPAV1@@Z)
void MultibodySystem::Destroy(MultibodySystem* system)
{
    if (system != nullptr) {
        MultibodySystemImpl* impl = static_cast<MultibodySystemImpl*>(system);
        impl->~MultibodySystemImpl();
        FreeAligned(impl);
    }
}

// 0x00515620 (PDB: 0x00517110 / ?AddMeshCollection@MultibodySystem@@QAEXHPAPAVMesh@@H@Z)
void MultibodySystem::AddMeshCollection(int id, Mesh** meshes, int numMeshes)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    MeshCollection& coll = self->meshCollections[id];
    coll.numMeshes = numMeshes;
    for (int i = 0; i < numMeshes; ++i) {
        coll.meshes[i] = meshes[i];
    }
}

// 0x00515670 (PDB: 0x00517160 / ?RemoveMeshCollection@MultibodySystem@@QAEXH@Z)
void MultibodySystem::RemoveMeshCollection(int id)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    MeshCollection& coll = self->meshCollections[id];
    for (int i = 0; i < coll.numMeshes; ++i) {
        coll.meshes[i] = nullptr;
    }
    coll.numMeshes = 0;
}

// 0x005156c0 (PDB: 0x005171b0 / ?SetNearCallback@MultibodySystem@@QAEXEEP6A_NPAVMesh@@PAVGeometry@@1@Z@Z)
void MultibodySystem::SetNearCallback(uint8_t cat1, uint8_t cat2, bool (*cb)(Mesh*, Geometry*, Geometry*))
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    self->responseArray[cat1][cat2].nearFcn = cb;
    self->responseArray[cat2][cat1].nearFcn = cb;
}

// 0x005156f0 (PDB: 0x005171e0 / ?SetContactCallback@MultibodySystem@@QAEXEEP6A_NPAUContact@@H@Z@Z)
void MultibodySystem::SetContactCallback(uint8_t cat1, uint8_t cat2, bool (*cb)(Contact*, int))
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    self->responseArray[cat1][cat2].contactFcn = cb;
    self->responseArray[cat2][cat1].contactFcn = cb;
}

// 0x00515720 (PDB: 0x00517210 / ?SetLimitCallback@MultibodySystem@@QAEXP6AXABULimitReport@@@Z@Z)
void MultibodySystem::SetLimitCallback(void (*cb)(const LimitReport&))
{
    static_cast<MultibodySystemImpl*>(this)->limitFcn = cb;
}

// 0x00515730 (PDB: 0x00517220 / ?SetJointDestructionCallback@MultibodySystem@@QAEXP6AXPBVJoint@@@Z@Z)
void MultibodySystem::SetJointDestructionCallback(void (*cb)(const Joint*))
{
    static_cast<MultibodySystemImpl*>(this)->jointDestructionFcn = cb;
}

// 0x00515740 (PDB: 0x00517230 / ?SetGravity@MultibodySystem@@QAEXABVVector3@cdc@@@Z)
void MultibodySystem::SetGravity(const cdc::Vector3& gravity)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    self->gravity = gravity;
    self->gravityMagnitude = gravity.Len3();
}

// 0x005157c0 (PDB: 0x005172b0 / ?GetBodyID@MultibodySystem@@QBEHPBVBody@@@Z)
int MultibodySystem::GetBodyID(const Body* body) const
{
    if (body == nullptr) {
        return -1;
    }
    const MultibodySystemImpl* self = static_cast<const MultibodySystemImpl*>(this);
    return static_cast<int>(static_cast<const BodyImpl*>(body) - self->bodyPool);
}

// 0x005157f0 (PDB: 0x005172e0 / ?GetJointID@MultibodySystem@@QBEHPBVJoint@@@Z)
int MultibodySystem::GetJointID(const Joint* joint) const
{
    if (joint == nullptr) {
        return -1;
    }
    const MultibodySystemImpl* self = static_cast<const MultibodySystemImpl*>(this);
    return static_cast<int>(static_cast<const JointImpl*>(joint) - self->jointPool);
}

// 0x00515810 (PDB: 0x00517300 / ?QueryBox@MultibodySystem@@QAEHABVVector3@cdc@@0QAPAVGeometry@@H@Z)
int MultibodySystem::QueryBox(const cdc::Vector3& lo, const cdc::Vector3& hi, Geometry** results, int maxResults)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    return self->sweepAndPrune->QueryBox(lo, hi, reinterpret_cast<void**>(results), maxResults);
}

// 0x00515820 (PDB: 0x00517310 / ?SetDrawContactPoints@MultibodySystem@@QAEX_N@Z)
void MultibodySystem::SetDrawContactPoints(bool draw)
{
    static_cast<MultibodySystemImpl*>(this)->drawContacts = draw;
}

// 0x00515830 (PDB: 0x00517320 / ?SetDrawLineProbes@MultibodySystem@@QAEX_NABVVector3@cdc@@@Z)
void MultibodySystem::SetDrawLineProbes(bool draw, const cdc::Vector3& color)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    self->drawLineProbes = draw;
    self->lineProbeColor = color;
}

// 0x00515860 (PDB: 0x00517350 / ?SetContactSoftness@MultibodySystem@@QAEXM@Z)
void MultibodySystem::SetContactSoftness(float gamma)
{
    static_cast<MultibodySystemImpl*>(this)->contactGamma = gamma;
}

// 0x00515870 (PDB: 0x00517360 / ?SetBaumgarte@MultibodySystem@@QAEXM@Z)
void MultibodySystem::SetBaumgarte(float beta)
{
    static_cast<MultibodySystemImpl*>(this)->beta = beta;
}

// 0x00515880 (PDB: 0x00517370 / ?SetCollisionThreshold@MultibodySystem@@QAEXM@Z)
void MultibodySystem::SetCollisionThreshold(float threshold)
{
    static_cast<MultibodySystemImpl*>(this)->threshold = threshold;
}

// 0x00515890 (PDB: 0x00517380 / ?SetBoundaryThickness@MultibodySystem@@QAEXM@Z)
void MultibodySystem::SetBoundaryThickness(float thickness)
{
    static_cast<MultibodySystemImpl*>(this)->thickness = thickness;
}

// 0x005158a0 (PDB: 0x00517390 / ?SetContactTolerance@MultibodySystem@@QAEXM@Z)
void MultibodySystem::SetContactTolerance(float tolerance)
{
    static_cast<MultibodySystemImpl*>(this)->contactTolerance = tolerance;
}

// 0x005158b0 (PDB: 0x005173a0 / ?SetSleepDelay@MultibodySystem@@QAEXM@Z)
void MultibodySystem::SetSleepDelay(float delay)
{
    static_cast<MultibodySystemImpl*>(this)->sleepTimerInit = delay;
}

// 0x005164c0 (PDB: 0x00517ff0 / ?SetStepIterations@MultibodySystem@@QAEXE@Z)
void MultibodySystem::SetStepIterations(uint8_t iterations)
{
    static_cast<MultibodySystemImpl*>(this)->stepIterations = iterations < 1 ? 1 : static_cast<short>(iterations);
}

// 0x00515e80 (PDB: 0x00517970 / ?CreateBody@MultibodySystem@@QAEPAVBody@@XZ)
Body* MultibodySystem::CreateBody()
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    BodyImpl* body = self->freeBody;
    if (body == nullptr) {
        MB_Warning("WARNING: Out of space for Bodies!\n");
        return nullptr;
    }
    if ((self->lock & 1) != 0) {
        return nullptr;
    }

    self->freeBody = body->next;
    body->Clear(self);
    PushActiveNode(body, self->bodyList, self->nb);
    return body;
}

// 0x00515f00 (PDB: 0x005179f0 / ?DestroyBody@MultibodySystem@@QAEXPAVBody@@@Z)
void MultibodySystem::DestroyBody(Body* bodyArg)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    if (bodyArg == nullptr || (self->lock & 1) != 0) {
        return;
    }

    BodyImpl* body = static_cast<BodyImpl*>(bodyArg);
    GeometryImpl* geom = body->firstGeometry;
    while (geom != nullptr) {
        GeometryImpl* nextGeom = geom->bnext;
        if (geom->clientDestruct != nullptr) {
            geom->clientDestruct(geom->clientData, geom);
        }
        InvalidateGeometryContactCache(self, geom);
        if (geom->aabb != nullptr) {
            self->sweepAndPrune->DestroyBox(geom->aabb);
        }
        RemoveFromList(geom, &self->geometryList);
        ReturnFreeNode(geom, self->freeGeometry, self->ng);
        geom = nextGeom;
    }

    JointNode* node = body->firstJoint;
    while (node != nullptr) {
        JointNode* nextNode = node->next;
        JointImpl* joint = node->joint;
        if (joint->clientDestruct != nullptr) {
            joint->clientDestruct(joint->clientData, joint);
        }
        if (self->jointDestructionFcn != nullptr) {
            self->jointDestructionFcn(joint);
        }
        if (node->otherBody != nullptr) {
            JointNode* otherNode = (node == &joint->node1) ? &joint->node2 : &joint->node1;
            node->otherBody->RemoveJoint(otherNode);
        }
        RemoveFromList(joint, &self->firstJoint);
        ReturnFreeNode(joint, self->freeJoint, self->nj);
        node = nextNode;
    }

    RemoveFromList(body, &self->bodyList);
    ReturnFreeNode(body, self->freeBody, self->nb);
}

// 0x005161a0 (PDB: 0x00517ca0 / ?CreateGeometry@MultibodySystem@@QAEPAVGeometry@@XZ)
Geometry* MultibodySystem::CreateGeometry()
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    GeometryImpl* geom = self->freeGeometry;
    if (geom == nullptr) {
        MB_Warning("WARNING: Out of space for Geometries!\n");
        return nullptr;
    }
    if ((self->lock & 1) != 0) {
        return nullptr;
    }

    self->freeGeometry = geom->next;
    geom->Clear();
    geom->sweepAndPrune = self->sweepAndPrune;
    PushActiveNode(geom, self->geometryList, self->ng);
    return geom;
}

// 0x00516220 (PDB: 0x00517d30 / ?DestroyGeometry@MultibodySystem@@QAEXPAVGeometry@@@Z)
void MultibodySystem::DestroyGeometry(Geometry* geomArg)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    if (geomArg == nullptr || (self->lock & 1) != 0) {
        return;
    }

    GeometryImpl* geom = static_cast<GeometryImpl*>(geomArg);
    if (geom->clientDestruct != nullptr) {
        geom->clientDestruct(geom->clientData, geom);
    }
    InvalidateGeometryContactCache(self, geom);
    if (geom->aabb != nullptr) {
        self->sweepAndPrune->DestroyBox(geom->aabb);
    }
    if (geom->body != nullptr) {
        geom->body->RemoveGeometry(geom);
    }
    RemoveFromList(geom, &self->geometryList);
    ReturnFreeNode(geom, self->freeGeometry, self->ng);
}

// 0x00516340 (PDB: 0x00517e60 / ?CreateJoint@MultibodySystem@@QAEPAVJoint@@XZ)
Joint* MultibodySystem::CreateJoint()
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    JointImpl* joint = self->freeJoint;
    if (joint == nullptr) {
        MB_Warning("WARNING: Out of space for Joints!\n");
        return nullptr;
    }
    if ((self->lock & 1) != 0) {
        return nullptr;
    }

    self->freeJoint = joint->next;
    joint->Clear();
    PushActiveNode(joint, self->firstJoint, self->nj);
    return joint;
}

// 0x005163c0 (PDB: 0x00517ee0 / ?DestroyJoint@MultibodySystem@@QAEXPAVJoint@@@Z)
void MultibodySystem::DestroyJoint(Joint* jointArg)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    if (jointArg == nullptr || (self->lock & 1) != 0) {
        return;
    }

    JointImpl* joint = static_cast<JointImpl*>(jointArg);
    if (self->jointDestructionFcn != nullptr) {
        self->jointDestructionFcn(joint);
    }
    if (joint->body1 != nullptr) {
        joint->body1->WakeUp();
    }
    if (joint->body2 != nullptr) {
        joint->body2->WakeUp();
    }
    joint->Disconnect();
    RemoveFromList(joint, &self->firstJoint);
    ReturnFreeNode(joint, self->freeJoint, self->nj);
}

// 0x00516440 (PDB: 0x00517f70 / ?QueryGeometry@MultibodySystem@@QAEHPAUContact@@HPAVGeometry@@@Z)
int MultibodySystem::QueryGeometry(Contact* contacts, int maxContacts, Geometry* geom)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    GeometryImpl* g = static_cast<GeometryImpl*>(geom);
    int numContacts = CollideGeometryAndMesh(contacts, maxContacts, self, g);
    numContacts += CollideGeometryAndGeometries(contacts + numContacts, maxContacts - numContacts, self, g);
    return numContacts;
}

// 0x00517470 (PDB: 0x00518fa0 / ?Relocate@MultibodySystem@@QAEXABVVector3@cdc@@@Z)
void MultibodySystem::Relocate(const cdc::Vector3& offset)
{
    static_cast<MultibodySystemImpl*>(this)->RelocateImpl(offset);
}

// 0x00517510 (PDB: 0x00519040 / ?Step@MultibodySystem@@QAEXM_N@Z)
void MultibodySystem::Step(float dt, bool autoSleep)
{
    static_cast<MultibodySystemImpl*>(this)->StepImpl(dt, autoSleep);
}

// 0x005164f0 (PDB: 0x00518020 / ??0MultibodySystemImpl@@QAE@ABUMultibodySize@@@Z)
MultibodySystemImpl::MultibodySystemImpl(const MultibodySize& sz)
{
    InitColliderArray();

    size = sz;
    memUsed = static_cast<int>(sizeof(MultibodySystemImpl));

    if (size.numBodies != 0) {
        bodyPool = AllocPoolArray<BodyImpl>(size.numBodies, memUsed);
        LinkFreeList(bodyPool, size.numBodies);
        freeBody = bodyPool;
    } else {
        bodyPool = nullptr;
        freeBody = nullptr;
    }
    bodyList = nullptr;
    nb = 0;

    if (size.numGeometries != 0) {
        geometryPool = AllocPoolArray<GeometryImpl>(size.numGeometries, memUsed);
        for (int i = 0; i < size.numGeometries; ++i) {
            geometryPool[i].uniqueIndex = static_cast<unsigned int>(i);
        }
        LinkFreeList(geometryPool, size.numGeometries);
        freeGeometry = geometryPool;
    } else {
        geometryPool = nullptr;
        freeGeometry = nullptr;
    }
    geometryList = nullptr;
    ng = 0;

    if (size.numJoints != 0) {
        jointPool = AllocPoolArray<JointImpl>(size.numJoints, memUsed);
        LinkFreeList(jointPool, size.numJoints);
        freeJoint = jointPool;
    } else {
        jointPool = nullptr;
        freeJoint = nullptr;
    }
    firstJoint = nullptr;
    nj = 0;

    if (size.numContactPoints != 0) {
        contactPool = AllocPoolArray<ContactJoint>(size.numContactPoints, memUsed);
    } else {
        contactPool = nullptr;
    }
    nc = 0;

    if (size.numContactPoints != 0 && size.numPairsPerGeometry != 0) {
        const int numEntries = size.numContactPoints * ContactJoint::MAX_POINTS;
        const int totalPairs = size.numGeometries * size.numPairsPerGeometry;
        contactEntryPool = AllocPoolArray<ContactEntry>(numEntries, memUsed);
        contactPairs = AllocPoolArray<ContactPair>(totalPairs, memUsed);
        for (int i = 0; i < totalPairs; ++i) {
            contactPairs[i].other = nullptr;
            contactPairs[i].timeStamp = 0;
        }
    } else {
        contactEntryPool = nullptr;
        contactPairs = nullptr;
    }
    freeContactEntry = 0;
    timeStamp = 1;
    maxContactsUsed = 0;

    const int maxRows = 6 * size.numJoints + 8 * size.numContactPoints;
    const int stackCount = (size.numJoints + size.numContactPoints < size.numBodies)
        ? (size.numJoints + size.numContactPoints)
        : size.numBodies;

    bodies = AllocPoolArray<BodyImpl*>(size.numBodies, memUsed);
    joints = AllocPoolArray<JointImpl*>(size.numJoints, memUsed);
    contactJoints = AllocPoolArray<ContactJoint*>(size.numContactPoints, memUsed);
    islandStack = AllocPoolArray<BodyImpl*>(stackCount, memUsed);
    islandQueue = AllocPoolArray<BodyImpl*>(size.numBodies, memUsed);
    invI_global = AllocPoolArray<cdc::Matrix>(size.numBodies, memUsed);
    J = AllocPoolArray<ContactJacobianRow>(maxRows, memUsed, 64);
    B = AllocPoolArray<ContactJacobianRow>(maxRows, memUsed, 64);
    fc = AllocPoolArray<cdc::Vector3>(size.numBodies * 2, memUsed);
    JMap = AllocPoolArray<int[2]>(maxRows, memUsed);
    cPos = AllocPoolArray<float>(maxRows, memUsed);
    cVel = AllocPoolArray<float>(maxRows, memUsed);
    lo = AllocPoolArray<float>(maxRows, memUsed);
    hi = AllocPoolArray<float>(maxRows, memUsed);
    gamma = AllocPoolArray<float>(maxRows, memUsed);
    rhs = AllocPoolArray<float>(maxRows, memUsed);
    lambda = AllocPoolArray<float>(maxRows, memUsed);
    d = AllocPoolArray<float>(maxRows, memUsed);

    if (size.numMeshCollections > 0) {
        meshCollections = AllocPoolArray<MeshCollection>(size.numMeshCollections, memUsed);
        for (int i = 0; i < size.numMeshCollections; ++i) {
            meshCollections[i].meshes = AllocPoolArray<Mesh*>(size.numMeshesPerCollection, memUsed);
            std::memset(meshCollections[i].meshes, 0, static_cast<size_t>(size.numMeshesPerCollection) * sizeof(Mesh*));
            meshCollections[i].boxes = nullptr;
            meshCollections[i].numMeshes = 0;
        }
    } else {
        meshCollections = nullptr;
    }

    gravity = cdc::Vector3::Make(0.0f, 0.0f, -3.0f, 0.0f);
    gravityMagnitude = 3.0f;
    contactGamma = 0.0f;
    threshold = 0.1f;
    beta = 0.1f;
    thickness = 1.0f;
    sorOmega = 1.0f;
    contactTolerance = 10.0f;
    sleepTimerInit = 30.0f;
    inv_dt = 0.0f;
    limitFcn = nullptr;
    jointDestructionFcn = nullptr;
    lock = 0;
    drawContacts = false;
    drawLineProbes = false;
    stepIterations = 5;
    lineProbeColor = cdc::Vector3::Make(1.0f, 1.0f, 1.0f, 0.0f);

    sweepAndPrune = SweepAndPrune::Create(size.numGeometries, size.numGeometries * 4);
    memUsed += sweepAndPrune->GetMemoryUsed();

    std::memset(responseArray, 0, sizeof(responseArray));
}

// 0x005158c0 (PDB: 0x005173b0 / ??1MultibodySystemImpl@@QAE@XZ)
MultibodySystemImpl::~MultibodySystemImpl()
{
    FreeAligned(bodyPool);
    FreeAligned(geometryPool);
    FreeAligned(jointPool);
    FreeAligned(contactPool);
    FreeAligned(contactEntryPool);
    FreeAligned(contactPairs);
    FreeAligned(bodies);
    FreeAligned(joints);
    FreeAligned(contactJoints);
    FreeAligned(islandStack);
    FreeAligned(islandQueue);
    FreeAligned(invI_global);
    FreeAligned(J);
    FreeAligned(B);
    FreeAligned(JMap);
    FreeAligned(cPos);
    FreeAligned(cVel);
    FreeAligned(lo);
    FreeAligned(hi);
    FreeAligned(fc);
    FreeAligned(rhs);
    FreeAligned(lambda);
    FreeAligned(d);
    FreeAligned(gamma);

    if (meshCollections != nullptr) {
        for (int i = 0; i < size.numMeshCollections; ++i) {
            FreeAligned(meshCollections[i].meshes);
        }
        FreeAligned(meshCollections);
    }

    SweepAndPrune::Destroy(sweepAndPrune);
}

// 0x00515c40 (PDB: 0x00517730 / ?CreateContactJoint@MultibodySystemImpl@@QAEPAVContactJoint@@XZ)
ContactJoint* MultibodySystemImpl::CreateContactJoint()
{
    if (nc >= size.numContactPoints) {
        MB_Warning("WARNING: Out of space for Contacts!\n");
        return nullptr;
    }
    return &contactPool[nc++];
}

// 0x00515c80 (PDB: 0x00517770 / ?DestroyContactJoints@MultibodySystemImpl@@AAEXXZ)
void MultibodySystemImpl::DestroyContactJoints()
{
    for (int i = 0; i < nc; ++i) {
        if (contactPool[i].body1 != nullptr) {
            contactPool[i].body1->firstContact = nullptr;
        }
        if (contactPool[i].body2 != nullptr) {
            contactPool[i].body2->firstContact = nullptr;
        }
    }
    nc = 0;
}

// 0x00515ce0 (PDB: 0x005177d0 / ?QueryContactCache@MultibodySystemImpl@@QBEXPAUContact@@H@Z)
void MultibodySystemImpl::QueryContactCache(Contact* contacts, int numContacts) const
{
    if (numContacts == 0) {
        return;
    }

    if (contactPairs == nullptr || contactEntryPool == nullptr) {
        ZeroContactForces(contacts, numContacts);
        return;
    }

    const GeometryImpl* g1 = static_cast<const GeometryImpl*>(contacts[0].geometry1);
    const GeometryImpl* g2 = static_cast<const GeometryImpl*>(contacts[0].geometry2);
    const GeometryImpl* keyGeom = (g1 != nullptr) ? g1 : g2;
    const GeometryImpl* matchOther = (g1 != nullptr) ? g2 : nullptr;

    ContactPair* row = &contactPairs[keyGeom->uniqueIndex * static_cast<unsigned int>(size.numPairsPerGeometry)];
    for (int i = 0; i < size.numPairsPerGeometry; ++i) {
        if (row[i].other == matchOther && row[i].timeStamp == timeStamp) {
            row[i].FitForces(contacts, static_cast<unsigned int>(numContacts), contactTolerance);
            return;
        }
    }

    ZeroContactForces(contacts, numContacts);
}

// 0x00516e70 (PDB: 0x005189a0 / ?RelocateImpl@MultibodySystemImpl@@QAEXABVVector3@cdc@@@Z)
void MultibodySystemImpl::RelocateImpl(const cdc::Vector3& offset)
{
    for (BodyImpl* body = bodyList; body != nullptr; body = body->next) {
        body->x = body->x + offset;
    }

    for (GeometryImpl* geom = geometryList; geom != nullptr; geom = geom->next) {
        geom->x0 = geom->x0 + offset;
        if (geom->body != nullptr) {
            geom->x = geom->x + offset;
        } else {
            geom->dx = geom->dx + offset;
            geom->x = geom->dx;
        }
    }

    sweepAndPrune->Relocate(offset);

    for (JointImpl* joint = firstJoint; joint != nullptr; joint = joint->next) {
        if (joint->body1 == nullptr) {
            joint->anchor1 = joint->anchor1 + offset;
        }
        if (joint->type == 6) {
            joint->bx1 = joint->bx1 + offset;
            joint->bx2 = joint->bx2 + offset;
        }
        if (joint->type == 0x0c) {
            if ((joint->flags & 0x80u) != 0 && joint->body1 != nullptr) {
                joint->linkCoord1 += cdc::Dot3(joint->body1->q.Rotate(joint->bz1), offset);
            }
            if ((joint->flags & 0x100u) != 0 && joint->body2 != nullptr) {
                joint->linkCoord2 += cdc::Dot3(joint->body2->q.Rotate(joint->bz2), offset);
            }
        }
    }

    for (int i = 0; i < freeContactEntry; ++i) {
        contactEntryPool[i].position = contactEntryPool[i].position + offset;
    }
}

// 0x00517070 (PDB: 0x00518ba0 / ?StoreContactForces@MultibodySystemImpl@@AAEXXZ)
void MultibodySystemImpl::StoreContactForces()
{
    if (contactPairs == nullptr || contactEntryPool == nullptr) {
        return;
    }

    ++timeStamp;
    if (timeStamp == 0) {
        timeStamp = 1;
        const int totalPairs = size.numPairsPerGeometry * size.numGeometries;
        for (int i = 0; i < totalPairs; ++i) {
            contactPairs[i].timeStamp = 0;
        }
    }

    freeContactEntry = 0;
    for (int c = 0; c < nc; ++c) {
        const ContactJoint* cj = &contactPool[c];
        cdc::Vector3 u;
        cdc::Vector3 v;
        ComputeBasis(&u, &v, cj->normal);

        const int numPts = static_cast<int>(cj->flags & 0xfu);
        for (int p = 0; p < numPts; ++p) {
            const cdc::Vector3 force =
                cj->normal * cj->lambda[p] +
                u * cj->lambda[numPts] +
                v * cj->lambda[numPts + 1];

            GeometryImpl* g1 = cj->geometry1[p];
            GeometryImpl* g2 = cj->geometry2[p];

            ContactEntry* entry = &contactEntryPool[freeContactEntry];
            entry->position = cj->p[p];
            entry->force = force;
            entry->onto = g2;
            entry->used = false;

            ContactPair* pair1 = (g1 != nullptr) ? FindOrAllocContactPair(this, g1, g2) : nullptr;
            ContactPair* pair2 = (g2 != nullptr) ? FindOrAllocContactPair(this, g2, g1) : nullptr;

            const bool ok1 = (g1 == nullptr) || (pair1 != nullptr);
            const bool ok2 = (g2 == nullptr) || (pair2 != nullptr);
            if (ok1 && ok2 && (pair1 != nullptr || pair2 != nullptr)) {
                ContactPair* headPair = (pair1 != nullptr) ? pair1 : pair2;
                entry->next = headPair->entries;
                if (pair1 != nullptr) {
                    pair1->entries = entry;
                    pair1->timeStamp = timeStamp;
                }
                if (pair2 != nullptr) {
                    pair2->entries = entry;
                    pair2->timeStamp = timeStamp;
                }
                ++freeContactEntry;
            } else {
                MB_Warning("WARNING: Multibody Contact Cache Full!\n");
            }
        }
    }
}

// 0x00517480 (PDB: 0x00518fb0 / ?StepImpl@MultibodySystemImpl@@QAEXM_N@Z)
void MultibodySystemImpl::StepImpl(float dt, bool autoSleep)
{
    lock = 3;
    CollidePhantoms(this);
    CollideGeometries(this);
    CollideMesh(this);
    if (maxContactsUsed < nc) {
        maxContactsUsed = nc;
    }
    lock |= 8;
    if (dt > 0.0f) {
        TimeStepIslands(dt, autoSleep);
    }
    StoreContactForces();
    DestroyContactJoints();
    lock = 0;
}
