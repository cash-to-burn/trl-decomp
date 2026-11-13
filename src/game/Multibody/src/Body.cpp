#include "game/Multibody/src/Body.h"

#include <cmath>
#include <cstring>
#include "game/Multibody/src/Geometry.h"
#include "game/Multibody/src/MultibodyTypes.h"

// External diagnostics handler (0x00552e70)
extern void MB_Warning(const char* fmt, ...);

namespace {

inline float AbsMax3(float a, float b, float c)
{
    const float aa = a < 0.0f ? -a : a;
    const float ab = b < 0.0f ? -b : b;
    const float ac = c < 0.0f ? -c : c;
    float m = aa > ab ? aa : ab;
    return ac > m ? ac : m;
}

inline void ClampVectorMaxComponent(cdc::Vector3& vec, float maxSq, float limit)
{
    const float lenSq = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
    if (lenSq > maxSq) {
        const float maxComp = AbsMax3(vec.x, vec.y, vec.z);
        const float scale = limit / maxComp;
        vec.x *= scale;
        vec.y *= scale;
        vec.z *= scale;
    }
}

} // namespace

// 0x00503770 (PDB: 0x00505410 / ?InvertSymmetric3x3@@YA_NAAVMatrix@cdc@@ABV12@@Z)
bool InvertSymmetric3x3(cdc::Matrix& outInv, const cdc::Matrix& m)
{
    const float a00 = m.col0.x;
    const float a01 = m.col1.x;
    const float a02 = m.col2.x;
    const float a11 = m.col1.y;
    const float a12 = m.col2.y;
    const float a22 = m.col2.z;

    const float c00 = a11 * a22 - a12 * a12;
    const float c01 = a02 * a12 - a01 * a22;
    const float c02 = a01 * a12 - a02 * a11;

    const float det = a00 * c00 + a01 * c01 + a02 * c02;
    constexpr float kMinDet = 1.0e-5f;
    if (det <= kMinDet) {
        return false;
    }

    const float invDet = 1.0f / det;
    const float inv01 = c01 * invDet;
    const float inv02 = c02 * invDet;
    const float inv12 = (a01 * a02 - a00 * a12) * invDet;

    outInv.col0.x = c00 * invDet;
    outInv.col1.y = (a00 * a22 - a02 * a02) * invDet;
    outInv.col2.z = (a00 * a11 - a01 * a01) * invDet;

    outInv.col0.y = inv01;
    outInv.col1.x = inv01;
    outInv.col0.z = inv02;
    outInv.col2.x = inv02;
    outInv.col1.z = inv12;
    outInv.col2.y = inv12;
    return true;
}

// 0x00503880 (PDB: 0x00505520 / ?SetClientData@Body@@QAEXPAX@Z)
void Body::SetClientData(void* clientData)
{
    static_cast<BodyImpl*>(this)->clientData = clientData;
}

// 0x00503890 (PDB: 0x00505530 / ?GetClientData@Body@@QBEPAXXZ)
void* Body::GetClientData() const
{
    return static_cast<const BodyImpl*>(this)->clientData;
}

// 0x005038a0 (PDB: 0x00505540 / ?SetName@Body@@QAEXPBD@Z)
void Body::SetName(const char* name)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (name != nullptr) {
        std::strncpy(self->name, name, sizeof(self->name) - 1);
        self->name[sizeof(self->name) - 1] = '\0';
    } else {
        self->name[0] = '\0';
    }
}

// 0x005038d0 (PDB: 0x00505570 / ?GetName@Body@@QBEPBDXZ)
const char* Body::GetName() const
{
    return static_cast<const BodyImpl*>(this)->name;
}

// 0x005038e0 (PDB: 0x00505580 / ?SetMassProperties@Body@@QAEXABUMassProperties@@@Z)
void Body::SetMassProperties(const MassProperties& massProps)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (!massProps.CheckValidity()) {
        MB_Warning("WARNING: Body %s has bad mass properties\n", self->name);
        self->mass = 666.0f;
        self->I.col0 = cdc::Vector3::Make(66666.0f, 0.0f, 0.0f, 0.0f);
        self->I.col1 = cdc::Vector3::Make(0.0f, 66666.0f, 0.0f, 0.0f);
        self->I.col2 = cdc::Vector3::Make(0.0f, 0.0f, 66666.0f, 0.0f);
        self->I.col3 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 1.0f);
        self->flags |= FLAG_BAD_MASS_PROPS;
    } else {
        self->mass = massProps.mass;
        self->I = massProps.I;
    }
    self->invMass = 1.0f / self->mass;
    InvertSymmetric3x3(self->invI, self->I);
}

// 0x00503980 (PDB: 0x00505620 / ?GetMass@Body@@QBEMXZ)
float Body::GetMass() const
{
    return static_cast<const BodyImpl*>(this)->mass;
}

// 0x00503990 (PDB: 0x00505630 / ?GetInvMass@Body@@QBEMXZ)
float Body::GetInvMass() const
{
    return static_cast<const BodyImpl*>(this)->invMass;
}

// PDB: 0x00411a00 / ?GetI@Body@@QBEABVMatrix@cdc@@XZ
const cdc::Matrix& Body::GetI() const
{
    return static_cast<const BodyImpl*>(this)->I;
}

// PDB: 0x00612b40 / ?GetInvI@Body@@QBEABVMatrix@cdc@@XZ
const cdc::Matrix& Body::GetInvI() const
{
    return static_cast<const BodyImpl*>(this)->invI;
}

// 0x005039a0 (PDB: 0x00505640 / ?SetSleepProperties@Body@@QAEXABUSleepProperties@@@Z)
void Body::SetSleepProperties(const SleepProperties& sleepProps)
{
    static_cast<BodyImpl*>(this)->sleep = sleepProps;
}

// 0x005039c0 (PDB: 0x00505660 / ?SetLinearDamping@Body@@QAEXM@Z)
void Body::SetLinearDamping(float damping)
{
    static_cast<BodyImpl*>(this)->linearDamping = damping;
}

// 0x005039d0 (PDB: 0x00505670 / ?GetLinearDamping@Body@@QBEMXZ)
float Body::GetLinearDamping() const
{
    return static_cast<const BodyImpl*>(this)->linearDamping;
}

// 0x005039e0 (PDB: 0x00505680 / ?SetAngularDamping@Body@@QAEXM@Z)
void Body::SetAngularDamping(float damping)
{
    static_cast<BodyImpl*>(this)->angularDamping = damping;
}

// 0x005039f0 (PDB: 0x00505690 / ?GetAngularDamping@Body@@QBEMXZ)
float Body::GetAngularDamping() const
{
    return static_cast<const BodyImpl*>(this)->angularDamping;
}

// 0x00503a00 (PDB: 0x005056a0 / ?SetContactRank@Body@@QAEXH@Z)
void Body::SetContactRank(int rank)
{
    static_cast<BodyImpl*>(this)->rank = rank;
}

// 0x00503a10 (PDB: 0x005056b0 / ?GetFirstGeometry@Body@@QAEPAVGeometry@@XZ)
Geometry* Body::GetFirstGeometry()
{
    return static_cast<BodyImpl*>(this)->firstGeometry;
}

// 0x00503a20 (PDB: 0x005056c0 / ?GetNextGeometry@Body@@QAEPAVGeometry@@PAV2@@Z)
Geometry* Body::GetNextGeometry(Geometry* geom)
{
    return geom != nullptr ? static_cast<GeometryImpl*>(geom)->bnext : nullptr;
}

// 0x00503a40 (PDB: 0x005056e0 / ?SetPosition@Body@@QAEXABVVector3@cdc@@@Z)
void Body::SetPosition(const cdc::Vector3& pos)
{
    if (!pos.IsValid()) {
        return;
    }
    BodyImpl* self = static_cast<BodyImpl*>(this);
    self->x = pos;
    for (GeometryImpl* g = self->firstGeometry; g != nullptr; g = g->bnext) {
        g->UpdatePosition();
    }
}

// 0x00503aa0 (PDB: 0x00505740 / ?GetPosition@Body@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Body::GetPosition() const
{
    return static_cast<const BodyImpl*>(this)->x;
}

// 0x00503ad0 (PDB: 0x00505770 / ?GetQuaternion@Body@@QBE?AVQuat@cdc@@XZ)
cdc::Quat Body::GetQuaternion() const
{
    return static_cast<const BodyImpl*>(this)->q;
}

// 0x00503b00 (PDB: 0x005057a0 / ?SetLinearVelocity@Body@@QAEXABVVector3@cdc@@@Z)
void Body::SetLinearVelocity(const cdc::Vector3& vel)
{
    if (!vel.IsValid()) {
        return;
    }
    static_cast<BodyImpl*>(this)->v = vel;
}

// PDB: 0x0050b960 / ?GetLinearVelocity@Body@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Body::GetLinearVelocity() const
{
    return static_cast<const BodyImpl*>(this)->v;
}

// 0x00503b40 (PDB: 0x005057e0 / ?SetAngularVelocity@Body@@QAEXABVVector3@cdc@@@Z)
void Body::SetAngularVelocity(const cdc::Vector3& omega)
{
    if (!omega.IsValid()) {
        return;
    }
    static_cast<BodyImpl*>(this)->omega = omega;
}

// PDB: 0x0050b990 / ?GetAngularVelocity@Body@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Body::GetAngularVelocity() const
{
    return static_cast<const BodyImpl*>(this)->omega;
}

// 0x00503b80 (PDB: 0x00505820 / ?LocalToGlobalPoint@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
cdc::Vector3 Body::LocalToGlobalPoint(const cdc::Vector3& localPt) const
{
    const BodyImpl* self = static_cast<const BodyImpl*>(this);
    return self->x + self->q.Rotate(localPt);
}

// 0x00503bc0 (PDB: 0x00505860 / ?LocalToGlobalNormal@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
cdc::Vector3 Body::LocalToGlobalNormal(const cdc::Vector3& localNormal) const
{
    return static_cast<const BodyImpl*>(this)->q.Rotate(localNormal);
}

// 0x00503be0 (PDB: 0x00505880 / ?LocalPointVelocity@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
cdc::Vector3 Body::LocalPointVelocity(const cdc::Vector3& localPt) const
{
    const BodyImpl* self = static_cast<const BodyImpl*>(this);
    const cdc::Vector3 globalOffset = self->q.Rotate(localPt);
    cdc::Vector3 crossVel;
    cdc::Cross3(&crossVel, self->omega, globalOffset);
    return self->v + crossVel;
}

// 0x00503cb0 (PDB: 0x005058f0 / ?GlobalPointVelocity@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
cdc::Vector3 Body::GlobalPointVelocity(const cdc::Vector3& globalPt) const
{
    const BodyImpl* self = static_cast<const BodyImpl*>(this);
    cdc::Vector3 crossVel;
    cdc::Cross3(&crossVel, self->omega, globalPt - self->x);
    return self->v + crossVel;
}

// 0x00503d10 (PDB: 0x00505950 / ?WakeUp@Body@@QAEXXZ)
void Body::WakeUp()
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    self->sleepTimer = self->multibody->sleepTimerInit;
    self->flags &= ~FLAG_SLEEPING;
}

// 0x00503d40 (PDB: 0x00505980 / ?PutToSleep@Body@@QAEXXZ)
void Body::PutToSleep()
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    self->sleepTimer = 0.0f;
    self->flags |= FLAG_SLEEPING;
}

// 0x00503d60 (PDB: 0x005059a0 / ?IsSleeping@Body@@QBE_NXZ)
bool Body::IsSleeping() const
{
    return (static_cast<const BodyImpl*>(this)->flags & FLAG_SLEEPING) != 0;
}

// 0x00503d70 (PDB: 0x005059b0 / ?Enable@Body@@QAEXXZ)
void Body::Enable()
{
    static_cast<BodyImpl*>(this)->flags &= ~FLAG_DISABLED;
}

// 0x00503d80 (PDB: 0x005059c0 / ?Disable@Body@@QAEXXZ)
void Body::Disable()
{
    static_cast<BodyImpl*>(this)->flags |= FLAG_DISABLED;
}

// 0x00503d90 (PDB: 0x005059d0 / ?RemoveGeometry@BodyImpl@@QAEXPAVGeometryImpl@@@Z)
void BodyImpl::RemoveGeometry(GeometryImpl* geom)
{
    if (firstGeometry == geom) {
        firstGeometry = geom->bnext;
    } else {
        GeometryImpl* prevGeom = firstGeometry;
        while (prevGeom != nullptr && prevGeom->bnext != geom) {
            prevGeom = prevGeom->bnext;
        }
        if (prevGeom != nullptr) {
            prevGeom->bnext = geom->bnext;
        }
    }
    geom->bnext = nullptr;
    geom->body = nullptr;
}

// 0x00503de0 (PDB: 0x00505a20 / ?AddJoint@BodyImpl@@QAEXPAUJointNode@@@Z)
void BodyImpl::AddJoint(JointNode* node)
{
    node->next = firstJoint;
    firstJoint = node;
}

// 0x00503e00 (PDB: 0x00505a40 / ?RemoveJoint@BodyImpl@@QAEXPAUJointNode@@@Z)
void BodyImpl::RemoveJoint(JointNode* node)
{
    if (firstJoint == node) {
        firstJoint = node->next;
    } else {
        JointNode* prevNode = firstJoint;
        while (prevNode != nullptr && prevNode->next != node) {
            prevNode = prevNode->next;
        }
        if (prevNode != nullptr) {
            prevNode->next = node->next;
        }
    }
}

// 0x00503e40 (PDB: 0x00505a80 / ?SetQuaternion@Body@@QAEXABVQuat@cdc@@@Z)
void Body::SetQuaternion(const cdc::Quat& rot)
{
    if (!rot.IsValid()) {
        return;
    }
    BodyImpl* self = static_cast<BodyImpl*>(this);
    self->q = rot;
    for (GeometryImpl* g = self->firstGeometry; g != nullptr; g = g->bnext) {
        g->UpdatePosition();
    }
}

// 0x00503e90 (PDB: 0x00505ad0 / ?GlobalToLocalPoint@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
cdc::Vector3 Body::GlobalToLocalPoint(const cdc::Vector3& globalPt) const
{
    const BodyImpl* self = static_cast<const BodyImpl*>(this);
    return (~self->q).Rotate(globalPt - self->x);
}

// 0x00503ee0 (PDB: 0x00505b20 / ?GlobalToLocalNormal@Body@@QBE?AVVector3@cdc@@ABV23@@Z)
cdc::Vector3 Body::GlobalToLocalNormal(const cdc::Vector3& globalNormal) const
{
    const BodyImpl* self = static_cast<const BodyImpl*>(this);
    return (~self->q).Rotate(globalNormal);
}

// 0x00503f20 (PDB: 0x00505b60 / ?AddForce@Body@@QAEXABVVector3@cdc@@@Z)
void Body::AddForce(const cdc::Vector3& force)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (!force.IsValid() || IsSleeping()) {
        return;
    }
    self->Fc = self->Fc + force;
}

// 0x00503f60 (PDB: 0x00505ba0 / ?AddForce@Body@@QAEXABVVector3@cdc@@0@Z)
void Body::AddForce(const cdc::Vector3& force, const cdc::Vector3& position)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (!force.IsValid() || !position.IsValid() || IsSleeping()) {
        return;
    }
    self->Fc = self->Fc + force;
    cdc::Vector3 torque;
    cdc::Cross3(&torque, position - self->x, force);
    self->Tc = self->Tc + torque;
}

// 0x00503ff0 (PDB: 0x00505c30 / ?AddTorque@Body@@QAEXABVVector3@cdc@@@Z)
void Body::AddTorque(const cdc::Vector3& torque)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (!torque.IsValid() || IsSleeping()) {
        return;
    }
    self->Tc = self->Tc + torque;
}

// 0x00504030 (PDB: 0x00505c70 / ?Clear@BodyImpl@@QAEXPAVMultibodySystemImpl@@@Z)
void BodyImpl::Clear(MultibodySystemImpl* system)
{
    x = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    q = cdc::Quat::Identity();
    v = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    omega = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    F = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    T = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    Fc = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    Tc = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    multibody = system;
    std::strcpy(name, "NO NAME");
    clientData = nullptr;
    firstGeometry = nullptr;
    firstJoint = nullptr;
    firstContact = nullptr;
    sleep.autoSleep = true;
    sleep.angularThreshold = 0.01f;
    sleep.linearThreshold = 0.01f;
    linearDamping = 0.0f;
    angularDamping = 0.0f;
    flags = 0;
    sleepTimer = system->sleepTimerInit;
    rank = 0;
}

// 0x00504130 (PDB: 0x00505d70 / ?ComputeQuaternionUpdate@BodyImpl@@QBE?AVQuat@cdc@@M@Z)
cdc::Quat BodyImpl::ComputeQuaternionUpdate(float dt) const
{
    cdc::Quat spin;
    const float halfDt = 0.5f * dt;
    spin.x = omega.x * halfDt;
    spin.y = omega.y * halfDt;
    spin.z = omega.z * halfDt;
    spin.w = 0.0f;

    const cdc::Quat dq = cdc::QuatMul(spin, q);
    cdc::Quat nextQ;
    nextQ.x = q.x + dq.x;
    nextQ.y = q.y + dq.y;
    nextQ.z = q.z + dq.z;
    nextQ.w = q.w + dq.w;
    nextQ.Normalize();
    return nextQ;
}

// 0x00504200 (PDB: 0x00505e40 / ?UpdatePosition@BodyImpl@@QAEXM@Z)
void BodyImpl::UpdatePosition(float dt)
{
    if (IsSleeping()) {
        return;
    }

    cdc::Vector3 nextX = x + v * dt;
    cdc::Quat nextQ = ComputeQuaternionUpdate(dt);

    if ((flags & FLAG_TUNNEL_RISK) == 0) {
        for (GeometryImpl* g = firstGeometry; g != nullptr; g = g->bnext) {
            if (g->CheckTunnelRisk(nextX, nextQ)) {
                flags |= FLAG_TUNNEL_RISK;
                break;
            }
        }
    }

    if ((flags & FLAG_TUNNEL_RISK) != 0) {
        for (GeometryImpl* g = firstGeometry; g != nullptr; g = g->bnext) {
            const float halfRadius = 0.5f * g->radius;
            int iter = 0;
            TunnelReport report;
            while (iter < 4 &&
                   multibody->CheckTunnel(&report, g, nextX, nextQ, halfRadius) &&
                   report.separation < halfRadius) {
                nextX = nextX + report.normal * (halfRadius - report.separation);
                ++iter;
            }
            if (iter == 4) {
                nextX = x;
                nextQ = q;
                v = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
                omega = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
                break;
            }
        }
    }

    x = nextX;
    q = nextQ;

    for (GeometryImpl* g = firstGeometry; g != nullptr; g = g->bnext) {
        g->UpdatePosition();
    }
}

// 0x005043e0 (PDB: 0x00506020 / ?AttachGeometry@Body@@QAEXPAVGeometry@@@Z)
void Body::AttachGeometry(Geometry* geom)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    GeometryImpl* gi = static_cast<GeometryImpl*>(geom);
    gi->body = self;
    gi->bnext = self->firstGeometry;
    self->firstGeometry = gi;
    gi->UpdatePosition();
}

// 0x00504430 (PDB: 0x00506070 / ?AddImpulseForce@Body@@QAEXABVVector3@cdc@@@Z)
void Body::AddImpulseForce(const cdc::Vector3& impulse)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (!impulse.IsValid() || IsSleeping()) {
        return;
    }

    self->v = self->v + impulse * self->invMass;

    constexpr float kMaxSpeedSq = 360000.0f * 0.64f;
    ClampVectorMaxComponent(self->v, kMaxSpeedSq, 480.0f);
}

// 0x00504590 (PDB: 0x005061d0 / ?AddImpulseForce@Body@@QAEXABVVector3@cdc@@0@Z)
void Body::AddImpulseForce(const cdc::Vector3& impulse, const cdc::Vector3& position)
{
    BodyImpl* self = static_cast<BodyImpl*>(this);
    if (!impulse.IsValid() || !position.IsValid() || IsSleeping()) {
        return;
    }

    self->v = self->v + impulse * self->invMass;

    cdc::Vector3 torque;
    cdc::Cross3(&torque, position - self->x, impulse);
    const cdc::Vector3 localTorque = (~self->q).Rotate(torque);
    const cdc::Vector localOmegaVec = cdc::Mul3x3(self->invI, static_cast<const cdc::Vector&>(localTorque));
    const cdc::Vector3 localOmega = cdc::Vector3::Make(
        localOmegaVec.x, localOmegaVec.y, localOmegaVec.z, localOmegaVec.w);
    self->omega = self->omega + self->q.Rotate(localOmega);

    constexpr float kMaxSpeedSq = 360000.0f * 0.64f;
    ClampVectorMaxComponent(self->v, kMaxSpeedSq, 480.0f);

    if ((self->flags & FLAG_NO_ANGULAR_CLAMP) == 0) {
        constexpr float kMaxOmegaSq = 2.25f * 0.64f;
        ClampVectorMaxComponent(self->omega, kMaxOmegaSq, 1.2f);
    }
}
