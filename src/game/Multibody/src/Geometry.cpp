#include "game/Multibody/src/Geometry.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include "game/Multibody/collision/CollideUtils.h"
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/Buoyancy.h"
#include "game/Multibody/src/MultibodyTypes.h"
#include "game/Multibody/src/SweepAndPrune.h"

namespace {

inline cdc::Vector3 ComputeAbsRotatedExtents(const cdc::Quat& rot, const cdc::Vector3& halfExtents)
{
    cdc::Matrix mat;
    mat.Build(rot);
    cdc::Matrix absMat;
    absMat.col0 = MultibodyCollision::AbsVector3(
        cdc::Vector3::Make(mat.col0.x, mat.col0.y, mat.col0.z, mat.col0.w));
    absMat.col1 = MultibodyCollision::AbsVector3(
        cdc::Vector3::Make(mat.col1.x, mat.col1.y, mat.col1.z, mat.col1.w));
    absMat.col2 = MultibodyCollision::AbsVector3(
        cdc::Vector3::Make(mat.col2.x, mat.col2.y, mat.col2.z, mat.col2.w));
    absMat.col3 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 1.0f);

    const cdc::Vector res = cdc::Mul3x3(absMat, static_cast<const cdc::Vector&>(halfExtents));
    return cdc::Vector3::Make(res.x, res.y, res.z, 0.0f);
}

inline void ComputeSphereSweptBounds(
    const cdc::Vector3& x,
    const cdc::Vector3& x0,
    const cdc::Vector3& halfSize,
    bool swept,
    cdc::Vector3& outLo,
    cdc::Vector3& outHi)
{
    if (swept) {
        outLo = cdc::Vector3::Make(
            std::min(x0.x, x.x) - halfSize.x,
            std::min(x0.y, x.y) - halfSize.y,
            std::min(x0.z, x.z) - halfSize.z,
            0.0f);
        outHi = cdc::Vector3::Make(
            std::max(x0.x, x.x) + halfSize.x,
            std::max(x0.y, x.y) + halfSize.y,
            std::max(x0.z, x.z) + halfSize.z,
            0.0f);
    } else {
        outLo = x - halfSize;
        outHi = x + halfSize;
    }
}

inline float ComputeCapsuleVolume(float radius, float length)
{
    constexpr float kFourPiOverThree = 4.188790321350098f;
    constexpr float kTwoPi = 6.2831854820251465f;
    return (kFourPiOverThree * radius + kTwoPi * length) * radius * radius;
}

inline bool ComputeShapeBounds(
    const GeometryImpl* self,
    bool allowSweptSphere,
    cdc::Vector3& outLo,
    cdc::Vector3& outHi)
{
    switch (self->type) {
    case Geometry::Box: {
        const cdc::Vector3 absExt = ComputeAbsRotatedExtents(self->q, self->width);
        const cdc::Vector3 pad = cdc::Vector3::Make(1.0f, 1.0f, 1.0f, 0.0f);
        const cdc::Vector3 halfSize = absExt * 0.5f + pad;
        outLo = self->x - halfSize;
        outHi = self->x + halfSize;
        return true;
    }
    case Geometry::Capsule: {
        const cdc::Vector3 zAxis = self->q.Rotate(cdc::Vector3::Make(0.0f, 0.0f, 1.0f, 0.0f));
        const cdc::Vector3 absZ = MultibodyCollision::AbsVector3(zAxis);
        const float padR = self->radius + 1.0f;
        const cdc::Vector3 padVec = cdc::Vector3::Make(padR, padR, padR, 0.0f);
        const cdc::Vector3 halfSize = absZ * (0.5f * self->length) + padVec;
        outLo = self->x - halfSize;
        outHi = self->x + halfSize;
        return true;
    }
    case Geometry::Plane:
        if (allowSweptSphere) {
            return false;
        }
        outLo = cdc::Vector3::Make(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);
        outHi = cdc::Vector3::Make(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        return true;
    case Geometry::Sphere: {
        const float padR = self->radius + 1.0f;
        const cdc::Vector3 halfSize = cdc::Vector3::Make(padR, padR, padR, 0.0f);
        ComputeSphereSweptBounds(
            self->x, self->x0, halfSize, allowSweptSphere && (self->flags & 0x40) != 0, outLo, outHi);
        return true;
    }
    default:
        return !allowSweptSphere;
    }
}

inline void SyncPoseFromBody(GeometryImpl* self)
{
    if (self->body != nullptr) {
        self->x = self->body->x + self->body->q.Rotate(self->dx);
        self->q = cdc::QuatMul(self->body->q, self->dq);
    } else {
        self->x = self->dx;
        self->q = self->dq;
    }
}

} // namespace

// 0x00508790 (PDB: 0x0050a260 / ?GetType@Geometry@@QBE?AW4Type@1@XZ)
Geometry::Type Geometry::GetType() const
{
    return static_cast<Type>(static_cast<const GeometryImpl*>(this)->type);
}

// 0x005087a0 (PDB: 0x0050a270 / ?SetFlags@Geometry@@QAEXE@Z)
void Geometry::SetFlags(uint8_t newFlags)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    self->flags = newFlags;
    if (self->aabb != nullptr) {
        self->aabb->flags = newFlags;
    }
}

// 0x005087d0 (PDB: 0x0050a2a0 / ?GetFlags@Geometry@@QBEEXZ)
uint8_t Geometry::GetFlags() const
{
    return static_cast<const GeometryImpl*>(this)->flags;
}

// 0x005087e0 (PDB: 0x0050a2b0 / ?SetClientData@Geometry@@QAEXPAX@Z)
void Geometry::SetClientData(void* data)
{
    static_cast<GeometryImpl*>(this)->clientData = data;
}

// 0x005087f0 (PDB: 0x0050a2c0 / ?GetClientData@Geometry@@QBEPAXXZ)
void* Geometry::GetClientData() const
{
    return static_cast<const GeometryImpl*>(this)->clientData;
}

// 0x00508800 (PDB: 0x0050a2d0 / ?SetCategory@Geometry@@QAEXE@Z)
void Geometry::SetCategory(uint8_t newCategory)
{
    static_cast<GeometryImpl*>(this)->category = newCategory;
}

// 0x00508830 (PDB: 0x0050a300 / ?GetCategory@Geometry@@QBEEXZ)
uint8_t Geometry::GetCategory() const
{
    return static_cast<const GeometryImpl*>(this)->category;
}

// 0x00508840 (PDB: 0x0050a310 / ?GetSphereRadius@Geometry@@QBEMXZ / ?GetCapsuleRadius@Geometry@@QBEMXZ)
float Geometry::GetSphereRadius() const
{
    return static_cast<const GeometryImpl*>(this)->radius;
}

float Geometry::GetCapsuleRadius() const
{
    return static_cast<const GeometryImpl*>(this)->radius;
}

// 0x00508850 (PDB: 0x0050a320 / ?GetCapsuleLength@Geometry@@QBEMXZ)
float Geometry::GetCapsuleLength() const
{
    return static_cast<const GeometryImpl*>(this)->length;
}

// 0x00508860 (PDB: 0x0050a330 / ?GetGenericRadius@Geometry@@QBEMXZ)
float Geometry::GetGenericRadius() const
{
    const GeometryImpl* self = static_cast<const GeometryImpl*>(this);
    if (self->type == Box) {
        return std::sqrt(
            self->width.x * self->width.x +
            self->width.y * self->width.y +
            self->width.z * self->width.z);
    }
    if (self->type <= Sphere) {
        return self->radius;
    }
    return 0.0f;
}

// PDB: 0x00479550 / ?GetBoxWidth@Geometry@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Geometry::GetBoxWidth() const
{
    return static_cast<const GeometryImpl*>(this)->width;
}

// 0x005088d0 (PDB: 0x0050a3a0 / ?GetSurfaceArea@Geometry@@QBEMXZ)
float Geometry::GetSurfaceArea() const
{
    const GeometryImpl* self = static_cast<const GeometryImpl*>(this);
    switch (self->type) {
    case Box:
        return 2.0f * (
            (self->width.x + self->width.y) * self->width.z +
            self->width.x * self->width.y);
    case Capsule: {
        constexpr float kTwoPi = 6.2831854820251465f;
        return (2.0f * self->radius + self->length) * self->radius * kTwoPi;
    }
    case Plane:
        return FLT_MAX;
    case Sphere: {
        constexpr float kFourPi = 12.566370964050293f;
        return self->radius * self->radius * kFourPi;
    }
    default:
        return 0.0f;
    }
}

// 0x00508980 (PDB: 0x0050a450 / ?SetFriction@Geometry@@QAEXM@Z)
void Geometry::SetFriction(float friction)
{
    static_cast<GeometryImpl*>(this)->mu = friction;
}

// 0x00508990 (PDB: 0x0050a460 / ?GetFriction@Geometry@@QBEMXZ)
float Geometry::GetFriction() const
{
    return static_cast<const GeometryImpl*>(this)->mu;
}

// 0x005089a0 (PDB: 0x0050a470 / ?SetRestitution@Geometry@@QAEXM@Z)
void Geometry::SetRestitution(float newRestitution)
{
    static_cast<GeometryImpl*>(this)->restitution = newRestitution;
}

// 0x005089b0 (PDB: 0x0050a480 / ?GetQuaternion@Geometry@@QBE?AVQuat@cdc@@XZ)
cdc::Quat Geometry::GetQuaternion() const
{
    return static_cast<const GeometryImpl*>(this)->q;
}

// PDB: 0x005e5440 / ?GetPosition@Geometry@@QBE?AVVector3@cdc@@XZ
cdc::Vector3 Geometry::GetPosition() const
{
    return static_cast<const GeometryImpl*>(this)->x;
}

// PDB: 0x004e3520 / ?GetBody@Geometry@@QAEPAVBody@@XZ
Body* Geometry::GetBody()
{
    return static_cast<GeometryImpl*>(this)->body;
}

// 0x005089e0 (PDB: 0x0050a4b0 / ?Clear@GeometryImpl@@QAEXXZ)
void GeometryImpl::Clear()
{
    dx = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    x = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    x0 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    dq = cdc::Quat::Identity();
    q = cdc::Quat::Identity();
    body = nullptr;
    bnext = nullptr;
    clientData = nullptr;
    clientDestruct = nullptr;
    type = None;
    restitution = 0.0f;
    mu = 1.0f;
    radius = 0.0f;
    sweepAndPrune = nullptr;
    aabb = nullptr;
    category = 1;
    flags = 0x0f;
}

// 0x00508a90 (PDB: 0x0050a560 / ?CheckTunnelRisk@GeometryImpl@@QBE_NABVVector3@cdc@@ABVQuat@3@@Z)
bool GeometryImpl::CheckTunnelRisk(const cdc::Vector3& pos, const cdc::Quat& rot) const
{
    if ((flags & 0x03) == 0 || (flags & 0x38) == 0) {
        return false;
    }
    if ((flags & 0x20) != 0) {
        return true;
    }

    const cdc::Vector3 nextPos = pos + rot.Rotate(dx);
    const cdc::Vector3 delta = nextPos - x;
    const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    const float thresh = 1.5f * radius;
    return distSq > thresh * thresh;
}

// 0x00508b50 (PDB: 0x0050a620 / ?AddBuoyancyForce@Geometry@@QAEXABVVector3@cdc@@MM0MM@Z)
void Geometry::AddBuoyancyForce(
    const cdc::Vector3& planeNormal,
    float planeDist,
    float liquidDensity,
    const cdc::Vector3& liquidVelocity,
    float linearDrag,
    float angularDrag)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    BodyImpl* body = self->body;
    if (body == nullptr) {
        return;
    }

    cdc::Vector3 subCenter = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    float subVol = 0.0f;
    float effectiveRadius = 0.0f;

    switch (self->type) {
    case Box:
        subVol = BoxBuoyancy(&subCenter, self->x, self->q, self->width, planeNormal, planeDist);
        effectiveRadius = std::sqrt(
            self->width.x * self->width.x +
            self->width.y * self->width.y +
            self->width.z * self->width.z);
        break;
    case Capsule:
    case Sphere:
        subVol = SphereBuoyancy(&subCenter, self->x, planeNormal, planeDist, self->radius);
        effectiveRadius = self->radius;
        break;
    default:
        return;
    }

    if (subVol <= 0.0f) {
        return;
    }

    const float clampedVol = subVol < self->volume ? subVol : self->volume;
    const cdc::Vector3 buoyancyForce =
        planeNormal * (clampedVol * liquidDensity * body->multibody->gravityMagnitude);
    const float submergedMass = (body->mass / self->volume) * clampedVol;

    const cdc::Vector3 relVel = liquidVelocity - body->GlobalPointVelocity(subCenter);
    const cdc::Vector3 totalForce = buoyancyForce + relVel * (submergedMass * linearDrag);
    body->AddForce(totalForce, subCenter);

    const cdc::Vector3 omega = body->GetAngularVelocity();
    const float angDragScale = -angularDrag * submergedMass * effectiveRadius * effectiveRadius;
    body->AddTorque(omega * angDragScale);
}

// 0x00508cf0 (PDB: 0x0050a7c0 / ?GetClosestPoint@Geometry@@QBEXPAVVector3@cdc@@0ABV23@@Z)
void Geometry::GetClosestPoint(
    cdc::Vector3* outPoint,
    cdc::Vector3* outNormal,
    const cdc::Vector3& point) const
{
    const GeometryImpl* self = static_cast<const GeometryImpl*>(this);
    switch (self->type) {
    case Box:
        PointToBoxDistance(outPoint, outNormal, point, self->x, self->q, self->width * 0.5f);
        break;
    case Capsule: {
        const cdc::Vector3 halfAxis = self->q.Rotate(cdc::Vector3::Make(0.0f, 0.0f, 0.5f * self->length, 0.0f));
        cdc::Vector3 segPt;
        PointToSegmentDistance(nullptr, &segPt, point, self->x - halfAxis, self->x + halfAxis);

        cdc::Vector3 delta = point - segPt;
        const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (distSq > self->radius * self->radius) {
            delta.Normalize3();
            *outNormal = delta;
            *outPoint = segPt + delta * self->radius;
        } else {
            *outPoint = point;
            *outNormal = cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
        }
        break;
    }
    case Plane: {
        const cdc::Vector3 planeNormal = self->width;
        const float dist =
            planeNormal.x * point.x +
            planeNormal.y * point.y +
            planeNormal.z * point.z -
            self->width.w;
        const float clampedDist = dist < 0.0f ? 0.0f : dist;
        *outPoint = point - planeNormal * clampedDist;
        *outNormal = planeNormal;
        break;
    }
    case Sphere: {
        cdc::Vector3 delta = point - self->x;
        const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (distSq > self->radius * self->radius) {
            delta.Normalize3();
            *outNormal = delta;
            *outPoint = self->x + delta * self->radius;
        } else {
            *outPoint = point;
            *outNormal = cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
        }
        break;
    }
    default:
        *outPoint = point;
        *outNormal = cdc::Vector3::Make(1.0f, 0.0f, 0.0f, 0.0f);
        break;
    }
}

// 0x005090b0 (PDB: 0x0050ab80 / ?CreateAABB@GeometryImpl@@QAEXXZ)
void GeometryImpl::CreateAABB()
{
    if (aabb != nullptr) {
        sweepAndPrune->DestroyBox(aabb);
        aabb = nullptr;
    }

    SyncPoseFromBody(this);
    x0 = x;

    cdc::Vector3 lo = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    cdc::Vector3 hi = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    ComputeShapeBounds(this, false, lo, hi);

    const unsigned int staticFlag = (body == nullptr && (flags & 0x40) == 0) ? 1u : 0u;
    aabb = sweepAndPrune->CreateBox(lo, hi, staticFlag);
    aabb->clientData = this;
}

// 0x005093d0 (PDB: 0x0050aea0 / ?UpdatePosition@GeometryImpl@@QAEXXZ)
void GeometryImpl::UpdatePosition()
{
    SyncPoseFromBody(this);

    cdc::Vector3 lo = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    cdc::Vector3 hi = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    if (!ComputeShapeBounds(this, true, lo, hi)) {
        return;
    }

    sweepAndPrune->MoveBox(aabb, lo, hi);
}

// 0x00509760 (PDB: 0x0050b230 / ?SetBox@Geometry@@QAEXABVVector3@cdc@@@Z)
void Geometry::SetBox(const cdc::Vector3& newWidth)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    self->type = Box;
    self->width = newWidth;

    float minDim = newWidth.x < newWidth.y ? newWidth.x : newWidth.y;
    if (newWidth.z < minDim) {
        minDim = newWidth.z;
    }
    self->radius = 0.5f * minDim;
    self->volume = newWidth.x * newWidth.y * newWidth.z;
    self->CreateAABB();
}

// 0x00509800 (PDB: 0x0050b2d0 / ?SetSphere@Geometry@@QAEXM@Z)
void Geometry::SetSphere(float newRadius)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    self->type = Sphere;
    self->radius = newRadius;
    constexpr float kFourPiOverThree = 4.188790321350098f;
    self->volume = kFourPiOverThree * newRadius * newRadius * newRadius;
    self->CreateAABB();
}

// 0x00509830 (PDB: 0x0050b300 / ?SetCapsule@Geometry@@QAEXMM@Z)
void Geometry::SetCapsule(float newRadius, float newLength)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    self->type = Capsule;
    self->radius = newRadius;
    self->length = newLength;
    self->volume = ComputeCapsuleVolume(newRadius, newLength);
    self->CreateAABB();
}

// 0x00509870 (PDB: 0x0050b340 / ?SetCapsuleRadius@Geometry@@QAEXM@Z)
void Geometry::SetCapsuleRadius(float newRadius)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    self->radius = newRadius;
    self->volume = ComputeCapsuleVolume(newRadius, self->length);
    self->UpdatePosition();
}

// 0x005098a0 (PDB: 0x0050b370 / ?SetCapsuleLength@Geometry@@QAEXM@Z)
void Geometry::SetCapsuleLength(float newLength)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    self->length = newLength;
    self->volume = ComputeCapsuleVolume(self->radius, newLength);
    self->UpdatePosition();
}

// 0x005098d0 (PDB: 0x0050b3a0 / ?SetRelativePosition@Geometry@@QAEXABVVector3@cdc@@@Z)
void Geometry::SetRelativePosition(const cdc::Vector3& relPos)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    if (relPos.IsValid()) {
        self->dx = relPos;
        self->UpdatePosition();
    }
}

// 0x00509930 (PDB: 0x0050b400 / ?SetRelativeQuaternion@Geometry@@QAEXABVQuat@cdc@@@Z)
void Geometry::SetRelativeQuaternion(const cdc::Quat& relRot)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    if (relRot.IsValid()) {
        self->dq = relRot;
        self->UpdatePosition();
    }
}

// 0x00509970 (PDB: 0x0050b440 / ?MoveTo@Geometry@@QAEXABVVector3@cdc@@@Z)
void Geometry::MoveTo(const cdc::Vector3& pos)
{
    GeometryImpl* self = static_cast<GeometryImpl*>(this);
    if (!pos.IsValid()) {
        return;
    }
    self->flags |= 0x40;
    self->x0 = self->x;
    self->dx = pos;
    self->UpdatePosition();
}
