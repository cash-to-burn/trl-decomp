#ifndef GAME_MULTIBODY_SRC_GEOMETRY_H
#define GAME_MULTIBODY_SRC_GEOMETRY_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"

class Body;
struct BodyImpl;
struct SAPBox;
struct SweepAndPrune;

class Geometry {
public:
    enum Type {
        Box = 0,
        Capsule = 1,
        Plane = 2,
        Sphere = 3,
        None = 4
    };

    // 0x00508790 (PDB: 0x0050a260 / ?GetType@Geometry@@QBE?AW4Type@1@XZ)
    Type GetType() const;

    // 0x005087a0 (PDB: 0x0050a270 / ?SetFlags@Geometry@@QAEXE@Z)
    void SetFlags(uint8_t flags);

    // 0x005087d0 (PDB: 0x0050a2a0 / ?GetFlags@Geometry@@QBEEXZ)
    uint8_t GetFlags() const;

    // 0x005087e0 (PDB: 0x0050a2b0 / ?SetClientData@Geometry@@QAEXPAX@Z)
    void SetClientData(void* clientData);

    // 0x005087f0 (PDB: 0x0050a2c0 / ?GetClientData@Geometry@@QBEPAXXZ)
    void* GetClientData() const;

    // 0x00508800 (PDB: 0x0050a2d0 / ?SetCategory@Geometry@@QAEXE@Z)
    void SetCategory(uint8_t category);

    // 0x00508830 (PDB: 0x0050a300 / ?GetCategory@Geometry@@QBEEXZ)
    uint8_t GetCategory() const;

    // 0x00508840 (PDB: 0x0050a310 / ?GetSphereRadius@Geometry@@QBEMXZ)
    float GetSphereRadius() const;

    // 0x00508840 (PDB: 0x0050a310 / ?GetCapsuleRadius@Geometry@@QBEMXZ)
    float GetCapsuleRadius() const;

    // 0x00508850 (PDB: 0x0050a320 / ?GetCapsuleLength@Geometry@@QBEMXZ)
    float GetCapsuleLength() const;

    // 0x00508860 (PDB: 0x0050a330 / ?GetGenericRadius@Geometry@@QBEMXZ)
    float GetGenericRadius() const;

    // PDB: 0x00479550 / ?GetBoxWidth@Geometry@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetBoxWidth() const;

    // 0x005088d0 (PDB: 0x0050a3a0 / ?GetSurfaceArea@Geometry@@QBEMXZ)
    float GetSurfaceArea() const;

    // 0x00508980 (PDB: 0x0050a450 / ?SetFriction@Geometry@@QAEXM@Z)
    void SetFriction(float friction);

    // 0x00508990 (PDB: 0x0050a460 / ?GetFriction@Geometry@@QBEMXZ)
    float GetFriction() const;

    // 0x005089a0 (PDB: 0x0050a470 / ?SetRestitution@Geometry@@QAEXM@Z)
    void SetRestitution(float restitution);

    // 0x005089b0 (PDB: 0x0050a480 / ?GetQuaternion@Geometry@@QBE?AVQuat@cdc@@XZ)
    cdc::Quat GetQuaternion() const;

    // PDB: 0x005e5440 / ?GetPosition@Geometry@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetPosition() const;

    // PDB: 0x004e3520 / ?GetBody@Geometry@@QAEPAVBody@@XZ
    Body* GetBody();

    // 0x00508b50 (PDB: 0x0050a620 / ?AddBuoyancyForce@Geometry@@QAEXABVVector3@cdc@@MM0MM@Z)
    void AddBuoyancyForce(
        const cdc::Vector3& planeNormal,
        float planeDist,
        float liquidDensity,
        const cdc::Vector3& liquidVelocity,
        float linearDrag,
        float angularDrag);

    // 0x00508cf0 (PDB: 0x0050a7c0 / ?GetClosestPoint@Geometry@@QBEXPAVVector3@cdc@@0ABV23@@Z)
    void GetClosestPoint(
        cdc::Vector3* outPoint,
        cdc::Vector3* outNormal,
        const cdc::Vector3& point) const;

    // 0x00509760 (PDB: 0x0050b230 / ?SetBox@Geometry@@QAEXABVVector3@cdc@@@Z)
    void SetBox(const cdc::Vector3& width);

    // 0x00509800 (PDB: 0x0050b2d0 / ?SetSphere@Geometry@@QAEXM@Z)
    void SetSphere(float radius);

    // 0x00509830 (PDB: 0x0050b300 / ?SetCapsule@Geometry@@QAEXMM@Z)
    void SetCapsule(float radius, float length);

    // 0x00509870 (PDB: 0x0050b340 / ?SetCapsuleRadius@Geometry@@QAEXM@Z)
    void SetCapsuleRadius(float radius);

    // 0x005098a0 (PDB: 0x0050b370 / ?SetCapsuleLength@Geometry@@QAEXM@Z)
    void SetCapsuleLength(float length);

    // 0x005098d0 (PDB: 0x0050b3a0 / ?SetRelativePosition@Geometry@@QAEXABVVector3@cdc@@@Z)
    void SetRelativePosition(const cdc::Vector3& relPos);

    // 0x00509930 (PDB: 0x0050b400 / ?SetRelativeQuaternion@Geometry@@QAEXABVQuat@cdc@@@Z)
    void SetRelativeQuaternion(const cdc::Quat& relRot);

    // 0x00509970 (PDB: 0x0050b440 / ?MoveTo@Geometry@@QAEXABVVector3@cdc@@@Z)
    void MoveTo(const cdc::Vector3& pos);
};

// PDB: GeometryImpl (size: 0xa0)
struct alignas(16) GeometryImpl : public Geometry {
    cdc::Vector3 dx;                // 0x00
    cdc::Vector3 x;                 // 0x10
    cdc::Vector3 x0;                // 0x20
    cdc::Quat dq;                   // 0x30
    cdc::Quat q;                    // 0x40
    cdc::Vector3 width;             // 0x50
    float radius;                   // 0x60
    float length;                   // 0x64
    float volume;                   // 0x68
    float restitution;              // 0x6c
    float mu;                       // 0x70
    GeometryImpl* prev;             // 0x74
    GeometryImpl* next;             // 0x78
    BodyImpl* body;                 // 0x7c
    GeometryImpl* bnext;            // 0x80
    void* clientData;               // 0x84
    void (*clientDestruct)(void*);  // 0x88
    SweepAndPrune* sweepAndPrune;   // 0x8c
    SAPBox* aabb;                   // 0x90
    unsigned int uniqueIndex;       // 0x94
    uint8_t type;                   // 0x98
    uint8_t category;               // 0x99
    uint8_t flags;                  // 0x9a
    uint8_t pad9b[5];               // 0x9b..0x9f

    // 0x005089e0 (PDB: 0x0050a4b0 / ?Clear@GeometryImpl@@QAEXXZ)
    void Clear();

    // 0x00508a90 (PDB: 0x0050a560 / ?CheckTunnelRisk@GeometryImpl@@QBE_NABVVector3@cdc@@ABVQuat@3@@Z)
    bool CheckTunnelRisk(const cdc::Vector3& pos, const cdc::Quat& rot) const;

    // 0x005090b0 (PDB: 0x0050ab80 / ?CreateAABB@GeometryImpl@@QAEXXZ)
    void CreateAABB();

    // 0x005093d0 (PDB: 0x0050aea0 / ?UpdatePosition@GeometryImpl@@QAEXXZ)
    void UpdatePosition();
};
static_assert(sizeof(GeometryImpl) == 0xa0, "GeometryImpl size mismatch");

#endif // GAME_MULTIBODY_SRC_GEOMETRY_H
