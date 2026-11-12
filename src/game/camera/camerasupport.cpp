#include "game/camera/camerasupport.h"
#include "game/camera/camera.h"
#include "game/main.h"
#include <cmath>

static const float CS_PI = 3.14159265358979323846f;
static const float CS_TWOPI = 6.28318530717958647692f;
static const float CS_HALF_PI = 1.57079632679489661923f;

// 0x0048ca00 (PDB: 0x0048e510)
float CAMERA_CircleAnglef(float angle)
{
    if (angle >= 0.0f && angle < CS_TWOPI) {
        return angle;
    }
    float rem = std::fmod(angle, CS_TWOPI);
    if (rem < 0.0f) {
        rem += CS_TWOPI;
    }
    return rem;
}

// 0x0048ca60 (PDB: 0x0048e570)
void CAMERA_SetZRotation(Camera* camera, float rotZ)
{
    float wrapped = CAMERA_CircleAnglef(rotZ);
    camera->core.Rotation.z = wrapped;
    camera->FocusRotation.z = wrapped;
}

// 0x0048ca90 (PDB: 0x0048e5a0)
float CAMERA_SignedAngleDifference(float a, float b)
{
    float diff = a - b;
    if (diff < -CS_TWOPI || diff >= CS_TWOPI) {
        diff = std::fmod(diff, CS_TWOPI);
    }
    if (diff > CS_PI) {
        diff -= CS_TWOPI;
    } else if (diff < -CS_PI) {
        diff += CS_TWOPI;
    }
    return diff;
}

// 0x0048d380 (PDB: 0x0048ee90)
float CAMERA_AngleDifference(float a, float b)
{
    float ca = CAMERA_CircleAnglef(a);
    float cb = CAMERA_CircleAnglef(b);
    if (cb - ca > CS_PI) {
        ca += CS_TWOPI;
    } else if (ca - cb > CS_PI) {
        cb += CS_TWOPI;
    }
    return (ca > cb) ? (ca - cb) : (cb - ca);
}

// 0x0048c8d0 (PDB: 0x0048e3e0)
void CAMERA_CalcRotation(cdc::Euler* outRot, const cdc::Vector3* target, const cdc::Vector3* source)
{
    float dx = source->x - target->x;
    float dy = source->y - target->y;
    float dz = source->z - target->z;
    float horizDist = std::sqrt(dx * dx + dy * dy);

    outRot->x = -std::atan2(dz, horizDist);
    outRot->y = 0.0f;
    outRot->z = std::atan2(dy, dx) + CS_HALF_PI;
}

// 0x0048d340 (PDB: 0x0048ee50)
float CAMERA_CalcZRotation(const cdc::Vector3* target, const cdc::Vector3* source)
{
    float dx = source->x - target->x;
    float dy = source->y - target->y;
    return CAMERA_CircleAnglef(std::atan2(dy, dx) + CS_HALF_PI);
}

// 0x0048caf0 (PDB: 0x0048e600)
bool CAMERA_FocusInstanceMoved(Camera* camera)
{
    if (camera->oldFocusInstancePos.x != camera->newFocusInstancePos.x ||
        camera->oldFocusInstancePos.y != camera->newFocusInstancePos.y ||
        camera->oldFocusInstancePos.z != camera->newFocusInstancePos.z) {
        return true;
    }
    return false;
}

// 0x0048cb80 (PDB: 0x0048e690)
void CAMERA_AngleMoveTowardf(float* current, float target, float step)
{
    float diff = CAMERA_SignedAngleDifference(target, *current);
    if (diff == 0.0f) {
        *current = target;
        return;
    }
    float absDiff = (diff < 0.0f) ? -diff : diff;
    if (absDiff <= step) {
        *current = target;
        return;
    }
    float val = *current + ((diff > 0.0f) ? step : -step);
    if (val < 0.0f) {
        val += CS_TWOPI;
    } else if (val >= CS_TWOPI) {
        val -= CS_TWOPI;
    }
    *current = val;
}

// 0x0048cc50 (PDB: 0x0048e760)
void AngleMoveToward(float* current, float target, float step)
{
    CAMERA_AngleMoveTowardf(current, CAMERA_CircleAnglef(target), step);
}

// 0x0048cc70 (PDB: 0x0048e780)
void Decouple_AngleMoveToward(float* current, float target, float step)
{
    if (g_gameTracker.timeMult != 1.0f) {
        step *= g_gameTracker.timeMult;
    }
    CAMERA_AngleMoveTowardf(current, CAMERA_CircleAnglef(target), step);
}

// 0x0048c6d0 (PDB: 0x0048e1e0)
void CriticalDampValue(int smoothType, float* current, float target, float* vel, float* acc, float maxVel)
{
    if (smoothType == 0) {
        *current = target;
        return;
    }

    float factor = 0.25f;
    switch (smoothType) {
    case 7:
        factor = 0.5f;
        break;
    case 8:
        factor = 1.0f / 6.0f;
        break;
    case 9:
        factor = 0.125f;
        break;
    default:
        factor = 0.25f;
        break;
    }

    float diff = (target - *current) * factor;
    float a = diff - *vel;
    *acc = a;
    float v = *vel + a;
    *vel = v;

    if (v > maxVel) {
        *vel = maxVel;
        *current += maxVel * g_gameTracker.timeMult * 2.0f;
    } else if (v < -maxVel) {
        *vel = -maxVel;
        *current += maxVel * g_gameTracker.timeMult * -2.0f;
    } else if (smoothType == 0 || smoothType == 1 || smoothType == 6 || smoothType >= 7) {
        *current += v;
    } else {
        *current += v * g_gameTracker.timeMult * 2.0f;
    }

    if (*vel == 0.0f) {
        *current = target;
    }
}

// 0x0048c840 (PDB: 0x0048e350)
void CriticalDampValueControl(float* current, float target, float* vel, float* acc, float maxVel, float dampDivisor)
{
    float diff = (target - *current) / dampDivisor;
    float a = diff - *vel;
    *acc = a;
    float v = *vel + a;
    *vel = v;

    if (v > maxVel) {
        *vel = maxVel;
        *current += maxVel * g_gameTracker.timeMult * 2.0f;
    } else if (v < -maxVel) {
        *vel = -maxVel;
        *current += maxVel * g_gameTracker.timeMult * -2.0f;
    } else {
        *current += v;
    }
}

// 0x0048ccc0 (PDB: 0x0048e7d0)
void CriticalDampPosition(int smoothType, cdc::Vector3* current, const cdc::Vector3* target, cdc::Vector3* vel, cdc::Vector3* acc, float maxVel)
{
    CriticalDampValue(smoothType, &current->x, target->x, &vel->x, &acc->x, maxVel);
    CriticalDampValue(smoothType, &current->y, target->y, &vel->y, &acc->y, maxVel);
    CriticalDampValue(smoothType, &current->z, target->z, &vel->z, &acc->z, maxVel);
}

// 0x0048d100 (PDB: 0x0048ec10)
void CriticalDampPositionControl(cdc::Vector3* current, const cdc::Vector3* target, cdc::Vector3* vel, cdc::Vector3* acc, float maxVel, float dampDivisor)
{
    cdc::Vector3 diff;
    diff.x = (target->x - current->x) / dampDivisor;
    diff.y = (target->y - current->y) / dampDivisor;
    diff.z = (target->z - current->z) / dampDivisor;

    acc->x = diff.x - vel->x;
    acc->y = diff.y - vel->y;
    acc->z = diff.z - vel->z;

    vel->x += acc->x;
    vel->y += acc->y;
    vel->z += acc->z;

    float mag = std::sqrt(vel->x * vel->x + vel->y * vel->y + vel->z * vel->z);
    if (mag > maxVel && mag > 0.0f) {
        float scale = maxVel / mag;
        vel->x *= scale;
        vel->y *= scale;
        vel->z *= scale;
    }

    current->x += vel->x * g_gameTracker.timeMult;
    current->y += vel->y * g_gameTracker.timeMult;
    current->z += vel->z * g_gameTracker.timeMult;
}

// 0x0048d200 (PDB: 0x0048ed10)
void CriticalDampAngle(int smoothType, float* current, float target, float* vel, float* acc, float maxVel)
{
    float cur = CAMERA_CircleAnglef(*current);
    float tgt = CAMERA_CircleAnglef(target);
    if (tgt - cur > CS_PI) {
        cur += CS_TWOPI;
    } else if (cur - tgt > CS_PI) {
        tgt += CS_TWOPI;
    }
    CriticalDampValue(smoothType, &cur, tgt, vel, acc, maxVel);
    *current = CAMERA_CircleAnglef(cur);
}

// 0x0048d2a0 (PDB: 0x0048edb0)
void CriticalDampAngleControl(float* current, float target, float* vel, float* acc, float maxVel, float dampDivisor)
{
    float cur = CAMERA_CircleAnglef(*current);
    float tgt = CAMERA_CircleAnglef(target);
    if (tgt - cur > CS_PI) {
        cur += CS_TWOPI;
    } else if (cur - tgt > CS_PI) {
        tgt += CS_TWOPI;
    }
    CriticalDampValueControl(&cur, tgt, vel, acc, maxVel, dampDivisor);
    *current = CAMERA_CircleAnglef(cur);
}

// 0x0048d410 (PDB: 0x0048ef20)
float CAMERA_LimitRotation(Camera* camera, float rotZ)
{
    if (camera->followRotationRangeStart == 0.0f && camera->followRotationRangeEnd == 0.0f) {
        return rotZ;
    }
    float wrapped = CAMERA_CircleAnglef(rotZ);
    float diff = CAMERA_SignedAngleDifference(wrapped, camera->followRotationRangeEnd);
    if (diff > camera->followRotationRangeSoftRange) {
        return CAMERA_CircleAnglef(camera->followRotationRangeEnd + camera->followRotationRangeSoftRange);
    }
    return wrapped;
}

// 0x0048d580 (PDB: 0x0048f090)
void CAMERA_CalcPosition(cdc::Vector3* outPos, const cdc::Vector3* focusPos, const cdc::Euler* rot, float dist)
{
    float sx = std::sin(rot->x);
    float cx = std::cos(rot->x);
    float sy = std::sin(rot->y);
    float cy = std::cos(rot->y);
    float sz = std::sin(rot->z);
    float cz = std::cos(rot->z);

    // Local offset (0, -dist, 0) rotated by Euler X, Y, Z
    float vx = 0.0f;
    float vy = -dist;
    float vz = 0.0f;

    // Apply RotX
    float y1 = vy * cx - vz * sx;
    float z1 = vy * sx + vz * cx;
    float x1 = vx;

    // Apply RotY
    float x2 = x1 * cy + z1 * sy;
    float y2 = y1;
    float z2 = -x1 * sy + z1 * cy;

    // Apply RotZ
    float x3 = x2 * cz - y2 * sz;
    float y3 = x2 * sz + y2 * cz;
    float z3 = z2;

    outPos->x = focusPos->x + x3;
    outPos->y = focusPos->y + y3;
    outPos->z = focusPos->z + z3;
}
