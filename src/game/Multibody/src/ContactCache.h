#ifndef GAME_MULTIBODY_SRC_CONTACTCACHE_H
#define GAME_MULTIBODY_SRC_CONTACTCACHE_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/src/MultibodyTypes.h"

struct GeometryImpl;

// PDB: ContactEntry (size: 0x30)
struct alignas(16) ContactEntry {
    cdc::Vector3 position;  // 0x00
    cdc::Vector3 force;     // 0x10
    ContactEntry* next;     // 0x20
    GeometryImpl* onto;     // 0x24
    bool used;              // 0x28
    uint8_t pad29[0x07];    // 0x29..0x2f
};
static_assert(sizeof(ContactEntry) == 0x30, "ContactEntry size mismatch");

// PDB: ContactPair (size: 0x0c)
struct ContactPair {
    GeometryImpl* other;    // 0x00
    ContactEntry* entries;  // 0x04
    unsigned int timeStamp; // 0x08

    // 0x00507920 (PDB: 0x005093f0 / ?FitForces@ContactPair@@QAEXPAUContact@@IM@Z)
    void FitForces(Contact* contacts, unsigned int numContacts, float tolerance);
};
static_assert(sizeof(ContactPair) == 0x0c, "ContactPair size mismatch");

#endif // GAME_MULTIBODY_SRC_CONTACTCACHE_H
