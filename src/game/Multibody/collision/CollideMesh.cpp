#include "game/Multibody/collision/CollideMesh.h"
#include "game/Multibody/collision/CollideTriAndCapsule.h"
#include "game/Multibody/collision/CollideTriAndSphere.h"

#include <cstdint>

// Cross-module Multibody collision extern
extern int CollideTriAndBox(
    CPoint* cp,
    int maxContacts,
    const OBox& box,
    const MTriangle& tri,
    unsigned char adj); // 0x00500980 (PDB: 0x00502680)

namespace {

template <typename TCollideTri>
int CollideMeshFaces(
    CPoint* cp,
    int maxContacts,
    const Mesh* mesh,
    const BBox& queryBox,
    unsigned char flags,
    TCollideTri collideTri)
{
    constexpr uint16_t kMaxQueryFaces = 128;
    uint16_t faceIndices[kMaxQueryFaces];
    // Retail calls non-const Mesh::Query and Mesh::GetTriangle on the const Mesh* parameter
    Mesh* nonConstMesh = const_cast<Mesh*>(mesh);
    const uint16_t numFaces = nonConstMesh->Query(faceIndices, kMaxQueryFaces, queryBox, flags);

    const IndexedFace* faces = mesh->m_faces;
    int totalContacts = 0;

    for (uint16_t i = 0; i < numFaces && totalContacts < maxContacts; ++i) {
        const uint16_t faceIdx = faceIndices[i];
        const IndexedFace& face = faces[faceIdx];
        MTriangle tri;
        nonConstMesh->GetTriangle(&tri, &face);

        CPoint* out = &cp[totalContacts];
        const int added = collideTri(out, maxContacts - totalContacts, tri, face.adjacencyFlags);
        for (int j = 0; j < added; ++j) {
            out[j].faceIndex = faceIdx;
        }
        totalContacts += added;
    }

    for (int i = 0; i < totalContacts; ++i) {
        cp[i].position = cp[i].position + mesh->m_position;
    }

    return totalContacts;
}

} // namespace

// 0x004fcd40 (PDB: 0x004fea40)
int CollideMeshAndSphere(
    CPoint* cp,
    int maxContacts,
    const MSphere& sphere,
    const Mesh* mesh,
    unsigned char flags)
{
    const cdc::Vector3 localCenter = sphere.x - mesh->m_position;
    const cdc::Vector3 extent = cdc::Vector3::Make(sphere.radius, sphere.radius, sphere.radius, 0.0f);

    BBox queryBox;
    queryBox.bMin = sphere.x - extent;
    queryBox.bMax = sphere.x + extent;

    return CollideMeshFaces(
        cp,
        maxContacts,
        mesh,
        queryBox,
        flags,
        [&](CPoint* out, int /*remaining*/, const MTriangle& tri, unsigned char adj) {
            return CollideTriAndSphere(out, localCenter, sphere.radius, tri, adj);
        });
}

// 0x004fcea0 (PDB: 0x004feba0)
int CollideMeshAndBox(
    CPoint* cp,
    int maxContacts,
    const OBox& box,
    const Mesh* mesh,
    unsigned char flags)
{
    OBox localBox = box;
    localBox.x = localBox.x - mesh->m_position;

    return CollideMeshFaces(
        cp,
        maxContacts,
        mesh,
        localBox.aabb,
        flags,
        [&](CPoint* out, int remaining, const MTriangle& tri, unsigned char adj) {
            return CollideTriAndBox(out, remaining, localBox, tri, adj);
        });
}

// 0x004fcfd0 (PDB: 0x004fecd0)
int CollideMeshAndCapsule(
    CPoint* cp,
    int maxContacts,
    const Mesh* mesh,
    const cdc::Vector3& capA,
    const cdc::Vector3& capB,
    float capRad,
    const BBox& aabb,
    unsigned char flags)
{
    const cdc::Vector3 localA = capA - mesh->m_position;
    const cdc::Vector3 localB = capB - mesh->m_position;

    return CollideMeshFaces(
        cp,
        maxContacts,
        mesh,
        aabb,
        flags,
        [&](CPoint* out, int remaining, const MTriangle& tri, unsigned char adj) {
            return CollideTriAndCapsule(out, remaining, tri, localA, localB, capRad, adj);
        });
}
