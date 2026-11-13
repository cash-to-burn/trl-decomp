#include "game/Multibody/src/Tunnel.h"

#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/CollisionManager.h"
#include "game/Multibody/src/Geometry.h"
#include "game/Multibody/src/MultibodyTypes.h"
#include "game/Multibody/src/SweepAndPrune.h"
#include "game/camera/cameracollide.h"

// 0x0051b570 (PDB: 0x0051d0a0 / ?RegulateNearGeometryCallback@@YA_NPAVGeometry@@PAX@Z)
bool RegulateNearGeometryCallback(Geometry* otherVoid, void* userData)
{
    RegulateTunnelData* data = static_cast<RegulateTunnelData*>(userData);
    GeometryImpl* geom = data->geom;
    GeometryImpl* other = static_cast<GeometryImpl*>(otherVoid);

    if ((other->flags & 0x40u) != 0 || (other->flags & 0x2u) == 0) {
        return false;
    }

    if (geom == other || geom->body == other->body) {
        return false;
    }

    if (DoesJointTrumpCollision(geom->body, other->body)) {
        return false;
    }

    bool (*nearFcn)(Mesh*, Geometry*, Geometry*) =
        data->system->responseArray[geom->category][other->category].nearFcn;
    if (nearFcn != nullptr) {
        return nearFcn(nullptr, geom, other);
    }

    return true;
}

// 0x0051b5e0 (PDB: 0x0051d110 / ?CheckTunnel@MultibodySystemImpl@@QAE_NPAUTunnelReport@@PAVGeometryImpl@@ABVVector3@cdc@@ABVQuat@5@M@Z)
bool MultibodySystemImpl::CheckTunnel(
    TunnelReport* report,
    GeometryImpl* geom,
    const cdc::Vector3& pos,
    const cdc::Quat& rot,
    float radius)
{
    const cdc::Vector3 oldPos = geom->x;
    const cdc::Vector3 newPos = pos + rot.Rotate(geom->dx);
    const cdc::Vector3 delta = newPos - oldPos;
    if (delta.IsZero3()) {
        return false;
    }

    const cdc::Vector3 dir = delta.UnitVec3();
    const cdc::Vector3 endPos = newPos + dir * radius;

    if ((geom->flags & 0x28u) != 0 && !DoesJointTrumpCollision(nullptr, geom->body)) {
        bool (*nearFcn)(Mesh*, Geometry*, Geometry*) = responseArray[geom->category][0].nearFcn;
        for (int i = 0; i < size.numMeshCollections; ++i) {
            const MeshCollection& coll = meshCollections[i];
            for (int j = 0; j < coll.numMeshes; ++j) {
                Mesh* mesh = coll.meshes[j];
                if (nearFcn != nullptr && !nearFcn(mesh, nullptr, geom)) {
                    continue;
                }

                cdc::Vector3 hitPoint;
                cdc::Vector3 hitNormal;
                if (mesh->CollideSegment(&hitPoint, &hitNormal, oldPos, endPos, 0xff) != nullptr) {
                    report->normal = hitNormal;
                    report->separation = cdc::Dot3(newPos - hitPoint, hitNormal);
                    return true;
                }
            }
        }
    }

    if ((geom->flags & 0x10u) != 0) {
        RegulateTunnelData data;
        data.system = this;
        data.geom = geom;

        LineProbeReport probeReport;
        probeReport.meshCollection = 0;
        probeReport.mesh = nullptr;
        probeReport.face = nullptr;
        if (LineProbeGeometries(
                &probeReport,
                oldPos,
                endPos,
                SweepAndPrune::TEST_XY,
                RegulateNearGeometryCallback,
                &data))
        {
            report->normal = probeReport.normal;
            report->separation = cdc::Dot3(newPos - probeReport.point, probeReport.normal);
            return true;
        }
    }

    return false;
}
