#ifndef GAME_MULTIBODY_SRC_COLLISIONBROKER_H
#define GAME_MULTIBODY_SRC_COLLISIONBROKER_H

#include "game/Multibody/src/MultibodyTypes.h"

struct GeometryImpl;

// 0x005052b0 (PDB: 0x00506ef0 / ?CollideSphereAndSphereWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideSphereAndSphereWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* geomA,
    const GeometryImpl* geomB);

// 0x005052e0 (PDB: 0x00506f20 / ?CollideSphereAndBoxWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideSphereAndBoxWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* sphere,
    const GeometryImpl* box);

// 0x00505340 (PDB: 0x00506f80 / ?CollideSphereAndPlaneWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideSphereAndPlaneWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* sphere,
    const GeometryImpl* plane);

// 0x00505370 (PDB: 0x00506fb0 / ?CollideBoxAndBoxWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideBoxAndBoxWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* boxA,
    const GeometryImpl* boxB);

// 0x00505420 (PDB: 0x00507060 / ?CollideCapsuleAndSphereWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndSphereWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capsule,
    const GeometryImpl* sphere);

// 0x005054f0 (PDB: 0x00507130 / ?CollideCapsuleAndBoxWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndBoxWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capsule,
    const GeometryImpl* box);

// 0x00505660 (PDB: 0x005072a0 / ?CollideCapsuleAndCapsuleWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndCapsuleWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capA,
    const GeometryImpl* capB);

// 0x005057d0 (PDB: 0x00507410 / ?CollideCapsuleAndPlaneWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndPlaneWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capsule,
    const GeometryImpl* plane);

// 0x005058e0 (PDB: 0x00507520 / ?CollideBoxAndPlaneWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideBoxAndPlaneWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* box,
    const GeometryImpl* plane);

#endif // GAME_MULTIBODY_SRC_COLLISIONBROKER_H
