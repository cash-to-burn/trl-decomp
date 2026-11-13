#ifndef GAME_MULTIBODY_SRC_BODY_H
#define GAME_MULTIBODY_SRC_BODY_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/src/MassProperties.h"

struct ContactNode;
struct Geometry;
struct GeometryImpl;
struct JointNode;
struct MultibodySystemImpl;

// PDB: TunnelReport (size: 0x20)
struct alignas(16) TunnelReport {
    cdc::Vector3 normal;    // 0x00
    float separation;       // 0x10
    uint32_t pad14[3];      // 0x14..0x1f
};
static_assert(sizeof(TunnelReport) == 0x20, "TunnelReport size mismatch");

// PDB: SleepProperties (size: 0x0c)
struct SleepProperties {
    float linearThreshold;  // 0x00
    float angularThreshold; // 0x04
    bool autoSleep;         // 0x08
    uint8_t pad09[3];       // 0x09..0x0b
};
static_assert(sizeof(SleepProperties) == 0x0c, "SleepProperties size mismatch");

// 0x00503770 (PDB: 0x00505410 / ?InvertSymmetric3x3@@YA_NAAVMatrix@cdc@@ABV12@@Z)
bool InvertSymmetric3x3(cdc::Matrix& outInv, const cdc::Matrix& m);

class Body {
public:
    enum Flags {
        FLAG_SLEEPING = 0x1,
        FLAG_BAD_MASS_PROPS = 0x4,
        FLAG_NO_ANGULAR_CLAMP = 0x10,
        FLAG_TUNNEL_RISK = 0x20,
        FLAG_DISABLED = 0x40
    };

    // 0x00503880 (PDB: 0x00505520 / ?SetClientData@Body@@QAEXPAX@Z)
    void SetClientData(void* clientData);

    // 0x00503890 (PDB: 0x00505530 / ?GetClientData@Body@@QBEPAXXZ)
    void* GetClientData() const;

    // 0x005038a0 (PDB: 0x00505540 / ?SetName@Body@@QAEXPBD@Z)
    void SetName(const char* name);

    // 0x005038d0 (PDB: 0x00505570 / ?GetName@Body@@QBEPBDXZ)
    const char* GetName() const;

    // 0x005038e0 (PDB: 0x00505580 / ?SetMassProperties@Body@@QAEXABUMassProperties@@@Z)
    void SetMassProperties(const MassProperties& massProps);

    // 0x00503980 (PDB: 0x00505620 / ?GetMass@Body@@QBEMXZ)
    float GetMass() const;

    // 0x00503990 (PDB: 0x00505630 / ?GetInvMass@Body@@QBEMXZ)
    float GetInvMass() const;

    // PDB: 0x00411a00 / ?GetI@Body@@QBEABVMatrix@cdc@@XZ
    const cdc::Matrix& GetI() const;

    // PDB: 0x00612b40 / ?GetInvI@Body@@QBEABVMatrix@cdc@@XZ
    const cdc::Matrix& GetInvI() const;

    // 0x005039a0 (PDB: 0x00505640 / ?SetSleepProperties@Body@@QAEXABUSleepProperties@@@Z)
    void SetSleepProperties(const SleepProperties& sleepProps);

    // 0x005039c0 (PDB: 0x00505660 / ?SetLinearDamping@Body@@QAEXM@Z)
    void SetLinearDamping(float damping);

    // 0x005039d0 (PDB: 0x00505670 / ?GetLinearDamping@Body@@QBEMXZ)
    float GetLinearDamping() const;

    // 0x005039e0 (PDB: 0x00505680 / ?SetAngularDamping@Body@@QAEXM@Z)
    void SetAngularDamping(float damping);

    // 0x005039f0 (PDB: 0x00505690 / ?GetAngularDamping@Body@@QBEMXZ)
    float GetAngularDamping() const;

    // 0x00503a00 (PDB: 0x005056a0 / ?SetContactRank@Body@@QAEXH@Z)
    void SetContactRank(int rank);

    // 0x00503a10 (PDB: 0x005056b0 / ?GetFirstGeometry@Body@@QAEPAVGeometry@@XZ)
    Geometry* GetFirstGeometry();

    // 0x00503a20 (PDB: 0x005056c0 / ?GetNextGeometry@Body@@QAEPAVGeometry@@PAV2@@Z)
    Geometry* GetNextGeometry(Geometry* geom);

    // 0x00503a40 (PDB: 0x005056e0 / ?SetPosition@Body@@QAEXABVVector3@cdc@@@Z)
    void SetPosition(const cdc::Vector3& pos);

    // 0x00503aa0 (PDB: 0x00505740 / ?GetPosition@Body@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetPosition() const;

    // 0x00503ad0 (PDB: 0x00505770 / ?GetQuaternion@Body@@QBE?AVQuat@cdc@@XZ)
    cdc::Quat GetQuaternion() const;

    // 0x00503b00 (PDB: 0x005057a0 / ?SetLinearVelocity@Body@@QAEXABVVector3@cdc@@@Z)
    void SetLinearVelocity(const cdc::Vector3& vel);

    // PDB: 0x0050b960 / ?GetLinearVelocity@Body@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetLinearVelocity() const;

    // 0x00503b40 (PDB: 0x005057e0 / ?SetAngularVelocity@Body@@QAEXABVVector3@cdc@@@Z)
    void SetAngularVelocity(const cdc::Vector3& omega);

    // PDB: 0x0050b990 / ?GetAngularVelocity@Body@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetAngularVelocity() const;

    // 0x00503b80 (PDB: 0x00505820 / ?LocalToGlobalPoint@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
    cdc::Vector3 LocalToGlobalPoint(const cdc::Vector3& localPt) const;

    // 0x00503bc0 (PDB: 0x00505860 / ?LocalToGlobalNormal@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
    cdc::Vector3 LocalToGlobalNormal(const cdc::Vector3& localNormal) const;

    // 0x00503be0 (PDB: 0x00505880 / ?LocalPointVelocity@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
    cdc::Vector3 LocalPointVelocity(const cdc::Vector3& localPt) const;

    // 0x00503cb0 (PDB: 0x005058f0 / ?GlobalPointVelocity@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
    cdc::Vector3 GlobalPointVelocity(const cdc::Vector3& globalPt) const;

    // 0x00503d10 (PDB: 0x00505950 / ?WakeUp@Body@@QAEXXZ)
    void WakeUp();

    // 0x00503d40 (PDB: 0x00505980 / ?PutToSleep@Body@@QAEXXZ)
    void PutToSleep();

    // 0x00503d60 (PDB: 0x005059a0 / ?IsSleeping@Body@@QBE_NXZ)
    bool IsSleeping() const;

    // 0x00503d70 (PDB: 0x005059b0 / ?Enable@Body@@QAEXXZ)
    void Enable();

    // 0x00503d80 (PDB: 0x005059c0 / ?Disable@Body@@QAEXXZ)
    void Disable();

    // 0x00503e40 (PDB: 0x00505a80 / ?SetQuaternion@Body@@QAEXABVQuat@cdc@@@Z)
    void SetQuaternion(const cdc::Quat& rot);

    // 0x00503e90 (PDB: 0x00505ad0 / ?GlobalToLocalPoint@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
    cdc::Vector3 GlobalToLocalPoint(const cdc::Vector3& globalPt) const;

    // 0x00503ee0 (PDB: 0x00505b20 / ?GlobalToLocalNormal@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
    cdc::Vector3 GlobalToLocalNormal(const cdc::Vector3& globalNormal) const;

    // 0x00503f20 (PDB: 0x00505b60 / ?AddForce@Body@@QAEXABVVector3@cdc@@@Z)
    void AddForce(const cdc::Vector3& force);

    // 0x00503f60 (PDB: 0x00505ba0 / ?AddForce@Body@@QAEXABVVector3@cdc@@0@Z)
    void AddForce(const cdc::Vector3& force, const cdc::Vector3& position);

    // 0x00503ff0 (PDB: 0x00505c30 / ?AddTorque@Body@@QAEXABVVector3@cdc@@@Z)
    void AddTorque(const cdc::Vector3& torque);

    // 0x005043e0 (PDB: 0x00506020 / ?AttachGeometry@Body@@QAEXPAVGeometry@@@Z)
    void AttachGeometry(Geometry* geom);

    // 0x00504430 (PDB: 0x00506070 / ?AddImpulseForce@Body@@QAEXABVVector3@cdc@@@Z)
    void AddImpulseForce(const cdc::Vector3& impulse);

    // 0x00504590 (PDB: 0x005061d0 / ?AddImpulseForce@Body@@QAEXABVVector3@cdc@@0@Z)
    void AddImpulseForce(const cdc::Vector3& impulse, const cdc::Vector3& position);
};

// PDB: BodyImpl (size: 0x170)
struct alignas(16) BodyImpl : public Body {
    cdc::Matrix I;                  // 0x000
    cdc::Matrix invI;               // 0x040
    cdc::Vector3 x;                 // 0x080
    cdc::Quat q;                    // 0x090
    cdc::Vector3 v;                 // 0x0a0
    cdc::Vector3 omega;             // 0x0b0
    cdc::Vector3 F;                 // 0x0c0
    cdc::Vector3 T;                 // 0x0d0
    cdc::Vector3 Fc;                // 0x0e0
    cdc::Vector3 Tc;                // 0x0f0
    SleepProperties sleep;          // 0x100
    char name[32];                  // 0x10c
    MultibodySystemImpl* multibody; // 0x12c
    BodyImpl* prev;                 // 0x130
    BodyImpl* next;                 // 0x134
    GeometryImpl* firstGeometry;    // 0x138
    JointNode* firstJoint;          // 0x13c
    ContactNode* firstContact;      // 0x140
    void* clientData;               // 0x144
    float mass;                     // 0x148
    float invMass;                  // 0x14c
    float linearDamping;            // 0x150
    float angularDamping;           // 0x154
    unsigned int flags;             // 0x158
    float sleepTimer;               // 0x15c
    int index;                      // 0x160
    int level;                      // 0x164
    int rank;                       // 0x168
    uint32_t pad16c;                // 0x16c

    // 0x00503d90 (PDB: 0x005059d0 / ?RemoveGeometry@BodyImpl@@QAEXPAVGeometryImpl@@@Z)
    void RemoveGeometry(GeometryImpl* geom);

    // 0x00503de0 (PDB: 0x00505a20 / ?AddJoint@BodyImpl@@QAEXPAUJointNode@@@Z)
    void AddJoint(JointNode* node);

    // 0x00503e00 (PDB: 0x00505a40 / ?RemoveJoint@BodyImpl@@QAEXPAUJointNode@@@Z)
    void RemoveJoint(JointNode* node);

    // 0x00504030 (PDB: 0x00505c70 / ?Clear@BodyImpl@@QAEXPAVMultibodySystemImpl@@@Z)
    void Clear(MultibodySystemImpl* system);

    // 0x00504130 (PDB: 0x00505d70 / ?ComputeQuaternionUpdate@BodyImpl@@QBE?AVQuat@cdc@@M@Z)
    cdc::Quat ComputeQuaternionUpdate(float dt) const;

    // 0x00504200 (PDB: 0x00505e40 / ?UpdatePosition@BodyImpl@@QAEXM@Z)
    void UpdatePosition(float dt);
};
static_assert(sizeof(BodyImpl) == 0x170, "BodyImpl size mismatch");

#endif // GAME_MULTIBODY_SRC_BODY_H
