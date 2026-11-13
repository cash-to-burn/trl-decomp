#ifndef GAME_MULTIBODY_SRC_COLLISIONMANAGER_H
#define GAME_MULTIBODY_SRC_COLLISIONMANAGER_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/src/MultibodyTypes.h"

struct BodyImpl;
struct GeometryImpl;

// 0x00505980 (PDB: 0x005075c0 / ?DoesJointTrumpCollision@@YA_NPBVBodyImpl@@0@Z)
bool DoesJointTrumpCollision(const BodyImpl* b1, const BodyImpl* b2);

// 0x005059f0 (PDB: 0x00507630 / ?ClusterContacts@@YAHPAVVector3@cdc@@QAY0CA@HPAHPBUContact@@H@Z)
int ClusterContacts(
    cdc::Vector3* clusterNormals,
    int (*clusterIndices)[32],
    int* clusterCounts,
    const Contact* contacts,
    int numContacts);

// Initializes the 4x4 geometry-pair collision dispatch table (lazy-called by CollideGeometryPair).
void InitColliderArray();

// 0x00505ce0 (PDB: 0x00507920 / ?CollideGeometryPair@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideGeometryPair(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* g1,
    const GeometryImpl* g2);

// 0x00505d90 (PDB: 0x005079d0 / ?DrawContactPoint@@YAXABVVector3@cdc@@0_N@Z)
void DrawContactPoint(const cdc::Vector3& pos, const cdc::Vector3& normal, bool hit);

// 0x00506210 (PDB: 0x00507e50 / ?CollideGeometryAndMesh@@YAHPAUContact@@HPBVMultibodySystemImpl@@PBVGeometryImpl@@@Z)
int CollideGeometryAndMesh(
    Contact* contacts,
    int maxContacts,
    const MultibodySystemImpl* system,
    const GeometryImpl* geom);

// 0x005067f0 (PDB: 0x00508430 / ?CollideMesh@@YAXPAVMultibodySystemImpl@@@Z)
void CollideMesh(MultibodySystemImpl* system);

// 0x00507110 (PDB: 0x00508d50 / ?CollideGeometryAndGeometries@@YAHPAUContact@@HPBVMultibodySystemImpl@@PAVGeometryImpl@@@Z)
int CollideGeometryAndGeometries(
    Contact* contacts,
    int maxContacts,
    const MultibodySystemImpl* system,
    GeometryImpl* geom);

// 0x00507330 (PDB: 0x00508f70 / ?CollideGeometries@@YAXPAVMultibodySystemImpl@@@Z)
void CollideGeometries(MultibodySystemImpl* system);

#endif // GAME_MULTIBODY_SRC_COLLISIONMANAGER_H
