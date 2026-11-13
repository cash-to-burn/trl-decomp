#include "game/Multibody/src/CollisionManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "game/Multibody/MultiBodySettings.h"
#include "game/Multibody/collision/CollideBoxAndBox.h"
#include "game/Multibody/collision/CollideMesh.h"
#include "game/Multibody/collision/CollideSegment.h"
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/CollisionBroker.h"
#include "game/Multibody/src/ContactJoint.h"
#include "game/Multibody/src/Geometry.h"
#include "game/Multibody/src/SweepAndPrune.h"

// External diagnostics and debug state
extern void MULTIBODY_PrintWarning(const char* fmt, ...); // 0x00552e70 (PDB: 0x00554e70)
extern const char* MB_CurrentEntity;                      // 0x01116144 (PDB: 0x011172fc)

namespace {

typedef unsigned int (*ColliderFn)(Contact*, unsigned int, const GeometryImpl*, const GeometryImpl*);

struct ColliderInfo {
    ColliderFn fcn;
    bool reverse;
};

constexpr int NUM_GEOMETRY_TYPES = 4;
constexpr int MAX_MESH_CONTACTS = 32;
constexpr int MAX_CLUSTER_POINTS = 32;
constexpr int MAX_PAIR_CONTACTS = 4;

ColliderInfo sColliders[NUM_GEOMETRY_TYPES][NUM_GEOMETRY_TYPES];
bool sCollidersInitialized = false;

inline cdc::Vector3 ComputePointVelocity(const BodyImpl* body, const cdc::Vector3& worldPt)
{
    cdc::Vector3 angVel;
    cdc::Cross3(&angVel, body->omega, worldPt - body->x);
    return body->v + angVel;
}

inline void WakeBodyBoxes(BodyImpl* body, SAPBox*& queueTail)
{
    body->flags &= ~Body::FLAG_SLEEPING;
    for (GeometryImpl* g = body->firstGeometry; g != nullptr; g = g->bnext) {
        SAPBox* wokenBox = g->aabb;
        wokenBox->next = nullptr;
        queueTail->next = wokenBox;
        queueTail = wokenBox;
    }
}

inline int ReduceClusterContacts(
    Contact* outReduced,
    const Contact* contacts,
    const int* clusterIndices,
    int clusterCount,
    const cdc::Vector3& clusterNormal)
{
    if (clusterCount <= ContactJoint::MAX_POINTS) {
        for (int i = 0; i < clusterCount; ++i) {
            outReduced[i] = contacts[clusterIndices[i]];
        }
        return clusterCount;
    }

    cdc::Vector3 u;
    cdc::Vector3 v;
    ComputeSkewedBasis(&u, &v, clusterNormal);

    int minUIdx = 0;
    int maxUIdx = 0;
    int minVIdx = 0;
    int maxVIdx = 0;
    int deepestIdx = 0;

    const Contact& first = contacts[clusterIndices[0]];
    float minU = cdc::Dot3(first.cp.position, u);
    float maxU = minU;
    float minV = cdc::Dot3(first.cp.position, v);
    float maxV = minV;
    float deepestSep = first.cp.separation;

    for (int i = 1; i < clusterCount; ++i) {
        const Contact& c = contacts[clusterIndices[i]];
        const float pu = cdc::Dot3(c.cp.position, u);
        if (pu < minU) {
            minU = pu;
            minUIdx = i;
        }
        if (pu > maxU) {
            maxU = pu;
            maxUIdx = i;
        }

        const float pv = cdc::Dot3(c.cp.position, v);
        if (pv < minV) {
            minV = pv;
            minVIdx = i;
        }
        if (pv > maxV) {
            maxV = pv;
            maxVIdx = i;
        }

        if (c.cp.separation < deepestSep) {
            deepestSep = c.cp.separation;
            deepestIdx = i;
        }
    }

    bool keep[MAX_CLUSTER_POINTS] = {};
    keep[minUIdx] = true;
    keep[maxUIdx] = true;
    keep[minVIdx] = true;
    keep[maxVIdx] = true;
    keep[deepestIdx] = true;

    int reducedCount = 0;
    for (int i = 0; i < clusterCount; ++i) {
        if (keep[i]) {
            outReduced[reducedCount++] = contacts[clusterIndices[i]];
        }
    }
    return reducedCount;
}

inline bool BodyHasActiveWorldPinJoint(const BodyImpl* body)
{
    for (const JointNode* node = body->firstJoint; node != nullptr; node = node->next) {
        if (node->otherBody == nullptr && !(node->joint->flags & JointImpl::FLAG_DISABLED)) {
            return true;
        }
    }
    return false;
}

} // namespace

// 0x00505980 (PDB: 0x005075c0 / ?DoesJointTrumpCollision@@YA_NPBVBodyImpl@@0@Z)
bool DoesJointTrumpCollision(const BodyImpl* b1, const BodyImpl* b2)
{
    if (b1 == nullptr || b2 == nullptr) {
        return false;
    }

    for (const JointNode* node = b1->firstJoint; node != nullptr; node = node->next) {
        if (node->otherBody == b2 &&
            !(node->joint->flags & JointImpl::FLAG_DISABLED) &&
            (node->joint->flags & JointImpl::FLAG_NO_COLLIDE)) {
            return true;
        }
    }
    return false;
}

// 0x005059f0 (PDB: 0x00507630 / ?ClusterContacts@@YAHPAVVector3@cdc@@QAY0CA@HPAHPBUContact@@H@Z)
int ClusterContacts(
    cdc::Vector3* clusterNormals,
    int (*clusterIndices)[32],
    int* clusterCounts,
    const Contact* contacts,
    int numContacts)
{
    if (numContacts <= 0) {
        return 0;
    }

    clusterNormals[0] = contacts[0].cp.normal;
    clusterIndices[0][0] = 0;
    clusterCounts[0] = 1;
    int numClusters = 1;

    for (int i = 1; i < numContacts; ++i) {
        const cdc::Vector3& normal = contacts[i].cp.normal;
        int matched = -1;
        for (int c = 0; c < numClusters; ++c) {
            if (cdc::Dot3(clusterNormals[c], normal) > 0.95f) {
                matched = c;
                break;
            }
        }

        if (matched >= 0) {
            int& count = clusterCounts[matched];
            if (count < MAX_CLUSTER_POINTS) {
                clusterIndices[matched][count++] = i;
            }
        } else if (numClusters < MAX_CLUSTER_POINTS) {
            clusterNormals[numClusters] = normal;
            clusterIndices[numClusters][0] = i;
            clusterCounts[numClusters] = 1;
            ++numClusters;
        }
    }

    return numClusters;
}

void InitColliderArray()
{
    sColliders[Geometry::Box][Geometry::Box] = { CollideBoxAndBoxWrapper, false };
    sColliders[Geometry::Box][Geometry::Capsule] = { CollideCapsuleAndBoxWrapper, true };
    sColliders[Geometry::Box][Geometry::Plane] = { CollideBoxAndPlaneWrapper, false };
    sColliders[Geometry::Box][Geometry::Sphere] = { CollideSphereAndBoxWrapper, true };

    sColliders[Geometry::Capsule][Geometry::Box] = { CollideCapsuleAndBoxWrapper, false };
    sColliders[Geometry::Capsule][Geometry::Capsule] = { CollideCapsuleAndCapsuleWrapper, false };
    sColliders[Geometry::Capsule][Geometry::Plane] = { CollideCapsuleAndPlaneWrapper, false };
    sColliders[Geometry::Capsule][Geometry::Sphere] = { CollideCapsuleAndSphereWrapper, false };

    sColliders[Geometry::Plane][Geometry::Box] = { CollideBoxAndPlaneWrapper, true };
    sColliders[Geometry::Plane][Geometry::Capsule] = { CollideCapsuleAndPlaneWrapper, true };
    sColliders[Geometry::Plane][Geometry::Plane] = { nullptr, false };
    sColliders[Geometry::Plane][Geometry::Sphere] = { CollideSphereAndPlaneWrapper, true };

    sColliders[Geometry::Sphere][Geometry::Box] = { CollideSphereAndBoxWrapper, false };
    sColliders[Geometry::Sphere][Geometry::Capsule] = { CollideCapsuleAndSphereWrapper, true };
    sColliders[Geometry::Sphere][Geometry::Plane] = { CollideSphereAndPlaneWrapper, false };
    sColliders[Geometry::Sphere][Geometry::Sphere] = { CollideSphereAndSphereWrapper, false };

    sCollidersInitialized = true;
}

// 0x00505ce0 (PDB: 0x00507920 / ?CollideGeometryPair@@YAIPAUContact@@IPBVGeometryImpl@@1@Z)
unsigned int CollideGeometryPair(
    Contact* contacts,
    unsigned int maxContacts,
    const GeometryImpl* g1,
    const GeometryImpl* g2)
{
    if (!sCollidersInitialized) {
        InitColliderArray();
    }

    if (g1->type >= NUM_GEOMETRY_TYPES || g2->type >= NUM_GEOMETRY_TYPES) {
        return 0;
    }

    const ColliderInfo& info = sColliders[g1->type][g2->type];
    if (info.fcn == nullptr) {
        return 0;
    }

    const unsigned int count = info.reverse
        ? info.fcn(contacts, maxContacts, g2, g1)
        : info.fcn(contacts, maxContacts, g1, g2);

    for (unsigned int i = 0; i < count; ++i) {
        if (info.reverse) {
            contacts[i].cp.normal = contacts[i].cp.normal * -1.0f;
        }
        contacts[i].cp.faceIndex = 0xffffu;
    }

    return count;
}

// 0x00505d90 (PDB: 0x005079d0 / ?DrawContactPoint@@YAXABVVector3@cdc@@0_N@Z)
void DrawContactPoint(const cdc::Vector3& pos, const cdc::Vector3& normal, bool hit)
{
    cdc::Vector3 u;
    cdc::Vector3 v;
    ComputeBasis(&u, &v, normal);

    const cdc::Vector3 normalColor = hit
        ? cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f)
        : cdc::Vector3::Make(1.0f, 0.0f, 1.0f, 0.0f);
    const cdc::Vector3 tangentColor = hit
        ? cdc::Vector3::Make(0.0f, 1.0f, 0.0f, 0.0f)
        : cdc::Vector3::Make(1.0f, 1.0f, 0.0f, 0.0f);

    MB_DrawLine(pos, pos + normal * 200.0f, normalColor);
    MB_DrawLine(pos, pos + u * 200.0f, tangentColor);
    MB_DrawLine(pos, pos + v * 200.0f, tangentColor);
}

// 0x00506210 (PDB: 0x00507e50 / ?CollideGeometryAndMesh@@YAHPAUContact@@HPBVMultibodySystemImpl@@PBVGeometryImpl@@@Z)
int CollideGeometryAndMesh(
    Contact* contacts,
    int maxContacts,
    const MultibodySystemImpl* system,
    const GeometryImpl* geom)
{
    if (maxContacts <= 0) {
        return 0;
    }

    if (geom->type != Geometry::Box &&
        geom->type != Geometry::Capsule &&
        geom->type != Geometry::Sphere) {
        return 0;
    }

    const int capacity = std::min(maxContacts, MAX_MESH_CONTACTS);
    auto nearFcn = system->responseArray[geom->category][0].nearFcn;

    MSphere sphere = {};
    cdc::Vector3 capA = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    cdc::Vector3 capB = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    BBox capAABB = {};
    OBox box = {};

    switch (geom->type) {
    case Geometry::Sphere:
        sphere.x = geom->x;
        sphere.radius = geom->radius;
        break;
    case Geometry::Capsule: {
        const cdc::Vector3 delta = geom->q.Rotate(cdc::Vector3::Make(0.0f, 0.0f, 0.5f * geom->length, 0.0f));
        capA = geom->x - delta;
        capB = geom->x + delta;
        capAABB.bMin = geom->aabb->lo;
        capAABB.bMax = geom->aabb->hi;
        break;
    }
    case Geometry::Box:
        box.x = geom->x;
        box.h = geom->width * 0.5f;
        box.R.Build(geom->q);
        box.RT.Build(~geom->q);
        box.aabb.bMin = geom->aabb->lo;
        box.aabb.bMax = geom->aabb->hi;
        break;
    default:
        return 0;
    }

    CPoint cpBuffer[MAX_MESH_CONTACTS];
    int totalContacts = 0;

    for (int mc = 0; mc < system->size.numMeshCollections && totalContacts < capacity; ++mc) {
        const MeshCollection& collection = system->meshCollections[mc];
        for (int m = 0; m < collection.numMeshes && totalContacts < capacity; ++m) {
            Mesh* mesh = collection.meshes[m];
            if (nearFcn != nullptr && !nearFcn(mesh, nullptr, const_cast<GeometryImpl*>(geom))) {
                continue;
            }

            const int remaining = capacity - totalContacts;
            CPoint* outCp = &cpBuffer[totalContacts];
            int count = 0;

            switch (geom->type) {
            case Geometry::Sphere:
                count = CollideMeshAndSphere(outCp, remaining, sphere, mesh, 0xffu);
                break;
            case Geometry::Capsule:
                count = CollideMeshAndCapsule(outCp, remaining, mesh, capA, capB, geom->radius, capAABB, 0xffu);
                break;
            case Geometry::Box:
                count = CollideMeshAndBox(outCp, remaining, box, mesh, 0xffu);
                break;
            default:
                break;
            }

            for (int i = 0; i < count; ++i) {
                Contact& dst = contacts[totalContacts + i];
                dst.cp = outCp[i];
                dst.meshCollection = mc;
                dst.mesh = mesh;
            }
            totalContacts += count;
        }
    }

    for (int i = 0; i < totalContacts; ++i) {
        contacts[i].body1 = nullptr;
        contacts[i].body2 = geom->body;
        contacts[i].geometry1 = nullptr;
        contacts[i].geometry2 = const_cast<GeometryImpl*>(geom);
    }

    return totalContacts;
}

// 0x005067f0 (PDB: 0x00508430 / ?CollideMesh@@YAXPAVMultibodySystemImpl@@@Z)
void CollideMesh(MultibodySystemImpl* system)
{
    const float gravityMag = system->gravity.Len3();

    for (BodyImpl* body = system->bodyList; body != nullptr; body = body->next) {
        if ((body->flags & (Body::FLAG_SLEEPING | Body::FLAG_DISABLED)) != 0) {
            continue;
        }
        if (BodyHasActiveWorldPinJoint(body)) {
            continue;
        }

        Contact contacts[MAX_MESH_CONTACTS];
        for (int i = 0; i < MAX_MESH_CONTACTS; ++i) {
            contacts[i].force = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            contacts[i].body1 = nullptr;
            contacts[i].body2 = nullptr;
            contacts[i].geometry1 = nullptr;
            contacts[i].geometry2 = nullptr;
            contacts[i].mesh = nullptr;
        }

        MB_CurrentEntity = body->name;

        int numContacts = 0;
        for (GeometryImpl* geom = body->firstGeometry; geom != nullptr && numContacts < MAX_MESH_CONTACTS; geom = geom->bnext) {
            if ((geom->flags & 0x1u) == 0) {
                continue;
            }

            const int count = CollideGeometryAndMesh(&contacts[numContacts], MAX_MESH_CONTACTS - numContacts, system, geom);
            if (count == 0) {
                continue;
            }

            const float friction = gravityMag * geom->mu * body->mass;
            for (int i = 0; i < count; ++i) {
                Contact& c = contacts[numContacts + i];
                c.body1 = nullptr;
                c.body2 = body;
                c.geometry1 = nullptr;
                c.geometry2 = geom;
                c.velocity = ComputePointVelocity(body, c.cp.position);
                c.restitution = geom->restitution;
                c.friction = friction;
                c.softness = system->contactGamma;
            }

            system->QueryContactCache(&contacts[numContacts], count);

            auto contactFcn = system->responseArray[geom->category][0].contactFcn;
            if (contactFcn == nullptr || contactFcn(&contacts[numContacts], count)) {
                numContacts += count;
            } else if (system->drawContacts) {
                for (int i = 0; i < count; ++i) {
                    DrawContactPoint(contacts[numContacts + i].cp.position, contacts[numContacts + i].cp.normal, false);
                }
            }
        }

        if (numContacts == 0) {
            continue;
        }

        if (numContacts == MAX_MESH_CONTACTS) {
            MULTIBODY_PrintWarning("WARNING: Terrain too complicated for body %s, generated %d points.\n", body->name, numContacts);
        }

        cdc::Vector3 clusterNormals[MAX_CLUSTER_POINTS];
        int clusterIndices[MAX_CLUSTER_POINTS][MAX_CLUSTER_POINTS];
        int clusterCounts[MAX_CLUSTER_POINTS] = {};
        const int numClusters = ClusterContacts(clusterNormals, clusterIndices, clusterCounts, contacts, numContacts);

        for (int c = 0; c < numClusters; ++c) {
            Contact reduced[ContactJoint::MAX_POINTS];
            const int reducedCount = ReduceClusterContacts(
                reduced,
                contacts,
                clusterIndices[c],
                clusterCounts[c],
                clusterNormals[c]);

            const float invCount = 1.0f / static_cast<float>(reducedCount);
            for (int j = 0; j < reducedCount; ++j) {
                reduced[j].friction *= invCount;
            }

            ContactJoint* joint = system->CreateContactJoint();
            if (joint == nullptr) {
                body->flags |= 0x2u;
                body->v = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
                body->omega = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
                break;
            }

            body->flags &= ~0x2u;
            if (system->drawContacts) {
                for (int j = 0; j < reducedCount; ++j) {
                    DrawContactPoint(reduced[j].cp.position, reduced[j].cp.normal, true);
                }
            }

            joint->Set(nullptr, body, reduced, reducedCount, clusterNormals[c], body->firstGeometry->mu);
        }
    }

    MB_CurrentEntity = nullptr;
}

// 0x00507110 (PDB: 0x00508d50 / ?CollideGeometryAndGeometries@@YAHPAUContact@@HPBVMultibodySystemImpl@@PAVGeometryImpl@@@Z)
int CollideGeometryAndGeometries(
    Contact* contacts,
    int maxContacts,
    const MultibodySystemImpl* system,
    GeometryImpl* geom)
{
    int numContacts = 0;
    for (BoxPair* pair = geom->aabb->pairList; pair != nullptr && numContacts < maxContacts; pair = pair->next) {
        GeometryImpl* other = static_cast<GeometryImpl*>(pair->otherBox->clientData);
        if ((other->flags & 0x40u) != 0 || (other->flags & 0x2u) == 0) {
            continue;
        }
        if (other->body != nullptr && (other->body->flags & Body::FLAG_DISABLED) != 0) {
            continue;
        }

        auto nearFcn = system->responseArray[geom->category][other->category].nearFcn;
        if (nearFcn != nullptr && !nearFcn(nullptr, geom, other)) {
            continue;
        }

        Contact localContacts[MAX_PAIR_CONTACTS];
        const int count = static_cast<int>(CollideGeometryPair(localContacts, MAX_PAIR_CONTACTS, other, geom));
        const int copyCount = std::min(count, maxContacts - numContacts);
        for (int i = 0; i < copyCount; ++i) {
            Contact& dst = contacts[numContacts + i];
            dst.cp = localContacts[i].cp;
            dst.cp.faceIndex = 0xffffu;
            dst.velocity = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            dst.force = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            dst.body1 = other->body;
            dst.body2 = geom->body;
            dst.geometry1 = other;
            dst.geometry2 = geom;
            dst.meshCollection = 0;
            dst.mesh = nullptr;
            dst.restitution = 0.0f;
            dst.friction = 0.0f;
            dst.softness = 0.0f;
        }
        numContacts += copyCount;
    }
    return numContacts;
}

// 0x00507330 (PDB: 0x00508f70 / ?CollideGeometries@@YAXPAVMultibodySystemImpl@@@Z)
void CollideGeometries(MultibodySystemImpl* system)
{
    const float gravityMag = system->gravity.Len3();
    SAPBox* queueHead = nullptr;
    SAPBox* queueTail = nullptr;

    for (GeometryImpl* geom = system->geometryList; geom != nullptr; geom = geom->next) {
        SAPBox* box = geom->aabb;
        box->flags &= ~SAPBox::FLAG_DEACTIVATED;

        if ((geom->flags & 0x2u) != 0 &&
            geom->body != nullptr &&
            !(box->flags & SAPBox::FLAG_STATIC) &&
            box->pairList != nullptr &&
            !(geom->body->flags & (Body::FLAG_SLEEPING | Body::FLAG_DISABLED))) {
            box->next = queueHead;
            queueHead = box;
            if (queueTail == nullptr) {
                queueTail = box;
            }
        }
    }

    for (SAPBox* box = queueHead; box != nullptr; box = box->next) {
        GeometryImpl* geom = static_cast<GeometryImpl*>(box->clientData);
        for (JointNode* jNode = geom->body->firstJoint; jNode != nullptr; jNode = jNode->next) {
            if (jNode->otherBody != nullptr && (jNode->otherBody->flags & Body::FLAG_SLEEPING) != 0) {
                WakeBodyBoxes(jNode->otherBody, queueTail);
            }
        }

        for (BoxPair* pair = box->pairList; pair != nullptr; pair = pair->next) {
            SAPBox* otherBox = pair->otherBox;
            if ((otherBox->flags & SAPBox::FLAG_DEACTIVATED) != 0) {
                continue;
            }

            GeometryImpl* other = static_cast<GeometryImpl*>(otherBox->clientData);
            if ((other->flags & 0x40u) != 0 || (other->flags & 0x2u) == 0) {
                continue;
            }

            BodyImpl* bodyA = geom->body;
            BodyImpl* bodyB = other->body;
            if (bodyA == bodyB || DoesJointTrumpCollision(bodyA, bodyB)) {
                continue;
            }
            if (bodyB != nullptr && (bodyB->flags & Body::FLAG_DISABLED) != 0) {
                continue;
            }

            const ResponseData& response = system->responseArray[geom->category][other->category];
            if (response.nearFcn != nullptr) {
                const bool nearOk = (bodyB != nullptr)
                    ? response.nearFcn(nullptr, geom, other)
                    : response.nearFcn(nullptr, other, geom);
                if (!nearOk) {
                    continue;
                }
            }

            Contact contacts[MAX_PAIR_CONTACTS];
            const int numContacts = static_cast<int>(CollideGeometryPair(contacts, MAX_PAIR_CONTACTS, geom, other));
            if (numContacts == 0) {
                continue;
            }

            if (bodyB != nullptr && (bodyB->flags & Body::FLAG_SLEEPING) != 0 && bodyA->rank >= bodyB->rank) {
                WakeBodyBoxes(bodyB, queueTail);
            }

            BodyImpl* b1 = nullptr;
            BodyImpl* b2 = nullptr;
            bool swapped = false;
            if (bodyB != nullptr && bodyB->rank <= bodyA->rank) {
                b1 = bodyA;
                b2 = bodyB;
                swapped = false;
            } else {
                b1 = bodyB;
                b2 = bodyA;
                swapped = true;
            }

            const float minMu = std::min(other->mu, geom->mu);
            const float minMass = (b1 != nullptr) ? std::min(b1->mass, b2->mass) : b2->mass;
            const float restitution = std::min(other->restitution, geom->restitution);
            const float friction = gravityMag * minMass * minMu / static_cast<float>(numContacts);

            for (int i = 0; i < numContacts; ++i) {
                Contact& c = contacts[i];
                if (swapped) {
                    c.cp.normal = c.cp.normal * -1.0f;
                    c.geometry1 = other;
                    c.geometry2 = geom;
                } else {
                    c.geometry1 = geom;
                    c.geometry2 = other;
                }

                c.body1 = b1;
                c.body2 = b2;
                c.meshCollection = -1;
                c.mesh = nullptr;

                cdc::Vector3 relVel = ComputePointVelocity(b2, c.cp.position);
                if (b1 != nullptr) {
                    relVel = relVel - ComputePointVelocity(b1, c.cp.position);
                }
                c.velocity = relVel;
                c.force = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
                c.restitution = restitution;
                c.friction = friction;
                c.softness = system->contactGamma;
            }

            system->QueryContactCache(contacts, numContacts);

            const bool keep = (response.contactFcn != nullptr) ? response.contactFcn(contacts, numContacts) : true;
            if (system->drawContacts) {
                for (int i = 0; i < numContacts; ++i) {
                    DrawContactPoint(contacts[i].cp.position, contacts[i].cp.normal, keep);
                }
            }

            if (keep) {
                ContactJoint* joint = system->CreateContactJoint();
                if (joint != nullptr) {
                    joint->Set(b1, b2, contacts, numContacts, contacts[0].cp.normal, minMu);
                }
            }
        }

        box->flags |= SAPBox::FLAG_DEACTIVATED;
    }
}
