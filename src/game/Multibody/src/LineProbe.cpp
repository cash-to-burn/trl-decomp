#include "game/Multibody/src/LineProbe.h"

#include "game/Multibody/MultiBodySettings.h"
#include "game/Multibody/collision/CollideSegment.h"
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/CollisionManager.h"
#include "game/Multibody/src/Geometry.h"
#include "game/Multibody/src/MultibodyTypes.h"
#include "game/camera/cameracollide.h"

// 0x00514b20 (PDB: 0x00516610 / ?LineProbeNarrowPhase@@YA_NPAMPAVVector3@cdc@@ABV12@2PAX3@Z)
bool LineProbeNarrowPhase(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    void* geomVoid,
    void* userData)
{
    GeometryImpl* geom = static_cast<GeometryImpl*>(geomVoid);
    if ((geom->flags & 0x4u) == 0) {
        return false;
    }

    if (geom->body != nullptr && (geom->body->flags & Body::FLAG_DISABLED) != 0) {
        return false;
    }

    const LineProbeGeometryData* data = static_cast<const LineProbeGeometryData*>(userData);
    if (data != nullptr && data->fcn != nullptr && !data->fcn(geom, data->clientData)) {
        return false;
    }

    float bestT = *t;
    CollideCode result = NO_HIT;

    switch (geom->type) {
    case Geometry::Sphere: {
        MSphere sphere;
        sphere.x = geom->x;
        sphere.radius = geom->radius;
        result = CollideSegmentAndSphere(&bestT, normal, start, dir, sphere);
        break;
    }

    case Geometry::Box: {
        MBox box;
        box.x = geom->x;
        box.q = geom->q;
        box.h = geom->width * 0.5f;
        result = CollideSegmentAndBox(&bestT, normal, start, dir, box);
        break;
    }

    case Geometry::Capsule: {
        const cdc::Vector3 halfAxis = geom->q.Rotate(cdc::Vector3::Make(0.0f, 0.0f, geom->length * 0.5f));
        const cdc::Vector3 capA = geom->x - halfAxis;
        const cdc::Vector3 capB = geom->x + halfAxis;
        result = CollideSegmentAndCapsule(&bestT, normal, start, dir, capA, capB, geom->radius);
        break;
    }

    case Geometry::Plane: {
        const cdc::Vector3 planeNormal = geom->width;
        const float denom = cdc::Dot3(planeNormal, dir);
        if (denom < 0.0f) {
            const float hitT = (geom->width.w - cdc::Dot3(planeNormal, start)) / denom;
            if (hitT >= 0.0f && hitT <= *t) {
                *t = hitT;
                *normal = planeNormal;
                return true;
            }
        }
        return false;
    }

    default:
        return false;
    }

    if (result == HIT) {
        *t = bestT;
        return true;
    }

    return false;
}

// 0x00514db0 (PDB: 0x005168a0 / ?LineProbeGeometries@MultibodySystem@@QAE_NPAULineProbeReport@@ABVVector3@cdc@@1W4TestAxes@SweepAndPrune@@P6A_NPAVGeometry@@PAX@Z4@Z)
bool MultibodySystem::LineProbeGeometries(
    LineProbeReport* report,
    const cdc::Vector3& start,
    const cdc::Vector3& end,
    SweepAndPrune::TestAxes testAxes,
    bool (*cb)(Geometry*, void*),
    void* clientData)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    if (self->drawLineProbes) {
        MB_DrawLine(start, end, self->lineProbeColor);
    }

    LineProbeGeometryData data;
    data.fcn = cb;
    data.clientData = clientData;

    const cdc::Vector3 dir = end - start;
    float lambda = 1.0f;
    void* hit = self->sweepAndPrune->LineProbe(
        &lambda,
        &report->normal,
        start,
        dir,
        static_cast<unsigned int>(testAxes),
        LineProbeNarrowPhase,
        &data);
    if (hit == nullptr) {
        return false;
    }

    report->point = start + dir * lambda;
    report->geometry = static_cast<Geometry*>(hit);
    if (lambda > 0.0f && self->drawContacts && self->drawLineProbes) {
        DrawContactPoint(report->point, report->normal, true);
    }
    return true;
}

// 0x00514eb0 (PDB: 0x005169a0 / ?LineProbeMeshes@MultibodySystem@@QAE_NPAULineProbeReport@@ABVVector3@cdc@@1P6A_NPAVMesh@@PAX@Z3E@Z)
bool MultibodySystem::LineProbeMeshes(
    LineProbeReport* report,
    const cdc::Vector3& start,
    const cdc::Vector3& end,
    bool (*nearMeshFcn)(Mesh*, void*),
    void* userData,
    unsigned char flags)
{
    MultibodySystemImpl* self = static_cast<MultibodySystemImpl*>(this);
    if (self->drawLineProbes) {
        MB_DrawLine(start, end, self->lineProbeColor);
    }

    cdc::Vector3 bestEnd = end;
    bool anyHit = false;
    for (int i = 0; i < self->size.numMeshCollections; ++i) {
        const MeshCollection& coll = self->meshCollections[i];
        for (int j = 0; j < coll.numMeshes; ++j) {
            Mesh* mesh = coll.meshes[j];
            if (nearMeshFcn != nullptr && !nearMeshFcn(mesh, userData)) {
                continue;
            }

            IndexedFace* face = mesh->CollideSegment(&report->point, &report->normal, start, bestEnd, flags);
            if (face != nullptr) {
                bestEnd = report->point;
                report->meshCollection = i;
                report->mesh = mesh;
                report->face = face;
                anyHit = true;
            }
        }
    }

    if (anyHit && self->drawContacts && self->drawLineProbes) {
        DrawContactPoint(report->point, report->normal, true);
    }
    return anyHit;
}
