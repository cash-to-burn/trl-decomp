#include "game/Multibody/src/ContactCache.h"

// 0x00507920 (PDB: 0x005093f0 / ?FitForces@ContactPair@@QAEXPAUContact@@IM@Z)
void ContactPair::FitForces(Contact* contacts, unsigned int numContacts, float tolerance)
{
    const float tolSq = tolerance * tolerance;

    for (unsigned int i = 0; i < numContacts; ++i) {
        float bestDistSq = tolSq;
        ContactEntry* bestEntry = nullptr;

        for (ContactEntry* entry = entries; entry != nullptr; entry = entry->next) {
            if (!entry->used) {
                const float distSq = (contacts[i].cp.position - entry->position).Len3Squared();
                if (distSq <= bestDistSq) {
                    bestDistSq = distSq;
                    bestEntry = entry;
                }
            }
        }

        if (bestEntry != nullptr) {
            if (bestEntry->onto == reinterpret_cast<GeometryImpl*>(contacts[i].geometry2)) {
                contacts[i].force = bestEntry->force;
            } else {
                contacts[i].force = bestEntry->force * -1.0f;
            }
            bestEntry->used = true;
        } else {
            contacts[i].force = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
}
