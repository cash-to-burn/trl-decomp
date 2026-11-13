#include "game/Multibody/src/CollisionBroker.h"

#include "game/Multibody/collision/CollideBoxAndBox.h"
#include "game/Multibody/collision/CollideBoxAndCapsule.h"
#include "game/Multibody/collision/CollideCapsule.h"
#include "game/Multibody/collision/CollidePlane.h"
#include "game/Multibody/collision/CollideSphere.h"
#include "game/Multibody/src/Geometry.h"

namespace {

inline void ComputeCapsuleEndpoints(const GeometryImpl* capsule, cdc::Vector3& outA, cdc::Vector3& outB)
{
    const cdc::Vector3 halfAxis = cdc::Vector3::Make(0.0f, 0.0f, 0.5f * capsule->length, 0.0f);
    const cdc::Vector3 delta = capsule->q.Rotate(halfAxis);
    outA = capsule->x + delta;
    outB = capsule->x - delta;
}

inline unsigned int CopyContactPoints(Contact* contacts, const CPoint* cps, int count)
{
    for (int i = 0; i < count; ++i) {
        contacts[i].cp = cps[i];
    }
    return static_cast<unsigned int>(count);
}

} // namespace

// 0x005052b0 (PDB: 0x00506ef0 / ?CollideSphereAndSphereWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideSphereAndSphereWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* geomA,
    const GeometryImpl* geomB)
{
    (void)maxContacts;
    return static_cast<unsigned int>(CollideSphereAndSphere(
        &contacts[0].cp,
        geomA->x,
        geomA->radius,
        geomB->x,
        geomB->radius));
}

// 0x005052e0 (PDB: 0x00506f20 / ?CollideSphereAndBoxWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideSphereAndBoxWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* sphere,
    const GeometryImpl* box)
{
    (void)maxContacts;
    const cdc::Vector3 halfExtents = box->width * 0.5f;
    return static_cast<unsigned int>(CollideSphereAndBox(
        &contacts[0].cp,
        sphere->x,
        sphere->radius,
        box->x,
        box->q,
        halfExtents));
}

// 0x00505340 (PDB: 0x00506f80 / ?CollideSphereAndPlaneWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideSphereAndPlaneWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* sphere,
    const GeometryImpl* plane)
{
    (void)maxContacts;
    return static_cast<unsigned int>(CollideSphereAndPlane(
        &contacts[0].cp,
        sphere->x,
        sphere->radius,
        plane->width,
        plane->width.w));
}

// 0x00505370 (PDB: 0x00506fb0 / ?CollideBoxAndBoxWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideBoxAndBoxWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* boxA,
    const GeometryImpl* boxB)
{
    (void)maxContacts;
    CPoint cps[4];
    const cdc::Vector3 halfA = boxA->width * 0.5f;
    const cdc::Vector3 halfB = boxB->width * 0.5f;
    const int count = CollideBoxAndBox(
        cps,
        halfA,
        halfB,
        boxA->x,
        boxB->x,
        boxA->q,
        boxB->q);
    return CopyContactPoints(contacts, cps, count);
}

// 0x00505420 (PDB: 0x00507060 / ?CollideCapsuleAndSphereWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndSphereWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capsule,
    const GeometryImpl* sphere)
{
    (void)maxContacts;
    cdc::Vector3 capA;
    cdc::Vector3 capB;
    ComputeCapsuleEndpoints(capsule, capA, capB);
    return static_cast<unsigned int>(CollideCapsuleAndSphere(
        &contacts[0].cp,
        capA,
        capB,
        capsule->radius,
        sphere->x,
        sphere->radius));
}

// 0x005054f0 (PDB: 0x00507130 / ?CollideCapsuleAndBoxWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndBoxWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capsule,
    const GeometryImpl* box)
{
    (void)maxContacts;
    CPoint cps[4];
    cdc::Vector3 capA;
    cdc::Vector3 capB;
    ComputeCapsuleEndpoints(capsule, capA, capB);
    const cdc::Vector3 halfExtents = box->width * 0.5f;
    const int count = CollideBoxAndCapsule(
        cps,
        box->x,
        box->q,
        halfExtents,
        capA,
        capB,
        capsule->radius);
    return CopyContactPoints(contacts, cps, count);
}

// 0x00505660 (PDB: 0x005072a0 / ?CollideCapsuleAndCapsuleWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndCapsuleWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capA,
    const GeometryImpl* capB)
{
    (void)maxContacts;
    cdc::Vector3 a0;
    cdc::Vector3 a1;
    cdc::Vector3 b0;
    cdc::Vector3 b1;
    ComputeCapsuleEndpoints(capA, a0, a1);
    ComputeCapsuleEndpoints(capB, b0, b1);
    return static_cast<unsigned int>(CollideCapsuleAndCapsule(
        &contacts[0].cp,
        a0,
        a1,
        capA->radius,
        b0,
        b1,
        capB->radius));
}

// 0x005057d0 (PDB: 0x00507410 / ?CollideCapsuleAndPlaneWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideCapsuleAndPlaneWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* capsule,
    const GeometryImpl* plane)
{
    (void)maxContacts;
    CPoint cps[4];
    cdc::Vector3 capA;
    cdc::Vector3 capB;
    ComputeCapsuleEndpoints(capsule, capA, capB);
    const int count = CollideCapsuleAndPlane(
        cps,
        capA,
        capB,
        capsule->radius,
        plane->width,
        plane->width.w);
    return CopyContactPoints(contacts, cps, count);
}

// 0x005058e0 (PDB: 0x00507520 / ?CollideBoxAndPlaneWrapper@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideBoxAndPlaneWrapper(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* box,
    const GeometryImpl* plane)
{
    (void)maxContacts;
    CPoint cps[4];
    const cdc::Vector3 halfExtents = box->width * 0.5f;
    const int count = CollideBoxAndPlane(
        cps,
        box->x,
        box->q,
        halfExtents,
        plane->width,
        plane->width.w);
    return CopyContactPoints(contacts, cps, count);
}
