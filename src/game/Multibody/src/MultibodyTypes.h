#ifndef GAME_MULTIBODY_SRC_MULTIBODYTYPES_H
#define GAME_MULTIBODY_SRC_MULTIBODYTYPES_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"

class Body;
class Geometry;
class Mesh;
struct BodyImpl;
struct ContactJoint;
struct GeometryImpl;
struct JointImpl;
struct SAPBox;

// PDB: JointNode (size: 0x0c)
struct JointNode {
    JointImpl* joint;     // 0x00
    BodyImpl* otherBody;  // 0x04
    JointNode* next;      // 0x08
};
static_assert(sizeof(JointNode) == 0x0c, "JointNode size mismatch");

// PDB: JointImpl (size: 0x180)
struct alignas(16) JointImpl {
    enum Flags {
        FLAG_DISABLED = 0x1,
        FLAG_NO_COLLIDE = 0x2
    };

    uint8_t pad000[0x174]; // 0x000..0x173
    unsigned int flags;    // 0x174
    uint8_t pad178[0x08];  // 0x178..0x17f
};
static_assert(sizeof(JointImpl) == 0x180, "JointImpl size mismatch");

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

// PDB: MultibodySystemImpl (size: 0x2130)
struct alignas(16) MultibodySystemImpl {
    MultibodySize size;                 // 0x0000
    ResponseData responseArray[32][32]; // 0x001c
    uint8_t pad201c[0x24];              // 0x201c..0x203f
    BodyImpl* bodyList;                 // 0x2040
    GeometryImpl* geometryList;         // 0x2044
    uint8_t pad2048[0x78];              // 0x2048..0x20bf
    cdc::Vector3 gravity;               // 0x20c0
    float gravityMagnitude;             // 0x20d0
    float contactGamma;                 // 0x20d4
    uint8_t pad20d8[0x10];              // 0x20d8..0x20e7
    float contactTolerance;             // 0x20e8
    float sleepTimerInit;               // 0x20ec
    uint8_t pad20f0[0x08];              // 0x20f0..0x20f7
    MeshCollection* meshCollections;    // 0x20f8
    uint8_t pad20fc[0x04];              // 0x20fc..0x20ff
    bool drawContacts;                  // 0x2100
    uint8_t pad2101[0x2f];              // 0x2101..0x212f

    // 0x00515c40 (PDB: 0x00517770 / ?CreateContactJoint@MultibodySystemImpl@@QAEPAUContactJoint@@XZ)
    ContactJoint* CreateContactJoint();

    // 0x00515ce0 (PDB: 0x00517810 / ?QueryContactCache@MultibodySystemImpl@@QAEXPAUContact@@H@Z)
    void QueryContactCache(Contact* contacts, int numContacts);

    // 0x0051b5e0 (PDB: 0x0051d110 / ?CheckTunnel@MultibodySystemImpl@@QAE_NPAUTunnelReport@@PAVGeometryImpl@@ABVVector3@cdc@@ABVQuat@5@M@Z)
    bool CheckTunnel(
        struct TunnelReport* report,
        GeometryImpl* geom,
        const cdc::Vector3& pos,
        const cdc::Quat& rot,
        float radius);
};
static_assert(sizeof(MultibodySystemImpl) == 0x2130, "MultibodySystemImpl size mismatch");

#endif // GAME_MULTIBODY_SRC_MULTIBODYTYPES_H
