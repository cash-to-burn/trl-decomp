#include "game/camera/camera.h"
#include "game/pc/pcinput.h"
#include "game/player/player.h"
#include <cmath>
#include <cstring>

// Subsystem globals
// 0x010fc5a0 (PDB: 0x010fd7a0 / ?theCamera@@3UCamera@@A)
Camera theCamera;

// 0x01075ac8
uint32_t g_cameraAxisFlags = 0;

// 0x010fc360 - 0x010fc560 (PDB: 0x001ed560 / camera_shakeOffset)
int16_t camera_shakeOffset[256];

// 0x010fc560 - 0x010fc56c (PDB: 0x001ed760)
float Cam_Vertigo_deg = 0.0f;
float Cam_Vertigo_deg2 = 0.0f;
float Cam_Vertigo_Ramp_Amount = 0.0f;
float Cam_Vertigo_Ramp_Speed = 0.0f;

static const float CAMERA_PI = 3.14159265358979323846f;
static const float CAMERA_TWOPI = 6.28318530717958647692f;
static const float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
static const float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

// 0x0048ca00 (PDB: 0x0048e510)
float CAMERA_CircleAnglef(float angle)
{
    if (angle >= 0.0f && angle < CAMERA_TWOPI) {
        return angle;
    }
    float rem = std::fmod(angle, CAMERA_TWOPI);
    if (rem < 0.0f) {
        rem += CAMERA_TWOPI;
    }
    return rem;
}

// 0x0048ca90 (PDB: 0x0048ee90)
float CAMERA_AngleDifference(float a, float b)
{
    float diff = a - b;
    if (diff >= -CAMERA_PI && diff <= CAMERA_PI) {
        return diff;
    }
    diff = std::fmod(diff, CAMERA_TWOPI);
    if (diff > CAMERA_PI) {
        diff -= CAMERA_TWOPI;
    } else if (diff < -CAMERA_PI) {
        diff += CAMERA_TWOPI;
    }
    return diff;
}

// 0x0047c030 (PDB: 0x0047d9d0)
void CAMERA_Initialize(Camera* camera)
{
    std::memset(camera, 0, sizeof(Camera));

    // Fill shake lookup table using MSVC LCG
    uint32_t seed = 0x12345678;
    for (int i = 0; i < 256; ++i) {
        seed = seed * 0x41c64e6d + 0x3039;
        int val = (seed >> 16) & 0x7fff;
        val = (val & 0xff) - 0x80;
        camera_shakeOffset[i] = static_cast<int16_t>(val);
    }

    camera->core.Rotation.x = 6.195919f;
    camera->FocusRotation.x = 6.195919f;
    camera->smooth = 1.0f;
    camera->targetFocusDistance = 2000.0f;
    camera->focusDistance = 2000.0f;
    camera->collisionTargetFocusDistance = 2000.0f;
    camera->signalFocusDistance = 2000.0f;
}

// 0x0047c570 (PDB: 0x0047df10)
void CAMERA_Lock(Camera* camera, int lockBits)
{
    camera->lock |= lockBits;
}

// 0x0047c590 (PDB: 0x0047df30)
void CAMERA_Unlock(Camera* camera, int lockBits)
{
    camera->lock &= ~lockBits;
}

// 0x0047c5b0 (PDB: 0x0047df50)
void CAMERA_SetSmoothValue(Camera* camera, int smooth)
{
    float fSmooth = static_cast<float>(smooth);
    camera->smooth = fSmooth;
    camera->nextCam.followCamSmooth = fSmooth;

    if (smooth == 0) {
        uint16_t m = camera->mode;
        if (m == 2 || m == 0 || m == 1 || m == 0xe) {
            camera->focusPoint = camera->targetFocusPoint;
        }
    }
}

// 0x0047c610 (PDB: 0x0047dfb0)
void CAMERA_Adjust_tilt_stay_in_place(Camera* camera, int tilt)
{
    float angle = CAMERA_CircleAnglef(static_cast<float>(tilt) * DEG_TO_RAD);
    camera->signalRot.x = angle;
    camera->tiltState = 0;

    if ((camera->instance_mode & 0x400000) == 0) {
        camera->TargetFocusRotation.x = angle;
        float base = CAMERA_CircleAnglef(camera->FocusRotation.x);
        float target = CAMERA_CircleAnglef(camera->tilt + camera->TargetFocusRotation.x);
        camera->extraXRot = CAMERA_AngleDifference(base, target);
    }
}

// 0x0047c6a0 (PDB: 0x0047e040)
void CAMERA_Adjust_tilt(Camera* camera, int tilt)
{
    camera->tiltState = 1;
    float val = CAMERA_CircleAnglef(static_cast<float>(tilt) * DEG_TO_RAD);
    camera->signalRot.x = val;
    camera->nextCam.splinecamParams.followTilt = val;

    if (camera->smooth == 0.0f || camera->cutFlag != 0) {
        camera->tilt = 0.0f;
        camera->extraXRot = 0.0f;
        camera->TargetFocusRotation.x = val;
        camera->FocusRotation.x = val;
        camera->tfaceTilt = val;
    }
}

// 0x0047c720 (PDB: 0x0047e0c0)
void CAMERA_Adjust_distance(Camera* camera, int dist)
{
    camera->distanceState = 1;
    float fDist = static_cast<float>(dist);
    camera->signalFocusDistance = fDist;

    if (camera->smooth == 0.0f) {
        camera->focusDistance = fDist;
        camera->targetFocusDistance = fDist;
        camera->collisionTargetFocusDistance = fDist;
    }
    camera->nextCam.splinecamParams.followDistance = fDist;
}

// 0x0047c770 (PDB: 0x0047e110)
void CAMERA_Adjust_ZOffset(Camera* camera, int offset)
{
    camera->ZOffset = static_cast<float>(offset);
}

// 0x0047c780 (PDB: 0x0047e120)
void CAMERA_Adjust_rotation(Camera* camera, int rot)
{
    camera->forced_movement = 0;
    camera->rotState = 1;
    float val = CAMERA_CircleAnglef(static_cast<float>(rot) * DEG_TO_RAD);
    camera->signalRot.z = val;

    if (camera->followDragToRotateSpeed != 0.0f) {
        camera->TargetFocusRotation.z = val;
    }

    if (camera->smooth == 0.0f || camera->cutFlag != 0) {
        camera->TargetFocusRotation.z = val;
        camera->FocusRotation.z = val;
        camera->teleportZRot = val;
        camera->CollisionTargetFocusRotation.z = camera->signalRot.z;
    }

    camera->nextCam.splinecamParams.followRotation = val;
    camera->always_rotate_flag = 1;
}

// 0x0047c820 (PDB: 0x0047e1c0)
void CAMERA_Adjust_roll(int roll, int duration)
{
    float target = static_cast<float>(roll) * DEG_TO_RAD;
    if (duration == 0) {
        theCamera.current_roll_amount = target;
        theCamera.roll_inc = 0.0f;
        theCamera.roll_target = target;
    } else {
        theCamera.roll_inc = (target - theCamera.current_roll_amount) / static_cast<float>(duration);
        theCamera.roll_target = target;
    }
}

// 0x0047dc00 (PDB: 0x0047f5a0)
void CAMERA_SetAxisInversionX(bool inverted)
{
    INPUT_SetAxisScale(12, inverted ? -1.0f : 1.0f, 1);
}

// 0x0047dc30 (PDB: 0x0047f5d0)
void CAMERA_SetAxisInversionY(bool inverted)
{
    if (inverted) {
        INPUT_SetAxisScale(13, 1.0f, 1);
        g_cameraAxisFlags &= ~0x08000000;
    } else {
        INPUT_SetAxisScale(13, -1.0f, 1);
        g_cameraAxisFlags |= 0x08000000;
    }
}

// 0x0047dc80 (PDB: 0x0047f620)
bool CAMERA_GetAxisInversionX(void)
{
    return INPUT_GetAxisScale(12) < 0.0f;
}

// 0x0047dca0 (PDB: 0x0047f640)
bool CAMERA_GetAxisInversionY(void)
{
    return INPUT_GetAxisScale(13) > 0.0f;
}

// 0x0047dcc0 (PDB: 0x0047f660)
void CAMERA_SetAimAxisInversionX(bool inverted)
{
    INPUT_SetAxisScale(14, inverted ? -1.0f : 1.0f, 1);
}

// 0x0047dcf0 (PDB: 0x0047f690)
void CAMERA_SetAimAxisInversionY(bool inverted)
{
    INPUT_SetAxisScale(15, inverted ? 1.0f : -1.0f, 1);
}

// 0x0047dd20 (PDB: 0x0047f6c0)
bool CAMERA_GetAimAxisInversionX(void)
{
    return INPUT_GetAxisScale(14) < 0.0f;
}

// 0x0047dd40 (PDB: 0x0047f6e0)
bool CAMERA_GetAimAxisInversionY(void)
{
    return INPUT_GetAxisScale(15) > 0.0f;
}

// 0x0047acc0 (PDB: 0x0047c660)
void CAMERA_SaveMode(Camera* camera, int mode)
{
    if (camera->stack < 3) {
        camera->savedMode[camera->stack] = mode;
        camera->savedTargetFocusDistance[camera->stack] = camera->targetFocusDistance;
        camera->stack++;
    }
}

// 0x0047ff50 (PDB: 0x004818f0)
void CAMERA_RestoreMode(Camera* camera)
{
    if (camera->stack > 0) {
        camera->stack--;
        int mode = camera->savedMode[camera->stack];
        camera->targetFocusDistance = camera->savedTargetFocusDistance[camera->stack];
        CAMERA_SetMode(camera, mode);
    }
}

// 0x0047b240 (PDB: 0x0047cbe0)
void CAMERA_SetMode(Camera* camera, int mode)
{
    if (camera->mode == mode) {
        return;
    }
    CAMERA_SaveMode(camera, camera->mode);
    camera->mode = static_cast<uint8_t>(mode);
}

// 0x0047dd60 (PDB: 0x0047f700)
void CAMERA_Restore(Camera* camera, int restoreBits)
{
    if (restoreBits & 1) {
        if (camera->targetStack > 0) {
            camera->targetStack--;
            camera->targetFocusDistance = camera->savedTargetFocusDistance[camera->targetStack];
            camera->signalFocusDistance = camera->targetFocusDistance;
        }
    }
    if (restoreBits & 2) {
        CAMERA_Adjust_tilt(camera, static_cast<int>(camera->savedFocusRotation.x * RAD_TO_DEG));
    }
    if (restoreBits & 4) {
        CAMERA_Adjust_rotation(camera, static_cast<int>(camera->savedFocusRotation.z * RAD_TO_DEG));
    }
    if (restoreBits & 0x100) {
        CAMERA_RestoreMode(camera);
    }
}

// 0x0047de30 (PDB: 0x0047f7d0)
int CAMERA_QueryMode(Camera* /*camera*/)
{
    int flags = 0;
    if (gPlayer != nullptr && gPlayerSM != nullptr) {
        // Evaluate player state flags
        flags |= 0x100;
    }
    return flags;
}

// 0x0047e1e0 (PDB: 0x0047fb80)
void CAMERA_Relocate(Camera* camera, cdc::Vector3* offset, int /*param3*/)
{
    if (offset == nullptr) {
        return;
    }
    camera->core.position.x += offset->x;
    camera->core.position.y += offset->y;
    camera->core.position.z += offset->z;

    camera->core.effectPosition.x += offset->x;
    camera->core.effectPosition.y += offset->y;
    camera->core.effectPosition.z += offset->z;

    camera->focusPoint.x += offset->x;
    camera->focusPoint.y += offset->y;
    camera->focusPoint.z += offset->z;

    camera->targetPos.x += offset->x;
    camera->targetPos.y += offset->y;
    camera->targetPos.z += offset->z;

    camera->targetFocusPoint.x += offset->x;
    camera->targetFocusPoint.y += offset->y;
    camera->targetFocusPoint.z += offset->z;
}

// 0x0047c990 (PDB: 0x0047e330)
void CAMERA_ChangeToUnderWater(Camera* camera, Instance* instance)
{
    if (instance != camera->focusInstance) {
        return;
    }
    if (camera->mode == 3) {
        CAMERA_SaveMode(camera, 3);
        camera->cutFlag = 1;
        CAMERA_SetMode(camera, 0);
        camera->targetFocusDistance = 1600.0f;
        camera->collisionTargetFocusDistance = 1600.0f;
        camera->signalFocusDistance = 1600.0f;
        camera->flags |= 0x10000;
        camera->smooth = 32.0f;
        camera->restoreToSplineFlag = 1;
    } else if (camera->mode == 4) {
        CAMERA_SaveMode(camera, 0);
        camera->flags |= 0x10000;
    } else {
        if (camera->mode != 0) {
            CAMERA_SetMode(camera, 0);
        }
        camera->targetFocusDistance = 1600.0f;
        camera->collisionTargetFocusDistance = 1600.0f;
        camera->signalFocusDistance = 1600.0f;
        camera->flags |= 0x10000;
        camera->smooth = 32.0f;
    }
}

// 0x0047cab0 (PDB: 0x0047e450)
void CAMERA_ChangeOutOfWade(Camera* camera, Instance* /*instance*/)
{
    camera->flags &= ~0x040000;
}

// 0x004803e0 (PDB: 0x00481d80)
void CAMERA_ChangeToOutOfWater(Camera* camera, Instance* instance)
{
    if (instance != camera->focusInstance) {
        return;
    }
    if (camera->mode == 4) {
        CAMERA_SaveMode(camera, 0);
    } else if (camera->restoreToSplineFlag != 0) {
        camera->cutFlag = 1;
        camera->nextCam.crossfade = 5;
        CAMERA_RestoreMode(camera);
        camera->flags &= ~0x10000;
    } else {
        CAMERA_SetMode(camera, 0);
        camera->targetTilt = 0.0f;
        camera->flags &= ~0x10000;
    }
}

// 0x0047e0e0 (PDB: 0x0047fa80)
void CAMERA_SetShakeNoPadshock(Camera* camera, int shake, int scale)
{
    camera->shake = static_cast<uint8_t>(shake);
    camera->shakeFrame = 0;
    camera->shakeScale = static_cast<float>(scale);
    camera->shakeFadeIn = 0;
    camera->shakeFadeOut = 0;
}

// 0x0047e180 (PDB: 0x0047fb20)
void CAMERA_SetShake(Camera* camera, int shake, int scale)
{
    CAMERA_SetShakeNoPadshock(camera, shake, scale);
    // Padshock rumble
    InputDevice* pad = InputDevice::GetDevicePtr(0);
    if (pad != nullptr) {
        float intensity = static_cast<float>(scale) / 255.0f;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
        pad->SetMotorIntensity(0, intensity);
        pad->ApplyIntensities();
    }
}

// 0x0047e860 (PDB: 0x00480200)
void CAMERA_SetShakeRamp(int fadeIn, int sustain, int fadeOut, int scale)
{
    CAMERA_SetShakeNoPadshock(&theCamera, fadeIn + sustain + fadeOut, scale);
    theCamera.shakeFadeIn = static_cast<uint8_t>(fadeIn);
    theCamera.shakeFadeOut = static_cast<uint8_t>(fadeOut);
}

// 0x0047d760 (PDB: 0x0047f100)
void CAMERA_SetVertigo(int mode, int param2, int param3)
{
    if (mode == 0) {
        Cam_Vertigo_deg = 0.0f;
        Cam_Vertigo_deg2 = 0.0f;
        Cam_Vertigo_Ramp_Amount = 0.0f;
        Cam_Vertigo_Ramp_Speed = 0.0f;
    } else {
        Cam_Vertigo_deg = static_cast<float>(param2);
        Cam_Vertigo_deg2 = static_cast<float>(param3);
    }
}

// 0x0047d860 (PDB: 0x0047f200)
void CAMERA_SetValueByEvent(Camera* camera, int eventId, int value)
{
    switch (eventId) {
    case 1:
        camera->nextCam.splinecamParams.sourceSplineWidth = static_cast<float>(value);
        break;
    case 2:
        camera->nextCam.splinecamParams.targetSplineWidth = static_cast<float>(value);
        break;
    case 3:
        camera->nextCam.splinecamParams.sourceSplineWidthZ = static_cast<float>(value);
        break;
    case 4:
        camera->nextCam.splinecamParams.targetSplineWidthZ = static_cast<float>(value);
        break;
    case 5:
        camera->nextCam.splinecamParams.combatCamLockedOut = static_cast<int8_t>(value);
        break;
    case 6:
        camera->cutFlag = static_cast<int8_t>(value);
        break;
    default:
        break;
    }
}
