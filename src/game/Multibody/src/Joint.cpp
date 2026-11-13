#include "game/Multibody/src/Joint.h"

#include <cfloat>
#include <cmath>
#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/ContactJoint.h"
#include "game/Multibody/src/MultibodyTypes.h"

extern void MULTIBODY_PrintWarning(const char* fmt, ...); // 0x00552e70 (PDB: 0x00554e70)

namespace {

constexpr float kTwoPi = 6.283185307f;
constexpr float kWrapLimitThreshold = 2.984513f;
constexpr float kAngularThickness = 0.035f;
constexpr float kShoulderConeThickness = 0.01f;
constexpr float kShoulderConeMaxAngle = 1.4835298f;
constexpr float kConicalMaxAngle = 3.054326f;
constexpr float kPulleyEps = 0.001f;

inline cdc::Vector3 ZeroVec()
{
    return cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
}

inline cdc::Vector3 NegVec(const cdc::Vector3& v)
{
    return cdc::Vector3::Make(-v.x, -v.y, -v.z);
}

inline float VecLength3(const cdc::Vector3& v)
{
    return std::sqrt(cdc::Dot3(v, v));
}

inline cdc::Vector3 ToLocalDir(const BodyImpl* body, const cdc::Vector3& worldDir)
{
    const cdc::Quat invQ = ~body->q;
    return invQ.Rotate(worldDir);
}

inline cdc::Vector3 ToLocalPos(const BodyImpl* body, const cdc::Vector3& worldPos)
{
    return ToLocalDir(body, worldPos - body->x);
}

inline void StoreWorldFrame(
    const BodyImpl* body,
    const cdc::Vector3& anchor,
    const cdc::Vector3& bx,
    const cdc::Vector3& by,
    const cdc::Vector3& bz,
    cdc::Vector3& outAnchor,
    cdc::Vector3& outBx,
    cdc::Vector3& outBy,
    cdc::Vector3& outBz)
{
    if (body != nullptr) {
        outAnchor = ToLocalPos(body, anchor);
        outBx = ToLocalDir(body, bx);
        outBy = ToLocalDir(body, by);
        outBz = ToLocalDir(body, bz);
    } else {
        outAnchor = anchor;
        outBx = bx;
        outBy = by;
        outBz = bz;
    }
}

inline void GetWorldAnchors(
    const JointImpl& j,
    cdc::Vector3& r1,
    cdc::Vector3& r2,
    cdc::Vector3& p1,
    cdc::Vector3& p2)
{
    if (j.body1 != nullptr) {
        r1 = j.body1->q.Rotate(j.anchor1);
        p1 = j.body1->x + r1;
    } else {
        r1 = ZeroVec();
        p1 = j.anchor1;
    }

    r2 = j.body2->q.Rotate(j.anchor2);
    p2 = j.body2->x + r2;
}

inline void GetWorldAxes12(
    const BodyImpl* b1,
    const BodyImpl* b2,
    const cdc::Vector3& a1,
    const cdc::Vector3& a2,
    cdc::Vector3& out1,
    cdc::Vector3& out2)
{
    out1 = (b1 != nullptr) ? b1->q.Rotate(a1) : a1;
    out2 = b2->q.Rotate(a2);
}

inline cdc::Vector3 ComputePointWorldPos(const BodyImpl* body, const cdc::Vector3& anchor)
{
    return (body != nullptr) ? (body->x + body->q.Rotate(anchor)) : anchor;
}

inline cdc::Vector3 ComputePointWorldVel(const BodyImpl* body, const cdc::Vector3& worldOffset)
{
    if (body == nullptr) {
        return ZeroVec();
    }
    cdc::Vector3 wCrossR;
    cdc::Cross3(&wCrossR, body->omega, worldOffset);
    return body->v + wCrossR;
}

inline float SafeNormalize(cdc::Vector3& v)
{
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    const float len = std::sqrt(lenSq);
    if (v.IsZero3()) {
        v = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
    } else if (len > 0.0f) {
        v = v * (1.0f / len);
    }
    return len;
}

inline void WriteLinearRow(
    ContactJacobianRow& row,
    const cdc::Vector3& axis,
    const BodyImpl* b1,
    const cdc::Vector3& r1,
    const cdc::Vector3& r2)
{
    if (b1 != nullptr) {
        row.v1 = NegVec(axis);
        cdc::Cross3(&row.w1, axis, r1);
    }
    row.v2 = axis;
    cdc::Cross3(&row.w2, r2, axis);
}

inline void WriteAngularRow(
    ContactJacobianRow& row,
    const cdc::Vector3& axis,
    const BodyImpl* b1)
{
    if (b1 != nullptr) {
        row.w1 = NegVec(axis);
    }
    row.w2 = axis;
}

inline void WriteTranslationRows3(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const BodyImpl* b1,
    const cdc::Vector3& r1,
    const cdc::Vector3& r2,
    const cdc::Vector3& p1,
    const cdc::Vector3& p2)
{
    if (b1 != nullptr) {
        J[rowIndex + 0].v1 = cdc::Vector3::Make(-1.0f, 0.0f, 0.0f);
        J[rowIndex + 0].w1 = cdc::Vector3::Make(0.0f, -r1.z, r1.y);
        J[rowIndex + 1].v1 = cdc::Vector3::Make(0.0f, -1.0f, 0.0f);
        J[rowIndex + 1].w1 = cdc::Vector3::Make(r1.z, 0.0f, -r1.x);
        J[rowIndex + 2].v1 = cdc::Vector3::Make(0.0f, 0.0f, -1.0f);
        J[rowIndex + 2].w1 = cdc::Vector3::Make(-r1.y, r1.x, 0.0f);
    }

    J[rowIndex + 0].v2 = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
    J[rowIndex + 0].w2 = cdc::Vector3::Make(0.0f, r2.z, -r2.y);
    J[rowIndex + 1].v2 = cdc::Vector3::Make(0.0f, 1.0f, 0.0f);
    J[rowIndex + 1].w2 = cdc::Vector3::Make(-r2.z, 0.0f, r2.x);
    J[rowIndex + 2].v2 = cdc::Vector3::Make(0.0f, 0.0f, 1.0f);
    J[rowIndex + 2].w2 = cdc::Vector3::Make(r2.y, -r2.x, 0.0f);

    const cdc::Vector3 diff = p2 - p1;
    err[rowIndex + 0] = diff.x;
    err[rowIndex + 1] = diff.y;
    err[rowIndex + 2] = diff.z;

    for (int k = 0; k < 3; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }
}

inline float ClampMinBound(float bound, float delta)
{
    const float shifted = bound - delta;
    return (shifted < bound) ? shifted : bound;
}

inline float WrapAngleToLimits(float angle, float low, float high)
{
    const float diffLow = angle - low;
    const float diffHigh = high - angle;
    if (diffLow < -kWrapLimitThreshold && diffHigh > kWrapLimitThreshold) {
        return angle + kTwoPi;
    }
    if (diffLow > kWrapLimitThreshold && diffHigh < -kWrapLimitThreshold) {
        return angle - kTwoPi;
    }
    return angle;
}

inline void FireLimitCallback(
    const JointImpl* joint,
    float speed,
    int limitNumber,
    LimitReport::HitType hitType)
{
    if ((joint->flags & JointImpl::HAS_LIMIT_LISTENER) == 0) {
        return;
    }
    MultibodySystemImpl* sys = joint->body2->multibody;
    if (sys == nullptr || sys->limitFcn == nullptr) {
        return;
    }
    LimitReport report;
    report.joint = joint;
    report.speed = speed;
    report.limitNumber = limitNumber;
    report.hitType = hitType;
    sys->limitFcn(report);
}

inline void UpdateAxisLimitState(
    JointImpl* joint,
    int limitIndex,
    float value,
    float speed,
    int lambdaIndex,
    bool countRowOnActive,
    bool keepMotorRow = true)
{
    JointLimit& lim = joint->limit[limitIndex];
    lim.value = value;

    const unsigned int motorBit = static_cast<unsigned int>(JointImpl::MOTOR1) << limitIndex;
    const bool hasMotor = (joint->flags & motorBit) != 0;

    if (value <= lim.low) {
        if (lim.status == JointLimit::NOT_AT_LIMIT) {
            joint->lambda[lambdaIndex] = 0.0f;
            FireLimitCallback(joint, speed, limitIndex, LimitReport::HIT_LOWER_LIMIT);
        }
        lim.status = JointLimit::AT_LOW_LIMIT;
        if (countRowOnActive && (!keepMotorRow || !hasMotor)) {
            ++joint->numRows;
        }
    } else if (value >= lim.high) {
        if (lim.status == JointLimit::NOT_AT_LIMIT) {
            joint->lambda[lambdaIndex] = 0.0f;
            FireLimitCallback(joint, speed, limitIndex, LimitReport::HIT_UPPER_LIMIT);
        }
        lim.status = JointLimit::AT_HIGH_LIMIT;
        if (countRowOnActive && (!keepMotorRow || !hasMotor)) {
            ++joint->numRows;
        }
    } else {
        lim.status = JointLimit::NOT_AT_LIMIT;
    }
}

inline bool HasDynamicMotorRow(uint8_t type)
{
    return type == Joint::RevoluteJoint ||
           type == Joint::PrismaticJoint ||
           type == Joint::PlanarJoint ||
           type == Joint::UniversalJoint ||
           type == Joint::SwingJoint ||
           type == Joint::CartesianJoint;
}

inline void ApplyMotorFightForce(
    JointImpl* joint,
    const cdc::Vector3& axis,
    bool isLinear,
    const cdc::Vector3& r1,
    const cdc::Vector3& r2,
    float signedForce)
{
    const cdc::Vector3 vec = axis * signedForce;
    if (isLinear) {
        if (joint->body1 != nullptr) {
            joint->body1->F = joint->body1->F + vec;
            cdc::Vector3 tau1;
            cdc::Cross3(&tau1, r1, vec);
            joint->body1->T = joint->body1->T + tau1;
        }
        joint->body2->F = joint->body2->F - vec;
        cdc::Vector3 tau2;
        cdc::Cross3(&tau2, r2, vec);
        joint->body2->T = joint->body2->T - tau2;
    } else {
        if (joint->body1 != nullptr) {
            joint->body1->T = joint->body1->T + vec;
        }
        joint->body2->T = joint->body2->T - vec;
    }
}

inline int ComputeAxisHelper(
    JointImpl* joint,
    ContactJacobianRow* row,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    const JointAxisParams& params,
    bool isLinear)
{
    const int idx = params.index;
    const unsigned int limitBit = static_cast<unsigned int>(JointImpl::LIMIT1) << idx;
    const unsigned int motorBit = static_cast<unsigned int>(JointImpl::MOTOR1) << idx;
    const bool hasLimit = (joint->flags & limitBit) != 0;
    const bool hasMotor = (joint->flags & motorBit) != 0;

    if (!hasLimit && !hasMotor) {
        return 0;
    }

    if (isLinear) {
        WriteLinearRow(*row, params.axis, joint->body1, params.r1, params.r2);
    } else {
        WriteAngularRow(*row, params.axis, joint->body1);
    }

    const JointLimit& lim = joint->limit[idx];
    const JointMotor& mot = joint->motor[idx];
    const float thickness = isLinear ? params.linearThickness : params.angularThickness;

    if (hasLimit && hasMotor) {
        float loBound = lim.low;
        float hiBound = lim.high;
        if ((joint->flags & JointImpl::SMOOTH_LIMITS) != 0) {
            const float step = mot.velocity * params.invBeta;
            loBound = ClampMinBound(loBound, step);
            hiBound = ClampMinBound(hiBound, step);
        }

        if (lim.value <= loBound) {
            *err = lim.value - lim.low + thickness;
            *vel = 0.0f;
            *lo = -mot.maxForce;
            *hi = FLT_MAX;
            if (mot.velocity > 0.0f) {
                ApplyMotorFightForce(joint, params.axis, isLinear, params.r1, params.r2, -mot.maxForce);
                *lo = 0.0f;
            }
            return 1;
        }

        if (lim.value >= hiBound) {
            *err = lim.value - lim.high - thickness;
            *vel = 0.0f;
            *lo = -FLT_MAX;
            *hi = mot.maxForce;
            if (mot.velocity < 0.0f) {
                ApplyMotorFightForce(joint, params.axis, isLinear, params.r1, params.r2, mot.maxForce);
                *hi = 0.0f;
            }
            return 1;
        }
    } else if (hasLimit) {
        if (lim.status == JointLimit::AT_LOW_LIMIT) {
            *err = lim.value - lim.low + thickness;
            *vel = 0.0f;
            *lo = 0.0f;
            *hi = FLT_MAX;
            return 1;
        }
        if (lim.status == JointLimit::AT_HIGH_LIMIT) {
            *err = lim.value - lim.high - thickness;
            *vel = 0.0f;
            *lo = -FLT_MAX;
            *hi = 0.0f;
            return 1;
        }
        return 0;
    }

    *err = 0.0f;
    *vel = mot.velocity;
    *lo = -mot.maxForce;
    *hi = mot.maxForce;
    return 1;
}

inline int ResolveAxisRow(const JointImpl* j, int axisIndex, int baseRow)
{
    int row = baseRow;
    for (int i = 0; i < axisIndex; ++i) {
        const unsigned int priorLimitBit = static_cast<unsigned int>(JointImpl::LIMIT1) << i;
        const unsigned int priorMotorBit = static_cast<unsigned int>(JointImpl::MOTOR1) << i;
        const bool activeLimit = (j->flags & priorLimitBit) != 0 &&
                                 j->limit[i].status != JointLimit::NOT_AT_LIMIT;
        if (activeLimit || (j->flags & priorMotorBit) != 0) {
            ++row;
        }
    }
    return row;
}

inline JointImpl* AsImpl(Joint* joint)
{
    return reinterpret_cast<JointImpl*>(joint);
}

inline const JointImpl* AsImpl(const Joint* joint)
{
    return reinterpret_cast<const JointImpl*>(joint);
}

} // namespace

// 0x005099b0 (PDB: 0x0050b4a0 / ?IsLimitNumberValid@JointImpl@@QBE_NH@Z)
bool JointImpl::IsLimitNumberValid(int n) const
{
    switch (type) {
    case Joint::RevoluteJoint:
    case Joint::PrismaticJoint:
    case Joint::ConicalLimit:
        return n == 0;
    case Joint::UniversalJoint:
        return n == 0 || n == 1;
    case Joint::PlanarJoint:
    case Joint::SwingJoint:
    case Joint::CartesianJoint:
        return n >= 0 && n <= 2;
    default:
        return false;
    }
}

// 0x00509a10 (PDB: 0x0050b500 / ?IsMotorNumberValid@JointImpl@@QBE_NH@Z)
bool JointImpl::IsMotorNumberValid(int n) const
{
    switch (type) {
    case Joint::RevoluteJoint:
    case Joint::PrismaticJoint:
    case Joint::RopeJoint:
    case Joint::AngularMotor:
        return n == 0;
    case Joint::UniversalJoint:
        return n == 0 || n == 1;
    case Joint::PlanarJoint:
    case Joint::SwingJoint:
    case Joint::CartesianJoint:
    case Joint::PointMotor:
        return n >= 0 && n <= 2;
    default:
        return false;
    }
}

// 0x0050b750 (PDB: 0x0050d220 / ?Connect@JointImpl@@QAEXPAVBodyImpl@@0@Z)
void JointImpl::Connect(BodyImpl* b1, BodyImpl* b2)
{
    body1 = b1;
    body2 = b2;
    node2.joint = this;
    node2.otherBody = b1;
    node2.next = b2->firstJoint;
    b2->firstJoint = &node2;

    if (b1 != nullptr) {
        node1.joint = this;
        node1.otherBody = b2;
        node1.next = b1->firstJoint;
        b1->firstJoint = &node1;
    }
}

// 0x00509a70 (PDB: 0x0050b540 / ?UpdateWheel2@Joint@@QAEXABVVector3@cdc@@00MM0MMM@Z)
void Joint::UpdateWheel2(
    const cdc::Vector3& anchor,
    const cdc::Vector3& bx,
    const cdc::Vector3& by,
    float maxForce1,
    float maxForce2,
    const cdc::Vector3& bz,
    float radius,
    float coord1,
    float coord2)
{
    JointImpl* j = AsImpl(this);
    j->anchor1 = anchor;
    j->anchor2 = anchor;
    j->bx2 = bx;
    j->by2 = by;
    j->motor[0].maxForce = maxForce1;
    j->motor[1].maxForce = maxForce2;
    j->bz2 = bz;
    j->length = radius;
    j->linkCoord1 = coord1;
    j->linkCoord2 = coord2;
}

// 0x00509b40 (PDB: 0x0050b610 / ?UpdateRope@Joint@@QAEXABVVector3@cdc@@@Z)
void Joint::UpdateRope(const cdc::Vector3& anchor)
{
    JointImpl* j = AsImpl(this);
    j->anchor1 = (j->body1 != nullptr) ? ToLocalPos(j->body1, anchor) : anchor;
}

// 0x00509b70 (PDB: 0x0050b640 / ?UpdateUniversal@Joint@@QAEXABVVector3@cdc@@000@Z)
void Joint::UpdateUniversal(
    const cdc::Vector3& y1,
    const cdc::Vector3& z1,
    const cdc::Vector3& y2,
    const cdc::Vector3& z2)
{
    JointImpl* j = AsImpl(this);
    j->by1 = y1;
    j->bz1 = z1;
    cdc::Cross3(&j->bx1, j->by1, j->bz1);
    j->by2 = y2;
    j->bz2 = z2;
    cdc::Cross3(&j->bx2, j->by2, j->bz2);
}

// 0x00509ca0 (PDB: 0x0050b770 / ?SetSoftness@Joint@@QAEXQAM@Z)
void Joint::SetSoftness(float* softness)
{
    JointImpl* j = AsImpl(this);
    for (int i = 0; i < 6; ++i) {
        j->gamma[i] = softness[i];
    }
}

// 0x00509cf0 (PDB: 0x0050b7c0 / ?SetHasLimitListener@Joint@@QAEX_N@Z)
void Joint::SetHasLimitListener(bool hasListener)
{
    JointImpl* j = AsImpl(this);
    if (hasListener) {
        j->flags |= JointImpl::HAS_LIMIT_LISTENER;
    } else {
        j->flags &= ~JointImpl::HAS_LIMIT_LISTENER;
    }
}

// 0x00509ce0 (PDB: 0x0050b7b0 / ?SetStepIterations@Joint@@QAEXE@Z)
void Joint::SetStepIterations(uint8_t iterations)
{
    AsImpl(this)->stepIterations = iterations;
}

// 0x00509d20 (PDB: 0x0050b7f0 / ?SetSmoothLimits@Joint@@QAEX_N@Z)
void Joint::SetSmoothLimits(bool smooth)
{
    JointImpl* j = AsImpl(this);
    if (smooth) {
        j->flags |= JointImpl::SMOOTH_LIMITS;
    } else {
        j->flags &= ~JointImpl::SMOOTH_LIMITS;
    }
}

// 0x00509d50 (PDB: 0x0050b820 / ?GetType@Joint@@QBE?AW4Type@1@XZ)
Joint::Type Joint::GetType() const
{
    return static_cast<Joint::Type>(AsImpl(this)->type);
}

// 0x00509d60 (PDB: 0x0050b830 / ?GetBody1@Joint@@QBEPAVBody@@XZ)
Body* Joint::GetBody1() const
{
    return reinterpret_cast<Body*>(AsImpl(this)->body1);
}

// 0x00509d70 (PDB: 0x0050b840 / ?GetBody2@Joint@@QBEPAVBody@@XZ)
Body* Joint::GetBody2() const
{
    return reinterpret_cast<Body*>(AsImpl(this)->body2);
}

// 0x00509d80 (PDB: 0x0050b850 / ?GetAnchor1@Joint@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Joint::GetAnchor1() const
{
    const JointImpl* j = AsImpl(this);
    return ComputePointWorldPos(j->body1, j->anchor1);
}

// 0x00509df0 (PDB: 0x0050b8c0 / ?GetAnchor2@Joint@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Joint::GetAnchor2() const
{
    const JointImpl* j = AsImpl(this);
    return ComputePointWorldPos(j->body2, j->anchor2);
}

// PDB: 0x005e5490 / ?GetPulleyGroundAnchor1@Joint@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Joint::GetPulleyGroundAnchor1() const
{
    return AsImpl(this)->bx1;
}

// PDB: 0x00479550 / ?GetPulleyGroundAnchor2@Joint@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Joint::GetPulleyGroundAnchor2() const
{
    return AsImpl(this)->bx2;
}

// ?GetPulleyLength1@Joint@@QBEMXZ
float Joint::GetPulleyLength1() const
{
    const JointImpl* j = AsImpl(this);
    const cdc::Vector3 p1 = ComputePointWorldPos(j->body1, j->anchor1);
    return VecLength3(p1 - j->bx1);
}

// ?GetPulleyLength2@Joint@@QBEMXZ
float Joint::GetPulleyLength2() const
{
    const JointImpl* j = AsImpl(this);
    const cdc::Vector3 p2 = ComputePointWorldPos(j->body2, j->anchor2);
    return VecLength3(p2 - j->bx2);
}

// 0x00509e30 (PDB: 0x0050b900 / ?SetClientData@Joint@@QAEXPAX@Z)
void Joint::SetClientData(void* data)
{
    AsImpl(this)->clientData = data;
}

// 0x00509e40 (PDB: 0x0050b910 / ?GetClientData@Joint@@QBEPAXXZ)
void* Joint::GetClientData() const
{
    return AsImpl(this)->clientData;
}

// 0x00509e50 (PDB: 0x0050b920 / ?SetDestructionCallback@Joint@@QAEXP6AXPAXPAV1@@Z@Z)
void Joint::SetDestructionCallback(void (*cb)(void*, Joint*))
{
    AsImpl(this)->clientDestruct = cb;
}

// 0x00509e60 (PDB: 0x0050b930 / ?SetCollideConnected@Joint@@QAEX_N@Z)
void Joint::SetCollideConnected(bool collide)
{
    JointImpl* j = AsImpl(this);
    if (collide) {
        j->flags |= JointImpl::COLLIDE;
    } else {
        j->flags &= ~JointImpl::COLLIDE;
    }
}

// ?GetCollideConnected@Joint@@QBE_NXZ
bool Joint::GetCollideConnected() const
{
    return (AsImpl(this)->flags & JointImpl::COLLIDE) != 0;
}

// 0x00509e90 (PDB: 0x0050b960 / ?GetReactionForce@Joint@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Joint::GetReactionForce() const
{
    return AsImpl(this)->force;
}

// 0x00509ec0 (PDB: 0x0050b990 / ?GetReactionTorque@Joint@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Joint::GetReactionTorque() const
{
    return AsImpl(this)->torque;
}

// 0x00509ef0 (PDB: 0x0050b9c0 / ?GetSplineMotorForce@Joint@@QBEMXZ)
float Joint::GetSplineMotorForce() const
{
    return AsImpl(this)->lambda[0];
}

// 0x00509f00 (PDB: 0x0050b9d0 / ?GetSphericalAngularVelocity@Joint@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Joint::GetSphericalAngularVelocity() const
{
    const JointImpl* j = AsImpl(this);
    return (j->body1 != nullptr) ? (j->body2->omega - j->body1->omega) : j->body2->omega;
}

// 0x00509f70 (PDB: 0x0050ba40 / ?GetPulleySpeed1@Joint@@QBEMXZ)
float Joint::GetPulleySpeed1() const
{
    const JointImpl* j = AsImpl(this);
    const cdc::Vector3 r1 = j->body1->q.Rotate(j->anchor1);
    cdc::Vector3 u = (j->body1->x + r1) - j->bx1;
    if (u.IsZero3()) {
        return 0.0f;
    }
    u.Normalize3();
    return cdc::Dot3(u, ComputePointWorldVel(j->body1, r1));
}

// 0x0050a050 (PDB: 0x0050bb20 / ?GetPulleySpeed2@Joint@@QBEMXZ)
float Joint::GetPulleySpeed2() const
{
    const JointImpl* j = AsImpl(this);
    const cdc::Vector3 r2 = j->body2->q.Rotate(j->anchor2);
    cdc::Vector3 u = (j->body2->x + r2) - j->bx2;
    if (u.IsZero3()) {
        return 0.0f;
    }
    u.Normalize3();
    return cdc::Dot3(u, ComputePointWorldVel(j->body2, r2));
}

// 0x0050a130 (PDB: 0x0050bc00 / ?GetSplineSpeed@Joint@@QBEMXZ)
float Joint::GetSplineSpeed() const
{
    return AsImpl(this)->motor[0].velocity;
}

// 0x0050a140 (PDB: 0x0050bc10 / ?GetLinkData@Joint@@QAEXAAM0@Z)
void Joint::GetLinkData(float& coord1, float& coord2)
{
    const JointImpl* j = AsImpl(this);
    coord1 = j->linkCoord1;
    coord2 = j->linkCoord2;
}

// 0x0050a160 (PDB: 0x0050bc30 / ?SetLinkData@Joint@@QAEXMM@Z)
void Joint::SetLinkData(float coord1, float coord2)
{
    JointImpl* j = AsImpl(this);
    j->linkCoord1 = coord1;
    j->linkCoord2 = coord2;
}

// 0x0050a190 (PDB: 0x0050bc60 / ?EnableLimit@Joint@@QAEXH_N@Z)
void Joint::EnableLimit(int limitNumber, bool enable)
{
    JointImpl* j = AsImpl(this);
    if (!j->IsLimitNumberValid(limitNumber)) {
        return;
    }
    const unsigned int bit = static_cast<unsigned int>(JointImpl::LIMIT1) << limitNumber;
    if (enable) {
        j->flags |= bit;
    } else {
        j->flags &= ~bit;
    }
}

// 0x0050a1e0 (PDB: 0x0050bcb0 / ?IsLimitEnabled@Joint@@QAE_NH@Z)
bool Joint::IsLimitEnabled(int limitNumber)
{
    const JointImpl* j = AsImpl(this);
    if (!j->IsLimitNumberValid(limitNumber)) {
        return false;
    }
    return (j->flags & (static_cast<unsigned int>(JointImpl::LIMIT1) << limitNumber)) != 0;
}

// 0x0050a210 (PDB: 0x0050bce0 / ?SetLimit@Joint@@QAEXHMM@Z)
void Joint::SetLimit(int limitNumber, float low, float high)
{
    JointImpl* j = AsImpl(this);
    j->limit[limitNumber].low = low;
    j->limit[limitNumber].high = high;
}

// 0x0050a230 (PDB: 0x0050bd00 / ?GetLimit@Joint@@QAEXHAAM0@Z)
void Joint::GetLimit(int limitNumber, float& low, float& high)
{
    const JointImpl* j = AsImpl(this);
    low = j->limit[limitNumber].low;
    high = j->limit[limitNumber].high;
}

// ?GetLimitForce@Joint@@QBEMH@Z
float Joint::GetLimitForce(int limitNumber) const
{
    const JointImpl* j = AsImpl(this);
    if (!j->IsLimitNumberValid(limitNumber)) {
        return 0.0f;
    }
    const unsigned int limitBit = static_cast<unsigned int>(JointImpl::LIMIT1) << limitNumber;
    if ((j->flags & limitBit) == 0 || j->limit[limitNumber].status == JointLimit::NOT_AT_LIMIT) {
        return 0.0f;
    }

    switch (j->type) {
    case RevoluteJoint:
    case PrismaticJoint:
        return j->lambda[5];
    case PlanarJoint:
    case SwingJoint:
    case CartesianJoint:
        return j->lambda[ResolveAxisRow(j, limitNumber, 3)];
    case UniversalJoint:
        return j->lambda[ResolveAxisRow(j, limitNumber, 4)];
    case ConicalLimit:
        return j->lambda[0];
    default:
        return 0.0f;
    }
}

// 0x0050a260 (PDB: 0x0050bd30 / ?EnableMotor@Joint@@QAEXH_N@Z)
void Joint::EnableMotor(int motorNumber, bool enable)
{
    JointImpl* j = AsImpl(this);
    if (!j->IsMotorNumberValid(motorNumber)) {
        return;
    }
    const unsigned int bit = static_cast<unsigned int>(JointImpl::MOTOR1) << motorNumber;
    if (enable) {
        if ((j->flags & bit) == 0) {
            j->flags |= bit;
            if (HasDynamicMotorRow(j->type)) {
                ++j->numRowsNoLimits;
            }
        }
    } else {
        if ((j->flags & bit) != 0) {
            j->flags &= ~bit;
            if (HasDynamicMotorRow(j->type) && j->numRowsNoLimits > 0) {
                --j->numRowsNoLimits;
            }
        }
    }
}

// 0x0050a2d0 (PDB: 0x0050bda0 / ?IsMotorEnabled@Joint@@QAE_NH@Z)
bool Joint::IsMotorEnabled(int motorNumber)
{
    const JointImpl* j = AsImpl(this);
    if (!j->IsMotorNumberValid(motorNumber)) {
        return false;
    }
    return (j->flags & (static_cast<unsigned int>(JointImpl::MOTOR1) << motorNumber)) != 0;
}

// 0x0050a300 (PDB: 0x0050bdd0 / ?SetMotor@Joint@@QAEXHM@Z)
void Joint::SetMotor(int motorNumber, float velocity)
{
    AsImpl(this)->motor[motorNumber].velocity = velocity;
}

// 0x0050a320 (PDB: 0x0050bdf0 / ?GetMotor@Joint@@QAEMH@Z)
float Joint::GetMotor(int motorNumber)
{
    return AsImpl(this)->motor[motorNumber].velocity;
}

// 0x0050a330 (PDB: 0x0050be00 / ?SetMotorMaxForce@Joint@@QAEXHM@Z)
void Joint::SetMotorMaxForce(int motorNumber, float maxForce)
{
    AsImpl(this)->motor[motorNumber].maxForce = maxForce;
}

// 0x0050a350 (PDB: 0x0050be20 / ?GetMotorMaxForce@Joint@@QAEMH@Z)
float Joint::GetMotorMaxForce(int motorNumber)
{
    return AsImpl(this)->motor[motorNumber].maxForce;
}

// 0x0050a360 (PDB: 0x0050be30 / ?GetMotorForce@Joint@@QBEMH@Z)
float Joint::GetMotorForce(int motorNumber) const
{
    const JointImpl* j = AsImpl(this);
    if (!j->IsMotorNumberValid(motorNumber)) {
        return 0.0f;
    }
    const unsigned int motorBit = static_cast<unsigned int>(JointImpl::MOTOR1) << motorNumber;
    if ((j->flags & motorBit) == 0) {
        return 0.0f;
    }

    switch (j->type) {
    case RevoluteJoint:
    case PrismaticJoint:
        return j->lambda[5];
    case PlanarJoint:
    case SwingJoint:
    case CartesianJoint:
        return j->lambda[ResolveAxisRow(j, motorNumber, 3)];
    case RopeJoint:
    case AngularMotor:
        return j->lambda[0];
    case UniversalJoint:
        return j->lambda[ResolveAxisRow(j, motorNumber, 4)];
    case PointMotor:
        return j->lambda[motorNumber];
    default:
        return 0.0f;
    }
}

// 0x0050a400 (PDB: 0x0050bef0 / ?Clear@JointImpl@@QAEXXZ)
void JointImpl::Clear()
{
    body1 = nullptr;
    body2 = nullptr;
    linkChild1 = nullptr;
    linkChild2 = nullptr;
    clientData = nullptr;
    clientDestruct = nullptr;
    flags = 0;
    type = Joint::UnknownJoint;
    numRowsNoLimits = 0;
    numRows = 0;
    stepIterations = 0;
    relaxation = 0.0f;
    for (int i = 0; i < 6; ++i) {
        gamma[i] = 0.0f;
        lambda[i] = 0.0f;
    }
    force = ZeroVec();
    torque = ZeroVec();
    for (int i = 0; i < 3; ++i) {
        limit[i].low = -FLT_MAX;
        limit[i].high = FLT_MAX;
        limit[i].value = 0.0f;
        limit[i].status = JointLimit::NOT_AT_LIMIT;
        motor[i].velocity = 0.0f;
        motor[i].maxForce = FLT_MAX;
    }
}

// 0x0050a540 (PDB: 0x0050c0a0 / ?ComputePrismaticTranslation@JointImpl@@QBEMXZ)
float JointImpl::ComputePrismaticTranslation() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 z2 = body2->q.Rotate(bz2);
    return cdc::Dot3(p2 - p1, z2);
}

// 0x0050a630 (PDB: 0x0050c190 / ?ComputePrismaticTranslationSpeed@JointImpl@@QBEMXZ)
float JointImpl::ComputePrismaticTranslationSpeed() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 v1 = ComputePointWorldVel(body1, r1);
    const cdc::Vector3 v2 = ComputePointWorldVel(body2, r2);
    const cdc::Vector3 z2 = body2->q.Rotate(bz2);
    return cdc::Dot3(v2 - v1, z2);
}

// 0x0050a990 (PDB: 0x0050cc00 / ?ComputeRevoluteAngle@JointImpl@@QBEMXZ)
float JointImpl::ComputeRevoluteAngle() const
{
    cdc::Vector3 x1, x2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    const cdc::Vector3 y2 = body2->q.Rotate(by2);
    return std::atan2(-cdc::Dot3(x1, y2), cdc::Dot3(x1, x2));
}

// 0x0050b0c0 (PDB: 0x0050c010 / ?ComputeRevoluteAngleSpeed@JointImpl@@QBEMXZ)
float JointImpl::ComputeRevoluteAngleSpeed() const
{
    const cdc::Vector3 z2 = body2->q.Rotate(bz2);
    const cdc::Vector3 w = (body1 != nullptr) ? (body2->omega - body1->omega) : body2->omega;
    return cdc::Dot3(w, z2);
}

// 0x0050a750 (PDB: 0x0050c2b0 / ?ComputePlanarTranslation1@JointImpl@@QBEMXZ)
float JointImpl::ComputePlanarTranslation1() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 x2 = body2->q.Rotate(bx2);
    return cdc::Dot3(p2 - p1, x2);
}

// 0x0050a870 (PDB: 0x0050c3d0 / ?ComputePlanarTranslation2@JointImpl@@QBEMXZ)
float JointImpl::ComputePlanarTranslation2() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 y2 = body2->q.Rotate(by2);
    return cdc::Dot3(p2 - p1, y2);
}

// 0x0050a990 (PDB: 0x0050cc00 / ?ComputePlanarAngle@JointImpl@@QBEMXZ)
float JointImpl::ComputePlanarAngle() const
{
    return ComputeRevoluteAngle();
}

// 0x0050aac0 (PDB: 0x0050c4f0 / ?ComputePlanarTranslationSpeed1@JointImpl@@QBEMXZ)
float JointImpl::ComputePlanarTranslationSpeed1() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 x2 = body2->q.Rotate(bx2);
    return cdc::Dot3(ComputePointWorldVel(body2, r2) - ComputePointWorldVel(body1, r1), x2);
}

// 0x0050ac20 (PDB: 0x0050c650 / ?ComputePlanarTranslationSpeed2@JointImpl@@QBEMXZ)
float JointImpl::ComputePlanarTranslationSpeed2() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 y2 = body2->q.Rotate(by2);
    return cdc::Dot3(ComputePointWorldVel(body2, r2) - ComputePointWorldVel(body1, r1), y2);
}

// 0x0050ad80 (PDB: 0x0050c7b0 / ?ComputePlanarAngleSpeed@JointImpl@@QBEMXZ)
float JointImpl::ComputePlanarAngleSpeed() const
{
    return ComputeRevoluteAngleSpeed();
}

// 0x0050ae10 (PDB: 0x0050c840 / ?ComputeUniversalAngle1@JointImpl@@QBEMXZ)
float JointImpl::ComputeUniversalAngle1() const
{
    cdc::Vector3 x1, x2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    const cdc::Vector3 z1 = (body1 != nullptr) ? body1->q.Rotate(bz1) : bz1;
    return std::atan2(cdc::Dot3(z1, x2), cdc::Dot3(x1, x2));
}

// 0x0050af40 (PDB: 0x0050c970 / ?ComputeUniversalAngle2@JointImpl@@QBEMXZ)
float JointImpl::ComputeUniversalAngle2() const
{
    const cdc::Vector3 y1 = (body1 != nullptr) ? body1->q.Rotate(by1) : by1;
    const cdc::Vector3 x2 = body2->q.Rotate(bx2);
    const cdc::Vector3 y2 = body2->q.Rotate(by2);
    return std::atan2(cdc::Dot3(x2, y1), cdc::Dot3(y2, y1));
}

// 0x0050b030 (PDB: 0x0050ca60 / ?ComputeUniversalAngleSpeed1@JointImpl@@QBEMXZ)
float JointImpl::ComputeUniversalAngleSpeed1() const
{
    const cdc::Vector3 y1 = (body1 != nullptr) ? body1->q.Rotate(by1) : by1;
    const cdc::Vector3 w = (body1 != nullptr) ? (body2->omega - body1->omega) : body2->omega;
    return cdc::Dot3(w, y1);
}

// 0x0050b0c0 (PDB: 0x0050c010 / ?ComputeUniversalAngleSpeed2@JointImpl@@QBEMXZ)
float JointImpl::ComputeUniversalAngleSpeed2() const
{
    return ComputeRevoluteAngleSpeed();
}

// 0x0050b150 (PDB: 0x0050caf0 / ?ComputeSwingTranslation@JointImpl@@QBEMXZ)
float JointImpl::ComputeSwingTranslation() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 z1 = (body1 != nullptr) ? body1->q.Rotate(bz1) : bz1;
    return cdc::Dot3(p2 - p1, z1);
}

// 0x0050a990 (PDB: 0x0050cc00 / ?ComputeSwingAngle1@JointImpl@@QBEMXZ)
float JointImpl::ComputeSwingAngle1() const
{
    const cdc::Vector3 y1 = (body1 != nullptr) ? body1->q.Rotate(by1) : by1;
    const cdc::Vector3 z1 = (body1 != nullptr) ? body1->q.Rotate(bz1) : bz1;
    const cdc::Vector3 x2 = body2->q.Rotate(bx2);
    return std::atan2(cdc::Dot3(x2, y1), cdc::Dot3(x2, z1));
}

// 0x0050b260 (PDB: 0x0050cd30 / ?ComputeSwingAngle2@JointImpl@@QBEMXZ)
float JointImpl::ComputeSwingAngle2() const
{
    const cdc::Vector3 y1 = (body1 != nullptr) ? body1->q.Rotate(by1) : by1;
    const cdc::Vector3 z1 = (body1 != nullptr) ? body1->q.Rotate(bz1) : bz1;
    const cdc::Vector3 y2 = body2->q.Rotate(by2);
    return std::atan2(-cdc::Dot3(y2, y1), cdc::Dot3(y2, z1));
}

// 0x0050b350 (PDB: 0x0050ce20 / ?ComputeSwingTranslationSpeed@JointImpl@@QBEMXZ)
float JointImpl::ComputeSwingTranslationSpeed() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    const cdc::Vector3 z1 = (body1 != nullptr) ? body1->q.Rotate(bz1) : bz1;
    return cdc::Dot3(ComputePointWorldVel(body2, r2) - ComputePointWorldVel(body1, r1), z1);
}

// 0x0050b470 (PDB: 0x0050cf40 / ?ComputeSwingAngleSpeed1@JointImpl@@QBEMXZ)
float JointImpl::ComputeSwingAngleSpeed1() const
{
    const cdc::Vector3 x1 = (body1 != nullptr) ? body1->q.Rotate(bx1) : bx1;
    const cdc::Vector3 w = (body1 != nullptr) ? (body2->omega - body1->omega) : body2->omega;
    return cdc::Dot3(w, x1);
}

// 0x0050b500 (PDB: 0x0050cfd0 / ?ComputeSwingAngleSpeed2@JointImpl@@QBEMXZ)
float JointImpl::ComputeSwingAngleSpeed2() const
{
    return ComputeRevoluteAngleSpeed();
}

// 0x0050b590 (PDB: 0x0050d060 / ?ComputeCartesianTranslation@JointImpl@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 JointImpl::ComputeCartesianTranslation() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    return p2 - p1;
}

// 0x0050b640 (PDB: 0x0050d110 / ?ComputeCartesianTranslationSpeed@JointImpl@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 JointImpl::ComputeCartesianTranslationSpeed() const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    return ComputePointWorldVel(body2, r2) - ComputePointWorldVel(body1, r1);
}

// 0x0050edc0 (PDB: 0x00510890 / ?GetPrismaticTranslation@Joint@@QBEMXZ)
float Joint::GetPrismaticTranslation() const
{
    return AsImpl(this)->ComputePrismaticTranslation();
}

// 0x0050edd0 (PDB: 0x005108a0 / ?GetPrismaticTranslationSpeed@Joint@@QBEMXZ)
float Joint::GetPrismaticTranslationSpeed() const
{
    return AsImpl(this)->ComputePrismaticTranslationSpeed();
}

// 0x0050ede0 (PDB: 0x005108b0 / ?GetPlanarTranslation1@Joint@@QBEMXZ)
float Joint::GetPlanarTranslation1() const
{
    return AsImpl(this)->ComputePlanarTranslation1();
}

// 0x0050edf0 (PDB: 0x005108c0 / ?GetPlanarTranslation2@Joint@@QBEMXZ)
float Joint::GetPlanarTranslation2() const
{
    return AsImpl(this)->ComputePlanarTranslation2();
}

// 0x0050ee80 (PDB: 0x00510950 / ?GetPlanarAngle@Joint@@QBEMXZ)
float Joint::GetPlanarAngle() const
{
    return AsImpl(this)->ComputePlanarAngle();
}

// 0x0050ee00 (PDB: 0x005108d0 / ?GetPlanarTranslationSpeed1@Joint@@QBEMXZ)
float Joint::GetPlanarTranslationSpeed1() const
{
    return AsImpl(this)->ComputePlanarTranslationSpeed1();
}

// 0x0050ee10 (PDB: 0x005108e0 / ?GetPlanarTranslationSpeed2@Joint@@QBEMXZ)
float Joint::GetPlanarTranslationSpeed2() const
{
    return AsImpl(this)->ComputePlanarTranslationSpeed2();
}

// 0x0050ee20 (PDB: 0x005108f0 / ?GetPlanarAngleSpeed@Joint@@QBEMXZ)
float Joint::GetPlanarAngleSpeed() const
{
    return AsImpl(this)->ComputePlanarAngleSpeed();
}

// 0x0050ee30 (PDB: 0x00510900 / ?GetUniversalAngle1@Joint@@QBEMXZ)
float Joint::GetUniversalAngle1() const
{
    return AsImpl(this)->ComputeUniversalAngle1();
}

// 0x0050ee40 (PDB: 0x00510910 / ?GetUniversalAngle2@Joint@@QBEMXZ)
float Joint::GetUniversalAngle2() const
{
    return AsImpl(this)->ComputeUniversalAngle2();
}

// 0x0050ee50 (PDB: 0x00510920 / ?GetUniversalAngleSpeed1@Joint@@QBEMXZ)
float Joint::GetUniversalAngleSpeed1() const
{
    return AsImpl(this)->ComputeUniversalAngleSpeed1();
}

// 0x0050ee60 (PDB: 0x00510930 / ?GetRevoluteAngleSpeed@Joint@@QBEMXZ)
float Joint::GetRevoluteAngleSpeed() const
{
    return AsImpl(this)->ComputeRevoluteAngleSpeed();
}

// 0x0050ee60 (PDB: 0x00510930 / ?GetUniversalAngleSpeed2@Joint@@QBEMXZ)
float Joint::GetUniversalAngleSpeed2() const
{
    return AsImpl(this)->ComputeUniversalAngleSpeed2();
}

// 0x0050ee70 (PDB: 0x00510940 / ?GetSwingTranslation@Joint@@QBEMXZ)
float Joint::GetSwingTranslation() const
{
    return AsImpl(this)->ComputeSwingTranslation();
}

// 0x0050ee80 (PDB: 0x00510950 / ?GetRevoluteAngle@Joint@@QBEMXZ)
float Joint::GetRevoluteAngle() const
{
    return AsImpl(this)->ComputeRevoluteAngle();
}

// 0x0050ee80 (PDB: 0x00510950 / ?GetSwingAngle1@Joint@@QBEMXZ)
float Joint::GetSwingAngle1() const
{
    return AsImpl(this)->ComputeSwingAngle1();
}

// 0x0050ee90 (PDB: 0x00510960 / ?GetSwingAngle2@Joint@@QBEMXZ)
float Joint::GetSwingAngle2() const
{
    return AsImpl(this)->ComputeSwingAngle2();
}

// 0x0050eea0 (PDB: 0x00510970 / ?GetSwingTranslationSpeed@Joint@@QBEMXZ)
float Joint::GetSwingTranslationSpeed() const
{
    return AsImpl(this)->ComputeSwingTranslationSpeed();
}

// 0x0050eeb0 (PDB: 0x00510980 / ?GetSwingAngleSpeed1@Joint@@QBEMXZ)
float Joint::GetSwingAngleSpeed1() const
{
    return AsImpl(this)->ComputeSwingAngleSpeed1();
}

// 0x0050eec0 (PDB: 0x00510990 / ?GetSwingAngleSpeed2@Joint@@QBEMXZ)
float Joint::GetSwingAngleSpeed2() const
{
    return AsImpl(this)->ComputeSwingAngleSpeed2();
}

// ?GetCartesianTranslation@Joint@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Joint::GetCartesianTranslation() const
{
    return AsImpl(this)->ComputeCartesianTranslation();
}

// 0x0050eed0 (PDB: 0x005109a0 / ?GetCartesianTranslationSpeed@Joint@@QBE?AVVector3@cdc@@XZ)
cdc::Vector3 Joint::GetCartesianTranslationSpeed() const
{
    return AsImpl(this)->ComputeCartesianTranslationSpeed();
}

// 0x0050cf30 (PDB: 0x0050ea20 / ?SetRevolute@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1@Z)
void Joint::SetRevolute(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& axis)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    cdc::Vector3 u, v;
    ComputeBasis(&u, &v, axis);
    StoreWorldFrame(bi1, anchor, u, v, axis, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, u, v, axis, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = RevoluteJoint;
    j->numRowsNoLimits = 5;
}

// 0x0050d140 (PDB: 0x0050ec30 / ?SetPrismatic@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1@Z)
void Joint::SetPrismatic(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& axis)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    cdc::Vector3 u, v;
    ComputeBasis(&u, &v, axis);
    StoreWorldFrame(bi1, anchor, u, v, axis, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, u, v, axis, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = PrismaticJoint;
    j->numRowsNoLimits = 5;
}

// 0x0050d350 (PDB: 0x0050ee40 / ?SetPlanar@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11@Z)
void Joint::SetPlanar(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& normal,
    const cdc::Vector3& tangent)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    cdc::Vector3 bitangent;
    cdc::Cross3(&bitangent, normal, tangent);
    bitangent.Normalize3();
    StoreWorldFrame(bi1, anchor, normal, tangent, bitangent, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, normal, tangent, bitangent, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = PlanarJoint;
    j->numRowsNoLimits = 3;
}

// 0x0050d5c0 (PDB: 0x0050f0b0 / ?SetSpherical@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
void Joint::SetSpherical(Body* b1, Body* b2, const cdc::Vector3& anchor)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    j->anchor1 = (bi1 != nullptr) ? ToLocalPos(bi1, anchor) : anchor;
    j->anchor2 = ToLocalPos(bi2, anchor);
    j->Connect(bi1, bi2);
    j->type = SphericalJoint;
    j->numRowsNoLimits = 3;
}

// 0x0050d6d0 (PDB: 0x0050f1c0 / ?SetRope@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1M@Z)
void Joint::SetRope(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor1,
    const cdc::Vector3& anchor2,
    float maxLength)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    if (maxLength > 0.0f) {
        j->length = maxLength;
    } else {
        const cdc::Vector3 diff = anchor2 - anchor1;
        j->length = VecLength3(diff);
        if (j->length < 0.01f) {
            MULTIBODY_PrintWarning("Rope length too short = %g on %s", j->length, bi2->name);
            j->length = 0.01f;
        }
    }
    j->anchor1 = (bi1 != nullptr) ? ToLocalPos(bi1, anchor1) : anchor1;
    j->anchor2 = ToLocalPos(bi2, anchor2);
    j->Connect(bi1, bi2);
    j->type = RopeJoint;
    j->numRowsNoLimits = 1;
}

// 0x0050d890 (PDB: 0x0050f380 / ?SetPulley@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@111M@Z)
void Joint::SetPulley(
    Body* b1,
    Body* b2,
    const cdc::Vector3& groundAnchor1,
    const cdc::Vector3& anchor1,
    const cdc::Vector3& groundAnchor2,
    const cdc::Vector3& anchor2,
    float ratio)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);

    const float len1 = VecLength3(anchor1 - groundAnchor1);
    const float len2 = VecLength3(anchor2 - groundAnchor2);
    j->length = ratio * len1 + len2;
    j->bx1 = groundAnchor1;
    j->anchor1 = ToLocalPos(bi1, anchor1);
    j->bx2 = groundAnchor2;
    j->anchor2 = ToLocalPos(bi2, anchor2);
    j->ratio = ratio;

    j->Connect(bi1, bi2);
    j->type = PulleyJoint;
    j->numRowsNoLimits = 1;
}

// 0x0050da40 (PDB: 0x0050f530 / ?SetUniversal@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11@Z)
void Joint::SetUniversal(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& axis1,
    const cdc::Vector3& axis2)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    cdc::Vector3 z;
    cdc::Cross3(&z, axis2, axis1);
    z.Normalize3();
    cdc::Vector3 x1;
    cdc::Cross3(&x1, axis1, z);
    cdc::Vector3 x2;
    cdc::Cross3(&x2, z, axis2);
    StoreWorldFrame(bi1, anchor, x1, axis1, z, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, x2, z, axis2, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = UniversalJoint;
    j->numRowsNoLimits = 4;
}

// 0x0050dce0 (PDB: 0x0050f7d0 / ?SetSwing@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11@Z)
void Joint::SetSwing(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& axis1,
    const cdc::Vector3& axis2)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    cdc::Vector3 n1 = axis1;
    n1.Normalize3();
    cdc::Vector3 n2 = axis2;
    n2.Normalize3();
    cdc::Vector3 y;
    cdc::Cross3(&y, n1, n2);
    y.Normalize3();
    cdc::Vector3 x1;
    cdc::Cross3(&x1, y, n1);
    cdc::Vector3 x2;
    cdc::Cross3(&x2, y, n2);
    StoreWorldFrame(bi1, anchor, x1, y, n1, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, x2, y, n2, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = SwingJoint;
    j->numRowsNoLimits = 3;
}

// 0x0050df80 (PDB: 0x0050fa70 / ?SetCartesian@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
void Joint::SetCartesian(Body* b1, Body* b2, const cdc::Vector3& anchor)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    const cdc::Vector3 ex = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
    const cdc::Vector3 ey = cdc::Vector3::Make(0.0f, 1.0f, 0.0f);
    const cdc::Vector3 ez = cdc::Vector3::Make(0.0f, 0.0f, 1.0f);
    StoreWorldFrame(bi1, anchor, ex, ey, ez, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, ex, ey, ez, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = CartesianJoint;
    j->numRowsNoLimits = 3;
}

// 0x0050e240 (PDB: 0x0050fd30 / ?SetWeld@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
void Joint::SetWeld(Body* b1, Body* b2, const cdc::Vector3& anchor)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    const cdc::Vector3 ex = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
    const cdc::Vector3 ey = cdc::Vector3::Make(0.0f, 1.0f, 0.0f);
    const cdc::Vector3 ez = cdc::Vector3::Make(0.0f, 0.0f, 1.0f);
    StoreWorldFrame(bi1, anchor, ex, ey, ez, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, anchor, ex, ey, ez, j->anchor2, j->bx2, j->by2, j->bz2);
    j->Connect(bi1, bi2);
    j->type = WeldJoint;
    j->numRowsNoLimits = 6;
}

// 0x0050e580 (PDB: 0x00510050 / ?SetPointMotor@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1@Z)
void Joint::SetPointMotor(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor1,
    const cdc::Vector3& anchor2)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    j->anchor1 = (bi1 != nullptr) ? ToLocalPos(bi1, anchor1) : anchor1;
    j->anchor2 = ToLocalPos(bi2, anchor2);
    j->Connect(bi1, bi2);
    j->type = PointMotor;
    j->numRowsNoLimits = 3;
    j->flags |= JointImpl::MOTOR1 | JointImpl::MOTOR2 | JointImpl::MOTOR3;
    for (int i = 0; i < 3; ++i) {
        j->motor[i].velocity = 0.0f;
        j->motor[i].maxForce = FLT_MAX;
    }
}

// 0x0050e6c0 (PDB: 0x00510190 / ?SetLink@Joint@@QAEXPAV1@0M@Z)
void Joint::SetLink(Joint* joint1, Joint* joint2, float ratio)
{
    JointImpl* j = AsImpl(this);
    JointImpl* ji1 = AsImpl(joint1);
    JointImpl* ji2 = AsImpl(joint2);
    BodyImpl* b1 = ji1->body2;
    BodyImpl* b2 = ji2->body2;

    if (ji1->type == RevoluteJoint) {
        j->bz1 = ji1->bz2;
        j->linkCoord1 = ji1->ComputeRevoluteAngle();
        j->flags &= ~JointImpl::LINK_POS1;
    } else if (ji1->type == PrismaticJoint) {
        j->bz1 = ji1->bz2;
        j->anchor1 = ji1->anchor2;
        const cdc::Vector3 worldZ1 = b1->q.Rotate(j->bz1);
        const cdc::Vector3 worldP1 = b1->x + b1->q.Rotate(j->anchor1);
        j->linkCoord1 = cdc::Dot3(worldP1, worldZ1);
        j->flags |= JointImpl::LINK_POS1;
    }

    if (ji2->type == RevoluteJoint) {
        j->bz2 = ji2->bz2;
        j->linkCoord2 = ji2->ComputeRevoluteAngle();
        j->flags &= ~JointImpl::LINK_POS2;
    } else if (ji2->type == PrismaticJoint) {
        j->bz2 = ji2->bz2;
        j->anchor2 = ji2->anchor2;
        const cdc::Vector3 worldZ2 = b2->q.Rotate(j->bz2);
        const cdc::Vector3 worldP2 = b2->x + b2->q.Rotate(j->anchor2);
        j->linkCoord2 = cdc::Dot3(worldP2, worldZ2);
        j->flags |= JointImpl::LINK_POS2;
    }

    j->ratio = ratio;
    j->linkChild1 = ji1;
    j->linkChild2 = ji2;
    j->flags |= JointImpl::LINK_ROT2;
    j->Connect(b1, b2);
    j->type = LinkJoint;
    j->numRowsNoLimits = 1;
}

// ?SetWheel@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11MM@Z
void Joint::SetWheel(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& suspensionAxis,
    const cdc::Vector3& wheelAxis,
    float maxSuspensionForce,
    float maxWheelForce)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    j->anchor1 = anchor;
    j->anchor2 = anchor;
    j->bx2 = suspensionAxis;
    j->by2 = wheelAxis;
    j->motor[0].maxForce = maxSuspensionForce;
    j->motor[1].maxForce = maxWheelForce;
    j->Connect(bi1, bi2);
    j->flags |= JointImpl::COLLIDE;
    j->type = WheelJoint;
    j->numRowsNoLimits = 2;
}

// 0x0050e970 (PDB: 0x00510440 / ?SetWheel2@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11MM1MMM@Z)
void Joint::SetWheel2(
    Body* b1,
    Body* b2,
    const cdc::Vector3& anchor,
    const cdc::Vector3& bx,
    const cdc::Vector3& by,
    float maxForce1,
    float maxForce2,
    const cdc::Vector3& bz,
    float radius,
    float coord1,
    float coord2)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    UpdateWheel2(anchor, bx, by, maxForce1, maxForce2, bz, radius, coord1, coord2);
    j->Connect(bi1, bi2);
    j->flags |= JointImpl::COLLIDE;
    j->type = WheelJoint2;
    j->numRowsNoLimits = 3;
}

// 0x0050ea70 (PDB: 0x00510540 / ?UpdatePrismaticAnchor@Joint@@QAEXXZ)
void Joint::UpdatePrismaticAnchor()
{
    JointImpl* j = AsImpl(this);
    if (j->body1 != nullptr || j->type != PrismaticJoint) {
        return;
    }

    const cdc::Vector3 diff = j->body2->x - j->anchor1;
    const float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    if (distSq > 2.5e9f) {
        const float step = (cdc::Dot3(j->bz2, diff) >= 0.0f) ? 50000.0f : -50000.0f;
        j->anchor1 = j->anchor1 + j->bz2 * step;
    }
}

// 0x0050eb40 (PDB: 0x00510610 / ?SetConicalLimit@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1M@Z)
void Joint::SetConicalLimit(
    Body* b1,
    Body* b2,
    const cdc::Vector3& axis1,
    const cdc::Vector3& axis2,
    float angle)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    j->anchor1 = ZeroVec();
    j->anchor2 = ZeroVec();
    j->bz1 = axis1;
    j->bz2 = axis2;
    j->Connect(bi1, bi2);

    float clamped = angle;
    if (clamped > kConicalMaxAngle) {
        clamped = kConicalMaxAngle;
    } else if (clamped < 0.0f) {
        clamped = 0.0f;
    }

    j->flags |= JointImpl::LIMIT1;
    j->type = ConicalLimit;
    j->numRowsNoLimits = 0;
    j->limit[0].low = std::cos(clamped);
}

// 0x0050ec10 (PDB: 0x005106e0 / ?SetAngularMotor@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
void Joint::SetAngularMotor(Body* b1, Body* b2, const cdc::Vector3& axis)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(b1);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(b2);
    j->bz2 = axis;
    j->Connect(bi1, bi2);
    j->type = AngularMotor;
    j->numRowsNoLimits = 1;
    j->flags |= JointImpl::MOTOR1;
    j->motor[0].velocity = 0.0f;
    j->motor[0].maxForce = FLT_MAX;
}

// 0x0050ec90 (PDB: 0x00510760 / ?SetSpline@Joint@@QAEXPAVBody@@ABVVector3@cdc@@P6AXPAUSplineJointParams@@PBV1@M@Z@Z)
void Joint::SetSpline(
    Body* b,
    const cdc::Vector3& anchor,
    void (*callback)(SplineJointParams*, const Joint*, float))
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi = reinterpret_cast<BodyImpl*>(b);
    j->anchor2 = ToLocalPos(bi, anchor);
    j->splineCallback = callback;
    j->Connect(nullptr, bi);
    j->type = SplineJoint;
    j->numRowsNoLimits = 6;
}

// 0x0050ed60 (PDB: 0x00510830 / ?SetRelaxation@Joint@@QAEXM@Z)
void Joint::SetRelaxation(float relaxation)
{
    AsImpl(this)->relaxation = relaxation;
}

// 0x00514700 (PDB: 0x005161f0 / ?SetShoulder@Joint@@QAEXAAUShoulderParams@@@Z)
void Joint::SetShoulder(ShoulderParams& params)
{
    JointImpl* j = AsImpl(this);
    BodyImpl* bi1 = reinterpret_cast<BodyImpl*>(params.parent);
    BodyImpl* bi2 = reinterpret_cast<BodyImpl*>(params.child);

    cdc::Vector3 z = params.twistAxis;
    cdc::Vector3 y;
    cdc::Cross3(&y, params.coneAxis, z);
    if (y.IsZero3()) {
        cdc::Vector3 xTmp, yTmp;
        ComputeBasis(&xTmp, &yTmp, params.coneAxis);
        cdc::Cross3(&y, params.coneAxis, yTmp);
        y.Normalize3();
        cdc::Cross3(&z, params.coneAxis, y);
    } else {
        y.Normalize3();
        cdc::Cross3(&z, params.coneAxis, y);
        z.Normalize3();
    }

    StoreWorldFrame(bi1, params.anchor, y, z, params.coneAxis, j->anchor1, j->bx1, j->by1, j->bz1);
    StoreWorldFrame(bi2, params.anchor, y, z, params.coneAxis, j->anchor2, j->bx2, j->by2, j->bz2);

    float clampedCone = params.coneAngle;
    if (clampedCone > kShoulderConeMaxAngle) {
        clampedCone = kShoulderConeMaxAngle;
    } else if (clampedCone < 0.0f) {
        clampedCone = 0.0f;
    }

    j->limit[0].low = std::cos(clampedCone);
    j->limit[1].low = params.twistLow;
    j->limit[1].high = params.twistHigh;
    j->motor[0].maxForce = params.friction;

    j->Connect(bi1, bi2);
    j->type = ShoulderJoint;
    j->numRowsNoLimits = 6;
}

// 0x0050eef0 (PDB: 0x005109c0 / ?ComputeAngularAxis@JointImpl@@QBEHPAVVector3@cdc@@PAM111ABUJointAxisParams@@@Z)
int JointImpl::ComputeAngularAxis(
    ContactJacobianRow* row,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    const JointAxisParams& axisParams) const
{
    return ComputeAxisHelper(const_cast<JointImpl*>(this), row, err, vel, lo, hi, axisParams, false);
}

// 0x0050f3c0 (PDB: 0x00510e90 / ?ComputeLinearAxis@JointImpl@@QBEHPAVVector3@cdc@@PAM111ABUJointAxisParams@@@Z)
int JointImpl::ComputeLinearAxis(
    ContactJacobianRow* row,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    const JointAxisParams& axisParams) const
{
    return ComputeAxisHelper(const_cast<JointImpl*>(this), row, err, vel, lo, hi, axisParams, true);
}

// 0x0050fac0 (PDB: 0x00511590 / ?ComputeRevolute@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeRevolute(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, body1, r1, r2, p1, p2);

    cdc::Vector3 x1, y1, z1, x2, y2, z2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    GetWorldAxes12(body1, body2, by1, by2, y1, y2);
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);

    cdc::Vector3 wAxis3, wAxis4;
    cdc::Cross3(&wAxis3, z2, x1);
    cdc::Cross3(&wAxis4, z2, y1);
    WriteAngularRow(J[rowIndex + 3], wAxis3, body1);
    WriteAngularRow(J[rowIndex + 4], wAxis4, body1);

    err[rowIndex + 3] = cdc::Dot3(z2, x1);
    err[rowIndex + 4] = cdc::Dot3(z2, y1);
    for (int k = 3; k < 5; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }

    if (numRows > 5) {
        JointAxisParams ap;
        ap.axis = z2;
        ap.r1 = ZeroVec();
        ap.r2 = ZeroVec();
        ap.invBeta = params->invBeta;
        ap.angularThickness = kAngularThickness;
        ap.linearThickness = 0.0f;
        ap.index = 0;
        ComputeAngularAxis(&J[rowIndex + 5], &err[rowIndex + 5], &vel[rowIndex + 5], &lo[rowIndex + 5], &hi[rowIndex + 5], ap);
    }
}

// 0x005100a0 (PDB: 0x00511b70 / ?ComputePrismatic@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputePrismatic(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);

    cdc::Vector3 x1, y1, z1, x2, y2, z2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    GetWorldAxes12(body1, body2, by1, by2, y1, y2);
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);

    const cdc::Vector3 diff = p2 - p1;
    WriteLinearRow(J[rowIndex + 0], x2, body1, r1, r2);
    WriteLinearRow(J[rowIndex + 1], y2, body1, r1, r2);
    err[rowIndex + 0] = cdc::Dot3(diff, x2);
    err[rowIndex + 1] = cdc::Dot3(diff, y2);

    cdc::Vector3 w2, w3, w4;
    cdc::Cross3(&w2, y2, z1);
    cdc::Cross3(&w3, x2, z1);
    cdc::Cross3(&w4, y2, x1);
    WriteAngularRow(J[rowIndex + 2], w2, body1);
    WriteAngularRow(J[rowIndex + 3], w3, body1);
    WriteAngularRow(J[rowIndex + 4], w4, body1);
    err[rowIndex + 2] = cdc::Dot3(y2, z1);
    err[rowIndex + 3] = cdc::Dot3(x2, z1);
    err[rowIndex + 4] = cdc::Dot3(y2, x1);

    for (int k = 0; k < 5; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }

    if (numRows > 5) {
        JointAxisParams ap;
        ap.axis = z2;
        ap.r1 = (body1 != nullptr) ? (p2 - body1->x) : p2;
        ap.r2 = r2;
        ap.invBeta = params->invBeta;
        ap.angularThickness = kAngularThickness;
        ap.linearThickness = params->thickness;
        ap.index = 0;
        ComputeLinearAxis(&J[rowIndex + 5], &err[rowIndex + 5], &vel[rowIndex + 5], &lo[rowIndex + 5], &hi[rowIndex + 5], ap);
    }
}

// 0x00510810 (PDB: 0x005122e0 / ?ComputePlanar@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputePlanar(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);

    cdc::Vector3 x1, y1, z1, x2, y2, z2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    GetWorldAxes12(body1, body2, by1, by2, y1, y2);
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);

    WriteLinearRow(J[rowIndex + 0], z2, body1, r1, r2);
    err[rowIndex + 0] = cdc::Dot3(p2 - p1, z2);

    cdc::Vector3 w1, w2;
    cdc::Cross3(&w1, y2, z1);
    cdc::Cross3(&w2, x2, z1);
    WriteAngularRow(J[rowIndex + 1], w1, body1);
    WriteAngularRow(J[rowIndex + 2], w2, body1);
    err[rowIndex + 1] = cdc::Dot3(y2, z1);
    err[rowIndex + 2] = cdc::Dot3(x2, z1);

    for (int k = 0; k < 3; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }

    if (numRows > 3) {
        int nextRow = rowIndex + 3;
        JointAxisParams ap;
        ap.r1 = (body1 != nullptr) ? (p2 - body1->x) : p2;
        ap.r2 = r2;
        ap.invBeta = params->invBeta;
        ap.angularThickness = kAngularThickness;
        ap.linearThickness = params->thickness;

        ap.axis = x2;
        ap.index = 0;
        nextRow += ComputeLinearAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);

        ap.axis = y2;
        ap.index = 1;
        nextRow += ComputeLinearAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);

        ap.axis = z2;
        ap.index = 2;
        ComputeAngularAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);
    }
}

// 0x0050b7b0 (PDB: 0x0050d280 / ?ComputeSpherical@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeSpherical(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, body1, r1, r2, p1, p2);
}

// 0x0050ba90 (PDB: 0x0050d560 / ?ComputeRope@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeRope(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);

    cdc::Vector3 u = p2 - p1;
    const float dist = SafeNormalize(u);

    WriteLinearRow(J[rowIndex], u, body1, r1, r2);
    if ((flags & MOTOR1) != 0) {
        err[rowIndex] = 0.0f;
        vel[rowIndex] = motor[0].velocity;
        lo[rowIndex] = -motor[0].maxForce;
        hi[rowIndex] = 0.0f;
    } else {
        err[rowIndex] = dist - length;
        vel[rowIndex] = 0.0f;
        lo[rowIndex] = -FLT_MAX;
        hi[rowIndex] = 0.0f;
    }
}

// 0x00510f40 (PDB: 0x00512a10 / ?ComputePulley@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputePulley(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    const cdc::Vector3 r1 = body1->q.Rotate(anchor1);
    cdc::Vector3 u1 = (body1->x + r1) - bx1;
    u1.Normalize3();
    const float len1 = VecLength3((body1->x + r1) - bx1);

    const cdc::Vector3 r2 = body2->q.Rotate(anchor2);
    cdc::Vector3 u2 = (body2->x + r2) - bx2;
    u2.Normalize3();
    const float len2 = VecLength3((body2->x + r2) - bx2);

    const float totalLen = ratio * len1 + len2;
    if (std::fabs(totalLen - kPulleyEps) < 1e-6f) {
        u2 = cdc::Vector3::Make(1.0f, 0.0f, 0.0f);
    }

    const cdc::Vector3 u1Scaled = u1 * ratio;
    J[rowIndex].v1 = u1Scaled;
    cdc::Cross3(&J[rowIndex].w1, r1, u1Scaled);
    J[rowIndex].v2 = u2;
    cdc::Cross3(&J[rowIndex].w2, r2, u2);

    err[rowIndex] = totalLen - length;
    vel[rowIndex] = 0.0f;
    lo[rowIndex] = -FLT_MAX;
    hi[rowIndex] = 0.0f;
}

// 0x00511230 (PDB: 0x00512d00 / ?ComputeUniversal@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeUniversal(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, body1, r1, r2, p1, p2);

    const cdc::Vector3 y1 = (body1 != nullptr) ? body1->q.Rotate(by1) : by1;
    const cdc::Vector3 z2 = body2->q.Rotate(bz2);
    cdc::Vector3 crossAxis;
    cdc::Cross3(&crossAxis, y1, z2);
    WriteAngularRow(J[rowIndex + 3], crossAxis, body1);
    err[rowIndex + 3] = cdc::Dot3(y1, z2);
    vel[rowIndex + 3] = 0.0f;
    lo[rowIndex + 3] = -FLT_MAX;
    hi[rowIndex + 3] = FLT_MAX;

    if (numRows > 4) {
        int nextRow = rowIndex + 4;
        JointAxisParams ap;
        ap.r1 = ZeroVec();
        ap.r2 = ZeroVec();
        ap.invBeta = params->invBeta;
        ap.angularThickness = kAngularThickness;
        ap.linearThickness = 0.0f;

        ap.axis = y1;
        ap.index = 0;
        nextRow += ComputeAngularAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);

        ap.axis = z2;
        ap.index = 1;
        ComputeAngularAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);
    }
}

// 0x00511740 (PDB: 0x00513210 / ?ComputeSwing@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeSwing(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);

    const cdc::Vector3 x1 = (body1 != nullptr) ? body1->q.Rotate(bx1) : bx1;
    const cdc::Vector3 y1 = (body1 != nullptr) ? body1->q.Rotate(by1) : by1;
    const cdc::Vector3 z1 = (body1 != nullptr) ? body1->q.Rotate(bz1) : bz1;
    const cdc::Vector3 z2 = body2->q.Rotate(bz2);

    const cdc::Vector3 r1AtP2 = (body1 != nullptr) ? (p2 - body1->x) : ZeroVec();
    const cdc::Vector3 diff = p2 - p1;
    WriteLinearRow(J[rowIndex + 0], x1, body1, r1AtP2, r2);
    WriteLinearRow(J[rowIndex + 1], y1, body1, r1AtP2, r2);
    err[rowIndex + 0] = cdc::Dot3(diff, x1);
    err[rowIndex + 1] = cdc::Dot3(diff, y1);

    cdc::Vector3 ortho;
    cdc::Cross3(&ortho, x1, z2);
    WriteAngularRow(J[rowIndex + 2], ortho, body1);
    err[rowIndex + 2] = cdc::Dot3(x1, z2);

    for (int k = 0; k < 3; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }

    if (numRows > 3) {
        int nextRow = rowIndex + 3;
        JointAxisParams ap;
        ap.r1 = r1AtP2;
        ap.r2 = r2;
        ap.invBeta = params->invBeta;
        ap.angularThickness = kAngularThickness;
        ap.linearThickness = params->thickness;

        ap.axis = z1;
        ap.index = 0;
        nextRow += ComputeLinearAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);

        ap.axis = x1;
        ap.index = 1;
        nextRow += ComputeAngularAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);

        ap.axis = z2;
        ap.index = 2;
        ComputeAngularAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);
    }
}

// 0x00511e20 (PDB: 0x005138f0 / ?ComputeCartesian@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeCartesian(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);

    cdc::Vector3 x1, y1, z1, x2, y2, z2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    GetWorldAxes12(body1, body2, by1, by2, y1, y2);
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);

    cdc::Vector3 w0, w1, w2;
    cdc::Cross3(&w0, y2, z1);
    cdc::Cross3(&w1, x2, z1);
    cdc::Cross3(&w2, y2, x1);
    WriteAngularRow(J[rowIndex + 0], w0, body1);
    WriteAngularRow(J[rowIndex + 1], w1, body1);
    WriteAngularRow(J[rowIndex + 2], w2, body1);

    err[rowIndex + 0] = cdc::Dot3(y2, z1);
    err[rowIndex + 1] = cdc::Dot3(x2, z1);
    err[rowIndex + 2] = cdc::Dot3(y2, x1);
    for (int k = 0; k < 3; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }

    if (numRows > 3) {
        int nextRow = rowIndex + 3;
        JointAxisParams ap;
        ap.r1 = (body1 != nullptr) ? (p2 - body1->x) : ZeroVec();
        ap.r2 = r2;
        ap.invBeta = params->invBeta;
        ap.angularThickness = kAngularThickness;
        ap.linearThickness = 0.0f;

        const cdc::Vector3 axes[3] = {
            cdc::Vector3::Make(1.0f, 0.0f, 0.0f),
            cdc::Vector3::Make(0.0f, 1.0f, 0.0f),
            cdc::Vector3::Make(0.0f, 0.0f, 1.0f)
        };
        for (int i = 0; i < 3; ++i) {
            ap.axis = axes[i];
            ap.index = i;
            nextRow += ComputeLinearAxis(&J[nextRow], &err[nextRow], &vel[nextRow], &lo[nextRow], &hi[nextRow], ap);
        }
    }
}

// 0x0050bdc0 (PDB: 0x0050d890 / ?ComputeWeld@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeWeld(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, body1, r1, r2, p1, p2);

    cdc::Vector3 x1, y1, z1, x2, y2, z2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    GetWorldAxes12(body1, body2, by1, by2, y1, y2);
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);

    cdc::Vector3 w3, w4, w5;
    cdc::Cross3(&w3, y2, z1);
    cdc::Cross3(&w4, x2, z1);
    cdc::Cross3(&w5, y2, x1);
    WriteAngularRow(J[rowIndex + 3], w3, body1);
    WriteAngularRow(J[rowIndex + 4], w4, body1);
    WriteAngularRow(J[rowIndex + 5], w5, body1);

    err[rowIndex + 3] = cdc::Dot3(y2, z1);
    err[rowIndex + 4] = cdc::Dot3(x2, z1);
    err[rowIndex + 5] = cdc::Dot3(y2, x1);
    for (int k = 3; k < 6; ++k) {
        vel[rowIndex + k] = 0.0f;
        lo[rowIndex + k] = -FLT_MAX;
        hi[rowIndex + k] = FLT_MAX;
    }
}

// 0x0050c3e0 (PDB: 0x0050deb0 / ?ComputePointMotor@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputePointMotor(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, body1, r1, r2, p1, p2);

    for (int k = 0; k < 3; ++k) {
        err[rowIndex + k] = 0.0f;
        vel[rowIndex + k] = motor[k].velocity;
        lo[rowIndex + k] = -motor[k].maxForce;
        hi[rowIndex + k] = motor[k].maxForce;
    }
}

// 0x0050c690 (PDB: 0x0050e160 / ?ComputeLink@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeLink(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    const cdc::Vector3 z1 = body1->q.Rotate(bz1);
    const cdc::Vector3 z2 = body2->q.Rotate(bz2);
    ContactJacobianRow& row = J[rowIndex];

    float val1 = linkCoord1;
    if ((flags & LINK_POS1) != 0) {
        const cdc::Vector3 r1 = body1->q.Rotate(anchor1);
        const cdc::Vector3 p1 = body1->x + r1;
        val1 = cdc::Dot3(p1, z1) - linkCoord1;
        const cdc::Vector3 scaledZ1 = z1 * ratio;
        row.v1 = NegVec(scaledZ1);
        cdc::Cross3(&row.w1, scaledZ1, r1);
    } else {
        row.w1 = z1 * (-ratio);
    }

    float val2 = linkCoord2;
    if ((flags & LINK_POS2) != 0) {
        const cdc::Vector3 r2 = body2->q.Rotate(anchor2);
        const cdc::Vector3 p2 = body2->x + r2;
        val2 = cdc::Dot3(p2, z2) - linkCoord2;
        row.v2 = z2;
        cdc::Cross3(&row.w2, r2, z2);
    } else {
        row.w2 = z2;
    }

    err[rowIndex] = val2 - ratio * val1;
    vel[rowIndex] = 0.0f;
    lo[rowIndex] = -FLT_MAX;
    hi[rowIndex] = FLT_MAX;
}

// 0x0050c950 (PDB: 0x0050e420 / ?ComputeWheel@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeWheel(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    const cdc::Vector3 r1 = (body1 != nullptr) ? (anchor1 - body1->x) : ZeroVec();
    const cdc::Vector3 r2 = anchor1 - body2->x;
    WriteLinearRow(J[rowIndex + 0], bx2, body1, r1, r2);
    WriteLinearRow(J[rowIndex + 1], by2, body1, r1, r2);

    err[rowIndex + 0] = 0.0f;
    err[rowIndex + 1] = 0.0f;
    vel[rowIndex + 0] = 0.0f;
    vel[rowIndex + 1] = 0.0f;
    lo[rowIndex + 0] = -motor[0].maxForce;
    hi[rowIndex + 0] = motor[0].maxForce;
    lo[rowIndex + 1] = -motor[1].maxForce;
    hi[rowIndex + 1] = motor[1].maxForce;
}

// 0x005123e0 (PDB: 0x00513eb0 / ?ComputeWheel2@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeWheel2(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    const cdc::Vector3 r1 = (body1 != nullptr) ? (anchor1 - body1->x) : ZeroVec();
    const cdc::Vector3 r2 = anchor2 - body2->x;

    WriteLinearRow(J[rowIndex + 0], bx2, body1, r1, r2);
    WriteLinearRow(J[rowIndex + 1], by2, body1, r1, r2);
    WriteLinearRow(J[rowIndex + 2], bz2, body1, r1, r2);

    err[rowIndex + 0] = 0.0f;
    vel[rowIndex + 0] = 0.0f;
    lo[rowIndex + 0] = -motor[0].maxForce;
    hi[rowIndex + 0] = motor[0].maxForce;

    err[rowIndex + 1] = 0.0f;
    vel[rowIndex + 1] = 0.0f;
    lo[rowIndex + 1] = -motor[1].maxForce;
    hi[rowIndex + 1] = motor[1].maxForce;

    const float erp = linkCoord1 / (linkCoord1 * params->dt + linkCoord2);
    const float suspErr = 4.0f * params->thickness + length;
    err[rowIndex + 2] = 0.0f;
    vel[rowIndex + 2] = -((suspErr < 0.0f) ? suspErr : 0.0f) * erp;
    lo[rowIndex + 2] = 0.0f;
    hi[rowIndex + 2] = FLT_MAX;
}

// 0x0050cb60 (PDB: 0x0050e630 / ?ComputeConicalLimit@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeConicalLimit(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    cdc::Vector3 z1, z2;
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);
    if (limit[0].status == JointLimit::AT_LOW_LIMIT) {
        cdc::Vector3 axis;
        cdc::Cross3(&axis, z1, z2);
        WriteAngularRow(J[rowIndex], axis, body1);
        err[rowIndex] = limit[0].value - limit[0].low + kShoulderConeThickness;
        vel[rowIndex] = 0.0f;
        lo[rowIndex] = 0.0f;
        hi[rowIndex] = FLT_MAX;
    }
}

// 0x0050ccc0 (PDB: 0x0050e790 / ?ComputeAngularMotor@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeAngularMotor(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    WriteAngularRow(J[rowIndex], bz2, body1);
    err[rowIndex] = 0.0f;
    vel[rowIndex] = motor[0].velocity;
    lo[rowIndex] = -motor[0].maxForce;
    hi[rowIndex] = motor[0].maxForce;
}

// 0x00512710 (PDB: 0x005141e0 / ?ComputeSpline@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeSpline(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    SplineJointParams sjp;
    splineCallback(&sjp, reinterpret_cast<const Joint*>(this), params->dt);

    const cdc::Vector3 r2 = body2->q.Rotate(anchor2);
    const cdc::Vector3 p2 = body2->x + r2;
    const cdc::Vector3 v2 = ComputePointWorldVel(body2, r2);
    const cdc::Vector3 posErr = p2 - sjp.position;

    const_cast<JointImpl*>(this)->motor[0].velocity = cdc::Dot3(sjp.direction, v2);

    cdc::Vector3 dir = sjp.direction;
    if (!dir.IsZero3()) {
        dir.Normalize3();
        cdc::Vector3 u, v;
        ComputeBasis(&u, &v, dir);
        WriteLinearRow(J[rowIndex + 0], dir, nullptr, ZeroVec(), r2);
        WriteLinearRow(J[rowIndex + 1], u, nullptr, ZeroVec(), r2);
        WriteLinearRow(J[rowIndex + 2], v, nullptr, ZeroVec(), r2);

        err[rowIndex + 0] = 0.0f;
        vel[rowIndex + 0] = sjp.speed;
        lo[rowIndex + 0] = sjp.minMotorForce;
        hi[rowIndex + 0] = sjp.maxMotorForce;

        err[rowIndex + 1] = cdc::Dot3(posErr, u);
        err[rowIndex + 2] = cdc::Dot3(posErr, v);
        vel[rowIndex + 1] = 0.0f;
        vel[rowIndex + 2] = 0.0f;
        lo[rowIndex + 1] = -sjp.maxLateralForce;
        hi[rowIndex + 1] = sjp.maxLateralForce;
        lo[rowIndex + 2] = -sjp.maxLateralForce;
        hi[rowIndex + 2] = sjp.maxLateralForce;
    } else {
        WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, nullptr, ZeroVec(), r2, sjp.position, p2);
        for (int k = 0; k < 3; ++k) {
            lo[rowIndex + k] = -sjp.maxTorque;
            hi[rowIndex + k] = sjp.maxTorque;
        }
    }

    cdc::Quat dq = sjp.quaternion * (~body2->q);
    if (dq.w < 0.0f) {
        dq.x = -dq.x;
        dq.y = -dq.y;
        dq.z = -dq.z;
        dq.w = -dq.w;
    }

    const cdc::Vector3 axes[3] = {
        cdc::Vector3::Make(1.0f, 0.0f, 0.0f),
        cdc::Vector3::Make(0.0f, 1.0f, 0.0f),
        cdc::Vector3::Make(0.0f, 0.0f, 1.0f)
    };
    const float errs[3] = {-dq.x * 2.0f, -dq.y * 2.0f, -dq.z * 2.0f};
    for (int i = 0; i < 3; ++i) {
        WriteAngularRow(J[rowIndex + 3 + i], axes[i], nullptr);
        err[rowIndex + 3 + i] = errs[i];
        vel[rowIndex + 3 + i] = 0.0f;
        lo[rowIndex + 3 + i] = -sjp.maxTorque;
        hi[rowIndex + 3 + i] = sjp.maxTorque;
    }
}

// 0x00512ef0 (PDB: 0x005149e0 / ?ComputeShoulder@JointImpl@@ABEXPAVVector3@cdc@@PAM111HABUJointParams@@@Z)
void JointImpl::ComputeShoulder(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* /*params*/) const
{
    cdc::Vector3 r1, r2, p1, p2;
    GetWorldAnchors(*this, r1, r2, p1, p2);
    WriteTranslationRows3(J, err, vel, lo, hi, rowIndex, body1, r1, r2, p1, p2);

    cdc::Vector3 x1, y1, z1, x2, y2, z2;
    GetWorldAxes12(body1, body2, bx1, bx2, x1, x2);
    GetWorldAxes12(body1, body2, by1, by2, y1, y2);
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);

    cdc::Vector3 coneAxis;
    cdc::Cross3(&coneAxis, z1, z2);
    if (coneAxis.IsZero3()) {
        coneAxis = x1;
    } else {
        coneAxis.Normalize3();
    }

    WriteAngularRow(J[rowIndex + 3], coneAxis, body1);
    if (limit[0].status == JointLimit::AT_LOW_LIMIT) {
        err[rowIndex + 3] = limit[0].value - limit[0].low + kShoulderConeThickness;
        vel[rowIndex + 3] = 0.0f;
        lo[rowIndex + 3] = 0.0f;
        hi[rowIndex + 3] = FLT_MAX;
    } else {
        err[rowIndex + 3] = 0.0f;
        vel[rowIndex + 3] = 0.0f;
        lo[rowIndex + 3] = -motor[0].maxForce;
        hi[rowIndex + 3] = motor[0].maxForce;
    }

    WriteAngularRow(J[rowIndex + 4], z2, body1);
    if (limit[1].status == JointLimit::AT_LOW_LIMIT) {
        err[rowIndex + 4] = limit[1].value - limit[1].low + kAngularThickness;
        vel[rowIndex + 4] = 0.0f;
        lo[rowIndex + 4] = 0.0f;
        hi[rowIndex + 4] = FLT_MAX;
    } else if (limit[1].status == JointLimit::AT_HIGH_LIMIT) {
        err[rowIndex + 4] = limit[1].value - limit[1].high - kAngularThickness;
        vel[rowIndex + 4] = 0.0f;
        lo[rowIndex + 4] = -FLT_MAX;
        hi[rowIndex + 4] = 0.0f;
    } else {
        err[rowIndex + 4] = 0.0f;
        vel[rowIndex + 4] = 0.0f;
        lo[rowIndex + 4] = -motor[0].maxForce;
        hi[rowIndex + 4] = motor[0].maxForce;
    }

    cdc::Vector3 orthoAxis;
    cdc::Cross3(&orthoAxis, coneAxis, z2);
    orthoAxis.Normalize3();
    WriteAngularRow(J[rowIndex + 5], orthoAxis, body1);
    err[rowIndex + 5] = 0.0f;
    vel[rowIndex + 5] = 0.0f;
    lo[rowIndex + 5] = -motor[0].maxForce;
    hi[rowIndex + 5] = motor[0].maxForce;
}

// 0x00513560 (PDB: 0x00515050 / ?ComputeShoulderLimits@JointImpl@@AAEXPBUJointParams@@@Z)
void JointImpl::ComputeShoulderLimits(const JointParams* /*params*/)
{
    cdc::Vector3 z1, z2;
    GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);
    const float dotZ = cdc::Dot3(z1, z2);
    limit[0].value = dotZ;
    if (dotZ <= limit[0].low) {
        if (limit[0].status == JointLimit::NOT_AT_LIMIT) {
            lambda[3] = 0.0f;
        }
        limit[0].status = JointLimit::AT_LOW_LIMIT;
    } else {
        limit[0].status = JointLimit::NOT_AT_LIMIT;
    }

    const cdc::Vector3 x1 = (body1 != nullptr) ? body1->q.Rotate(bx1) : bx1;
    const cdc::Vector3 x2 = body2->q.Rotate(bx2);
    const cdc::Vector3 y2 = body2->q.Rotate(by2);
    const float twist = std::atan2(-cdc::Dot3(x1, y2), cdc::Dot3(x1, x2));
    UpdateAxisLimitState(this, 1, twist, ComputeRevoluteAngleSpeed(), 4, false, false);
}

// 0x00513780 (PDB: 0x00515270 / ?ComputeLimits@JointImpl@@QAEXPBUJointParams@@@Z)
void JointImpl::ComputeLimits(const JointParams* params)
{
    for (int i = 0; i < 6; ++i) {
        lambda[i] *= relaxation;
    }

    numRows = numRowsNoLimits;

    switch (type) {
    case Joint::RevoluteJoint:
        if ((flags & LIMIT1) != 0) {
            const float angle = WrapAngleToLimits(ComputeRevoluteAngle(), limit[0].low, limit[0].high);
            UpdateAxisLimitState(this, 0, angle, ComputeRevoluteAngleSpeed(), 5, true);
        }
        break;

    case Joint::PrismaticJoint:
        if ((flags & LIMIT1) != 0) {
            UpdateAxisLimitState(this, 0, ComputePrismaticTranslation(), ComputePrismaticTranslationSpeed(), 5, true);
        }
        break;

    case Joint::PlanarJoint: {
        if ((flags & LIMIT1) != 0) {
            UpdateAxisLimitState(this, 0, ComputePlanarTranslation1(), ComputePlanarTranslationSpeed1(), 3, true);
        }
        if ((flags & LIMIT2) != 0) {
            UpdateAxisLimitState(this, 1, ComputePlanarTranslation2(), ComputePlanarTranslationSpeed2(), 4, true);
        }
        if ((flags & LIMIT3) != 0) {
            const float angle = WrapAngleToLimits(ComputeRevoluteAngle(), limit[2].low, limit[2].high);
            UpdateAxisLimitState(this, 2, angle, ComputeRevoluteAngleSpeed(), 5, true);
        }
        break;
    }

    case Joint::UniversalJoint:
        if ((flags & LIMIT1) != 0) {
            UpdateAxisLimitState(this, 0, ComputeUniversalAngle1(), ComputeUniversalAngleSpeed1(), 4, true);
        }
        if ((flags & LIMIT2) != 0) {
            UpdateAxisLimitState(this, 1, ComputeUniversalAngle2(), ComputeRevoluteAngleSpeed(), 5, true);
        }
        break;

    case Joint::SwingJoint: {
        if ((flags & LIMIT1) != 0) {
            UpdateAxisLimitState(this, 0, ComputeSwingTranslation(), ComputeSwingTranslationSpeed(), 3, true);
        }
        if ((flags & LIMIT2) != 0) {
            const float a1 = WrapAngleToLimits(ComputeSwingAngle1(), limit[1].low, limit[1].high);
            UpdateAxisLimitState(this, 1, a1, ComputeSwingAngleSpeed1(), 4, true);
        }
        if ((flags & LIMIT3) != 0) {
            UpdateAxisLimitState(this, 2, ComputeSwingAngle2(), ComputeRevoluteAngleSpeed(), 5, true);
        }
        break;
    }

    case Joint::CartesianJoint: {
        const cdc::Vector3 t = ComputeCartesianTranslation();
        const cdc::Vector3 v = ComputeCartesianTranslationSpeed();
        if ((flags & LIMIT1) != 0) {
            UpdateAxisLimitState(this, 0, t.x, v.x, 3, true);
        }
        if ((flags & LIMIT2) != 0) {
            UpdateAxisLimitState(this, 1, t.y, v.y, 4, true);
        }
        if ((flags & LIMIT3) != 0) {
            UpdateAxisLimitState(this, 2, t.z, v.z, 5, true);
        }
        break;
    }

    case Joint::WheelJoint2:
        gamma[2] = 1.0f / (linkCoord1 * params->dt + linkCoord2);
        break;

    case Joint::ConicalLimit: {
        cdc::Vector3 z1, z2;
        GetWorldAxes12(body1, body2, bz1, bz2, z1, z2);
        const float dotZ = cdc::Dot3(z1, z2);
        limit[0].value = dotZ;
        if (dotZ <= limit[0].low) {
            if (limit[0].status == JointLimit::NOT_AT_LIMIT) {
                const cdc::Vector3 w = (body1 != nullptr) ? (body2->omega - body1->omega) : body2->omega;
                cdc::Vector3 axis;
                cdc::Cross3(&axis, z1, z2);
                FireLimitCallback(this, cdc::Dot3(axis, w), 0, LimitReport::HIT_LOWER_LIMIT);
            }
            limit[0].status = JointLimit::AT_LOW_LIMIT;
            numRows = 1;
        } else {
            limit[0].status = JointLimit::NOT_AT_LIMIT;
            lambda[0] = 0.0f;
            numRows = 0;
        }
        break;
    }

    case Joint::ShoulderJoint:
        ComputeShoulderLimits(params);
        break;

    default:
        break;
    }
}

// 0x00514640 (PDB: 0x00516130 / ?ComputeJacobian@JointImpl@@QBEXQAVVector3@cdc@@QAM111HPBUJointParams@@@Z)
void JointImpl::ComputeJacobian(
    ContactJacobianRow* J,
    float* err,
    float* vel,
    float* lo,
    float* hi,
    int rowIndex,
    const JointParams* params) const
{
    switch (type) {
    case Joint::RevoluteJoint:
        ComputeRevolute(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::PrismaticJoint:
        ComputePrismatic(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::PlanarJoint:
        ComputePlanar(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::SphericalJoint:
        ComputeSpherical(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::RopeJoint:
        ComputeRope(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::PulleyJoint:
        ComputePulley(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::UniversalJoint:
        ComputeUniversal(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::SwingJoint:
        ComputeSwing(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::CartesianJoint:
        ComputeCartesian(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::WeldJoint:
        ComputeWeld(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::PointMotor:
        ComputePointMotor(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::LinkJoint:
        ComputeLink(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::WheelJoint:
        ComputeWheel(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::WheelJoint2:
        ComputeWheel2(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::ConicalLimit:
        ComputeConicalLimit(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::AngularMotor:
        ComputeAngularMotor(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::SplineJoint:
        ComputeSpline(J, err, vel, lo, hi, rowIndex, params);
        break;
    case Joint::ShoulderJoint:
        ComputeShoulder(J, err, vel, lo, hi, rowIndex, params);
        break;
    default:
        break;
    }
}
