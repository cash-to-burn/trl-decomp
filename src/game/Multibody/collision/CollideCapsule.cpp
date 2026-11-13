#include "game/Multibody/collision/CollideCapsule.h"

// 0x004fcae0 (PDB: 0x004fe7e0)
int CollideCapsuleAndSphere(
    CPoint* cp,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    const cdc::Vector3& spherePos,
    float sphereRad)
{
    cdc::Vector3 closest;
    const float distSq = PointToSegmentDistance(nullptr, &closest, spherePos, capA, capB);
    return MultibodyCollision::ResolveSpherePairContact(cp, closest, capRad, spherePos, sphereRad, distSq);
}

// 0x004fcc10 (PDB: 0x004fe910)
int CollideCapsuleAndCapsule(
    CPoint* cp,
    const cdc::Vector3& a0,
    const cdc::Vector3& a1,
    float radA,
    const cdc::Vector3& b0,
    const cdc::Vector3& b1,
    float radB)
{
    cdc::Vector3 closestA;
    cdc::Vector3 closestB;
    const float distSq = SegmentToSegmentDistance(&closestA, &closestB, a0, a1, b0, b1);
    return MultibodyCollision::ResolveSpherePairContact(cp, closestA, radA, closestB, radB, distSq);
}
