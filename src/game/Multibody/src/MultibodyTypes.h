#ifndef GAME_MULTIBODY_SRC_MULTIBODYTYPES_H
#define GAME_MULTIBODY_SRC_MULTIBODYTYPES_H

#include <cstddef>
#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/Multibody/src/SweepAndPrune.h"

class Body;
class Geometry;
class Mesh;
struct BodyImpl;
struct ContactJacobianRow;
struct ContactJoint;
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
struct alignas(16) JointImpl {
    enum Flags {
        FLAG_DISABLED = 0x1,
        FLAG_NO_COLLIDE = 0x2,
        FLAG_TAG1 = 0x2000,
        FLAG_TAG2 = 0x4000,
        FLAG_TAGS = 0x6000
    };

    cdc::Vector3 anchor1;           // 0x000
    cdc::Vector3 anchor2;           // 0x010
    cdc::Vector3 bx1;               // 0x020
    cdc::Vector3 by1;               // 0x030
    cdc::Vector3 bz1;               // 0x040
    cdc::Vector3 bx2;               // 0x050
    cdc::Vector3 by2;               // 0x060
    cdc::Vector3 bz2;               // 0x070
    float linkCoord1;               // 0x080
    float linkCoord2;               // 0x084
    float ratio;                    // 0x088
    float length;                   // 0x08c
    void (*splineCallback)(void*);  // 0x090
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
    void* clientData;               // 0x16c
    void (*clientDestruct)(void*);  // 0x170
    unsigned int flags;             // 0x174
    uint8_t type;                   // 0x178
    uint8_t numRowsNoLimits;        // 0x179
    uint8_t numRows;                // 0x17a
    uint8_t stepIterations;         // 0x17b
    uint32_t pad17c;                // 0x17c

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
    uint8_t pad00[0x14];    // 0x00..0x13
    int numMeshCollections; // 0x14
    uint8_t pad18[0x04];    // 0x18..0x1b
};
static_assert(sizeof(MultibodySize) == 0x1c, "MultibodySize size mismatch");

// PDB: MultibodySystem (size: 0x01)
struct MultibodySystem {
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
};

// PDB: MultibodySystemImpl (size: 0x2130)
struct alignas(16) MultibodySystemImpl : public MultibodySystem {
    MultibodySize size;                 // 0x0000
    ResponseData responseArray[32][32]; // 0x001c
    uint8_t pad201c[0x24];              // 0x201c..0x203f
    BodyImpl* bodyList;                 // 0x2040
    GeometryImpl* geometryList;         // 0x2044
    JointImpl* firstJoint;              // 0x2048
    void* contactEntryPool;             // 0x204c
    int freeContactEntry;               // 0x2050
    void* contactPairs;                 // 0x2054
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

    // 0x00515c40 (PDB: 0x00517770 / ?CreateContactJoint@MultibodySystemImpl@@QAEPAUContactJoint@@XZ)
    ContactJoint* CreateContactJoint();

    // 0x00515ce0 (PDB: 0x00517810 / ?QueryContactCache@MultibodySystemImpl@@QAEXPAUContact@@H@Z)
    void QueryContactCache(Contact* contacts, int numContacts);

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
