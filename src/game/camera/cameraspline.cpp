#include "game/camera/cameraspline.h"
#include "game/camera/camera.h"
#include "game/camera/camerabehind.h"
#include "game/camera/camerafocus.h"
#include "game/camera/camerasupport.h"
#include "game/Stream/stream.h"
#include "game/instance.h"
#include "game/player/player.h"
#include <algorithm>
#include <cmath>
#include <cstring>

struct InputSystem {
    float GetAxisValue(int axis); // 0x004e3c50
};
extern InputSystem* INPUT_instances[]; // 0x011015c0

extern int FX_Frames;             // 0x01081e78 (PDB: 0x01082dc0)
extern float transition_camera;   // 0x010fc570 (PDB: 0x010fd770)
extern float transition_target;   // 0x010fc574 (PDB: 0x010fd774)

// File-scope globals from f:\tr7\game\camera\cameraSpline.c
static int splinecombat_combatFrames = 0;         // 0x010fda40 (PDB: 0x010fec40)
static float splinecombat_combatPct = 0.0f;       // 0x010fda44 (PDB: 0x010fec44)
static float splinecombat_combatPctVel = 0.0f;    // 0x010fda48 (PDB: 0x010fec48)
static float splinecombat_combatPctAccl = 0.0f;   // 0x010fda4c (PDB: 0x010fec4c)
static float splinecombat_targetDistVel = 0.0f;   // 0x010fda50 (PDB: 0x010fec50)
static float splinecombat_targetDistAccl = 0.0f;  // 0x010fda54 (PDB: 0x010fec54)
static float currentSplinecamLeadOffset = 0.0f;   // 0x010fda58 (PDB: 0x010fec58)
static float splinecamLeadOffsetVel = 0.0f;       // 0x010fda5c (PDB: 0x010fec5c)
static float splinecamLeadOffsetAccl = 0.0f;      // 0x010fda60 (PDB: 0x010fec60)
static float lock_control_speed = 0.0f;           // 0x010fda64 (PDB: 0x010fec64)
static float old_camera_zrot = 0.0f;              // 0x010fda68 (PDB: 0x010fec68)
float starting_lock_control_zrot = 0.0f;          // 0x010fda6c (PDB: 0x010fec6c)
static float starting_lock_control_x = 0.0f;      // 0x010fda70 (PDB: 0x010fec70)
static float starting_lock_control_y = 0.0f;      // 0x010fda74 (PDB: 0x010fec74)
static int starting_lock_time = 0;                // 0x010fda78 (PDB: 0x010fec78)
float splineLookX = 0.0f;                         // 0x010fda7c (PDB: 0x010fec7c)
float splineLookZ = 0.0f;                         // 0x010fda80 (PDB: 0x010fec80)
float splineLookVelX = 0.0f;                      // 0x010fda84 (PDB: 0x010fec84)
float splineLookVelZ = 0.0f;                      // 0x010fda88 (PDB: 0x010fec88)
float splineLookAcclX = 0.0f;                     // 0x010fda8c (PDB: 0x010fec8c)
float splineLookAcclZ = 0.0f;                     // 0x010fda90 (PDB: 0x010fec90)
static float splinecombat_targetDist = 2400.0f;   // 0x00f15ccc (PDB: 0x00f16004)

static inline int16_t ReadI16(const void* ptr)
{
    int16_t val = 0;
    std::memcpy(&val, ptr, sizeof(val));
    return val;
}

static inline void WriteI16(void* ptr, int16_t val)
{
    std::memcpy(ptr, &val, sizeof(val));
}

static inline cdc::Vector3 AddVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    out.x = a.x + b.x;
    out.y = a.y + b.y;
    out.z = a.z + b.z;
    out.w = a.w + b.w;
    return out;
}

static inline cdc::Vector3 SubVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    out.x = a.x - b.x;
    out.y = a.y - b.y;
    out.z = a.z - b.z;
    out.w = a.w - b.w;
    return out;
}

static inline cdc::Vector3 ScaleVec3(const cdc::Vector3& v, float s)
{
    cdc::Vector3 out;
    out.x = v.x * s;
    out.y = v.y * s;
    out.z = v.z * s;
    out.w = v.w * s;
    return out;
}

static inline float VecLen3(const cdc::Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float VecLen2(float x, float y)
{
    return std::sqrt(x * x + y * y);
}

static void SetSplineSlot(SplinecamData* dst, const Intro* spline, int slot)
{
    if (slot == 0) {
        dst->sourceSpline = spline->multiSpline;
        dst->sourceStartPoint = spline->position;
        dst->useStartPoint |= 0x1;
        WriteI16(&dst->sourceNumber, ReadI16(&spline->intronum));
    } else if (slot == 1) {
        dst->targetSpline = spline->multiSpline;
        dst->targetStartPoint = spline->position;
        dst->useStartPoint |= 0x2;
        WriteI16(&dst->targetNumber, ReadI16(&spline->intronum));
    } else {
        dst->thirdSpline = spline->multiSpline;
        dst->thirdStartPoint = spline->position;
        dst->useStartPoint |= 0x4;
        WriteI16(&dst->thirdNumber, ReadI16(&spline->intronum));
    }
}

static bool AssignLevelSplineSlot(int16_t uniqueId, SplinecamData* dst, int slot, bool updateStreamId)
{
    if (uniqueId == 0) {
        return true;
    }

    int streamId = 0;
    Intro* intro = INSTANCE_FindIntroUniqueID(uniqueId, &streamId);
    if (intro == nullptr) {
        if (slot == 0) {
            dst->sourceSpline = nullptr;
        } else if (slot == 1) {
            dst->targetSpline = nullptr;
        } else {
            dst->thirdSpline = nullptr;
        }
        return false;
    }

    SetSplineSlot(dst, intro, slot);
    if (updateStreamId) {
        theCamera.nextCam.Cinematic_Camera_StreamID = streamId;
    }
    return true;
}

// 0x00488130 (PDB: 0x00489b70)
void CAMERA_ClearSplineKeyValues(MultiSpline* spline)
{
    spline->SetFrameNumber(0.0f);
}

// 0x00488140 (PDB: 0x00489b80)
void CAMERA_ResetSplineCombatSettings(Camera* camera)
{
    (void)camera;
    splinecombat_combatFrames = 0;
    splinecombat_combatPct = 0.0f;
    splinecombat_combatPctVel = 0.0f;
    splinecombat_combatPctAccl = 0.0f;
    splinecombat_targetDist = 2400.0f;
    splinecombat_targetDistVel = 0.0f;
    splinecombat_targetDistAccl = 0.0f;
}

// 0x00488190 (PDB: 0x00489bd0)
void CAMERA_Lock_Control_Angle(Camera* camera)
{
    WriteI16(&camera->lock_control_angle, 1);
    lock_control_speed = -99999.0f;
    old_camera_zrot = 0.0f;
    starting_lock_time = 0;
    starting_lock_control_zrot = CAMERA_CircleAnglef(camera->focusInstance->rotation.z);

    float joyX = INPUT_instances[0]->GetAxisValue(0);
    starting_lock_control_x = joyX;
    float joyY = INPUT_instances[0]->GetAxisValue(1);
    starting_lock_control_y = joyY;

    float len = VecLen2(joyX, joyY);
    if (len == 0.0f) {
        len = 0.0001f;
    }
    float invLen = 1.0f / len;
    starting_lock_control_x *= invLen;
    starting_lock_control_y *= invLen;
}

// 0x004882a0 (PDB: 0x00489ce0)
void CAMERA_Unlock_Control_Angle(Camera* camera)
{
    WriteI16(&camera->lock_control_angle, 0);
}

// 0x004882b0 (PDB: 0x00489cf0)
void CAMERA_SetSpline0(Camera* camera, Intro* spline, int streamID, int alternate)
{
    if (spline == nullptr) {
        return;
    }

    SplinecamData* dst = alternate ? &camera->nextCam.alternateSplinecamData : &camera->nextCam.materialSplinecamData;
    SetSplineSlot(dst, spline, 0);
    if (alternate == 0) {
        camera->nextCam.Cinematic_Camera_StreamID = streamID;
    }
}

// 0x00488350 (PDB: 0x00489d90)
void CAMERA_SetSpline1(Camera* camera, Intro* spline, int streamID, int alternate)
{
    (void)streamID;
    if (spline == nullptr) {
        return;
    }

    SplinecamData* dst = alternate ? &camera->nextCam.alternateSplinecamData : &camera->nextCam.materialSplinecamData;
    SetSplineSlot(dst, spline, 1);
}

// 0x004883f0 (PDB: 0x00489e30)
void CAMERA_SetSpline2(Camera* camera, Intro* spline, int streamID, int alternate)
{
    (void)streamID;
    if (spline == nullptr) {
        return;
    }

    SplinecamData* dst = alternate ? &camera->nextCam.alternateSplinecamData : &camera->nextCam.materialSplinecamData;
    SetSplineSlot(dst, spline, 2);
}

// 0x00488490 (PDB: 0x00489ed0)
void CAMERA_SetSplineWidths(int srcW, int tgtW, int srcWZ, int tgtWZ)
{
    theCamera.nextCam.splinecamParams.sourceSplineWidth = static_cast<float>(srcW);
    theCamera.nextCam.splinecamParams.targetSplineWidth = static_cast<float>(tgtW);
    theCamera.nextCam.splinecamParams.sourceSplineWidthZ = static_cast<float>(srcWZ);
    theCamera.nextCam.splinecamParams.targetSplineWidthZ = static_cast<float>(tgtWZ);
}

// 0x004884c0 (PDB: 0x00489f00)
int CAMERA_SourceNumber(Camera* camera)
{
    int16_t mode = ReadI16(&camera->mode);
    if (mode != 4 && mode != 3) {
        return -1;
    }

    if (camera->alternateCameraFlag != 0 && camera->data.Cinematic.alternateSplinecamData.sourceSpline != nullptr) {
        return ReadI16(&camera->data.Cinematic.alternateSplinecamData.sourceNumber);
    }
    return ReadI16(&camera->data.Cinematic.materialSplinecamData.sourceNumber);
}

// 0x00488500 (PDB: 0x00489f40)
int CAMERA_ProcessSplineCamLevelData(Camera* camera, SplineCameraLevelData* data)
{
    (void)camera;

    if (!AssignLevelSplineSlot(data->CameraSpline0, &theCamera.nextCam.materialSplinecamData, 0, true)) {
        return 0;
    }
    if (!AssignLevelSplineSlot(data->CameraSpline1, &theCamera.nextCam.materialSplinecamData, 1, false)) {
        return 0;
    }
    AssignLevelSplineSlot(data->CameraSpline2, &theCamera.nextCam.materialSplinecamData, 2, false);

    AssignLevelSplineSlot(data->CameraAlternateSpline0, &theCamera.nextCam.alternateSplinecamData, 0, false);
    AssignLevelSplineSlot(data->CameraAlternateSpline1, &theCamera.nextCam.alternateSplinecamData, 1, false);
    AssignLevelSplineSlot(data->CameraAlternateSpline2, &theCamera.nextCam.alternateSplinecamData, 2, false);

    if (data->CameraSpline0Width != 0 || data->CameraSpline1Width != 0 ||
        data->CameraSpline0WidthZ != 0 || data->CameraSpline1WidthZ != 0) {
        theCamera.nextCam.splinecamParams.sourceSplineWidth = static_cast<float>(data->CameraSpline0Width);
        theCamera.nextCam.splinecamParams.targetSplineWidth = static_cast<float>(data->CameraSpline1Width);
        theCamera.nextCam.splinecamParams.sourceSplineWidthZ = static_cast<float>(data->CameraSpline0WidthZ);
        theCamera.nextCam.splinecamParams.targetSplineWidthZ = static_cast<float>(data->CameraSpline1WidthZ);
    }

    auto dispatchEvent = [](int eventId, int value) {
        if (value != 0) {
            CAMERA_SetValueByEvent(&theCamera, eventId, value);
        }
    };

    dispatchEvent(0x14c, data->CameraZoomDist);
    dispatchEvent(0xd8, data->CameraVelocity);
    dispatchEvent(0xd9, data->CameraDampening);
    dispatchEvent(0xda, data->CameraTargetVelocity);
    dispatchEvent(0xdb, data->CameraTargetDampening);
    dispatchEvent(0xdd, data->CameraCutFlag);
    dispatchEvent(0xde, data->CameraCrossfade);
    dispatchEvent(0x108, data->CameraLeadAmount);
    dispatchEvent(0x138, data->CameraCombatDistance);
    dispatchEvent(0x139, data->CameraCombatMinDistance);
    dispatchEvent(0x172, data->CameraCombatZOffset);
    dispatchEvent(0x175, data->CameraCombatLockedOut);
    dispatchEvent(0x19a, data->CameraDisableLookaroundFlag);
    dispatchEvent(0x19b, data->CameraCombatFraming);
    dispatchEvent(0xdc, data->CameraFollowCamOverrideEnabled);

    // Original copy-paste bug: tests CameraCutAngle (+0x0a) but passes CameraCutFlag (+0x3c).
    if (data->CameraCutAngle != 0) {
        CAMERA_SetValueByEvent(&theCamera, 0x1a1, data->CameraCutFlag);
    }

    if (data->CameraInterestInstId != 0) {
        theCamera.nextCam.splinecamParams.interestInst = INSTANCE_Find(data->CameraInterestInstId);
    }

    dispatchEvent(0x1a2, data->CameraInterestTune);
    dispatchEvent(0x1a3, data->CameraSwitchToFollowDist);
    dispatchEvent(0x1a4, data->CameraFollowHighTiltDistance);
    dispatchEvent(0x1a5, data->CameraFollowMedTiltDistance);
    dispatchEvent(0x1a6, data->CameraFollowZeroTiltDistance);
    dispatchEvent(0x1a7, data->CameraFollowLowTiltDistance);
    dispatchEvent(0x1a8, data->CameraFollowHighTiltAngle);

    // Original copy-paste bug: tests CameraFollowMedTiltAngle (+0x4e) but passes CameraFollowHighTiltAngle (+0x4a).
    if (data->CameraFollowMedTiltAngle != 0) {
        CAMERA_SetValueByEvent(&theCamera, 0x1a9, data->CameraFollowHighTiltAngle);
    }

    dispatchEvent(0x1aa, data->CameraFollowZeroTiltAngle);
    dispatchEvent(0x1ab, data->CameraFollowLowTiltAngle);
    dispatchEvent(0x1ac, data->CameraFollowVerticalBias);

    return 1;
}

// 0x00488b50 (PDB: 0x0048a590)
void CAMERA_SetSplineCameraFromLevelData(Camera* camera, Level* level, int index)
{
    CAMERA_ProcessSplineCamLevelData(camera, &level->splineCameraData[index]);
}

// 0x00488b70 (PDB: 0x0048a5b0)
float CAMERA_NearestPointOnLine(cdc::Vector3* outPt, cdc::Vector3* a, cdc::Vector3* b, cdc::Vector3* c)
{
    cdc::Vector3 dir = SubVec3(*b, *a);
    float dirDot = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    float aDot = a->x * dir.x + a->y * dir.y + a->z * dir.z;
    float cDot = c->x * dir.x + c->y * dir.y + c->z * dir.z;

    float t = aDot - cDot;
    if (dirDot != 0.0f) {
        t = -(t / dirDot);
    }

    *outPt = AddVec3(*a, ScaleVec3(dir, t));
    return t;
}

// 0x00488c60 (PDB: 0x0048a6a0)
void CAMERA_DoWideSplineCalc(Camera* camera, cdc::Vector3* pos, Spline* spline, int key, float t, cdc::Vector3* outOffset, float width1, float width2)
{
    (void)pos;

    float blend = width1 / width2;
    float absW2 = (width2 < 0.0f) ? -width2 : width2;

    const cdc::Vector3& k0 = spline->keys[key].point;
    const cdc::Vector3& k1 = spline->keys[key + 1].point;

    float sideX = (k0.y - k1.y) * 10.0f;
    float sideY = -(k0.x - k1.x) * 10.0f;
    float invLen = (sideX == 0.0f && sideY == 0.0f) ? 0.0f : (1.0f / std::sqrt(sideX * sideX + sideY * sideY));
    float nx1 = sideX * invLen;
    float ny1 = sideY * invLen;

    const cdc::Vector3& focusPos = camera->focusInstance->position;
    float dot1 = (nx1 * focusPos.x + ny1 * focusPos.y) - (nx1 * k0.x + ny1 * k0.y);
    if (dot1 > absW2) {
        dot1 = absW2;
    }
    if (dot1 < -absW2) {
        dot1 = -absW2;
    }

    float offX1 = nx1 * dot1 * blend;
    float offY1 = ny1 * dot1 * blend;

    if ((t > 0.5f && key < static_cast<int>(spline->numKeys) - 2) || (t < 0.5f && key > 0)) {
        float alpha;
        const cdc::Vector3* pA;
        const cdc::Vector3* pB;
        if (t > 0.5f) {
            alpha = t - 0.5f;
            pA = &spline->keys[key + 2].point;
            pB = &spline->keys[key + 1].point;
        } else {
            alpha = t + 0.5f;
            pA = &spline->keys[key].point;
            pB = &spline->keys[key - 1].point;
        }

        float dx2 = pA->x - pB->x;
        float dy2 = pA->y - pB->y;
        float invLen2 = (dy2 == 0.0f && dx2 == 0.0f) ? 0.0f : (1.0f / std::sqrt(dx2 * dx2 + dy2 * dy2));

        // Original code normalizes (dy2, dx2) without negating dx2 in the adjacent segment term.
        float nx2 = dy2 * invLen2;
        float ny2 = dx2 * invLen2;
        float dot2 = (nx2 * focusPos.x + ny2 * focusPos.y) - (nx2 * pB->x + ny2 * pB->y);
        if (dot2 > absW2) {
            dot2 = absW2;
        }
        if (dot2 < -absW2) {
            dot2 = -absW2;
        }

        float offX2 = nx2 * dot2 * blend;
        float offY2 = ny2 * dot2 * blend;

        float offX = (1.0f - alpha) * offX1 + alpha * offX2;
        float offY = (1.0f - alpha) * offY1 + alpha * offY2;

        outOffset->x += offX;
        outOffset->y += offY;
        if (camera->data.Cinematic.lookat_spline != 0) {
            camera->targetFocusPoint.x += offX;
            camera->targetFocusPoint.y += offY;
        }
    } else {
        outOffset->x += offX1;
        outOffset->y += offY1;
        if (camera->data.Cinematic.lookat_spline != 0) {
            camera->targetFocusPoint.x += offX1;
            camera->targetFocusPoint.y += offY1;
        }
    }
}

// 0x00488f70 (PDB: 0x0048a9b0)
void CAMERA_DoWideSplineCalcZ(Camera* camera, cdc::Vector3* outOffset, Spline* spline, cdc::Vector3* refPos, float widthZ1, float widthZ2)
{
    (void)spline;

    bool bothNeg = (widthZ1 < 0.0f && widthZ2 < 0.0f);
    bool clampNegToZero = (widthZ1 < 0.0f && widthZ2 >= 0.0f);
    bool clampPosToZero = (widthZ1 >= 0.0f && widthZ2 < 0.0f);

    float absW1 = (widthZ1 < 0.0f) ? -widthZ1 : widthZ1;
    float absW2 = (widthZ2 < 0.0f) ? -widthZ2 : widthZ2;
    if (absW2 <= 0.0001f) {
        return;
    }

    float scale = absW1 / absW2;
    if (bothNeg) {
        scale = -scale;
    }

    float dz = (CAMERA_FocusHeight(camera) + camera->focusInstance->position.z) - refPos->z;
    if (clampPosToZero && dz > 0.0f) {
        dz = 0.0f;
    }
    if (clampNegToZero && dz < 0.0f) {
        dz = 0.0f;
    }
    if (dz > widthZ2) {
        dz = widthZ2;
    }
    if (dz < -widthZ2) {
        dz = -widthZ2;
    }

    float addZ = dz * scale;
    outOffset->z += addZ;
    if (camera->data.Cinematic.lookat_spline != 0) {
        camera->targetFocusPoint.z += addZ;
    }
}

// 0x004890e0 (PDB: 0x0048ab20)
float CAMERA_SplineGetNearestPointTarget(Camera* camera, Spline* spline, cdc::Vector3* targetPt, int* outKey, cdc::Vector3* outPt, float* outT)
{
    float bestSegFrac = 0.0f;
    float accumTime = 0.0f;
    float bestDistSq = 1638400000.0f;

    if (spline->numKeys == 1) {
        *outPt = spline->GetPoint(0, spline->keys[0].t0);
        return 0.0f;
    }

    int numSegments = static_cast<int>(spline->numKeys) - 1;
    for (int key = 0; key < numSegments; ++key) {
        float segDuration = spline->keys[key].count;
        float prevTime = 0.0f;
        float nextTime = std::min(4.0f, segDuration);

        for (float sampleEnd = 4.0f; sampleEnd <= segDuration + 4.0f; sampleEnd += 4.0f) {
            float span = nextTime - prevTime;
            cdc::Vector3 pA = spline->GetPoint(static_cast<uint16_t>(key), spline->keys[key].t0 + prevTime);
            cdc::Vector3 pB = spline->GetPoint(static_cast<uint16_t>(key), spline->keys[key].t0 + nextTime);

            cdc::Vector3 candPt;
            float lineT = CAMERA_NearestPointOnLine(&candPt, &pA, &pB, &camera->targetFocusPoint);
            if (lineT < 0.0f) {
                lineT = 0.0f;
                candPt = pA;
            } else if (lineT > 1.0f) {
                lineT = 1.0f;
                candPt = pB;
            }

            cdc::Vector3 diff = SubVec3(candPt, *targetPt);
            float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            if (distSq <= bestDistSq) {
                bestDistSq = distSq;
                *outPt = candPt;
                *outKey = key;
                float localTime = prevTime + lineT * span;
                *outT = accumTime + localTime + spline->keys[0].t0;
                bestSegFrac = localTime / segDuration;
            }

            prevTime = nextTime;
            nextTime = std::min(sampleEnd + 4.0f, segDuration);
        }

        accumTime += segDuration;
    }

    return bestSegFrac;
}

// 0x004893c0 (PDB: 0x0048ae00)
static void CAMERA_CalcSplinecamPos(Camera* camera, SplinecamData* splineData, cdc::Vector3* camPos, cdc::Vector3* targetPos)
{
    float t = 0.0f;
    cdc::Vector3 nearestTargetPt;
    *targetPos = camera->targetFocusPoint;

    float segFrac = CAMERA_SplineGetNearestPointTarget(
        camera,
        splineData->targetSpline->positional,
        &camera->targetFocusPoint,
        &camera->data.Cinematic.splinecam_currkey,
        &nearestTargetPt,
        &t);

    if (splineData->thirdSpline != nullptr) {
        splineData->thirdSpline->SetFrameNumber(t);
        *targetPos = splineData->thirdSpline->GetPoint();
    }

    if ((splineData->thirdSpline != nullptr || (splineData->useStartPoint & 0x4) != 0) &&
        camera->data.Cinematic.lookat_spline == 0) {
        *targetPos = AddVec3(camera->targetFocusPoint, SubVec3(*targetPos, nearestTargetPt));
    }

    splineData->sourceSpline->SetFrameNumber(t);
    *camPos = splineData->sourceSpline->GetPoint();

    const SplinecamParams& params = camera->data.Cinematic.splinecamParams;
    if (params.sourceSplineWidth != 0.0f && params.targetSplineWidth != 0.0f) {
        CAMERA_DoWideSplineCalc(
            camera,
            &nearestTargetPt,
            splineData->targetSpline->positional,
            camera->data.Cinematic.splinecam_currkey,
            segFrac,
            camPos,
            params.sourceSplineWidth,
            params.targetSplineWidth);
    }

    if (params.sourceSplineWidthZ != 0.0f && params.targetSplineWidthZ != 0.0f) {
        CAMERA_DoWideSplineCalcZ(
            camera,
            camPos,
            splineData->targetSpline->positional,
            &nearestTargetPt,
            params.sourceSplineWidthZ,
            params.targetSplineWidthZ);
    }
}

// 0x004895a0 (PDB: 0x0048afe0)
void CAMERA_SplineProcess(Camera* camera)
{
    int combatActive = 0;
    if ((camera->instance_mode & 0x180) != 0) {
        ++splinecombat_combatFrames;
        if (splinecombat_combatFrames > 10) {
            splinecombat_combatFrames = 10;
        }
        combatActive = 1;
    } else {
        --splinecombat_combatFrames;
        if (splinecombat_combatFrames > 0) {
            combatActive = 1;
        } else {
            combatActive = 0;
            splinecombat_combatFrames = 0;
        }
    }

    if (splinecombat_combatPct > 0.005f) {
        combatActive = 1;
    }

    SplinecamData* splineData = &camera->data.Cinematic.materialSplinecamData;
    if (camera->alternateCameraFlag != 0 &&
        camera->data.Cinematic.alternateSplinecamData.sourceSpline != nullptr &&
        camera->data.Cinematic.alternateSplinecamData.targetSpline != nullptr) {
        splineData = &camera->data.Cinematic.alternateSplinecamData;
    }

    if ((camera->flags & 0x10000) != 0) {
        CAMERA_ChangeToUnderWater(camera, camera->focusInstance);
        camera->restoreToSplineFlag = 1;
        CAMERA_GenericCameraProcess(camera);
        return;
    }

    if (gBehindCamOverride != 0) {
        CAMERA_SaveMode(camera, ReadI16(&camera->mode));
        CAMERA_SetMode(camera, 0);
        camera->CenterFlag = -1000.0f;
        camera->forcedRotation = -1000.0f;
        camera->distanceState = 1;
        camera->signalFocusDistance = 1700.0f;
        camera->targetFocusDistance = 1700.0f;
        camera->extraXRot = 0.0f;
        if (gBehindCamOverride == 2) {
            gBehindCamOverride = 0;
            CAMERA_GenericCameraProcess(camera);
            return;
        }
        camera->restoreToSplineFlag = 1;
        CAMERA_GenericCameraProcess(camera);
        return;
    }

    CAMERA_SetFocus(camera, &camera->targetFocusPoint);
    cdc::Vector3 baseFocus = camera->targetFocusPoint;

    float camToFocusDist = VecLen3(SubVec3(baseFocus, camera->core.position));
    if (camToFocusDist > 512.0f) {
        float xyVel = std::min(camera->instance_xyvel, 60.0f);
        float clampedDist = std::min(camToFocusDist, 2500.0f);
        float leadFactor = (camera->leadAmount != 0.0f) ? (camera->leadAmount * 0.001f) : 0.2f;
        float desiredLead = (1.0f / 60.0f) * clampedDist * 0.5f * xyVel * leadFactor;

        if (desiredLead <= currentSplinecamLeadOffset) {
            if (camera->data.Cinematic.splinecamParams.targetDampening > 1500.0f) {
                currentSplinecamLeadOffset = desiredLead;
            } else {
                CriticalDampValueControl(
                    &currentSplinecamLeadOffset,
                    desiredLead,
                    &splinecamLeadOffsetVel,
                    &splinecamLeadOffsetAccl,
                    24.0f,
                    2.0f);
            }
        } else {
            CriticalDampValueControl(
                &currentSplinecamLeadOffset,
                desiredLead,
                &splinecamLeadOffsetVel,
                &splinecamLeadOffsetAccl,
                16.0f,
                4.0f);
        }

        float yaw = camera->focusInstance->rotation.z + 1.5707964f;
        float leadScale = (1.0f - splinecombat_combatPct) * currentSplinecamLeadOffset;
        baseFocus.x -= std::cos(yaw) * leadScale;
        baseFocus.y -= std::sin(yaw) * leadScale;
    }

    camera->targetFocusPoint = baseFocus;
    camera->real_focuspoint = baseFocus;

    if (splineData->targetSpline == nullptr || splineData->sourceSpline == nullptr) {
        return;
    }

    cdc::Vector3 splineCamPos;
    cdc::Vector3 splineTargetPos;
    CAMERA_CalcSplinecamPos(camera, splineData, &splineCamPos, &splineTargetPos);

    SplinecamParams& params = camera->data.Cinematic.splinecamParams;
    if (params.zoomDist != 0.0f) {
        cdc::Vector3 zoomVec = SubVec3(splineCamPos, splineTargetPos);
        float zoomLen = VecLen3(zoomVec);
        if (zoomLen < params.zoomDist) {
            splineCamPos = AddVec3(splineTargetPos, ScaleVec3(zoomVec, params.zoomDist / zoomLen));
        }
    }

    if (combatActive != 0) {
        float targetPct = ((camera->instance_mode & 0x180) != 0) ? 1.0f : 0.0f;
        CriticalDampValueControl(
            &splinecombat_combatPct,
            targetPct,
            &splinecombat_combatPctVel,
            &splinecombat_combatPctAccl,
            0.03125f,
            6.0f);

        splineTargetPos = AddVec3(
            ScaleVec3(splineTargetPos, 1.0f - splinecombat_combatPct),
            ScaleVec3(camera->targetFocusPoint, splinecombat_combatPct));

        float combatMinDist = params.combatCamMinDist;
        if ((camera->instance_mode & 0x180) != 0) {
            CriticalDampValueControl(
                &splinecombat_targetDist,
                params.combatCamDist,
                &splinecombat_targetDistVel,
                &splinecombat_targetDistAccl,
                80.0f,
                3.0f);
        }

        cdc::Vector3 targetToCam = SubVec3(splineTargetPos, splineCamPos);
        float curDist = VecLen3(targetToCam);
        float pushFactor;
        if (curDist < combatMinDist) {
            pushFactor = ((combatMinDist - curDist) * splinecombat_combatPct) / curDist;
        } else if (curDist > splinecombat_targetDist) {
            pushFactor = ((splinecombat_targetDist - curDist) * splinecombat_combatPct) / curDist;
        } else {
            pushFactor = 0.0f;
        }
        splineCamPos = SubVec3(splineCamPos, ScaleVec3(targetToCam, pushFactor));

        if (params.combatCamZOffset != 0.0f) {
            float planarDist = cdc::Dist2(splineCamPos, splineTargetPos);
            float combatCamZ = (planarDist * (1.0f / 4500.0f) * params.combatCamZOffset) + splineTargetPos.z;
            splineCamPos.z = combatCamZ * splinecombat_combatPct + splineCamPos.z * (1.0f - splinecombat_combatPct);
        }
    } else if (params.interestInst != nullptr) {
        float tune = params.interestInstTune * (1.0f / 4096.0f);
        splineTargetPos = AddVec3(
            ScaleVec3(camera->targetFocusPoint, 1.0f - tune),
            ScaleVec3(params.interestInst->position, tune));

        cdc::Vector3 targetToCam = SubVec3(splineTargetPos, splineCamPos);
        float dist = VecLen3(targetToCam);
        if (dist < params.combatCamMinDist) {
            splineCamPos = SubVec3(splineCamPos, ScaleVec3(targetToCam, params.combatCamMinDist / dist));
        }
    }

    camera->targetFocusPoint = splineTargetPos;
    camera->real_focuspoint = splineTargetPos;

    if (camera->cutFlag != 0) {
        camera->targetPos = splineCamPos;
        camera->focusPoint = splineTargetPos;
        camera->cutFlag = 0;
        transition_camera = 0.0f;
        transition_target = 0.0f;
        cdc::Vector3 zeroVec = {};
        camera->targetPointVel = zeroVec;
        camera->targetPointAccl = zeroVec;
        camera->focusPointVel = zeroVec;
        camera->focusPointAccl = zeroVec;
    }

    cdc::Vector3 desiredPos = camera->targetPos;
    CriticalDampPositionControl(
        &desiredPos,
        &splineCamPos,
        &camera->targetPointVel,
        &camera->targetPointAccl,
        params.cameraVelocity + camera->instance_vel * (1.0f / 3.0f),
        params.cameraDampening * (1.0f / 256.0f),
        &transition_camera);

    camera->maxVel = 512.0f;
    float tgtMaxVel;
    float tgtDamp;
    if (combatActive != 0) {
        tgtMaxVel = camera->instance_vel * 0.5f + 40.0f;
        tgtDamp = 8.0f;
    } else {
        tgtMaxVel = camera->instance_vel * 0.5f + params.targetVelocity;
        tgtDamp = params.targetDampening * (1.0f / 256.0f);
    }

    CriticalDampPositionControl(
        &camera->focusPoint,
        &camera->targetFocusPoint,
        &camera->focusPointVel,
        &camera->focusPointAccl,
        tgtMaxVel,
        tgtDamp,
        &transition_target);

    camera->targetPos = desiredPos;
    camera->core.position = desiredPos;

    CAMERA_CalcRotation(&camera->core.Rotation, &camera->focusPoint, &camera->targetPos);
    camera->CollisionTargetFocusRotation = camera->core.Rotation;
    camera->TargetFocusRotation = camera->core.Rotation;
    camera->FocusRotation = camera->core.Rotation;
    WriteI16(&camera->always_rotate_flag, 0);
    camera->rotState = 0;

    float finalDist = VecLen3(SubVec3(camera->focusPoint, camera->targetPos));
    camera->collisionTargetFocusDistance = finalDist;
    camera->targetFocusDistance = finalDist;
    camera->focusDistance = finalDist;

    if (ReadI16(&camera->lock_control_angle) == 0) {
        camera->lagZ = camera->core.Rotation.z;
    } else {
        if (starting_lock_time != 0) {
            starting_lock_time += FX_Frames;
        }

        float joyX = INPUT_instances[0]->GetAxisValue(0);
        float joyY = INPUT_instances[0]->GetAxisValue(1);
        if (std::fabs(joyX) <= 0.8f && std::fabs(joyY) <= 0.8f) {
            WriteI16(&camera->lock_control_angle, 0);
        }

        if (ReadI16(&camera->lock_control_angle) != 0) {
            float targetRot = CAMERA_CircleAnglef(camera->focusInstance->rotation.z + camera->core.Rotation.z);
            float invJoyLen = 1.0f / VecLen2(joyX, joyY);
            float dot = (joyX * invJoyLen) * starting_lock_control_x + (joyY * invJoyLen) * starting_lock_control_y;

            if (dot >= 0.93f && starting_lock_time == 0) {
                starting_lock_time = 1;
            }
            if (starting_lock_time > 0) {
                if (starting_lock_time < 30) {
                    float diff = CAMERA_AngleDifference(starting_lock_control_zrot, targetRot);
                    AngleMoveToward(&starting_lock_control_zrot, targetRot, diff / (30.0f - static_cast<float>(starting_lock_time)));
                } else {
                    WriteI16(&camera->lock_control_angle, 0);
                }
            }
        }

        camera->lagZ = camera->core.Rotation.z;
    }

    camera->core.Rotation.x += splineLookX;
    camera->core.Rotation.z += splineLookZ;
}

// 0x00489ff0 (PDB: 0x0048bab0)
void CAMERA_CalcFollowSplinecamPos(Camera* camera, cdc::Vector3* camPos, cdc::Vector3* targetPos)
{
    float t = 0.0f;
    cdc::Vector3 nearestTargetPt;
    int currKey = ReadI16(&camera->followSplineData.splinecam_currkey);

    float segFrac = CAMERA_SplineGetNearestPointTarget(
        camera,
        camera->followSplineData.targetSpline->positional,
        &camera->targetFocusPoint,
        &currKey,
        &nearestTargetPt,
        &t);

    WriteI16(&camera->followSplineData.splinecam_currkey, static_cast<int16_t>(currKey));
    *targetPos = nearestTargetPt;

    MultiSpline* sourceSpline = camera->followSplineData.sourceSpline;
    float numFrames = sourceSpline->GetNumFrames();
    if (t > numFrames) {
        t = numFrames;
    }
    sourceSpline->SetFrameNumber(t);
    *camPos = sourceSpline->GetPoint();

    if (camera->followSplineData.sourceSplineWidth != 0.0f &&
        camera->followSplineData.targetSplineWidth != 0.0f) {
        CAMERA_DoWideSplineCalc(
            camera,
            &nearestTargetPt,
            camera->followSplineData.targetSpline->positional,
            ReadI16(&camera->followSplineData.splinecam_currkey),
            segFrac,
            camPos,
            camera->followSplineData.sourceSplineWidth,
            camera->followSplineData.targetSplineWidth);
    }

    if (camera->followSplineData.sourceSplineWidthZ != 0.0f &&
        camera->followSplineData.targetSplineWidthZ != 0.0f) {
        CAMERA_DoWideSplineCalcZ(
            camera,
            camPos,
            camera->followSplineData.targetSpline->positional,
            &nearestTargetPt,
            camera->followSplineData.sourceSplineWidthZ,
            camera->followSplineData.targetSplineWidthZ);
    }
}
