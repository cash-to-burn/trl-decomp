#include "game/math3d.h"
#include "game/g2/quatg2.h"
#include <cmath>

namespace {

constexpr float kPi         = 3.14159274f;          // 0x00f003dc
constexpr float kTwoPi      = 6.28318548f;          // 0x00f003e0
constexpr float kHalfPi     = 1.57079637f;          // 0x00f003e4
constexpr float kInvTwoPi   = 0.1591549367f;        // 0x00efe4dc
constexpr float kLogEpsilon = 1.0e-8f;              // 0x00f003f8
constexpr float kMinBendRad = 0.000174532935f;      // 0x00f00400 (0.01 deg)

inline float WrapAngle0ToTwoPi(float angle)
{
    float turns = angle * kInvTwoPi;
    return (turns - std::floor(turns)) * kTwoPi;
}

inline cdc::Vector3 MakeVec3(float x, float y, float z, float w = 0.0f)
{
    cdc::Vector3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

inline cdc::Vector3 VecSub3(const cdc::Vector& a, const cdc::Vector& b)
{
    return MakeVec3(a.x - b.x, a.y - b.y, a.z - b.z, 0.0f);
}

inline cdc::Vector3 VecAdd3(const cdc::Vector& a, const cdc::Vector& b)
{
    return MakeVec3(a.x + b.x, a.y + b.y, a.z + b.z, 0.0f);
}

inline float VecDot3(const cdc::Vector& a, const cdc::Vector& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline cdc::Vector3 VecCross3(const cdc::Vector& a, const cdc::Vector& b)
{
    return MakeVec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
        0.0f
    );
}

// 0x005dec50 (PDB: 0x005e1680 / cdc::Vector::UnitVec3)
inline cdc::Vector3 UnitVec3(const cdc::Vector& v)
{
    float lenSq = VecDot3(v, v);
    if (lenSq <= 0.0f) {
        return MakeVec3(0.0f, 0.0f, 0.0f, 0.0f);
    }
    float invLen = 1.0f / std::sqrt(lenSq);
    return MakeVec3(v.x * invLen, v.y * invLen, v.z * invLen, 0.0f);
}

// 0x004275c0
inline float ClampedAcosf(float value)
{
    if (value < -1.0f) {
        value = -1.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return std::acos(value);
}

// 0x005df210 (PDB: 0x005e1c40 / cdc::Matrix::Build)
inline void BuildMatrixFromEuler(cdc::Matrix* m, const cdc::Euler& e)
{
    if (e.x == 0.0f && e.y == 0.0f && e.z == 0.0f) {
        m->col0 = MakeVec3(1.0f, 0.0f, 0.0f, 0.0f);
        m->col1 = MakeVec3(0.0f, 1.0f, 0.0f, 0.0f);
        m->col2 = MakeVec3(0.0f, 0.0f, 1.0f, 0.0f);
        m->col3 = MakeVec3(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float cx = std::cos(e.x);
    float cy = std::cos(e.y);
    float cz = std::cos(e.z);
    float sx = std::sin(e.x);
    float sy = std::sin(e.y);
    float sz = std::sin(e.z);

    float sy_sx = sy * sx;
    float sy_cx = sy * cx;

    m->col0.x = cy * cz;
    m->col0.y = sz * cy;
    m->col0.z = -sy;
    m->col0.w = 0.0f;

    m->col1.x = sy_sx * cz - sz * cx;
    m->col1.y = sy_sx * sz + cz * cx;
    m->col1.z = cy * sx;
    m->col1.w = 0.0f;

    m->col2.x = sy_cx * cz + sz * sx;
    m->col2.y = sy_cx * sz - cz * sx;
    m->col2.z = cx * cy;
    m->col2.w = 0.0f;

    m->col3.x = 0.0f;
    m->col3.y = 0.0f;
    m->col3.z = 0.0f;
    m->col3.w = 1.0f;
}

// 0x005df360 (PDB: 0x005e1d90 / cdc::Quat::Build axis-angle)
inline void QuatBuildAxisAngle(cdc::Quat* q, const cdc::Vector3& axis, float angle)
{
    cdc::Vector3 n = UnitVec3(axis);
    float halfAngle = angle * 0.5f;
    float s = std::sin(halfAngle);
    q->x = n.x * s;
    q->y = n.y * s;
    q->z = n.z * s;
    q->w = std::cos(halfAngle);
}

// 0x005de8d0 (PDB: 0x005e1300 / cdc::Quat::Rotate)
inline cdc::Vector3 QuatRotate(const cdc::Quat& q, const cdc::Vector3& v)
{
    cdc::Vector3 qv = MakeVec3(q.x, q.y, q.z, 0.0f);
    cdc::Vector3 uv = VecCross3(qv, v);
    cdc::Vector3 uuv = VecCross3(qv, uv);
    float twoW = 2.0f * q.w;
    return MakeVec3(
        v.x + uv.x * twoW + 2.0f * uuv.x,
        v.y + uv.y * twoW + 2.0f * uuv.y,
        v.z + uv.z * twoW + 2.0f * uuv.z,
        0.0f
    );
}

// 0x005defa0 (PDB: 0x005e19d0 / cdc::Quat::Build two vectors)
inline void QuatBuildBetweenVectors(cdc::Quat* q, const cdc::Vector3& from, const cdc::Vector3& to)
{
    cdc::Vector3 u0 = UnitVec3(from);
    cdc::Vector3 u1 = UnitVec3(to);
    float dot = VecDot3(u0, u1);
    if (dot >= 1.0f) {
        q->x = 0.0f;
        q->y = 0.0f;
        q->z = 0.0f;
        q->w = 1.0f;
        return;
    }

    cdc::Vector3 c = VecCross3(u0, u1);
    float cLenSq = VecDot3(c, c);
    if (dot < -0.999999f || cLenSq < 1.0e-12f) {
        cdc::Vector3 ortho = (std::fabs(u0.x) < 0.9f)
            ? VecCross3(u0, MakeVec3(1.0f, 0.0f, 0.0f))
            : VecCross3(u0, MakeVec3(0.0f, 1.0f, 0.0f));
        ortho = UnitVec3(ortho);
        q->x = ortho.x;
        q->y = ortho.y;
        q->z = ortho.z;
        q->w = 0.0f;
        return;
    }

    float s = std::sqrt((1.0f + dot) * 2.0f);
    float invS = 1.0f / s;
    q->x = c.x * invS;
    q->y = c.y * invS;
    q->z = c.z * invS;
    q->w = s * 0.5f;
}

} // namespace

// 0x0045f690 (PDB: 0x00460e40)
void MATH3D_QuatLogToMatrix(cdc::Matrix* mat, const cdc::Quat* q)
{
    float xx = q->x * q->x;
    float yy = q->y * q->y;
    float zz = q->z * q->z;
    float len = std::sqrt(xx + yy + zz);

    float halfLen = 0.5f * len;
    float sinHalf = std::sin(halfLen);
    float cosHalf = std::cos(halfLen);

    float scale = 0.0f;
    if (len > kLogEpsilon || len < -kLogEpsilon) {
        scale = sinHalf / len;
    }

    float scaleSq = scale * scale;
    float xxS = xx * scaleSq;
    float yyS = yy * scaleSq;
    float zzS = zz * scaleSq;
    float xyS = q->y * q->x * scaleSq;
    float xzS = q->z * q->x * scaleSq;
    float yzS = q->z * q->y * scaleSq;
    float wx = cosHalf * q->x * scale;
    float wy = cosHalf * q->y * scale;
    float wz = cosHalf * q->z * scale;

    mat->col0.x = 1.0f - 2.0f * (zzS + yyS);
    mat->col0.y = 2.0f * (wz + xyS);
    mat->col0.z = 2.0f * (xzS - wy);
    mat->col0.w = 0.0f;

    mat->col1.x = 2.0f * (xyS - wz);
    mat->col1.y = 1.0f - 2.0f * (zzS + xxS);
    mat->col1.z = 2.0f * (wx + yzS);
    mat->col1.w = 0.0f;

    mat->col2.x = 2.0f * (wy + xzS);
    mat->col2.y = 2.0f * (yzS - wx);
    mat->col2.z = 1.0f - 2.0f * (yyS + xxS);
    mat->col2.w = 0.0f;

    mat->col3.w = 1.0f;
}

// 0x0045f800 (PDB: 0x00460fb0)
float MATH3D_AngleFromVector(const cdc::Vector3* vec)
{
    float angle = std::atan2(-vec->x, -vec->y) - kHalfPi;
    return WrapAngle0ToTwoPi(angle);
}

// 0x0045f840 (PDB: 0x00460ff0)
float MATH3D_AngleFromPosToPosf(const cdc::Vector3* from, const cdc::Vector3* to)
{
    float angle = std::atan2(from->x - to->x, from->y - to->y) - kHalfPi;
    return WrapAngle0ToTwoPi(angle);
}

// 0x0045f880 (PDB: 0x00461030)
float MATH3D_ElevationFromPosToPosf(const cdc::Vector3* from, const cdc::Vector3* to)
{
    float dx = from->x - to->x;
    float dy = from->y - to->y;
    float dz = from->z - to->z;
    float horizLen = std::sqrt(dx * dx + dy * dy);
    float angle = -std::atan2(-dz, horizLen);
    return WrapAngle0ToTwoPi(angle);
}

// 0x0045f900 (PDB: 0x004610b0)
float MATH3D_ElevationFromVector(const cdc::Vector3* vec)
{
    float horizLen = std::sqrt(vec->x * vec->x + vec->y * vec->y);
    float angle = -std::atan2(vec->z, horizLen);
    return WrapAngle0ToTwoPi(angle);
}

// 0x0045f970 (PDB: 0x00461120)
void MATH3D_RotationFromPosToPosf(const cdc::Vector3* from, const cdc::Vector3* to, cdc::Euler* rot)
{
    rot->x = MATH3D_ElevationFromPosToPosf(from, to);
    rot->y = 0.0f;
    rot->z = MATH3D_AngleFromPosToPosf(from, to);
}

// 0x0045f9d0 (PDB: 0x00461180)
void MATH3D_ZYXtoXYZf(cdc::Euler* euler)
{
    cdc::Matrix mat;
    BuildMatrixFromEuler(&mat, *euler);

    _G2EulerAngles_Type g2Euler;
    G2EulerAngles_FromMatrixf(&g2Euler, &mat, 0x15);

    euler->x = g2Euler.x;
    euler->y = g2Euler.y;
    euler->z = g2Euler.z;
}

// 0x0045fa20 (PDB: 0x004611d0)
float MATH3D_AngleDifff(float current, float destination)
{
    float diff = WrapAngle0ToTwoPi(destination - current);
    if (diff > kPi) {
        diff -= kTwoPi;
    }
    return diff;
}

// 0x0045fa60 (PDB: 0x00461210)
int MATH3D_CheckAngleInRange(float zfacemin, float zfacemax, float ztest)
{
    float minAngle = WrapAngle0ToTwoPi(zfacemin);
    float maxAngle = WrapAngle0ToTwoPi(zfacemax);
    float testAngle = WrapAngle0ToTwoPi(ztest);

    if (minAngle <= maxAngle && minAngle <= testAngle && testAngle <= maxAngle) {
        return 1;
    }

    if (minAngle > maxAngle && (minAngle <= testAngle || testAngle <= maxAngle)) {
        return 1;
    }

    return 0;
}

// 0x0045fb40 (PDB: 0x004612f0)
void MATH3D_NearestPointOnPlane(cdc::Vector3* planept, const cdc::Vector3* planenorm, float d, const cdc::Vector3* position)
{
    float t = d - VecDot3(*planenorm, *position);
    planept->x = planenorm->x * t + position->x;
    planept->y = planenorm->y * t + position->y;
    planept->z = planenorm->z * t + position->z;
}

// 0x0045fba0 (PDB: 0x00461350)
int MATH3D_IntersectLineAndPlane(
    cdc::Vector3* intersection,
    const cdc::Vector3* pos0,
    const cdc::Vector3* pos1,
    const cdc::Vector3* planeNormal,
    float planeD)
{
    cdc::Vector3 dir = VecSub3(*pos1, *pos0);
    float denom = VecDot3(dir, *planeNormal);
    if (denom == 0.0f) {
        return 0;
    }

    float t = (planeD - VecDot3(*pos0, *planeNormal)) / denom;
    if (t < 0.0f || t > 1.0f) {
        return 0;
    }

    intersection->x = dir.x * t + pos0->x;
    intersection->y = dir.y * t + pos0->y;
    intersection->z = dir.z * t + pos0->z;
    return 1;
}

// 0x0045fc90 (PDB: 0x00461440)
float MATH3D_NearestPointOnLine(
    cdc::Vector3* outPoint,
    const cdc::Vector3* pos0,
    const cdc::Vector3* pos1,
    const cdc::Vector3* point)
{
    cdc::Vector3 dir = VecSub3(*pos1, *pos0);
    float lenSq = VecDot3(dir, dir);
    if (lenSq == 0.0f) {
        *outPoint = *pos0;
        return 0.0f;
    }

    float t = (VecDot3(*point, dir) - VecDot3(*pos0, dir)) / lenSq;
    if (t <= 0.0f) {
        *outPoint = *pos0;
    } else if (t >= 1.0f) {
        *outPoint = *pos1;
    } else {
        outPoint->x = dir.x * t + pos0->x;
        outPoint->y = dir.y * t + pos0->y;
        outPoint->z = dir.z * t + pos0->z;
    }
    return t;
}

// 0x0045fdb0 (PDB: 0x00461560)
float MATH3D_DistanceFromPointToLineSegment(const cdc::Vector3* pos0, const cdc::Vector3* pos1, const cdc::Vector3* point)
{
    cdc::Vector3 nearest;
    MATH3D_NearestPointOnLine(&nearest, pos0, pos1, point);
    cdc::Vector3 delta = VecSub3(nearest, *point);
    return std::sqrt(VecDot3(delta, delta));
}

// 0x0045fe20 (PDB: 0x004615d0)
float MATH3D_AngleBetweenVectorsf(const cdc::Vector3* v0, const cdc::Vector3* v1)
{
    cdc::Vector3 u0 = UnitVec3(*v0);
    cdc::Vector3 u1 = UnitVec3(*v1);
    float dot = VecDot3(u0, u1);
    if (dot >= 1.0f) {
        return 0.0f;
    }
    if (dot <= -1.0f) {
        return kPi;
    }
    return ClampedAcosf(dot);
}

// 0x0045feb0 (PDB: 0x00461660)
cdc::Vector3 MATH3D_ConvertAngleToVector(float angle)
{
    return MakeVec3(std::sin(angle), -std::cos(angle), 0.0f, 0.0f);
}

// 0x0045ff00 (PDB: 0x004616b0)
cdc::Euler MATH3D_EulerFromPosToPos(const cdc::Vector3* from, const cdc::Vector3* to)
{
    cdc::Euler out;
    out.x = MATH3D_ElevationFromPosToPosf(from, to);
    out.y = 0.0f;
    out.z = WrapAngle0ToTwoPi(std::atan2(from->x - to->x, from->y - to->y) - kHalfPi);
    out.w = 0.0f;
    return out;
}

// 0x0045ffc0 (PDB: 0x00461770)
float MATH3D_ClampAngleInRange(float minAngle, float maxAngle, float inputAngle)
{
    float diffFromMin = WrapAngle0ToTwoPi(inputAngle - minAngle);
    float diffToMax = WrapAngle0ToTwoPi(maxAngle - inputAngle);

    if (diffFromMin + diffToMax > kTwoPi) {
        if (diffFromMin > diffToMax) {
            return minAngle;
        }
        return maxAngle;
    }

    return inputAngle;
}

// 0x00460060 (PDB: 0x00461810)
void MATH3D_AngleMoveTowardf(float* current_ptr, float destination, float step)
{
    float diff = MATH3D_AngleDifff(*current_ptr, destination);
    if (diff == 0.0f || std::fabs(diff) < step) {
        *current_ptr = destination;
        return;
    }

    float delta = (diff > 0.0f) ? step : -step;
    *current_ptr = WrapAngle0ToTwoPi(*current_ptr + delta);
}

// 0x00460120 (PDB: 0x004618d0)
void MATH3D_RelativePositionf(const cdc::Matrix* mat, const cdc::Vector3* origin, const cdc::Vector3* target, cdc::Vector3* out)
{
    cdc::Vector3 delta = VecSub3(*target, *origin);
    cdc::Matrix invMat;
    OrthonormalInverse3x3(&invMat, *mat);
    *out = Mul3x3(invMat, delta);
}

// 0x00460190 (PDB: 0x00461940)
int MATH3D_ConeDetectf(const cdc::Vector3* vec, float xcone, float ycone)
{
    float absX = std::fabs(vec->x);
    float negY = -vec->y;
    if (std::atan2(absX, negY) >= xcone) {
        return 0;
    }

    float absZ = std::fabs(vec->z);
    float xyLen = std::sqrt(absX * absX + negY * negY);
    if (std::atan2(absZ, xyLen) >= ycone) {
        return 0;
    }

    return 1;
}

// 0x00460270 (PDB: 0x00461a20)
void MATH3D_Solve2BoneIK(
    const cdc::Vector3* rootPos,
    const cdc::Vector3* midPos,
    const cdc::Vector3* endPos,
    const cdc::Vector3* targetPos,
    const cdc::Vector3* bendAxisFallback,
    cdc::Quat* outUpperRot,
    cdc::Quat* outLowerRot)
{
    cdc::Vector3 upper = VecSub3(*midPos, *rootPos);
    cdc::Vector3 lower = VecSub3(*endPos, *midPos);
    cdc::Vector3 toTarget = VecSub3(*targetPos, *rootPos);

    float lenUpper = std::sqrt(VecDot3(upper, upper));
    float lenLower = std::sqrt(VecDot3(lower, lower));
    float lenTarget = std::sqrt(VecDot3(toTarget, toTarget));

    float curAngle = MATH3D_AngleBetweenVectorsf(&upper, &lower);
    if (curAngle < kMinBendRad) {
        lower = VecAdd3(lower, *bendAxisFallback);
    }

    cdc::Vector3 bendAxis = VecCross3(upper, lower);
    float cosTarget = ((lenTarget * lenTarget) - (lenUpper * lenUpper) - (lenLower * lenLower)) / (2.0f * lenUpper * lenLower);
    float targetAngle = ClampedAcosf(cosTarget);
    float deltaAngle = targetAngle - curAngle;

    cdc::Quat lowerRot;
    QuatBuildAxisAngle(&lowerRot, bendAxis, deltaAngle);

    cdc::Vector3 rotatedLower = QuatRotate(lowerRot, lower);
    cdc::Vector3 solvedEnd = VecAdd3(upper, rotatedLower);

    cdc::Quat upperRot;
    QuatBuildBetweenVectors(&upperRot, solvedEnd, toTarget);

    *outLowerRot = lowerRot;
    *outUpperRot = upperRot;
}

// 0x00460500 (PDB: 0x00461cb0)
float MATH3D_CheckMatrixAxisAlignment(const cdc::Matrix* mat0, const cdc::Matrix* mat1, const cdc::Vector3* axis)
{
    cdc::Vector3 v0 = Mul3x3(*mat0, *axis);
    cdc::Vector3 v1 = Mul3x3(*mat1, *axis);
    return VecDot3(v0, v1);
}
