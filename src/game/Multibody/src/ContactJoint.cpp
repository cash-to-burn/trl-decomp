#include "game/Multibody/src/ContactJoint.h"

#include <cmath>
#include "game/Multibody/src/Body.h"

namespace {

inline float ClampFloat(float val, float lo, float hi)
{
    if (val < lo) {
        return lo;
    }
    return val > hi ? hi : val;
}

inline cdc::Vector3 PointVelocity(const BodyImpl* body, const cdc::Vector3& worldPt)
{
    const cdc::Vector3 r = worldPt - body->x;
    cdc::Vector3 angVel;
    cdc::Cross3(&angVel, body->omega, r);
    return body->v + angVel;
}

inline void WriteLinearAngularRow(
    ContactJacobianRow& row,
    const BodyImpl* b1,
    bool hasBothBodies,
    const cdc::Vector3& dir,
    const cdc::Vector3& r1,
    const cdc::Vector3& r2,
    float* outBody1Vel)
{
    if (b1 != nullptr) {
        if (hasBothBodies) {
            row.v1 = dir * -1.0f;
            cdc::Vector3 r1xD;
            cdc::Cross3(&r1xD, r1, dir);
            row.w1 = r1xD * -1.0f;
        } else {
            cdc::Vector3 w1xR1;
            cdc::Cross3(&w1xR1, b1->omega, r1);
            *outBody1Vel = cdc::Dot3(dir, b1->v + w1xR1);
        }
    }

    row.v2 = dir;
    cdc::Vector3 r2xD;
    cdc::Cross3(&r2xD, r2, dir);
    row.w2 = r2xD;
}

} // namespace

// 0x0050cd80 (PDB: 0x0050e870 / ?ComputeBasis@@YAXPAVVector3@cdc@@0ABV12@@Z)
void ComputeBasis(cdc::Vector3* u, cdc::Vector3* v, const cdc::Vector3& n)
{
    if (std::fabs(n.z) > 0.5f) {
        *u = cdc::Vector3::Make(n.z, 0.0f, -n.x, 0.0f);
    } else {
        *u = cdc::Vector3::Make(n.y, -n.x, 0.0f, 0.0f);
    }
    u->Normalize3();
    cdc::Cross3(v, n, *u);
}

// 0x00507a50 (PDB: 0x00509520 / ?Set@ContactJoint@@QAEXPAVBodyImpl@@0PBUContact@@HABVVector3@cdc@@M@Z)
void ContactJoint::Set(
    BodyImpl* b1,
    BodyImpl* b2,
    const Contact* contacts,
    int numContacts,
    const cdc::Vector3& contactNormal,
    float frictionForceScale)
{
    flags = static_cast<uint8_t>(numContacts);
    body1 = b1;
    body2 = b2;
    node1.joint = this;
    node1.otherBody = b2;

    if (b1 != nullptr && b1->rank == b2->rank) {
        node1.next = b1->firstContact;
        b1->firstContact = &node1;
        flags |= FLAG_BOTH_BODIES;
    } else {
        node1.next = nullptr;
    }

    node2.joint = this;
    node2.otherBody = body1;
    node2.next = body2->firstContact;
    body2->firstContact = &node2;

    normal = contactNormal;

    cdc::Vector3 u;
    cdc::Vector3 v;
    ComputeBasis(&u, &v, normal);

    restitution = contacts[0].restitution;
    gamma = contacts[0].softness;
    friction = 0.0f;
    for (int i = 0; i < MAX_CONSTRAINTS; ++i) {
        lambda[i] = 0.0f;
    }

    cdc::Vector3 posSum = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    float lambdaSum = 0.0f;

    for (int i = 0; i < numContacts; ++i) {
        geometry1[i] = reinterpret_cast<GeometryImpl*>(contacts[i].geometry1);
        geometry2[i] = reinterpret_cast<GeometryImpl*>(contacts[i].geometry2);

        p[i] = contacts[i].cp.position;
        p[i].w = contacts[i].cp.separation;
        posSum += p[i];

        vrel[i] = cdc::Dot3(normal, contacts[i].velocity);
        const float normalForce = cdc::Dot3(normal, contacts[i].force);
        lambda[i] = normalForce > 0.0f ? normalForce : 0.0f;
        lambdaSum += lambda[i];
        friction += contacts[i].friction;
    }

    if (lambdaSum > 0.0f) {
        const float scaled = lambdaSum * frictionForceScale;
        const float maxMu = 4.0f * friction;
        friction = scaled < maxMu ? scaled : maxMu;
    } else {
        friction = 0.0f;
    }

    if (friction == 0.0f) {
        numRows = static_cast<uint8_t>(numContacts);
        return;
    }

    const cdc::Vector3 avgPos = posSum * (1.0f / static_cast<float>(numContacts));
    cdc::Vector3 relVel = PointVelocity(body2, avgPos);
    if (body1 != nullptr) {
        relVel = relVel - PointVelocity(body1, avgPos);
    }

    const cdc::Vector3 tangVel = relVel - normal * cdc::Dot3(normal, relVel);
    const float tangSpeed = tangVel.Len3();

    constexpr float kKineticThreshold = 2.0f;
    if (tangSpeed > kKineticThreshold) {
        flags |= FLAG_KINETIC;
        const cdc::Vector3 frictionForce = tangVel * (friction / tangSpeed);
        body2->Fc = body2->Fc - frictionForce;

        cdc::Vector3 torque2;
        cdc::Cross3(&torque2, avgPos - body2->x, frictionForce);
        body2->Tc = body2->Tc - torque2;

        if ((flags & FLAG_BOTH_BODIES) != 0) {
            body1->Fc += frictionForce;
            cdc::Vector3 torque1;
            cdc::Cross3(&torque1, avgPos - body1->x, frictionForce);
            body1->Tc += torque1;
        }

        numRows = (numContacts == 1) ? 1 : static_cast<uint8_t>(numContacts + 1);
        return;
    }

    lambda[numContacts] = ClampFloat(cdc::Dot3(contacts[0].force, u), -friction, friction);
    lambda[numContacts + 1] = ClampFloat(cdc::Dot3(contacts[0].force, v), -friction, friction);
    numRows = (numContacts == 1) ? 3 : static_cast<uint8_t>(numContacts + 3);
}

// 0x00508030 (PDB: 0x00509b00 / ?ComputeJacobian@ContactJoint@@QBEXQAVVector3@cdc@@QAM111HPBU1@@Z)
void ContactJoint::ComputeJacobian(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const ContactStepParams* params) const
{
    const int numContacts = static_cast<int>(flags & 0x0f);
    const bool hasBothBodies = (flags & FLAG_BOTH_BODIES) != 0;

    cdc::Vector3 u;
    cdc::Vector3 v;
    ComputeBasis(&u, &v, normal);

    const float gravityMag = body2->multibody->gravityMagnitude;
    float maxNormalForce;
    if (body1 != nullptr) {
        if (hasBothBodies) {
            constexpr uint32_t kHugeBits = 0x7dccccccu;
            union { uint32_t u32; float f32; } huge = { kHugeBits };
            maxNormalForce = huge.f32;
        } else {
            maxNormalForce = gravityMag * body2->mass * 50.0f;
        }
    } else {
        maxNormalForce = FLT_MAX;
    }

    const float maxFrictionForce = maxNormalForce < friction ? maxNormalForce : friction;

    cdc::Vector3 posSum = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    int row = rowIndex;

    for (int i = 0; i < numContacts; ++i, ++row) {
        vel[row] = 0.0f;

        const cdc::Vector3 r1 = (body1 != nullptr) ? (p[i] - body1->x) : cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        const cdc::Vector3 r2 = p[i] - body2->x;
        WriteLinearAngularRow(J[row], body1, hasBothBodies, normal, r1, r2, &vel[row]);

        const float slopError = p[i].w + params->contactSlop;
        err[row] = slopError < 0.0f ? slopError : 0.0f;

        if (restitution > 0.0f && -params->bounceThreshold > vrel[i]) {
            vel[row] -= vrel[i] * restitution;
        }

        lo[row] = 0.0f;
        hi[row] = maxNormalForce;
        posSum += p[i];
    }

    if (friction == 0.0f) {
        return;
    }

    const float invNumContacts = 1.0f / static_cast<float>(numContacts);
    const cdc::Vector3 avgPos = posSum * invNumContacts;

    if ((flags & FLAG_KINETIC) == 0) {
        vel[row] = 0.0f;
        vel[row + 1] = 0.0f;

        const cdc::Vector3 r1 = (body1 != nullptr) ? (avgPos - body1->x) : cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        const cdc::Vector3 r2 = avgPos - body2->x;

        WriteLinearAngularRow(J[row], body1, hasBothBodies, u, r1, r2, &vel[row]);
        WriteLinearAngularRow(J[row + 1], body1, hasBothBodies, v, r1, r2, &vel[row + 1]);

        err[row] = 0.0f;
        err[row + 1] = 0.0f;
        lo[row] = -maxFrictionForce;
        hi[row] = maxFrictionForce;
        lo[row + 1] = -maxFrictionForce;
        hi[row + 1] = maxFrictionForce;
        row += 2;
    }

    if (numContacts > 1) {
        vel[row] = 0.0f;
        J[row].v1 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        J[row].v2 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

        if (body1 != nullptr) {
            if (hasBothBodies) {
                J[row].w1 = normal * -1.0f;
            } else {
                vel[row] = cdc::Dot3(normal, body1->omega);
            }
        }
        J[row].w2 = normal;

        float distSum = 0.0f;
        for (int i = 0; i < numContacts; ++i) {
            distSum += (p[i] - avgPos).Len3();
        }

        const float twistLimit = distSum * invNumContacts * maxFrictionForce * 0.1f;
        err[row] = 0.0f;
        lo[row] = -twistLimit;
        hi[row] = twistLimit;
    }
}
