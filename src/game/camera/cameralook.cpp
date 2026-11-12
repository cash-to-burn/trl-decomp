#include "game/camera/cameralook.h"
#include "game/camera/camerabehind.h"
#include "game/camera/camerasupport.h"
#include "game/camera/cameravv.h"
#include "game/pc/pcinput.h"
#include "game/playercntrl.h"
#include "game/player/player.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

// External subsystem helpers not yet reconstructed in full headers
class InputSystem {
public:
    float GetAxisValue(int axis); // 0x004e3c50
};
extern InputSystem* INPUT_instances[]; // 0x011015c0

struct CSenseManager {
    int GetAllTargets(int senseType, int* outCount, SenseTarget*** outTargets); // 0x005630c0 (PDB: 0x005652f0)
};
extern CSenseManager* gSenseManager; // 0x01117490 (PDB: 0x01118640)

#include "game/camera/cameraspline.h"

extern void  RenderG2_MotionBlur(int blurPercent, int frames, int mode);                                      // 0x0040c6e0 (PDB: 0x0040cea0)
extern void  INSTANCE_GetPosFromSegment(Instance* inst, int seg, cdc::Vector3* outPos);                       // 0x00455f20 (PDB: 0x004575a0)
extern float CAMERA_DoVertigo(Camera* camera);                                                                // 0x0047cad0 (PDB: 0x0047e470)
extern int   CAMERA_DoCameraCollision(Camera* camera, cdc::Vector3* startPos, int param3);                    // 0x004857b0 (PDB: 0x004871e0)
extern void  PLAYER_ReSetLookAround(Instance* instance);                                                      // 0x00566480 (PDB: 0x00568760)
extern void  getTargetPosition(SenseTarget* target, cdc::Vector3* outPos);                                    // 0x00588be0 (PDB: 0x0058b270)
extern bool  Sense_LineOfSight(Instance* viewerInst, cdc::Vector3* startPos, SenseTarget* target, int flags); // 0x00592fc0 (PDB: 0x005955c0)
extern void  playerFindTargetPosition(cdc::Vector3* outScreenOffset, bool* outInFront,
                                      bool* outOnScreen, const cdc::Vector3* worldPos);                       // 0x005a8e00 (PDB: 0x005ab700)

// Globals defined in f:\tr7\game\camera\cameraLook.c
float cameraTurretInterp = 0.0f; // 0x010fc57c (PDB: 0x010fd77c)

namespace {

constexpr float CL_PI         = 3.1415927410125732f;   // 0x00f01728
constexpr float CL_HALF_PI    = 1.5707963705062866f;   // 0x00f01730
constexpr float CL_DEG_TO_RAD = 0.01745329238474369f;  // 0x00f01734
constexpr float CL_RAD_TO_DEG = 57.2957763671875f;     // 0x00f01738

static float ClampFloat(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static cdc::Vector3 AddVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    out.x = a.x + b.x;
    out.y = a.y + b.y;
    out.z = a.z + b.z;
    out.w = a.w + b.w;
    return out;
}

static cdc::Vector3 SubVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    out.x = a.x - b.x;
    out.y = a.y - b.y;
    out.z = a.z - b.z;
    out.w = a.w - b.w;
    return out;
}

static cdc::Vector3 ScaleVec3(const cdc::Vector3& v, float s)
{
    cdc::Vector3 out;
    out.x = v.x * s;
    out.y = v.y * s;
    out.z = v.z * s;
    out.w = v.w * s;
    return out;
}

static cdc::Vector3 LerpVec3(const cdc::Vector3& a, const cdc::Vector3& b, float t)
{
    cdc::Vector3 out;
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = a.z + (b.z - a.z) * t;
    out.w = a.w + (b.w - a.w) * t;
    return out;
}

static float VecLen3(const cdc::Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static void Normalize3(cdc::Vector3& v)
{
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    float inv = 1.0f / len;
    v.x *= inv;
    v.y *= inv;
    v.z *= inv;
    v.w *= inv;
}

// Reconstructs cdc::Quat::Quat(const cdc::Euler&) (0x005de3d0)
static cdc::Vector QuatFromEuler(const cdc::Euler& e)
{
    float cx = std::cos(e.x * 0.5f);
    float cy = std::cos(e.y * 0.5f);
    float cz = std::cos(e.z * 0.5f);
    float sx = std::sin(e.x * 0.5f);
    float sy = std::sin(e.y * 0.5f);
    float sz = std::sin(e.z * 0.5f);

    cdc::Vector q;
    q.x = cz * cy * sx - sz * sy * cx;
    q.y = sy * cz * cx + sx * sz * cy;
    q.z = sz * cy * cx - sy * sx * cz;
    q.w = cz * cy * cx + sx * sz * sy;
    return q;
}

// Reconstructs cdc::Quat::RotateVector (0x005de8d0): q * (v, 0) * conj(q)
static cdc::Vector3 QuatRotateVector(const cdc::Vector& q, const cdc::Vector3& v)
{
    auto mul = [](const cdc::Vector& a, const cdc::Vector& b) {
        cdc::Vector r;
        r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
        r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
        r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
        r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
        return r;
    };

    cdc::Vector vecQ;
    vecQ.x = v.x;
    vecQ.y = v.y;
    vecQ.z = v.z;
    vecQ.w = 0.0f;

    cdc::Vector conjQ;
    conjQ.x = -q.x;
    conjQ.y = -q.y;
    conjQ.z = -q.z;
    conjQ.w = q.w;

    cdc::Vector tmp = mul(q, vecQ);
    cdc::Vector res = mul(tmp, conjQ);

    cdc::Vector3 out;
    out.x = res.x;
    out.y = res.y;
    out.z = res.z;
    out.w = res.w;
    return out;
}

} // namespace

// 0x00486450 (PDB: 0x00487e90)
void CAMERA_SetStickyLookMode(void)
{
    *reinterpret_cast<uint16_t*>(&theCamera.CameraLookStickyFlag) = 1;
}

// 0x00486460 (PDB: 0x00487ea0)
void CAMERA_SetAccurateAimMode(void)
{
    theCamera.bAccAimFlag = true;
}

// 0x00486470 (PDB: 0x00487eb0)
void CAMERA_SetLookFocusAndBase(Instance* instance, cdc::Vector3* outFocus)
{
    cdc::Vector3 pos = instance->position;
    uint32_t lookZOff = INSTANCE_Query(instance, 0x65);
    pos.z += static_cast<float>(lookZOff);

    *outFocus = pos;
    theCamera.ThrowLookPoint = pos;

    theCamera.lookvel.z = 0.0f;
    theCamera.lookvel.x = 0.0f;
    theCamera.lookaccl.z = 0.0f;
    theCamera.lookaccl.x = 0.0f;
}

// 0x00486510 (PDB: 0x00487f50)
void CAMERA_EndLook(Camera* camera)
{
    Instance* focusInst = camera->focusInstance;
    uint16_t origMode = *reinterpret_cast<const uint16_t*>(&camera->mode);
    *reinterpret_cast<uint16_t*>(&camera->CameraLookStickyFlag) = 0;

    if (origMode != 5 && origMode != 13) {
        *reinterpret_cast<uint16_t*>(&camera->lookTimer) = 0;
        return;
    }

    cdc::Vector3 savedPos = camera->core.position;
    cdc::Vector3 savedFocus = camera->focusPoint;

    camera->cutFlag = 1;
    *reinterpret_cast<uint16_t*>(&camera->nextCam.crossfade) = 5;
    CAMERA_RestoreMode(camera);
    camera->targetFocusDistance = camera->savedFocusDistance;

    *reinterpret_cast<uint16_t*>(&camera->forced_movement) = 0;
    splineLookX = 0.0f;
    splineLookZ = 0.0f;

    if (*reinterpret_cast<const uint16_t*>(&camera->mode) == 3) {
        camera->targetPos = savedPos;
        camera->focusPoint = savedFocus;
        CAMERA_SplineProcess(camera);
    } else {
        camera->TargetFocusRotation = camera->savedFocusRotation;
        camera->smooth = 8.0f;

        float targetZ = CAMERA_CircleAnglef(focusInst->rotation.z + CL_PI);
        camera->TargetFocusRotation.z = targetZ;
        camera->CollisionTargetFocusRotation.z = targetZ;
        *reinterpret_cast<uint16_t*>(&camera->always_rotate_flag) = 1;
        camera->velocityLag = 0.0f;
        camera->FocusRotation.x = camera->tfaceTilt;
        camera->core.Rotation.x = camera->tfaceTilt;
    }

    if (*reinterpret_cast<const uint16_t*>(&camera->mode) == 5) {
        PLAYER_ReSetLookAround(focusInst);
    }

    camera->flags &= ~0x20000;
    focusInst->fadeValue = 0.0f;
    if (focusInst->LinkChild != nullptr) {
        focusInst->LinkChild->fadeValue = 0.0f;
    }
    focusInst->iflags &= ~0x800;

    *reinterpret_cast<uint16_t*>(&camera->lookTimer) = 0;
}

// 0x00486730 (PDB: 0x00488170)
static float CAMERA_BezierLine(float p1, float p2, float t)
{
    float c1 = 3.0f * p1;
    float c2 = 3.0f * (p2 - p1) - c1;
    float c3 = 1.0f - c1 - c2;
    return c3 * t * t * t + c2 * t * t + c1 * t;
}

// 0x00486780 (PDB: 0x004881c0)
float CAMERA_AccurateAimInterpTime(Camera* camera)
{
    if (camera->bAccAimFlag) {
        return camera->smooth;
    }
    return 0.0f;
}

// 0x004867a0 (PDB: 0x004881e0)
void CAMERA_InitAimAssist(void)
{
    cdc::Euler zero = {};
    gPlayer->pAimAssistTarget = nullptr;
    gPlayer->vAimAssistOldRot = zero;
    gPlayer->vAimAssistOldForce = zero;
}

// 0x00486820 (PDB: 0x00488260)
void CAMERA_AimAssist(cdc::Euler* outRot)
{
    cdc::Vector3 eyePos = {};
    cdc::Euler steerDelta = {};
    int senseTypes[5] = { -1, -1, -1, -1, -1 };

    SenseTarget* bestTarget = nullptr;
    float bestDistSq = -1.0f;
    Instance* viewerInst = nullptr;

    int stateID = gPlayerSM->stateMachine.activeState->state->stateInfo[1];
    if (stateID == 0x20) {
        eyePos = theCamera.core.position;
        viewerInst = s_playerInstance;
        senseTypes[0] = 0;
        senseTypes[1] = 1;
        senseTypes[2] = 2;
        senseTypes[3] = 3;
        senseTypes[4] = 7;
    } else if (stateID == 0x30) {
        Instance* turretInst = gPlayer->engagedList[23].instance;
        if (turretInst == nullptr) {
            return;
        }
        TurretProp* tprop = reinterpret_cast<TurretProp*>(turretInst->data);
        if (tprop->turretType == 0) {
            senseTypes[0] = 0;
            senseTypes[1] = 1;
            senseTypes[2] = 7;
        }
        INSTANCE_GetPosFromSegment(turretInst, tprop->firingSegment, &eyePos);
        viewerInst = turretInst;
    } else {
        return;
    }

    for (int i = 0; i < 5; ++i) {
        int senseType = senseTypes[i];
        if (senseType == -1) {
            break;
        }

        int targetCount = 0;
        SenseTarget** targets = nullptr;
        gSenseManager->GetAllTargets(senseType, &targetCount, &targets);

        for (uint32_t j = 0; j < static_cast<uint32_t>(targetCount); ++j) {
            SenseTarget* tgt = targets[j];
            cdc::Vector3 tgtPos;
            getTargetPosition(tgt, &tgtPos);

            bool inFront = false;
            bool onScreen = false;
            cdc::Vector3 screenOff;
            playerFindTargetPosition(&screenOff, &inFront, &onScreen, &tgtPos);
            if (!onScreen) {
                continue;
            }

            OldRazielData& aimData = playerData->oldData;
            if (std::fabs(screenOff.x) > aimData.fAimAssistOuterWidth) {
                continue;
            }
            if (std::fabs(screenOff.y) > aimData.fAimAssistOuterHeight) {
                continue;
            }
            if (bestDistSq >= 0.0f && screenOff.z > bestDistSq) {
                continue;
            }
            if (Sense_LineOfSight(viewerInst, &eyePos, tgt, 0)) {
                bestDistSq = screenOff.z;
                bestTarget = tgt;
            }
        }
    }

    if (bestTarget != nullptr) {
        cdc::Vector3 tgtPos;
        getTargetPosition(bestTarget, &tgtPos);

        bool inFront = false;
        bool onScreen = false;
        cdc::Vector3 screenOff;
        playerFindTargetPosition(&screenOff, &inFront, &onScreen, &tgtPos);

        if (bestTarget == gPlayer->pAimAssistTarget) {
            OldRazielData& aimData = playerData->oldData;
            steerDelta.x = gPlayer->vAimAssistOldRot.x - outRot->x;
            steerDelta.y = gPlayer->vAimAssistOldRot.y - outRot->y;
            steerDelta.z = gPlayer->vAimAssistOldRot.z - outRot->z;
            steerDelta.w = gPlayer->vAimAssistOldRot.w - outRot->w;

            float tX = ClampFloat(
                (std::fabs(screenOff.x) - aimData.fAimAssistInnerRadius) /
                    (aimData.fAimAssistOuterWidth - aimData.fAimAssistInnerRadius),
                0.0f,
                1.0f);
            float slowH = tX * (1.0f - aimData.fAimAssistTimeSlowH) + aimData.fAimAssistTimeSlowH;
            steerDelta.z *= 1.0f - (1.0f / slowH);

            float tY = ClampFloat(
                (std::fabs(screenOff.y) - aimData.fAimAssistInnerRadius) /
                    (aimData.fAimAssistOuterHeight - aimData.fAimAssistInnerRadius),
                0.0f,
                1.0f);
            float slowV = tY * (1.0f - aimData.fAimAssistTimeSlowV) + aimData.fAimAssistTimeSlowV;
            steerDelta.x *= 1.0f - (1.0f / slowV);
        } else if (gPlayer->pAimAssistTarget != nullptr) {
            steerDelta = gPlayer->vAimAssistOldForce;
        }

        outRot->x += steerDelta.x;
        outRot->y += steerDelta.y;
        outRot->z += steerDelta.z;
        outRot->w += steerDelta.w;
    }

    gPlayer->pAimAssistTarget = bestTarget;
    gPlayer->vAimAssistOldForce = steerDelta;
    gPlayer->vAimAssistOldRot = *outRot;
}

// 0x00486c90 (PDB: 0x004886d0)
void CAMERA_ResetStickyLookMode(void)
{
    *reinterpret_cast<uint16_t*>(&theCamera.CameraLookStickyFlag) = 0;
    CAMERA_EndLook(&theCamera);
}

// 0x00486cb0 (PDB: 0x004886f0)
void CAMERA_ResetAccurateAimMode(void)
{
    theCamera.bAccAimFlag = false;
    CAMERA_EndLook(&theCamera);
}

// 0x00486cd0 (PDB: 0x00488710)
void CAMERA_StartLookaroundMode(Camera* camera)
{
    int prevMode = *reinterpret_cast<const int16_t*>(&camera->mode);
    camera->Camera_lookDist = 0.0f;
    CAMERA_SaveMode(camera, prevMode);

    camera->savedFocusDistance = camera->targetFocusDistance;
    camera->savedFocusRotation = camera->TargetFocusRotation;
    *reinterpret_cast<uint16_t*>(&camera->mode) = 5;

    camera->targetFocusDistance = 0.0f;
    camera->LookRot.x = 0.0f;
    camera->LookRot.y = 0.0f;
    camera->LookRot.z = 0.0f;

    CAMERA_SetLookFocusAndBase(camera->focusInstance, &camera->targetFocusPoint);
    camera->focusInstance->rotation.z = camera->core.Rotation.z + CL_PI;

    if (prevMode == 2 || prevMode == 0) {
        float cosPitch = std::cos(camera->FocusRotation.x);
        float horizDist = std::fabs(cosPitch * camera->focusDistance);
        float newDist = std::fabs(horizDist / cosPitch);
        float sinPitch = std::sin(camera->FocusRotation.x);
        camera->real_focuspoint.z += sinPitch * newDist - sinPitch * camera->focusDistance;
        camera->focusDistance = newDist;
    }

    camera->focusPoint = camera->real_focuspoint;
}

// 0x00486df0 (PDB: 0x00488830)
void CAMERA_StartAccurateAimMode(Camera* camera, Instance* playerInst)
{
    PlayerProp* prop = reinterpret_cast<PlayerProp*>(playerInst->data);
    camera->focusInstance = playerInst;
    camera->Camera_lookDist = 0.0f;
    CAMERA_SaveMode(camera, *reinterpret_cast<const int16_t*>(&camera->mode));

    camera->savedFocusDistance = camera->targetFocusDistance;

    cdc::Euler zeroEuler = {};
    camera->LookRot = zeroEuler;
    camera->LookRot.z = playerInst->rotation.z + CL_PI;

    camera->savedFocusRotation = camera->TargetFocusRotation;
    *reinterpret_cast<uint16_t*>(&camera->mode) = 13;

    const float* boom = prop->oldData.vAimBoom;
    camera->targetFocusDistance = std::sqrt(boom[0] * boom[0] + boom[1] * boom[1] + boom[2] * boom[2]);
    camera->smooth = prop->oldData.fAimInterpIn;

    cdc::Vector3 zeroVec = {};
    camera->positionVel = zeroVec;
    camera->positionAccl = zeroVec;

    camera->core.newProjDistance *= prop->oldData.fAimProjDistFactor;
    if (prop->oldData.uAimProjInterpIn > 0) {
        camera->core.projDistanceSpd =
            (camera->core.newProjDistance - camera->core.projDistance) /
            static_cast<float>(prop->oldData.uAimProjInterpIn);
    } else {
        CAMERA_SetProjDistance(&camera->core, camera->core.newProjDistance);
    }

    CAMERA_InitAimAssist();
}

// 0x00486fb0 (PDB: 0x004889f0)
void CAMERA_ForceEndLookaroundMode(Camera* camera)
{
    // SecuROM obfuscated tail jump at 0x00486fb4 -> 0x00486510 (CAMERA_EndLook)
    CAMERA_EndLook(camera);
}

// 0x00486fd0 (PDB: 0x00488a10)
void CAMERA_LookProcess(Camera* camera)
{
    Instance* focusInst = camera->focusInstance;
    camera->targetFocusPoint = AddVec3(camera->targetFocusPoint, camera->focusInstanceVelVec);

    if (camera->focusDistance > camera->Camera_lookDist) {
        RenderG2_MotionBlur(100, 0, 0);
        RenderG2_MotionBlur(0, 10, 0);

        camera->focusDistance = camera->Camera_lookDist;
        camera->FocusRotation = camera->LookRot;
        camera->FocusRotation.z = CAMERA_CircleAnglef(
            focusInst->rotation.z + camera->FocusRotation.z + CL_PI);

        camera->core.Rotation = camera->FocusRotation;
        camera->focusPoint = camera->targetFocusPoint;
        camera->ThrowLookPoint = camera->targetFocusPoint;

        CAMERA_CalcPosition(
            &camera->targetPos,
            &camera->focusPoint,
            &camera->FocusRotation,
            camera->focusDistance);
        camera->core.position = camera->targetPos;
        CAMERA_SetProjDistance(&camera->core, 512.0f);
    }

    float fadeRaw = ClampFloat(4096.0f - (camera->focusDistance - 64.0f) * 8.0f, 0.0f, 4096.0f);
    float fade = fadeRaw * 0.000244140625f;
    focusInst->fadeType = 0;
    focusInst->fadeValue = fade;
    if (focusInst->LinkChild != nullptr) {
        focusInst->LinkChild->fadeValue = fade;
    }
    if (fade == 4096.0f) {
        focusInst->iflags |= 0x800;
    }

    float colDist = std::min(camera->focusDistance, camera->Camera_lookDist);
    CAMERA_CalcPosition(&camera->targetPos, &camera->focusPoint, &camera->FocusRotation, colDist);
    camera->data.Follow.hit = CAMERA_DoCameraCollision(camera, &camera->targetPos, 1);

    float desiredDist;
    float smoothRate;
    int smoothType;
    if (camera->data.Follow.hit != 0) {
        desiredDist = std::min(camera->collisionTargetFocusDistance, camera->Camera_lookDist);
        smoothRate = 512.0f;
        smoothType = 7;
    } else {
        desiredDist = camera->Camera_lookDist;
        smoothRate = 32.0f;
        smoothType = 3;
    }

    if ((camera->flags & 0x800) != 0) {
        camera->focusDistance = camera->targetFocusDistance;
        camera->focusPoint = camera->targetPos;
    } else {
        CriticalDampValue(
            smoothType,
            &camera->focusDistance,
            desiredDist,
            &camera->focusDistanceVel,
            &camera->focusDistanceAccl,
            smoothRate);
        CriticalDampPosition(
            3,
            &camera->focusPoint,
            &camera->targetPos,
            &camera->focusPointVel,
            &camera->focusPointAccl,
            256.0f);
    }

    if (camera->lookThrowMode == 1) {
        float pitch = CAMERA_CircleAnglef(camera->LookRot.x);
        if (pitch > 0.6135923266410828f && pitch <= CL_PI) {
            pitch = 0.6135923266410828f;
        }
        camera->TargetFocusRotation.x = pitch;
    } else {
        camera->TargetFocusRotation.x = camera->LookRot.x;
    }

    camera->TargetFocusRotation.y = camera->LookRot.y;
    camera->TargetFocusRotation.z = CAMERA_CircleAnglef(
        camera->LookRot.z + focusInst->rotation.z + CL_PI);

    INPUT_SetAxisScale(0x47, camera->TargetFocusRotation.z * CL_RAD_TO_DEG - 180.0f, 1);
    INPUT_SetAxisScale(0x46, camera->TargetFocusRotation.x * CL_RAD_TO_DEG, 1);

    float rotMax = (1.0f - camera->focusDistance / camera->savedFocusDistance) * 0.09817477315664291f;
    if (rotMax < 0.00920388475060463f) {
        rotMax = 0.00920388475060463f;
    }

    CriticalDampAngle(
        3,
        &camera->FocusRotation.z,
        camera->TargetFocusRotation.z,
        &camera->focusRotVel.z,
        &camera->focusRotAccl.z,
        rotMax);
    CriticalDampAngle(
        3,
        &camera->FocusRotation.x,
        camera->TargetFocusRotation.x,
        &camera->focusRotVel.x,
        &camera->focusRotAccl.x,
        rotMax);

    float distFactor = CAMERA_DoVertigo(camera);
    CAMERA_CalcPosition(
        &camera->targetPos,
        &camera->focusPoint,
        &camera->FocusRotation,
        distFactor * camera->focusDistance);
    camera->core.position = camera->targetPos;

    if (camera->lookThrowMode != 0) {
        cdc::Euler aimRot;
        CAMERA_CalcRotation(&aimRot, &camera->ThrowLookPoint, &camera->core.position);
        CriticalDampAngle(
            3,
            &camera->core.Rotation.z,
            aimRot.z,
            &camera->lookvel.z,
            &camera->lookaccl.z,
            0.09817477315664291f);
        CriticalDampAngle(
            3,
            &camera->core.Rotation.x,
            aimRot.x,
            &camera->lookvel.x,
            &camera->lookaccl.x,
            0.09817477315664291f);
    } else {
        camera->core.Rotation = camera->FocusRotation;
    }

    camera->lagZ = camera->core.Rotation.z;
    CriticalDampAngle(
        9,
        &camera->followDampedHorizontalBiasRotation,
        0.0f,
        &camera->followHBiasVel,
        &camera->followHBiasAcc,
        0.02761165425181389f);
    camera->core.Rotation.z += camera->followDampedHorizontalBiasRotation;
    camera->distanceState = 0;
}

// 0x00487540 (PDB: 0x00488f80)
void CAMERA_AccurateAimProcessInputs(Camera* camera, Instance* playerInst)
{
    PlayerProp* prop = reinterpret_cast<PlayerProp*>(playerInst->data);
    OldRazielData& aimData = prop->oldData;

    float ax0 = gPlayerControl_Enabled ? INPUT_instances[0]->GetAxisValue(0) : 0.0f;
    float ax2 = gPlayerControl_Enabled ? INPUT_instances[0]->GetAxisValue(2) : 0.0f;
    float ax1 = gPlayerControl_Enabled ? INPUT_instances[0]->GetAxisValue(1) : 0.0f;
    float ax3 = gPlayerControl_Enabled ? INPUT_instances[0]->GetAxisValue(3) : 0.0f;

    float domX = (std::fabs(ax0) > std::fabs(ax2)) ? ax0 : ax2;
    float joyX = INPUT_GetAxisScale(0xe) * domX;

    float domY = (std::fabs(ax1) > std::fabs(ax3)) ? ax1 : ax3;
    float joyY = INPUT_GetAxisScale(0xf) * domY;

    camera->LookRot.x += aimData.fAimPanVRate * CL_DEG_TO_RAD * s_timeScale * joyY;
    camera->LookRot.z -= aimData.fAimPanHRate * CL_DEG_TO_RAD * s_timeScale * joyX;

    float mouseX = INPUT_instances[0]->GetAxisValue(0x10) * INPUT_GetAxisScale(0xe);
    float mouseY = INPUT_instances[0]->GetAxisValue(0x11) * INPUT_GetAxisScale(0xf);

    camera->LookRot.x -= aimData.fAimPanVRate * CL_DEG_TO_RAD * (-mouseY) * 2.0f;
    camera->LookRot.z -= aimData.fAimPanHRate * CL_DEG_TO_RAD * mouseX * 2.0f;

    if (joyX != 0.0f || joyY != 0.0f) {
        CAMERA_AimAssist(&camera->LookRot);
    }

    float minPitch = aimData.fAimMinPitch * CL_DEG_TO_RAD;
    float maxPitch = aimData.fAimMaxPitch * CL_DEG_TO_RAD;
    if (camera->LookRot.x < minPitch) {
        camera->LookRot.x = minPitch;
    } else if (camera->LookRot.x > maxPitch) {
        camera->LookRot.x = maxPitch;
    }
}

// 0x004877d0 (PDB: 0x00489210)
void CAMERA_AccurateAimProcess(Camera* camera)
{
    PlayerProp* prop = reinterpret_cast<PlayerProp*>(s_playerInstance->data);
    OldRazielData& aimData = prop->oldData;

    if ((camera->flags & 0x800) != 0) {
        camera->focusDistance = camera->targetFocusDistance;
    }

    cdc::Vector3 anchorPos = camera->focusInstance->position;
    anchorPos.z += aimData.fAimAnchorOffset;

    cdc::Vector q = QuatFromEuler(camera->LookRot);

    cdc::Vector3 boomOffset = {};
    boomOffset.x = aimData.vAimBoom[0];
    boomOffset.y = aimData.vAimBoom[1];
    boomOffset.z = aimData.vAimBoom[2];
    camera->targetPos = AddVec3(anchorPos, QuatRotateVector(q, boomOffset));

    cdc::Vector3 fwdAxis = {};
    fwdAxis.y = -1.0f;
    cdc::Vector3 aimFarPt = AddVec3(anchorPos, ScaleVec3(QuatRotateVector(q, fwdAxis), aimData.fAimMaxRange));

    cdc::Vector3 aimDir = SubVec3(aimFarPt, camera->targetPos);
    Normalize3(aimDir);
    camera->targetFocusPoint = AddVec3(camera->targetPos, ScaleVec3(aimDir, camera->targetFocusDistance));

    if (camera->smooth > 0.0f) {
        camera->smooth -= s_timeScale;
        if (camera->smooth > 0.0f) {
            float t = CAMERA_BezierLine(
                aimData.fAimInterpCtrl1,
                aimData.fAimInterpCtrl2,
                1.0f - camera->smooth / aimData.fAimInterpIn);
            camera->targetPos = LerpVec3(camera->core.position, camera->targetPos, t);
            camera->focusPoint = LerpVec3(camera->focusPoint, camera->targetFocusPoint, t);
        } else {
            camera->focusPoint = camera->targetFocusPoint;
        }
    } else {
        camera->focusPoint = camera->targetFocusPoint;
    }

    camera->data.Follow.hit = CAMERA_DoCameraCollision(camera, &camera->targetPos, 1);
    if (camera->data.Follow.hit != 0) {
        cdc::Vector3 backDir = SubVec3(camera->targetPos, camera->targetFocusPoint);
        Normalize3(backDir);
        camera->core.position = AddVec3(
            camera->targetFocusPoint,
            ScaleVec3(backDir, camera->collisionTargetFocusDistance));
    } else {
        camera->core.position = camera->targetPos;
    }

    cdc::Vector3 lookVec = SubVec3(camera->focusPoint, camera->core.position);
    float dist = VecLen3(lookVec);
    camera->focusDistance = dist;

    cdc::Vector3 unitLook = ScaleVec3(lookVec, 1.0f / dist);
    float pitchSin = ClampFloat(unitLook.z, -1.0f, 1.0f);

    camera->TargetFocusRotation.x = std::asin(pitchSin);
    camera->TargetFocusRotation.y = 0.0f;
    camera->TargetFocusRotation.z = CAMERA_CircleAnglef(std::atan2(unitLook.y, unitLook.x) - CL_HALF_PI);

    INPUT_SetAxisScale(0x47, camera->TargetFocusRotation.z * CL_RAD_TO_DEG - 180.0f, 1);
    INPUT_SetAxisScale(0x46, camera->TargetFocusRotation.x * CL_RAD_TO_DEG, 1);

    camera->core.Rotation = camera->TargetFocusRotation;
}

// 0x00487c30 (PDB: 0x00489670)
void CAMERA_TurretProcess(Camera* camera)
{
    PlayerProp* prop = reinterpret_cast<PlayerProp*>(s_playerInstance->data);
    TurretState* tState = reinterpret_cast<TurretState*>(gPlayerSM->stateMachine.activeState->state);
    Instance* turretInst = gPlayer->engagedList[23].instance;
    TurretProp* tProp = reinterpret_cast<TurretProp*>(turretInst->data);
    TurretData* tData = reinterpret_cast<TurretData*>(turretInst->extraData);

    if ((camera->flags & 0x800) != 0) {
        camera->focusDistance = camera->targetFocusDistance;
    }

    cdc::Vector3 anchorPos = turretInst->position;
    anchorPos.z += tProp->fCameraAnchorOffset;

    cdc::Euler aimEuler = {};
    aimEuler.x = tState->fPitch;
    aimEuler.y = 0.0f;
    aimEuler.z = tData->fInitialZRot + tState->fYaw;

    cdc::Vector q = QuatFromEuler(aimEuler);

    cdc::Vector3 boomOffset = {};
    boomOffset.x = tProp->vCameraBoom.x;
    boomOffset.y = tProp->vCameraBoom.y;
    boomOffset.z = tProp->vCameraBoom.z;
    camera->targetPos = AddVec3(anchorPos, QuatRotateVector(q, boomOffset));

    cdc::Vector3 fwdAxis = {};
    fwdAxis.y = -1.0f;
    cdc::Vector3 aimFarPt = AddVec3(anchorPos, ScaleVec3(QuatRotateVector(q, fwdAxis), 10000.0f));

    cdc::Vector3 aimDir = SubVec3(aimFarPt, camera->targetPos);
    Normalize3(aimDir);
    camera->targetFocusPoint = AddVec3(camera->targetPos, ScaleVec3(aimDir, camera->targetFocusDistance));

    if (cameraTurretInterp > 0.0f) {
        cameraTurretInterp -= s_timeScale;
        if (cameraTurretInterp > 0.0f) {
            float t = CAMERA_BezierLine(
                prop->oldData.fAimInterpCtrl1,
                prop->oldData.fAimInterpCtrl2,
                1.0f - cameraTurretInterp / tProp->fCameraInterp);
            camera->core.position = LerpVec3(camera->core.position, camera->targetPos, t);
            camera->focusPoint = LerpVec3(camera->focusPoint, camera->targetFocusPoint, t);
        } else {
            camera->focusPoint = camera->targetFocusPoint;
            camera->core.position = camera->targetPos;
        }
    } else {
        camera->focusPoint = camera->targetFocusPoint;
        camera->core.position = camera->targetPos;
    }

    cdc::Vector3 lookVec = SubVec3(camera->focusPoint, camera->core.position);
    Normalize3(lookVec);

    float pitchSin = ClampFloat(lookVec.z, -1.0f, 1.0f);
    camera->TargetFocusRotation.x = std::asin(pitchSin);
    camera->TargetFocusRotation.y = 0.0f;
    camera->TargetFocusRotation.z = CAMERA_CircleAnglef(std::atan2(lookVec.y, lookVec.x) - CL_HALF_PI);

    INPUT_SetAxisScale(0x47, camera->TargetFocusRotation.z * CL_RAD_TO_DEG - 180.0f, 1);
    INPUT_SetAxisScale(0x46, camera->TargetFocusRotation.x * CL_RAD_TO_DEG, 1);

    camera->FocusRotation = camera->TargetFocusRotation;
    camera->core.Rotation = camera->TargetFocusRotation;
}
