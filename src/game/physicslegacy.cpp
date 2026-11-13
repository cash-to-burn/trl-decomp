#include "game/physicslegacy.h"
#include "game/main.h"

#include <cmath>

namespace {

inline void ClampAxisToTargetVelocity(float& vel, float& accl, float target)
{
    if ((accl < 0.0f && vel <= target) || (accl > 0.0f && vel >= target)) {
        accl = 0.0f;
        vel = target;
    }
}

} // namespace

// 0x00462270 (PDB: 0x00463c30)
void PHYSICS_ForceSetWater(Instance* instance, float* waterTime, float waterZLevel, float swimDepth, float maxDelta)
{
    float clampedZ = waterZLevel;
    if (clampedZ < -3072.0f) {
        clampedZ = -3072.0f;
    } else if (clampedZ > 0.0f) {
        clampedZ = 0.0f;
    }

    const float bobScale = (clampedZ * (-1.0f / 3072.0f)) * maxDelta;

    *waterTime += g_gameTracker.timeMult * swimDepth * 4.0f;
    if (*waterTime > 4096.0f) {
        *waterTime -= 4096.0f;
    }

    if (waterZLevel < 0.0f) {
        instance->position.z += std::cos(*waterTime * (1.0f / 4096.0f)) * bobScale;
    }
}

// 0x00462310 (PDB: 0x00463cd0)
void PHYSICS_SetVelFromZRot(Instance* instance, float angle, float speed)
{
    const float adjustedAngle = angle - 0.25f;
    instance->vel.x = std::cos(adjustedAngle) * speed;
    instance->vel.y = std::sin(adjustedAngle) * speed;
}

// 0x00462340 (PDB: 0x00463d00)
float PHYSICS_FindAFromDAndT(float dist, float time)
{
    if (time != 0.0f) {
        return (dist + dist) / (time * time);
    }
    return 0.0f;
}

// 0x00462370 (PDB: 0x00463d30)
float PHYSICS_FindVFromAAndD(float accl, float dist)
{
    const float vSq = (accl * dist) * 2.0f;
    if (vSq != 0.0f) {
        return std::sqrt(vSq);
    }
    return 0.0f;
}

// 0x004623a0 (PDB: 0x00463d60)
void PHYSICS_StopAcceleratingIfCloseToTargetVelocity(Instance* instance, float targetX, float targetY, float targetZ)
{
    ClampAxisToTargetVelocity(instance->vel.x, instance->accl.x, targetX);
    ClampAxisToTargetVelocity(instance->vel.y, instance->accl.y, targetY);
    ClampAxisToTargetVelocity(instance->vel.z, instance->accl.z, targetZ);
}

// 0x004624b0 (PDB: 0x00463e70)
void PhysicsMove(Instance* instance, cdc::Vector3* outOffset, float time)
{
    const float halfTimeSq = 0.5f * time * time;
    outOffset->x += instance->vel.x * time + instance->accl.x * halfTimeSq;
    outOffset->y += instance->vel.y * time + instance->accl.y * halfTimeSq;
    outOffset->z += instance->vel.z * time + instance->accl.z * halfTimeSq;
    outOffset->w += instance->vel.w * time + instance->accl.w * halfTimeSq;

    instance->vel.x += instance->accl.x * time;
    instance->vel.y += instance->accl.y * time;
    instance->vel.z += instance->accl.z * time;
    instance->vel.w += instance->accl.w * time;
}

// 0x00462540 (PDB: 0x00463f00)
void PHYSICS_SetVelFromRot(Instance* instance, cdc::Euler* rot, float magnitude)
{
    cdc::Matrix mat;
    mat.Build_XYZOrder(*rot);

    const cdc::Vector3 localDir = cdc::Vector3::Make(0.0f, -magnitude, 0.0f, 0.0f);
    instance->vel = cdc::Mul3x3(mat, localDir);
}

// 0x004625e0 (PDB: 0x00463fa0)
void PHYSICS_SetVAndAFromRot(Instance* instance, cdc::Euler* rot, float vMag, float aMag)
{
    cdc::Matrix mat;
    if (vMag != 0.0f || aMag != 0.0f) {
        mat.Build_XYZOrder(*rot);
    }

    if (vMag != 0.0f) {
        const cdc::Vector3 localVel = cdc::Vector3::Make(0.0f, -vMag, 0.0f, 0.0f);
        instance->vel = cdc::Mul3x3(mat, localVel);
    } else {
        instance->vel = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }

    if (aMag != 0.0f) {
        const cdc::Vector3 localAccl = cdc::Vector3::Make(0.0f, -aMag, 0.0f, 0.0f);
        instance->accl = cdc::Mul3x3(mat, localAccl);
    } else {
        instance->accl = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// 0x00462750 (PDB: 0x00464110)
void PhysicsMoveLocalZClamp(Instance* instance, int segment, float time, int clamp)
{
    cdc::Vector3 localOffset = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    PhysicsMove(instance, &localOffset, time);

    const cdc::Vector3 flippedLocal = cdc::Vector3::Make(localOffset.x, -localOffset.y, localOffset.z, localOffset.w);
    const cdc::Vector3 worldOffset = cdc::Mul3x3(instance->matrix[segment], flippedLocal);

    instance->position.x += worldOffset.x;
    instance->position.y += worldOffset.y;
    if (clamp == 0) {
        instance->position.z += worldOffset.z;
    }
}

// 0x00462840 (PDB: 0x00464200)
void PhysicsMoveLocalFall(Instance* instance, cdc::Vector3* accl, float time, float maxXYVel, float maxZVel)
{
    const float halfTimeSq = 0.5f * time * time;

    cdc::Vector3 newVel;
    newVel.x = instance->vel.x + instance->accl.x * time;
    newVel.y = instance->vel.y + instance->accl.y * time;
    newVel.z = instance->vel.z + instance->accl.z * time;
    newVel.w = instance->vel.w + instance->accl.w * time;

    cdc::Vector3 localOffset;
    localOffset.x = instance->vel.x * time + instance->accl.x * halfTimeSq;
    localOffset.y = -(instance->vel.y * time + instance->accl.y * halfTimeSq);
    localOffset.z = instance->vel.z * time + instance->accl.z * halfTimeSq;
    localOffset.w = instance->vel.w * time + instance->accl.w * halfTimeSq;

    const float maxXYSq = maxXYVel * maxXYVel;
    cdc::Vector3 xyAccl = cdc::Vector3::Make(accl->x, accl->y, 0.0f, accl->w);
    const float acclXYSq = xyAccl.x * xyAccl.x + xyAccl.y * xyAccl.y;

    cdc::Vector3 excessXY = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    if (acclXYSq > maxXYSq) {
        const float acclLen = std::sqrt(acclXYSq);
        const float ratio = (acclLen - maxXYVel) / acclLen;
        excessXY.x = xyAccl.x * ratio;
        excessXY.y = xyAccl.y * ratio;
        excessXY.z = xyAccl.z * ratio;
        excessXY.w = xyAccl.w * ratio;
    }

    cdc::Vector3 clampedDiff;
    clampedDiff.x = newVel.x - excessXY.x;
    clampedDiff.y = newVel.y - excessXY.y;
    clampedDiff.z = 0.0f;
    clampedDiff.w = newVel.w - excessXY.w;

    const float diffXYSq = clampedDiff.x * clampedDiff.x + clampedDiff.y * clampedDiff.y;
    if (diffXYSq > maxXYSq) {
        const float scale = maxXYVel / std::sqrt(diffXYSq);
        instance->vel.x = clampedDiff.x * scale + excessXY.x;
        instance->vel.y = clampedDiff.y * scale + excessXY.y;
        instance->vel.z = clampedDiff.z * scale + excessXY.z;
        instance->vel.w = clampedDiff.w * scale + excessXY.w;
    } else {
        instance->vel = newVel;
    }

    if (newVel.z < -maxZVel) {
        instance->vel.z = -maxZVel;
    } else {
        instance->vel.z = newVel.z;
    }

    cdc::Matrix rotMat;
    rotMat.Identity();
    rotMat.RotZ(std::sin(instance->rotation.z), std::cos(instance->rotation.z));
    const cdc::Vector3 worldOffset = cdc::Mul3x3(rotMat, localOffset);

    instance->position.x += worldOffset.x;
    instance->position.y += worldOffset.y;
    instance->position.z += worldOffset.z;
    instance->position.w += worldOffset.w;
}
