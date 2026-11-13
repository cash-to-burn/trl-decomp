#ifndef GAME_MULTIBODY_SRC_MULTIBODYTYPES_H
#define GAME_MULTIBODY_SRC_MULTIBODYTYPES_H

#include <cstddef>
#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/Multibody/src/Joint.h"
#include "game/Multibody/src/SweepAndPrune.h"

class Body;
class Geometry;
class Mesh;
struct BodyImpl;
struct ContactEntry;
struct ContactJacobianRow;
struct ContactJoint;
struct ContactPair;
struct GeometryImpl;
struct IndexedFace;
struct JointImpl;
struct SAPBox;
struct SweepAndPrune;
struct TunnelReport;

// PDB: JointNode (size: 0x0c)
struct JointNode {
    JointImpl* joint;     // 0x00
    BodyImpl* otherBody;  // 0x04
    JointNode* next;      // 0x08
};
static_assert(sizeof(JointNode) == 0x0c, "JointNode size mismatch");

// PDB: JointLimit (size: 0x10)
struct JointLimit {
    enum Status {
        NOT_AT_LIMIT = 0,
        AT_LOW_LIMIT = 1,
        AT_HIGH_LIMIT = 2
    };

    float low;   // 0x00
    float high;  // 0x04
    float value; // 0x08
    int status;  // 0x0c
};
static_assert(sizeof(JointLimit) == 0x10, "JointLimit size mismatch");

// PDB: JointMotor (size: 0x08)
struct JointMotor {
    float velocity; // 0x00
    float maxForce; // 0x04
};
static_assert(sizeof(JointMotor) == 0x08, "JointMotor size mismatch");

// PDB: JointParams (size: 0x10)
struct JointParams {
    float dt;        // 0x00
    float thickness; // 0x04
    float threshold; // 0x08
    float invBeta;   // 0x0c
};
static_assert(sizeof(JointParams) == 0x10, "JointParams size mismatch");

// PDB: JointImpl (size: 0x180)
struct alignas(16) JointImpl : public Joint {
    enum Flags {
        COLLIDE = 0x1,
        LIMIT1 = 0x2,
        LIMIT2 = 0x4,
        LIMIT3 = 0x8,
        ANY_LIMITS = 0xe,
        MOTOR1 = 0x10,
        MOTOR2 = 0x20,
        MOTOR3 = 0x40,
        ANY_MOTORS = 0x70,
        ANY_LIMITS_OR_MOTORS = 0x7e,
        LINK_POS1 = 0x80,
        LINK_POS2 = 0x100,
        LINK_ROT1 = 0x200,
        LINK_ROT2 = 0x400,
        HAS_LIMIT_LISTENER = 0x800,
        SMOOTH_LIMITS = 0x1000,
        TAG1 = 0x2000,
        TAG2 = 0x4000,
        TAGS = 0x6000,
        FLAG_DISABLED = 0x1,
        FLAG_NO_COLLIDE = 0x2,
        FLAG_TAG1 = 0x2000,
        FLAG_TAG2 = 0x4000,
        FLAG_TAGS = 0x6000
    };

    cdc::Vector3 anchor1;                                            // 0x000
    cdc::Vector3 anchor2;                                            // 0x010
    cdc::Vector3 bx1;                                                // 0x020
    cdc::Vector3 by1;                                                // 0x030
    cdc::Vector3 bz1;                                                // 0x040
    cdc::Vector3 bx2;                                                // 0x050
    cdc::Vector3 by2;                                                // 0x060
    cdc::Vector3 bz2;                                                // 0x070
    float linkCoord1;                                                // 0x080
    float linkCoord2;                                                // 0x084
    float ratio;                                                     // 0x088
    float length;                                                    // 0x08c
    void (*splineCallback)(SplineJointParams*, const Joint*, float); // 0x090
    uint32_t pad094[3];             // 0x094..0x09f
    cdc::Vector3 force;             // 0x0a0
    cdc::Vector3 torque;            // 0x0b0
    JointNode node1;                // 0x0c0
    JointNode node2;                // 0x0cc
    JointLimit limit[3];            // 0x0d8
    JointMotor motor[3];            // 0x108
    float gamma[6];                 // 0x120
    float lambda[6];                // 0x138
    float relaxation;               // 0x150
    JointImpl* prev;                // 0x154
    JointImpl* next;                // 0x158
    BodyImpl* body1;                // 0x15c
    BodyImpl* body2;                // 0x160
    JointImpl* linkChild1;          // 0x164
    JointImpl* linkChild2;          // 0x168
    void* clientData;                       // 0x16c
    void (*clientDestruct)(void*, Joint*);  // 0x170
    unsigned int flags;             // 0x174
    uint8_t type;                   // 0x178
    uint8_t numRowsNoLimits;        // 0x179
    uint8_t numRows;                // 0x17a
    uint8_t stepIterations;         // 0x17b
    uint32_t pad17c;                // 0x17c

    // 0x005099b0 (PDB: 0x0050b4a0 / ?IsLimitNumberValid@JointImpl@@QBE_NH@Z)
    bool IsLimitNumberValid(int n) const;

    // 0x00509a10 (PDB: 0x0050b500 / ?IsMotorNumberValid@JointImpl@@QBE_NH@Z)
    bool IsMotorNumberValid(int n) const;

    // 0x0050a400 (PDB: 0x0050bef0 / ?Clear@JointImpl@@QAEXXZ)
    void Clear();

    // 0x0050b750 (PDB: 0x0050d220 / ?Connect@JointImpl@@QAEXPAVBodyImpl@@0@Z)
    void Connect(BodyImpl* b1, BodyImpl* b2);

    // 0x005155c0 (PDB: 0x005170b0 / ?Disconnect@JointImpl@@QAEXXZ)
    void Disconnect();

    // 0x0050a990 (PDB: 0x0050cc00 / ?ComputeRevoluteAngle@JointImpl@@QBEMXZ)
    float ComputeRevoluteAngle() const;

    // 0x0050b0c0 (PDB: 0x0050c010 / ?ComputeRevoluteAngleSpeed@JointImpl@@QBEMXZ)
    float ComputeRevoluteAngleSpeed() const;

    // 0x0050a540 (PDB: 0x0050c0a0 / ?ComputePrismaticTranslation@JointImpl@@QBEMXZ)
    float ComputePrismaticTranslation() const;

    // 0x0050a630 (PDB: 0x0050c190 / ?ComputePrismaticTranslationSpeed@JointImpl@@QBEMXZ)
    float ComputePrismaticTranslationSpeed() const;

    // 0x0050a750 (PDB: 0x0050c2b0 / ?ComputePlanarTranslation1@JointImpl@@QBEMXZ)
    float ComputePlanarTranslation1() const;

    // 0x0050a870 (PDB: 0x0050c3d0 / ?ComputePlanarTranslation2@JointImpl@@QBEMXZ)
    float ComputePlanarTranslation2() const;

    // 0x0050a990 (PDB: 0x0050cc00 / ?ComputePlanarAngle@JointImpl@@QBEMXZ)
    float ComputePlanarAngle() const;

    // 0x0050aac0 (PDB: 0x0050c4f0 / ?ComputePlanarTranslationSpeed1@JointImpl@@QBEMXZ)
    float ComputePlanarTranslationSpeed1() const;

    // 0x0050ac20 (PDB: 0x0050c650 / ?ComputePlanarTranslationSpeed2@JointImpl@@QBEMXZ)
    float ComputePlanarTranslationSpeed2() const;

    // 0x0050ad80 (PDB: 0x0050c7b0 / ?ComputePlanarAngleSpeed@JointImpl@@QBEMXZ)
    float ComputePlanarAngleSpeed() const;

    // 0x0050ae10 (PDB: 0x0050c840 / ?ComputeUniversalAngle1@JointImpl@@QBEMXZ)
    float ComputeUniversalAngle1() const;

    // 0x0050af40 (PDB: 0x0050c970 / ?ComputeUniversalAngle2@JointImpl@@QBEMXZ)
    float ComputeUniversalAngle2() const;

    // 0x0050b030 (PDB: 0x0050ca60 / ?ComputeUniversalAngleSpeed1@JointImpl@@QBEMXZ)
    float ComputeUniversalAngleSpeed1() const;

    // 0x0050b0c0 (PDB: 0x0050c010 / ?ComputeUniversalAngleSpeed2@JointImpl@@QBEMXZ)
    float ComputeUniversalAngleSpeed2() const;

    // 0x0050b150 (PDB: 0x0050caf0 / ?ComputeSwingTranslation@JointImpl@@QBEMXZ)
    float ComputeSwingTranslation() const;

    // 0x0050a990 (PDB: 0x0050cc00 / ?ComputeSwingAngle1@JointImpl@@QBEMXZ)
    float ComputeSwingAngle1() const;

    // 0x0050b260 (PDB: 0x0050cd30 / ?ComputeSwingAngle2@JointImpl@@QBEMXZ)
    float ComputeSwingAngle2() const;

    // 0x0050b350 (PDB: 0x0050ce20 / ?ComputeSwingTranslationSpeed@JointImpl@@QBEMXZ)
    float ComputeSwingTranslationSpeed() const;

    // 0x0050b470 (PDB: 0x0050cf40 / ?ComputeSwingAngleSpeed1@JointImpl@@QBEMXZ)
    float ComputeSwingAngleSpeed1() const;

    // 0x0050b500 (PDB: 0x0050cfd0 / ?ComputeSwingAngleSpeed2@JointImpl@@QBEMXZ)
    float ComputeSwingAngleSpeed2() const;

    // 0x0050b590 (PDB: 0x0050d060 / ?ComputeCartesianTranslation@JointImpl@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 ComputeCartesianTranslation() const;

    // 0x0050b640 (PDB: 0x0050d110 / ?ComputeCartesianTranslationSpeed@JointImpl@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 ComputeCartesianTranslationSpeed() const;

    // 0x0050eef0 (PDB: 0x005109c0 / ?ComputeAngularAxis@JointImpl@@QBEHPAVVector3@cdc@@PAM111ABUJointAxisParams@@@Z)
    int ComputeAngularAxis(
        ContactJacobianRow* row,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        const JointAxisParams& axisParams) const;

    // 0x0050f3c0 (PDB: 0x00510e90 / ?ComputeLinearAxis@JointImpl@@QBEHPAVVector3@cdc@@PAM111ABUJointAxisParams@@@Z)
    int ComputeLinearAxis(
        ContactJacobianRow* row,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        const JointAxisParams& axisParams) const;

    // 0x0050b7b0 (PDB: 0x0050d280 / ?ComputeSpherical@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeSpherical(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050ba90 (PDB: 0x0050d560 / ?ComputeRope@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeRope(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050bdc0 (PDB: 0x0050d890 / ?ComputeWeld@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeWeld(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050c3e0 (PDB: 0x0050deb0 / ?ComputePointMotor@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputePointMotor(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050c690 (PDB: 0x0050e160 / ?ComputeLink@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeLink(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050c950 (PDB: 0x0050e420 / ?ComputeWheel@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeWheel(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050cb60 (PDB: 0x0050e630 / ?ComputeConicalLimit@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeConicalLimit(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050ccc0 (PDB: 0x0050e790 / ?ComputeAngularMotor@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeAngularMotor(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x0050fac0 (PDB: 0x00511590 / ?ComputeRevolute@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeRevolute(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x005100a0 (PDB: 0x00511b70 / ?ComputePrismatic@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputePrismatic(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00510810 (PDB: 0x005122e0 / ?ComputePlanar@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputePlanar(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00510f40 (PDB: 0x00512a10 / ?ComputePulley@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputePulley(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00511230 (PDB: 0x00512d00 / ?ComputeUniversal@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeUniversal(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00511740 (PDB: 0x00513210 / ?ComputeSwing@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeSwing(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00511e20 (PDB: 0x005138f0 / ?ComputeCartesian@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeCartesian(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x005123e0 (PDB: 0x00513eb0 / ?ComputeWheel2@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeWheel2(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00512710 (PDB: 0x005141e0 / ?ComputeSpline@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeSpline(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00512ef0 (PDB: 0x005149e0 / ?ComputeShoulder@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
    void ComputeShoulder(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;

    // 0x00513560 (PDB: 0x00515050 / ?ComputeShoulderLimits@JointImpl@@AAEXPBUJointParams@@@Z)
    void ComputeShoulderLimits(const JointParams* params);

    // 0x00513780 (PDB: 0x00515270 / ?ComputeLimits@JointImpl@@QAEXPBUJointParams@@@Z)
    void ComputeLimits(const JointParams* params);

    // 0x00514640 (PDB: 0x00516130 / ?ComputeJacobian@JointImpl@@QBEXQAVVector3@cdc@@QAM111HPBUJointParams@@@Z)
    void ComputeJacobian(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const JointParams* params) const;
};
static_assert(offsetof(JointImpl, force) == 0x0a0, "JointImpl::force offset mismatch");
static_assert(offsetof(JointImpl, gamma) == 0x120, "JointImpl::gamma offset mismatch");
static_assert(offsetof(JointImpl, lambda) == 0x138, "JointImpl::lambda offset mismatch");
static_assert(offsetof(JointImpl, flags) == 0x174, "JointImpl::flags offset mismatch");
static_assert(sizeof(JointImpl) == 0x180, "JointImpl size mismatch");

// PDB: LineProbeReport (size: 0x40)
struct alignas(16) LineProbeReport {
    cdc::Vector3 point;     // 0x00
    cdc::Vector3 normal;    // 0x10
    float lambda;           // 0x20
    int meshCollection;     // 0x24
    Mesh* mesh;             // 0x28
    IndexedFace* face;      // 0x2c
    Geometry* geometry;     // 0x30
    uint32_t pad34[3];      // 0x34..0x3f
};
static_assert(sizeof(LineProbeReport) == 0x40, "LineProbeReport size mismatch");

// PDB: ContactNode (size: 0x0c)
struct ContactNode {
    ContactJoint* joint;  // 0x00
    BodyImpl* otherBody;  // 0x04
    ContactNode* next;    // 0x08
};
static_assert(sizeof(ContactNode) == 0x0c, "ContactNode size mismatch");

// PDB: Contact (size: 0x80)
struct alignas(16) Contact {
    CPoint cp;                  // 0x00
    cdc::Vector3 velocity;      // 0x30
    cdc::Vector3 force;         // 0x40
    Body* body1;                // 0x50
    Body* body2;                // 0x54
    Geometry* geometry1;        // 0x58
    Geometry* geometry2;        // 0x5c
    int meshCollection;         // 0x60
    Mesh* mesh;                 // 0x64
    float restitution;          // 0x68
    float friction;             // 0x6c
    float softness;             // 0x70
    uint8_t pad74[0x0c];        // 0x74..0x7f
};
static_assert(sizeof(Contact) == 0x80, "Contact size mismatch");

// PDB: ResponseData (size: 0x08)
struct ResponseData {
    bool (*nearFcn)(Mesh*, Geometry*, Geometry*); // 0x00
    bool (*contactFcn)(Contact*, int);            // 0x04
};
static_assert(sizeof(ResponseData) == 0x08, "ResponseData size mismatch");

// PDB: MeshCollection (size: 0x0c)
struct MeshCollection {
    Mesh** meshes;  // 0x00
    SAPBox** boxes; // 0x04
    int numMeshes;  // 0x08
};
static_assert(sizeof(MeshCollection) == 0x0c, "MeshCollection size mismatch");

// PDB: MultibodySize (size: 0x1c)
struct MultibodySize {
    int numBodies;               // 0x00
    int numGeometries;           // 0x04
    int numJoints;               // 0x08
    int numContactPoints;        // 0x0c
    int numPairsPerGeometry;     // 0x10
    int numMeshCollections;      // 0x14
    int numMeshesPerCollection;  // 0x18
};
static_assert(sizeof(MultibodySize) == 0x1c, "MultibodySize size mismatch");

// PDB: MultibodySystem (size: 0x01)
struct MultibodySystem {
    // 0x00517400 (PDB: 0x00518f30 / ?Create@MultibodySystem@@SAPAV1@ABUMultibodySize@@@Z)
    static MultibodySystem* Create(const MultibodySize& size);

    // 0x00517440 (PDB: 0x00518f70 / ?Destroy@MultibodySystem@@SAXPAV1@@Z)
    static void Destroy(MultibodySystem* system);

    // 0x00514db0 (PDB: 0x005168a0 / ?LineProbeGeometries@MultibodySystem@@QAE_NPAULineProbeReport@@ABVVector3@cdc@@1W4TestAxes@SweepAndPrune@@P6A_NPAVGeometry@@PAX@Z4@Z)
    bool LineProbeGeometries(
        LineProbeReport* report,
        const cdc::Vector3& start,
        const cdc::Vector3& end,
        SweepAndPrune::TestAxes testAxes,
        bool (*cb)(Geometry*, void*),
        void* clientData);

    // 0x00514eb0 (PDB: 0x005169a0 / ?LineProbeMeshes@MultibodySystem@@QAE_NPAULineProbeReport@@ABVVector3@cdc@@1P6A_NPAVMesh@@PAX@Z3E@Z)
    bool LineProbeMeshes(
        LineProbeReport* report,
        const cdc::Vector3& start,
        const cdc::Vector3& end,
        bool (*nearMeshFcn)(Mesh*, void*),
        void* userData,
        unsigned char flags);

    // 0x00515620 (PDB: 0x00517110 / ?AddMeshCollection@MultibodySystem@@QAEXHPAPAVMesh@@H@Z)
    void AddMeshCollection(int id, Mesh** meshes, int numMeshes);

    // 0x00515670 (PDB: 0x00517160 / ?RemoveMeshCollection@MultibodySystem@@QAEXH@Z)
    void RemoveMeshCollection(int id);

    // 0x005156c0 (PDB: 0x005171b0 / ?SetNearCallback@MultibodySystem@@QAEXEEP6A_NPAVMesh@@PAVGeometry@@1@Z@Z)
    void SetNearCallback(uint8_t cat1, uint8_t cat2, bool (*cb)(Mesh*, Geometry*, Geometry*));

    // 0x005156f0 (PDB: 0x005171e0 / ?SetContactCallback@MultibodySystem@@QAEXEEP6A_NPAUContact@@H@Z@Z)
    void SetContactCallback(uint8_t cat1, uint8_t cat2, bool (*cb)(Contact*, int));

    // 0x00515720 (PDB: 0x00517210 / ?SetLimitCallback@MultibodySystem@@QAEXP6AXABULimitReport@@@Z@Z)
    void SetLimitCallback(void (*cb)(const LimitReport&));

    // 0x00515730 (PDB: 0x00517220 / ?SetJointDestructionCallback@MultibodySystem@@QAEXP6AXPBVJoint@@@Z@Z)
    void SetJointDestructionCallback(void (*cb)(const Joint*));

    // 0x00515740 (PDB: 0x00517230 / ?SetGravity@MultibodySystem@@QAEXABVVector3@cdc@@@Z)
    void SetGravity(const cdc::Vector3& gravity);

    // 0x005157c0 (PDB: 0x005172b0 / ?GetBodyID@MultibodySystem@@QBEHPBVBody@@@Z)
    int GetBodyID(const Body* body) const;

    // 0x005157f0 (PDB: 0x005172e0 / ?GetJointID@MultibodySystem@@QBEHPBVJoint@@@Z)
    int GetJointID(const Joint* joint) const;

    // 0x00515810 (PDB: 0x00517300 / ?QueryBox@MultibodySystem@@QAEHABVVector3@cdc@@0QAPAVGeometry@@H@Z)
    int QueryBox(const cdc::Vector3& lo, const cdc::Vector3& hi, Geometry** results, int maxResults);

    // 0x00515820 (PDB: 0x00517310 / ?SetDrawContactPoints@MultibodySystem@@QAEX_N@Z)
    void SetDrawContactPoints(bool draw);

    // 0x00515830 (PDB: 0x00517320 / ?SetDrawLineProbes@MultibodySystem@@QAEX_NABVVector3@cdc@@@Z)
    void SetDrawLineProbes(bool draw, const cdc::Vector3& color);

    // 0x00515860 (PDB: 0x00517350 / ?SetContactSoftness@MultibodySystem@@QAEXM@Z)
    void SetContactSoftness(float gamma);

    // 0x00515870 (PDB: 0x00517360 / ?SetBaumgarte@MultibodySystem@@QAEXM@Z)
    void SetBaumgarte(float beta);

    // 0x00515880 (PDB: 0x00517370 / ?SetCollisionThreshold@MultibodySystem@@QAEXM@Z)
    void SetCollisionThreshold(float threshold);

    // 0x00515890 (PDB: 0x00517380 / ?SetBoundaryThickness@MultibodySystem@@QAEXM@Z)
    void SetBoundaryThickness(float thickness);

    // 0x005158a0 (PDB: 0x00517390 / ?SetContactTolerance@MultibodySystem@@QAEXM@Z)
    void SetContactTolerance(float tolerance);

    // 0x005158b0 (PDB: 0x005173a0 / ?SetSleepDelay@MultibodySystem@@QAEXM@Z)
    void SetSleepDelay(float delay);

    // 0x00515e80 (PDB: 0x00517970 / ?CreateBody@MultibodySystem@@QAEPAVBody@@XZ)
    Body* CreateBody();

    // 0x00515f00 (PDB: 0x005179f0 / ?DestroyBody@MultibodySystem@@QAEXPAVBody@@@Z)
    void DestroyBody(Body* body);

    // 0x005161a0 (PDB: 0x00517ca0 / ?CreateGeometry@MultibodySystem@@QAEPAVGeometry@@XZ)
    Geometry* CreateGeometry();

    // 0x00516220 (PDB: 0x00517d30 / ?DestroyGeometry@MultibodySystem@@QAEXPAVGeometry@@@Z)
    void DestroyGeometry(Geometry* geom);

    // 0x00516340 (PDB: 0x00517e60 / ?CreateJoint@MultibodySystem@@QAEPAVJoint@@XZ)
    Joint* CreateJoint();

    // 0x005163c0 (PDB: 0x00517ee0 / ?DestroyJoint@MultibodySystem@@QAEXPAVJoint@@@Z)
    void DestroyJoint(Joint* joint);

    // 0x00516440 (PDB: 0x00517f70 / ?QueryGeometry@MultibodySystem@@QAEHPAUContact@@HPAVGeometry@@@Z)
    int QueryGeometry(Contact* contacts, int maxContacts, Geometry* geom);

    // 0x005164c0 (PDB: 0x00517ff0 / ?SetStepIterations@MultibodySystem@@QAEXE@Z)
    void SetStepIterations(uint8_t iterations);

    // 0x00517470 (PDB: 0x00518fa0 / ?Relocate@MultibodySystem@@QAEXABVVector3@cdc@@@Z)
    void Relocate(const cdc::Vector3& offset);

    // 0x00517510 (PDB: 0x00519040 / ?Step@MultibodySystem@@QAEXM_N@Z)
    void Step(float dt, bool autoSleep);
};

// PDB: MultibodySystemImpl (size: 0x2130)
struct alignas(16) MultibodySystemImpl : public MultibodySystem {
    MultibodySize size;                         // 0x0000
    ResponseData responseArray[32][32];         // 0x001c
    void (*limitFcn)(const LimitReport&);       // 0x201c
    void (*jointDestructionFcn)(const Joint*);  // 0x2020
    BodyImpl* bodyPool;                         // 0x2024
    BodyImpl* freeBody;                         // 0x2028
    GeometryImpl* geometryPool;                 // 0x202c
    GeometryImpl* freeGeometry;                 // 0x2030
    JointImpl* jointPool;                       // 0x2034
    JointImpl* freeJoint;                       // 0x2038
    ContactJoint* contactPool;                  // 0x203c
    BodyImpl* bodyList;                         // 0x2040
    GeometryImpl* geometryList;                 // 0x2044
    JointImpl* firstJoint;                      // 0x2048
    ContactEntry* contactEntryPool;             // 0x204c
    int freeContactEntry;                       // 0x2050
    ContactPair* contactPairs;                  // 0x2054
    unsigned int timeStamp;             // 0x2058
    BodyImpl** bodies;                  // 0x205c
    JointImpl** joints;                 // 0x2060
    ContactJoint** contactJoints;       // 0x2064
    BodyImpl** islandStack;             // 0x2068
    BodyImpl** islandQueue;             // 0x206c
    cdc::Matrix* invI_global;           // 0x2070
    ContactJacobianRow* J;              // 0x2074
    ContactJacobianRow* B;              // 0x2078
    cdc::Vector3* fc;                   // 0x207c
    int (*JMap)[2];                     // 0x2080
    float* cPos;                        // 0x2084
    float* cVel;                        // 0x2088
    float* lo;                          // 0x208c
    float* hi;                          // 0x2090
    float* rhs;                         // 0x2094
    float* lambda;                      // 0x2098
    float* d;                           // 0x209c
    float* gamma;                       // 0x20a0
    int memUsed;                        // 0x20a4
    int nb;                             // 0x20a8
    int ng;                             // 0x20ac
    int nj;                             // 0x20b0
    int nc;                             // 0x20b4
    int maxContactsUsed;                // 0x20b8
    uint32_t pad20bc;                   // 0x20bc
    cdc::Vector3 gravity;               // 0x20c0
    float gravityMagnitude;             // 0x20d0
    float contactGamma;                 // 0x20d4
    float threshold;                    // 0x20d8
    float beta;                         // 0x20dc
    float thickness;                    // 0x20e0
    float sorOmega;                     // 0x20e4
    float contactTolerance;             // 0x20e8
    float sleepTimerInit;               // 0x20ec
    float inv_dt;                       // 0x20f0
    SweepAndPrune* sweepAndPrune;       // 0x20f4
    MeshCollection* meshCollections;    // 0x20f8
    int lock;                           // 0x20fc
    bool drawContacts;                  // 0x2100
    bool drawLineProbes;                // 0x2101
    uint8_t pad2102[0x0e];              // 0x2102..0x210f
    cdc::Vector3 lineProbeColor;        // 0x2110
    short stepIterations;               // 0x2120
    uint8_t pad2122[0x0e];              // 0x2122..0x212f

    // 0x005164f0 (PDB: 0x00518020 / ??0MultibodySystemImpl@@QAE@ABUMultibodySize@@@Z)
    explicit MultibodySystemImpl(const MultibodySize& size);

    // 0x005158c0 (PDB: 0x005173b0 / ??1MultibodySystemImpl@@QAE@XZ)
    ~MultibodySystemImpl();

    // 0x00515c40 (PDB: 0x00517730 / ?CreateContactJoint@MultibodySystemImpl@@QAEPAVContactJoint@@XZ)
    ContactJoint* CreateContactJoint();

    // 0x00515c80 (PDB: 0x00517770 / ?DestroyContactJoints@MultibodySystemImpl@@AAEXXZ)
    void DestroyContactJoints();

    // 0x00515ce0 (PDB: 0x005177d0 / ?QueryContactCache@MultibodySystemImpl@@QBEXPAUContact@@H@Z)
    void QueryContactCache(Contact* contacts, int numContacts) const;

    // 0x00516e70 (PDB: 0x005189a0 / ?RelocateImpl@MultibodySystemImpl@@QAEXABVVector3@cdc@@@Z)
    void RelocateImpl(const cdc::Vector3& offset);

    // 0x00517070 (PDB: 0x00518ba0 / ?StoreContactForces@MultibodySystemImpl@@AAEXXZ)
    void StoreContactForces();

    // 0x00517480 (PDB: 0x00518fb0 / ?StepImpl@MultibodySystemImpl@@QAEXM_N@Z)
    void StepImpl(float dt, bool autoSleep);

    // 0x00519cb0 (PDB: 0x0051b7e0 / ?LCPSetup@MultibodySystemImpl@@QAEXHHM@Z)
    void LCPSetup(int nc, int nb, float invDt);

    // 0x0051a310 (PDB: 0x0051be40 / ?LCPIter@MultibodySystemImpl@@QAEXHHH@Z)
    void LCPIter(int nc, int nb, int iterations);

    // 0x0051a5a0 (PDB: 0x0051c0d0 / ?TimeStep@MultibodySystemImpl@@QAEXHHHMM@Z / 5-arg stack frame)
    void TimeStep(int nb, int nj, int nc, float dt, int unused);

    // 0x0051b2d0 (PDB: 0x0051ce00 / ?TimeStepIslands@MultibodySystemImpl@@QAEXM_N@Z)
    void TimeStepIslands(float dt, bool autoSleep);

    // 0x0051b5e0 (PDB: 0x0051d110 / ?CheckTunnel@MultibodySystemImpl@@QAE_NPAUTunnelReport@@PAVGeometryImpl@@ABVVector3@cdc@@ABVQuat@5@M@Z)
    bool CheckTunnel(
        TunnelReport* report,
        GeometryImpl* geom,
        const cdc::Vector3& pos,
        const cdc::Quat& rot,
        float radius);
};
static_assert(offsetof(MultibodySystemImpl, bodyList) == 0x2040, "MultibodySystemImpl::bodyList offset mismatch");
static_assert(offsetof(MultibodySystemImpl, firstJoint) == 0x2048, "MultibodySystemImpl::firstJoint offset mismatch");
static_assert(offsetof(MultibodySystemImpl, invI_global) == 0x2070, "MultibodySystemImpl::invI_global offset mismatch");
static_assert(offsetof(MultibodySystemImpl, nb) == 0x20a8, "MultibodySystemImpl::nb offset mismatch");
static_assert(offsetof(MultibodySystemImpl, gravity) == 0x20c0, "MultibodySystemImpl::gravity offset mismatch");
static_assert(offsetof(MultibodySystemImpl, inv_dt) == 0x20f0, "MultibodySystemImpl::inv_dt offset mismatch");
static_assert(offsetof(MultibodySystemImpl, drawContacts) == 0x2100, "MultibodySystemImpl::drawContacts offset mismatch");
static_assert(offsetof(MultibodySystemImpl, lineProbeColor) == 0x2110, "MultibodySystemImpl::lineProbeColor offset mismatch");
static_assert(offsetof(MultibodySystemImpl, stepIterations) == 0x2120, "MultibodySystemImpl::stepIterations offset mismatch");
static_assert(sizeof(MultibodySystemImpl) == 0x2130, "MultibodySystemImpl size mismatch");

#endif // GAME_MULTIBODY_SRC_MULTIBODYTYPES_H
