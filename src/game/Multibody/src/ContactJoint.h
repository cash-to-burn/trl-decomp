#ifndef GAME_MULTIBODY_SRC_CONTACTJOINT_H
#define GAME_MULTIBODY_SRC_CONTACTJOINT_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/src/MultibodyTypes.h"

struct BodyImpl;
struct GeometryImpl;

// 0x0050cd80 (PDB: 0x0050e870 / ?ComputeBasis@@YAXPAVVector3@cdc@@0ABV12@@Z)
void ComputeBasis(cdc::Vector3* u, cdc::Vector3* v, const cdc::Vector3& n);

struct alignas(16) ContactJacobianRow {
    cdc::Vector3 v1; // 0x00
    cdc::Vector3 w1; // 0x10
    cdc::Vector3 v2; // 0x20
    cdc::Vector3 w2; // 0x30
};
static_assert(sizeof(ContactJacobianRow) == 0x40, "ContactJacobianRow size mismatch");

struct ContactStepParams {
    float pad00;           // 0x00
    float contactSlop;     // 0x04
    float bounceThreshold; // 0x08
};

// PDB: ContactJoint (size: 0xf0)
struct alignas(16) ContactJoint {
    static const int MAX_POINTS = 5;
    static const int MAX_CONSTRAINTS = 8;

    enum Flags {
        FLAG_KINETIC = 0x40,
        FLAG_BOTH_BODIES = 0x80
    };

    cdc::Vector3 p[MAX_POINTS];            // 0x00
    cdc::Vector3 normal;                   // 0x50
    float vrel[MAX_POINTS];                // 0x60
    float restitution;                     // 0x74
    float friction;                        // 0x78
    float gamma;                           // 0x7c
    ContactNode node1;                     // 0x80
    ContactNode node2;                     // 0x8c
    float lambda[MAX_CONSTRAINTS];         // 0x98
    BodyImpl* body1;                       // 0xb8
    BodyImpl* body2;                       // 0xbc
    GeometryImpl* geometry1[MAX_POINTS];   // 0xc0
    GeometryImpl* geometry2[MAX_POINTS];   // 0xd4
    uint8_t numRows;                       // 0xe8
    uint8_t flags;                         // 0xe9
    uint8_t padEa[6];                      // 0xea..0xef

    // 0x00507a50 (PDB: 0x00509520 / ?Set@ContactJoint@@QAEXPAVBodyImpl@@0PBUContact@@HABVVector3@cdc@@M@Z)
    void Set(
        BodyImpl* b1,
        BodyImpl* b2,
        const Contact* contacts,
        int numContacts,
        const cdc::Vector3& contactNormal,
        float frictionForceScale);

    // 0x00508030 (PDB: 0x00509b00 / ?ComputeJacobian@ContactJoint@@QBEXQAVVector3@cdc@@QAM111HPBU1@@Z)
    void ComputeJacobian(
        ContactJacobianRow* J,
        float* err,
        float* vel,
        float* lo,
        float* hi,
        int rowIndex,
        const ContactStepParams* params) const;
};
static_assert(sizeof(ContactJoint) == 0xf0, "ContactJoint size mismatch");

#endif // GAME_MULTIBODY_SRC_CONTACTJOINT_H
