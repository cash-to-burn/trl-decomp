#include "game/Multibody/src/SweptSphere.h"

#include <algorithm>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideMesh.h"
#include "game/Multibody/collision/CollideSegment.h"
#include "game/Multibody/collision/CollideSweptSphere.h"
#include "game/Multibody/collision/CollideTriAndSphere.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/CollisionManager.h"
#include "game/Multibody/src/Geometry.h"
#include "game/Multibody/src/MultibodyTypes.h"
#include "game/Multibody/src/SweepAndPrune.h"
#include "game/camera/cameracollide.h"

namespace {

void BuildOBoxFromGeometry(OBox& box, const GeometryImpl* geom, bool buildInverseTranspose)
{
    box.x = geom->x;
    box.R.Build(geom->q);
    if (buildInverseTranspose) {
        cdc::Transpose(&box.RT, box.R);
    } else {
        box.RT.Build(~geom->q);
    }
    box.h = geom->width * 0.5f;
    box.aabb.bMin = geom->aabb->lo;
    box.aabb.bMax = geom->aabb->hi;
}

void PopulatePhantomContact(
    Contact& dst,
    const CPoint& cp,
    Body* body2,
    Geometry* geom1,
    Geometry* geom2,
    int meshCollection,
    Mesh* mesh)
{
    dst.cp = cp;
    dst.velocity = cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
    dst.force = cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
    dst.body1 = nullptr;
    dst.body2 = body2;
    dst.geometry1 = geom1;
    dst.geometry2 = geom2;
    dst.meshCollection = meshCollection;
    dst.mesh = mesh;
    dst.restitution = 0.0f;
    dst.friction = 0.0f;
    dst.softness = 0.0f;
}

} // namespace

// 0x005190b0 (PDB: 0x0051abe0 / ?CollidePhantomAndMesh@@YAXPAVMultibodySystemImpl@@PAVGeometryImpl@@@Z)
void CollidePhantomAndMesh(MultibodySystemImpl* system, GeometryImpl* geom)
{
    const ResponseData& response = system->responseArray[geom->category][0];
    if (response.contactFcn == nullptr) {
        return;
    }

    constexpr int kMaxPhantomContacts = 32;
    Contact contacts[kMaxPhantomContacts] = {};
    CPoint cpoints[kMaxPhantomContacts];
    int numContacts = 0;

    if (geom->type == Geometry::Sphere) {
        const float queryRadius = geom->radius + 1.0f;
        const cdc::Vector3 extent = cdc::Vector3::Make(queryRadius, queryRadius, queryRadius);
        const cdc::Vector3 start = geom->x0;
        const cdc::Vector3 end = geom->x;
        const cdc::Vector3 dir = end - start;

        BBox sweepBox;
        sweepBox.bMin = cdc::Vector3::Make(
            std::min(start.x, end.x) - extent.x,
            std::min(start.y, end.y) - extent.y,
            std::min(start.z, end.z) - extent.z);
        sweepBox.bMax = cdc::Vector3::Make(
            std::max(start.x, end.x) + extent.x,
            std::max(start.y, end.y) + extent.y,
            std::max(start.z, end.z) + extent.z);

        for (int collIdx = 0; collIdx < system->size.numMeshCollections && numContacts < kMaxPhantomContacts; ++collIdx) {
            const MeshCollection& coll = system->meshCollections[collIdx];
            for (int meshIdx = 0; meshIdx < coll.numMeshes && numContacts < kMaxPhantomContacts; ++meshIdx) {
                Mesh* mesh = coll.meshes[meshIdx];
                if (response.nearFcn != nullptr && !response.nearFcn(mesh, nullptr, geom)) {
                    continue;
                }

                uint16_t faceIndices[128];
                const uint16_t numFaces = mesh->Query(faceIndices, 128, sweepBox, 0xf);
                const cdc::Vector3 localStart = start - mesh->m_position;

                for (uint16_t f = 0; f < numFaces && numContacts < kMaxPhantomContacts; ++f) {
                    MTriangle tri;
                    mesh->GetTriangle(&tri, &mesh->m_faces[faceIndices[f]]);

                    CPoint& cp = cpoints[numContacts];
                    bool hit = false;
                    if (CollideTriAndSphere(&cp, localStart, geom->radius, tri, 0x3f) != 0) {
                        cp.position += mesh->m_position;
                        hit = true;
                    } else {
                        MSphere sphere;
                        sphere.x = localStart;
                        sphere.radius = geom->radius;

                        float hitT = 1.0f;
                        cdc::Vector3 hitNormal;
                        if (CollideSweptSphereAndTriangle(&hitT, &hitNormal, sphere, dir, tri) == HIT) {
                            cp.position = start + dir * hitT - hitNormal * geom->radius;
                            cp.normal = hitNormal;
                            cp.separation = 0.0f;
                            cp.faceIndex = faceIndices[f];
                            hit = true;
                        }
                    }

                    if (hit) {
                        PopulatePhantomContact(contacts[numContacts], cp, nullptr, nullptr, geom, collIdx, mesh);
                        ++numContacts;
                    }
                }
            }
        }
    } else if (geom->type == Geometry::Box) {
        OBox obox;
        BuildOBoxFromGeometry(obox, geom, false);

        for (int collIdx = 0; collIdx < system->size.numMeshCollections && numContacts < kMaxPhantomContacts; ++collIdx) {
            const MeshCollection& coll = system->meshCollections[collIdx];
            for (int meshIdx = 0; meshIdx < coll.numMeshes && numContacts < kMaxPhantomContacts; ++meshIdx) {
                Mesh* mesh = coll.meshes[meshIdx];
                if (response.nearFcn != nullptr && !response.nearFcn(mesh, nullptr, geom)) {
                    continue;
                }

                const int added = CollideMeshAndBox(
                    &cpoints[numContacts],
                    kMaxPhantomContacts - numContacts,
                    obox,
                    mesh,
                    0xff);
                for (int i = 0; i < added; ++i) {
                    PopulatePhantomContact(
                        contacts[numContacts + i],
                        cpoints[numContacts + i],
                        nullptr,
                        nullptr,
                        geom,
                        collIdx,
                        mesh);
                }
                numContacts += added;
            }
        }
    }

    if (numContacts > 0) {
        response.contactFcn(contacts, numContacts);
    }

    if (system->drawContacts) {
        for (int i = 0; i < numContacts; ++i) {
            DrawContactPoint(contacts[i].cp.position, contacts[i].cp.normal, false);
        }
    }
}

// 0x00519760 (PDB: 0x0051b290 / ?CollidePhantomAndGeometries@@YAXPAVMultibodySystemImpl@@PAVGeometryImpl@@@Z)
void CollidePhantomAndGeometries(MultibodySystemImpl* system, GeometryImpl* geom)
{
    SAPBox* box = geom->aabb;
    if (box == nullptr) {
        return;
    }

    for (BoxPair* pair = box->pairList; pair != nullptr; pair = pair->next) {
        GeometryImpl* other = static_cast<GeometryImpl*>(pair->otherBox->clientData);
        const ResponseData& response = system->responseArray[geom->category][other->category];
        if (response.nearFcn != nullptr && !response.nearFcn(nullptr, geom, other)) {
            continue;
        }
        if (response.contactFcn == nullptr) {
            continue;
        }

        const cdc::Vector3 savedX = geom->x;
        if (geom->type == Geometry::Sphere) {
            geom->x = geom->x0;
        }

        constexpr int kMaxGeomContacts = 12;
        Contact contacts[kMaxGeomContacts] = {};
        int numContacts = static_cast<int>(CollideGeometryPair(contacts, kMaxGeomContacts, geom, other));

        if (geom->type == Geometry::Sphere) {
            geom->x = savedX;
        }

        if (numContacts == 0 && geom->type == Geometry::Sphere) {
            const cdc::Vector3 dir = geom->x - geom->x0;
            MSphere sphereA;
            sphereA.x = geom->x0;
            sphereA.radius = geom->radius;

            float hitT = 1.0f;
            cdc::Vector3 hitNormal;
            CollideCode sweptResult = NO_HIT;

            if (other->type == Geometry::Sphere) {
                MSphere sphereB;
                sphereB.x = other->x;
                sphereB.radius = other->radius;
                sweptResult = CollideSweptSphereAndSphere(&hitT, &hitNormal, sphereA, dir, sphereB);
            } else if (other->type == Geometry::Box) {
                OBox boxB;
                BuildOBoxFromGeometry(boxB, other, true);
                sweptResult = CollideSweptSphereAndBox(&hitT, &hitNormal, sphereA, dir, boxB);
            }

            if (sweptResult == HIT) {
                CPoint& cp = contacts[0].cp;
                cp.position = geom->x0 + dir * hitT - hitNormal * geom->radius;
                cp.normal = hitNormal;
                cp.separation = 0.0f;
                numContacts = 1;
            }
        }

        if (numContacts > 0) {
            for (int i = 0; i < numContacts; ++i) {
                PopulatePhantomContact(contacts[i], contacts[i].cp, other->body, geom, other, 0, nullptr);
                if (system->drawContacts) {
                    DrawContactPoint(contacts[i].cp.position, contacts[i].cp.normal, false);
                }
            }
            response.contactFcn(contacts, numContacts);
        }
    }
}

// 0x00519b30 (PDB: 0x0051b660 / ?CollidePhantoms@@YAXPAVMultibodySystemImpl@@@Z)
void CollidePhantoms(MultibodySystemImpl* system)
{
    for (GeometryImpl* geom = system->geometryList; geom != nullptr; geom = geom->next) {
        if ((geom->flags & 0x40u) == 0) {
            continue;
        }
        if (geom->body != nullptr && (geom->body->flags & Body::FLAG_DISABLED) != 0) {
            continue;
        }

        if ((geom->flags & 0x1u) != 0) {
            CollidePhantomAndMesh(system, geom);
        }
        if ((geom->flags & 0x2u) != 0) {
            CollidePhantomAndGeometries(system, geom);
        }
    }
}
