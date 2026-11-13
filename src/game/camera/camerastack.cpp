#include "game/camera/camerastack.h"
#include "game/camera/camera.h"
#include "game/camera/camerabehind.h"
#include "game/camera/camerasupport.h"
#include "game/globalinfo.h"
#include "game/instance.h"
#include "game/player/markup.h"
#include "game/player/player.h"
#include <cmath>
#include <cstring>

// DTP data resolver (0x005b99e0 / PDB: 0x005bc350)
struct DTPDataSection {
    static void* GetPointer(unsigned int dtpID);
};

// CameraStack globals
CameraStack theCameraStack = {}; // 0x010fda98 (PDB: 0x010fec98)
static float safeGrabTimer = 0.0f; // 0x010fdd28

namespace {

constexpr float CS_PI = 3.1415927410125732f;
constexpr float CS_TWO_PI = 6.2831854820251465f;
constexpr float CS_DEG_TO_RAD = 0.01745329238474369f;
constexpr float CS_RAD_TO_ANGLE_UNITS = 651.89862060546875f; // 4096.0f / (2 * pi)
constexpr float CS_INV_4096 = 0.000244140625f;
constexpr float CS_TILT_RETURN_SCALE = 6.103515488575795e-07f; // 0.0025f / 4096.0f
constexpr float CS_SPLINE_TUNE_SCALE = 0.00004999999873689376f;

constexpr int16_t kCombineUseCurrent = -0x7fff;   // -32767
constexpr int16_t kCombineUseStoredBase = -0x7ffe; // -32766
constexpr int16_t kCombineUseDefault = -0x7ffd;    // -32765

// 0x0048a180 (PDB: 0x0048bc40)
static float CameraStackCircleAnglef(float angle)
{
    if (angle >= CS_TWO_PI || angle <= -CS_TWO_PI) {
        angle = static_cast<float>(std::fmod(static_cast<double>(angle), static_cast<double>(CS_TWO_PI)));
    }
    if (angle < 0.0f) {
        angle += CS_TWO_PI;
    }
    return angle;
}

inline void CombineSentinelInt16(CameraDTPData* dest, const CameraDTPData* src, int16_t CameraDTPData::* field)
{
    int16_t val = src->*field;
    if (val == kCombineUseCurrent) {
        dest->*field = theCameraStack.CurrentCombinedCamera.*field;
    } else if (val == kCombineUseStoredBase) {
        dest->*field = theCameraStack.CurrentStoredCombinedBaseCamera.*field;
    } else if (val == kCombineUseDefault) {
        dest->*field = theCameraStack.DefaultCamera->*field;
    } else {
        dest->*field = val;
    }
}

inline uint16_t CombineTriStateBitfield(uint16_t val, uint16_t combinedVal, uint16_t storedBaseVal, uint16_t defaultVal)
{
    if (val == 6) {
        return combinedVal;
    }
    if (val == 4) {
        return storedBaseVal;
    }
    if (val == 2) {
        return defaultVal;
    }
    return val;
}

} // namespace

// 0x0048a1e0 (PDB: 0x0048bca0)
void CameraStackCombineData(CameraDTPData* dest, CameraDTPData* src)
{
    static int16_t CameraDTPData::* const kSentinelFields[] = {
        &CameraDTPData::mode,
        &CameraDTPData::followTilt,
        &CameraDTPData::followTiltReturnSpeed,
        &CameraDTPData::followSmooth,
        &CameraDTPData::followTransition,
        &CameraDTPData::followRotation,
        &CameraDTPData::followZOffset,
        &CameraDTPData::minimumDistance,
        &CameraDTPData::autoCenterMultiplier,
        &CameraDTPData::autoCenterMaxSpeed,
        &CameraDTPData::maximumDistance,
        &CameraDTPData::followHighTiltSpringAngle,
        &CameraDTPData::followHighTiltDistance,
        &CameraDTPData::followHighTiltAngle,
        &CameraDTPData::followHighTiltFOV,
        &CameraDTPData::followHighTiltZOffset,
        &CameraDTPData::followHighTiltVerticalBias,
        &CameraDTPData::followMedTiltDistance,
        &CameraDTPData::followMedTiltAngle,
        &CameraDTPData::followMedTiltFOV,
        &CameraDTPData::followMedTiltZOffset,
        &CameraDTPData::followMedTiltVerticalBias,
        &CameraDTPData::followZeroTiltDistance,
        &CameraDTPData::followZeroTiltAngle,
        &CameraDTPData::followZeroTiltFOV,
        &CameraDTPData::followZeroTiltZOffset,
        &CameraDTPData::followZeroTiltVerticalBias,
        &CameraDTPData::followLowTiltDistance,
        &CameraDTPData::followLowTiltAngle,
        &CameraDTPData::followLowTiltFOV,
        &CameraDTPData::followLowTiltZOffset,
        &CameraDTPData::followLowTiltVerticalBias,
        &CameraDTPData::followLowTiltSpringAngle,
        &CameraDTPData::followRotationRangeStart,
        &CameraDTPData::followRotationRangeEnd,
        &CameraDTPData::followRotationRangeSpringAngle,
        &CameraDTPData::followVerticalBias,
        &CameraDTPData::followHorizontalBias,
        &CameraDTPData::leadAmount,
        &CameraDTPData::interestInstTune,
        &CameraDTPData::splineSourceWidth,
        &CameraDTPData::splineTargetWidth,
        &CameraDTPData::splineSourceWidthZ,
        &CameraDTPData::splineTargetWidthZ,
        &CameraDTPData::splineTune,
        &CameraDTPData::combatCamTiltOverride,
        &CameraDTPData::combatCamDistOverride,
    };

    for (size_t i = 0; i < sizeof(kSentinelFields) / sizeof(kSentinelFields[0]); ++i) {
        CombineSentinelInt16(dest, src, kSentinelFields[i]);
    }

    // Also applies sentinel substitution to tilt limits (included alongside the table above)
    CombineSentinelInt16(dest, src, &CameraDTPData::followHighTiltLimit);
    CombineSentinelInt16(dest, src, &CameraDTPData::followLowTiltLimit);

    if (src->interestInst != 0) {
        dest->interestInst = src->interestInst;
    }
    if (src->splineSource != 0) {
        dest->splineSource = src->splineSource;
    }
    if (src->splineTarget != 0) {
        dest->splineTarget = src->splineTarget;
    }

    dest->rotationPlayerRelative = CombineTriStateBitfield(
        src->rotationPlayerRelative,
        theCameraStack.CurrentCombinedCamera.rotationPlayerRelative,
        theCameraStack.CurrentStoredCombinedBaseCamera.rotationPlayerRelative,
        theCameraStack.DefaultCamera->rotationPlayerRelative);
    dest->rotationRangePlayerRelative = CombineTriStateBitfield(
        src->rotationRangePlayerRelative,
        theCameraStack.CurrentCombinedCamera.rotationRangePlayerRelative,
        theCameraStack.CurrentStoredCombinedBaseCamera.rotationRangePlayerRelative,
        theCameraStack.DefaultCamera->rotationRangePlayerRelative);
    dest->hybridSplineAdjustDist = CombineTriStateBitfield(
        src->hybridSplineAdjustDist,
        theCameraStack.CurrentCombinedCamera.hybridSplineAdjustDist,
        theCameraStack.CurrentStoredCombinedBaseCamera.hybridSplineAdjustDist,
        theCameraStack.DefaultCamera->hybridSplineAdjustDist);
    dest->hybridSplineAdjustTilt = CombineTriStateBitfield(
        src->hybridSplineAdjustTilt,
        theCameraStack.CurrentCombinedCamera.hybridSplineAdjustTilt,
        theCameraStack.CurrentStoredCombinedBaseCamera.hybridSplineAdjustTilt,
        theCameraStack.DefaultCamera->hybridSplineAdjustTilt);
    dest->lockInPlace = CombineTriStateBitfield(
        src->lockInPlace,
        theCameraStack.CurrentCombinedCamera.lockInPlace,
        theCameraStack.CurrentStoredCombinedBaseCamera.lockInPlace,
        theCameraStack.DefaultCamera->lockInPlace);

    dest->DragToNewTilt = src->DragToNewTilt;
    dest->followDragToRotationSpeed = src->followDragToRotationSpeed;

    if (src->lookaroundDisabled != 0) {
        dest->lookaroundDisabled = 1;
    }
    if (src->combatCamDisabled != 0) {
        dest->combatCamDisabled = 1;
    }
    if (src->grappleCamDisabled != 0) {
        dest->grappleCamDisabled = 1;
    }

    if ((src->lockInPlace & 1) != 0) {
        theCameraStack.NewLockInPlaceCamera = src;
    }
}

// 0x0048b240 (PDB: 0x0048cd00)
void CameraStackUpdateCameraSystem(CameraDTPData* data)
{
    if (!theCameraStack.CameraStackSystemInUse || theCamera.mode == 4 || theCamera.mode == 11) {
        return;
    }

    bool rotApplied = false;
    bool tiltWasSet = false;

    int currentMode = theCamera.mode;
    if (theCamera.mode == 2 && (theCamera.specialModeBits & 0x2000) != 0) {
        currentMode = 1;
    }
    if (currentMode != data->mode && (theCamera.mode == 0 || theCamera.mode == 2 || theCamera.mode == 1)) {
        CAMERA_SetMode(&theCamera, data->mode);
        theCameraStack.refreshCameraSystem = 0;
    }

    theCamera.smooth = static_cast<float>(data->followSmooth);

    CameraDTPData* latest = theCameraStack.LatestAddedCamera;
    if (((latest != nullptr && latest->DragToNewTilt != 0) || theCameraStack.refreshCameraSystem == 1) &&
        theCameraStack.refreshCameraSystem != 2) {
        CAMERA_Adjust_tilt_stay_in_place(&theCamera, (-data->followTilt * 4096) / 360);
    } else if (data->followTilt != theCameraStack.LastUpdatedCamera.followTilt ||
               theCameraStack.refreshCameraSystem == 2) {
        CAMERA_Adjust_tilt(&theCamera, (-data->followTilt * 4096) / 360);
        tiltWasSet = true;
    }

    if (gPlayer != nullptr && gPlayer->iCombatLockMode == 0) {
        theCamera.followTiltReturnSpeed = 0.0f;
    } else {
        theCamera.followTiltReturnSpeed = static_cast<float>(data->followTiltReturnSpeed) * CS_TILT_RETURN_SCALE * 0.5f;
    }

    theCamera.followDragToRotateSpeed = 0.0f;
    if (latest != nullptr) {
        if (latest->cutFlag != 0) {
            theCamera.cutFlag = 1;
        }
        if (latest->followRotation != kCombineUseCurrent) {
            int rot = (data->followRotation * 4096) / 360;
            if (data->rotationPlayerRelative != 0) {
                rot += static_cast<int>(theCamera.focusInstance->rotation.z * CS_RAD_TO_ANGLE_UNITS);
            }
            theCamera.followDragToRotateSpeed = static_cast<float>(data->followDragToRotationSpeed) * CS_INV_4096 * CS_PI;
            CAMERA_Adjust_rotation(&theCamera, rot);
            rotApplied = true;
        }
    }

    theCamera.ZOffset = static_cast<float>(data->followZOffset);
    theCamera.followHighTiltAngle = static_cast<float>(data->followHighTiltAngle);
    theCamera.followHighTiltLimit = static_cast<float>(data->followHighTiltLimit);
    theCamera.followLowTiltLimit = static_cast<float>(data->followLowTiltLimit);
    theCamera.followHighTiltAngle = static_cast<float>(data->followHighTiltAngle);
    theCamera.followMedTiltAngle = static_cast<float>(data->followMedTiltAngle);
    theCamera.followZeroTiltAngle = static_cast<float>(data->followZeroTiltAngle);
    theCamera.followLowTiltAngle = static_cast<float>(data->followLowTiltAngle);

    theCamera.followAutoCenterMultiplier = static_cast<float>(data->autoCenterMultiplier) * 0.001f;
    theCamera.followAutoCenterMaxSpeed = static_cast<float>(data->autoCenterMaxSpeed) * 0.001f;

    theCamera.followHighTiltDistance = static_cast<float>(data->followHighTiltDistance);
    theCamera.followMedTiltDistance = static_cast<float>(data->followMedTiltDistance);
    theCamera.followZeroTiltDistance = static_cast<float>(data->followZeroTiltDistance);
    theCamera.followLowTiltDistance = static_cast<float>(data->followLowTiltDistance);

    theCamera.followHighTiltFOV = static_cast<float>(data->followHighTiltFOV) * 0.01f;
    theCamera.followHighTiltZOffset = static_cast<float>(data->followHighTiltZOffset);
    theCamera.followHighTiltVerticalBias = static_cast<float>(data->followHighTiltVerticalBias) * 0.001f;

    theCamera.followMedTiltFOV = static_cast<float>(data->followMedTiltFOV) * 0.01f;
    theCamera.followMedTiltZOffset = static_cast<float>(data->followMedTiltZOffset);
    theCamera.followMedTiltVerticalBias = static_cast<float>(data->followMedTiltVerticalBias) * 0.001f;

    theCamera.followZeroTiltFOV = static_cast<float>(data->followZeroTiltFOV) * 0.01f;
    theCamera.followZeroTiltZOffset = static_cast<float>(data->followZeroTiltZOffset);
    theCamera.followZeroTiltVerticalBias = static_cast<float>(data->followZeroTiltVerticalBias) * 0.001f;

    theCamera.followLowTiltFOV = static_cast<float>(data->followLowTiltFOV) * 0.01f;
    theCamera.followLowTiltZOffset = static_cast<float>(data->followLowTiltZOffset);
    theCamera.followLowTiltVerticalBias = static_cast<float>(data->followLowTiltVerticalBias) * 0.001f;

    theCamera.followMinimumDistance = static_cast<float>(data->minimumDistance);
    theCamera.followMaximumDistance = static_cast<float>(data->maximumDistance);
    theCamera.followHighSpringDegree = static_cast<float>(data->followHighTiltSpringAngle);
    theCamera.followLowSpringDegree = static_cast<float>(data->followLowTiltSpringAngle);

    theCamera.followRotationRangeStart = static_cast<float>(data->followRotationRangeStart) * CS_DEG_TO_RAD;
    theCamera.followRotationRangeEnd = static_cast<float>(data->followRotationRangeEnd) * CS_DEG_TO_RAD;
    theCamera.rotationRangePlayerRelative = data->rotationRangePlayerRelative;
    theCamera.followRotationRangeSoftRange = static_cast<float>(data->followRotationRangeSpringAngle) * CS_DEG_TO_RAD;

    if (!rotApplied) {
        float wrappedRot = CameraStackCircleAnglef(theCamera.FocusRotation.z);
        if (CAMERA_LimitRotation(&theCamera, wrappedRot) != wrappedRot) {
            float limited = CAMERA_LimitRotation(&theCamera, wrappedRot);
            theCamera.followDragToRotateSpeed = 0.0f;
            CAMERA_Adjust_rotation(&theCamera, static_cast<int>(limited * CS_RAD_TO_ANGLE_UNITS));
            rotApplied = true;
        }
    }

    theCamera.followVerticalBias = static_cast<float>(data->followVerticalBias) * 0.001f;
    theCamera.followHorizontalBias = static_cast<float>(data->followHorizontalBias) * 0.001f;

    if (data->lockInPlace != 0) {
        theCamera.extraXRot = 0.0f;
        if (theCameraStack.NewLockInPlaceCamera != theCameraStack.CurrentLockInPlaceCamera) {
            theCameraStack.CurrentLockInPlaceCamera = theCameraStack.NewLockInPlaceCamera;
            if (data->splineSource != 0) {
                int unitID;
                Intro* intro = INSTANCE_FindIntroUniqueID(data->splineSource, &unitID);
                if (intro != nullptr) {
                    theCamera.followSplineData.sourceStartPoint = intro->position;
                    CAMERA_ResetFollowTransition(&theCamera);
                }
            } else {
                theCamera.followSplineData.sourceStartPoint = theCamera.core.position;
                theCamera.followTransitionData.transition = 1.0f;
                theCamera.followTransitionData.transitionLinear = 1.0f;
                theCamera.followTransitionData.transitionVel = 0.0f;
                theCamera.followTransitionData.transitionAccl = 0.0f;
            }
        }
    } else {
        theCameraStack.CurrentLockInPlaceCamera = nullptr;
    }

    // Matches bitwise test on splineSource & splineTarget at 0x0048b866
    if ((data->splineSource & data->splineTarget) != 0) {
        int unitID;
        bool splineChanged = false;
        Intro* srcIntro = INSTANCE_FindIntroUniqueID(data->splineSource, &unitID);
        if (srcIntro != nullptr) {
            if (srcIntro->multiSpline != theCamera.followSplineData.sourceSpline) {
                splineChanged = true;
            }
            theCamera.followSplineData.sourceSpline = srcIntro->multiSpline;
            theCamera.followSplineData.sourceStartPoint = srcIntro->position;
            theCamera.followSplineData.sourceNumber = srcIntro->intronum;
        }
        Intro* dstIntro = INSTANCE_FindIntroUniqueID(data->splineTarget, &unitID);
        if (dstIntro != nullptr) {
            if (dstIntro->multiSpline != theCamera.followSplineData.targetSpline) {
                splineChanged = true;
            }
            theCamera.followSplineData.targetSpline = dstIntro->multiSpline;
            theCamera.followSplineData.targetStartPoint = dstIntro->position;
            theCamera.followSplineData.targetNumber = dstIntro->intronum;
        }
        if (splineChanged) {
            CAMERA_ResetFollowTransition(&theCamera);
        }
    } else {
        theCamera.followSplineData.sourceSpline = nullptr;
        theCamera.followSplineData.targetSpline = nullptr;
    }

    if (data->followTransition != 0 && (tiltWasSet || rotApplied) && theCameraStack.NewMarkupCamera != 0) {
        float transitionTime = gPlayer->tTimeToTarget;
        if (transitionTime == 0.0f || transitionTime >= 10000.0f) {
            transitionTime = 30.0f;
        } else if (transitionTime >= 90.0f) {
            transitionTime = 90.0f;
        }
        CAMERA_ResetFollowTransition(&theCamera);
        theCamera.followCameraTimedTransition = 1;
        theCamera.followTransitionData.transitionVel =
            1.0f / (static_cast<float>(data->followTransition) * 0.01f * transitionTime);
        if (data->lockInPlace == 0) {
            theCamera.distanceState = 1;
        }
        theCamera.TargetFocusRotation.z = theCamera.FocusRotation.z;
        theCamera.CollisionTargetFocusRotation.z = theCamera.FocusRotation.z;
        if (theCamera.tiltState == 1) {
            theCamera.extraXRot = 0.0f;
            theCamera.tilt = 0.0f;
            theCamera.TargetFocusRotation.x = theCamera.FocusRotation.x;
        }
    } else {
        theCamera.followCameraTimedTransition = 0;
        CAMERA_ResetFollowTransition(&theCamera);
    }

    theCamera.markupCenter = theCamera.signalRot.z;
    theCamera.followSplineData.splineTune = static_cast<float>(data->splineTune) * CS_SPLINE_TUNE_SCALE;
    theCamera.followSplineData.sourceSplineWidth = static_cast<float>(data->splineSourceWidth);
    theCamera.followSplineData.targetSplineWidth = static_cast<float>(data->splineTargetWidth);
    theCamera.followSplineData.sourceSplineWidthZ = static_cast<float>(data->splineSourceWidthZ);
    theCamera.followSplineData.targetSplineWidthZ = static_cast<float>(data->splineTargetWidthZ);
    theCamera.followSplineData.splineAdjustDist = data->hybridSplineAdjustDist;
    theCamera.followSplineData.splineAdjustTilt = data->hybridSplineAdjustTilt & 0x3;
    theCamera.followSplineData.lockInPlace = data->lockInPlace;

    theCamera.followCameraCombatCamDisable = data->combatCamDisabled & 0x3;
    theCamera.followCameraLookaroundDisabled = data->lookaroundDisabled & 0x3;
    theCamera.followCameraGrappleCamDisable = data->grappleCamDisabled & 0x3;
    theCamera.followCombatCamTiltOverride = static_cast<float>(data->combatCamTiltOverride);
    theCamera.followCombatCamDistOverride = static_cast<float>(data->combatCamDistOverride);

    if (data->interestInst != 0) {
        theCamera.followInterestInst = INSTANCE_Find(data->interestInst);
    } else {
        theCamera.followInterestInst = nullptr;
    }
    theCamera.followInterestInstTune = static_cast<float>(data->interestInst);

    theCamera.leadAmount = static_cast<float>(data->leadAmount);
    theCameraStack.NewMarkupCamera = 0;
    theCameraStack.CameraSystemUpdated = 1;
    theCameraStack.LatestAddedCamera = nullptr;
    theCameraStack.refreshCameraSystem = 0;
    theCameraStack.LastUpdatedCamera = *data;

    if ((data->lockInPlace != 0 || (data->splineSource & data->splineTarget) != 0) &&
        data->hybridSplineAdjustTilt != 0) {
        theCameraStack.LastUpdatedCamera.followTilt = -99;
    }
}

// 0x0048bc40 (PDB: 0x0048d700)
void CameraStackRefreshCameraSystem(void)
{
    theCameraStack.refreshCameraSystem = 1;
}

// 0x0048bc50 (PDB: 0x0048d710)
void CameraStackRefreshCameraSystemResetTilt(void)
{
    theCameraStack.refreshCameraSystem = 2;
}

// 0x0048bc60 (PDB: 0x0048d720)
void CameraStackInit(CameraDTPData* defaultCamera)
{
    theCameraStack.CurrentStackSize = 0;
    theCameraStack.CameraSystemUpdated = 0;
    theCameraStack.DefaultCamera = defaultCamera;
    theCameraStack.CurrentBaseCamera = nullptr;
    theCameraStack.CurrentMovementCamera = nullptr;
    theCameraStack.CurrentMarkupCamera = nullptr;
    theCameraStack.CurrentMarkupCameraDTPID = 0;
    theCameraStack.CurrentMarkupData = nullptr;
    theCameraStack.CurrentStoredCombinedBaseCamera = *defaultCamera;
    theCameraStack.CurrentCombinedCamera = *defaultCamera;
    theCameraStack.LastUpdatedCamera = *defaultCamera;
    theCameraStack.MovementCameraInUse = 0;
    theCameraStack.MarkupCameraInUse = 0;
    theCameraStack.DisableMarkupCameras = 0;
    theCameraStack.DisableMovementCameras = 0;
    theCameraStack.MarkupCameraOverridesMovementCam = 0;
    theCameraStack.refreshCameraSystem = 0;
    theCameraStack.CurrentMovementMode = 0;
    theCameraStack.movementCamUnitOverrides = nullptr;
    theCameraStack.CameraStackSystemInUse = 0;
    theCameraStack.LastAddedCamera = nullptr;
    theCameraStack.LatestAddedCamera = nullptr;
    theCameraStack.CurrentLockInPlaceCamera = nullptr;
    theCameraStack.NewLockInPlaceCamera = nullptr;
}

// 0x0048bd10 (PDB: 0x0048d7d0)
void CameraStackInitDTPID(int dtpID)
{
    if (dtpID != 0) {
        CameraDTPData* cam = reinterpret_cast<CameraDTPData*>(DTPDataSection::GetPointer(static_cast<unsigned int>(dtpID)));
        if (cam != nullptr) {
            cam->DTPID = dtpID;
            CameraStackInit(cam);
        }
    }
}

// 0x0048bd40 (PDB: 0x0048d800)
void CameraStackClearMarkupCam(void)
{
    theCameraStack.MarkupCameraInUse = 0;
    theCameraStack.MarkupCameraOverridesMovementCam = 0;
    theCameraStack.CurrentMarkupCamera = nullptr;
    theCameraStack.CurrentMarkupCameraDTPID = 0;
    theCameraStack.CurrentMarkupData = nullptr;
}

// 0x0048bd70 (PDB: 0x0048d830)
void CameraStackCombineAll(void)
{
    theCameraStack.DisableMarkupCameras = 0;
    theCameraStack.DisableMovementCameras = 0;
    theCameraStack.NewLockInPlaceCamera = nullptr;
    theCameraStack.CurrentCombinedCamera = *theCameraStack.DefaultCamera;

    CameraStackCombineData(&theCameraStack.CurrentCombinedCamera, theCameraStack.CurrentBaseCamera);
    theCameraStack.CurrentStoredCombinedBaseCamera = theCameraStack.CurrentCombinedCamera;

    for (int i = 0; i < theCameraStack.CurrentStackSize; ++i) {
        CameraStackCombineData(&theCameraStack.CurrentCombinedCamera, theCameraStack.CameraStackList[i].DTPData);
        if ((theCameraStack.CameraStackList[i].signalFlags & 0x01) != 0) {
            theCameraStack.DisableMarkupCameras = 1;
        }
        if ((theCameraStack.CameraStackList[i].signalFlags & 0x02) != 0) {
            theCameraStack.DisableMovementCameras = 1;
        }
    }

    if (theCameraStack.MovementCameraInUse && !theCameraStack.DisableMovementCameras &&
        (!theCameraStack.MarkupCameraInUse || !theCameraStack.MarkupCameraOverridesMovementCam)) {
        CameraStackCombineData(&theCameraStack.CurrentCombinedCamera, theCameraStack.CurrentMovementCamera);
    }

    if (theCameraStack.MarkupCameraInUse && !theCameraStack.DisableMarkupCameras) {
        CameraStackCombineData(&theCameraStack.CurrentCombinedCamera, theCameraStack.CurrentMarkupCamera);
    }

    CameraStackUpdateCameraSystem(&theCameraStack.CurrentCombinedCamera);
}

// 0x0048be70 (PDB: 0x0048d930)
void CameraStackClear(void)
{
    theCameraStack.DisableMarkupCameras = 0;
    theCameraStack.DisableMovementCameras = 0;
    theCameraStack.MovementCameraInUse = 0;
    theCameraStack.MarkupCameraInUse = 0;
    theCameraStack.MarkupCameraOverridesMovementCam = 0;
    theCameraStack.CurrentStackSize = 0;
    theCameraStack.CameraSystemUpdated = 0;
    theCameraStack.LastAddedCamera = nullptr;
    theCameraStack.LatestAddedCamera = nullptr;
    theCameraStack.CurrentLockInPlaceCamera = nullptr;
    theCameraStack.NewLockInPlaceCamera = nullptr;
    theCameraStack.CurrentMovementCamera = nullptr;
    theCameraStack.CurrentMarkupCamera = nullptr;
    theCameraStack.CurrentMarkupCameraDTPID = 0;
    theCameraStack.CurrentMarkupData = nullptr;
    theCameraStack.CurrentMovementMode = 0;
    theCameraStack.movementCamUnitOverrides = nullptr;
    CameraStackCombineAll();
}

// 0x0048bed0 (PDB: 0x0048d990)
void CameraStackSetBase(CameraDTPData* data, movementCameraOverrides* overrides, int enable)
{
    if (data != nullptr && enable != 0) {
        theCameraStack.CurrentBaseCamera = data;
        theCameraStack.CameraStackSystemInUse = 1;
        theCameraStack.CurrentStackSize = 0;
        theCameraStack.CameraSystemUpdated = 0;
        theCameraStack.movementCamUnitOverrides = overrides;
        theCameraStack.NewLockInPlaceCamera = nullptr;
        CameraStackCombineAll();
    } else {
        theCameraStack.CameraStackSystemInUse = 0;
    }
}

// 0x0048bf20 (PDB: 0x0048d9e0)
void CameraStackSetBaseDTPID(int dtpID, movementCameraOverrides* overrides, int enable)
{
    if (dtpID != 0 && enable != 0) {
        CameraDTPData* cam = reinterpret_cast<CameraDTPData*>(DTPDataSection::GetPointer(static_cast<unsigned int>(dtpID)));
        if (cam != nullptr) {
            cam->DTPID = dtpID;
            CameraStackSetBase(cam, overrides, enable);
            return;
        }
    }
    theCameraStack.CameraStackSystemInUse = 0;
    theCameraStack.movementCamUnitOverrides = nullptr;
}

// 0x0048bf70 (PDB: 0x0048da30)
void CameraStackSetBaseDTPIDRecheck(int dtpID, movementCameraOverrides* overrides, int enable)
{
    if (overrides != theCameraStack.movementCamUnitOverrides) {
        CameraStackSetBaseDTPID(dtpID, overrides, enable);
    }
}

// 0x0048bf90 (PDB: 0x0048da50)
void CameraStackAddCamera(CameraDTPData* data, int signalID, int signalMode, int signalFlags)
{
    int stackSize = theCameraStack.CurrentStackSize;
    if (data == nullptr || !theCameraStack.CameraStackSystemInUse) {
        return;
    }

    for (int i = 0; i < stackSize; ++i) {
        if (theCameraStack.CameraStackList[i].DTPData == data &&
            theCameraStack.CameraStackList[i].signalID == signalID) {
            return;
        }
    }

    if (stackSize < 32) {
        theCameraStack.CameraStackList[stackSize].DTPData = data;
        theCameraStack.CameraStackList[stackSize].signalID = static_cast<int16_t>(signalID);
        theCameraStack.CameraStackList[stackSize].signalFlags = static_cast<uint8_t>(signalFlags);
        theCameraStack.CurrentStackSize++;
        theCameraStack.LastAddedCamera = data;
        theCameraStack.LatestAddedCamera = data;
        CameraStackCombineAll();
        if (signalMode == 2 || signalMode == 1) {
            theCameraStack.CurrentStackSize--;
        }
    }
}

// 0x0048c020 (PDB: 0x0048db10)
int CameraStackRemoveCamera(CameraDTPData* data, int signalID, int /*unused*/)
{
    int removed = 0;
    if (data == nullptr || !theCameraStack.CameraStackSystemInUse) {
        return 0;
    }

    int stackSize = theCameraStack.CurrentStackSize;
    int idx = 0;
    while (idx < stackSize) {
        if (theCameraStack.CameraStackList[idx].DTPData == data &&
            theCameraStack.CameraStackList[idx].signalID == signalID) {
            removed = 1;
            if (theCameraStack.LatestAddedCamera == data) {
                theCameraStack.LatestAddedCamera = nullptr;
            }
            break;
        }
        ++idx;
    }

    if (idx != stackSize) {
        if (idx < stackSize - 1) {
            std::memmove(
                &theCameraStack.CameraStackList[idx],
                &theCameraStack.CameraStackList[idx + 1],
                static_cast<size_t>(stackSize - 1 - idx) * sizeof(CameraStackInfo));
        }
        theCameraStack.CurrentStackSize--;
        CameraStackCombineAll();
    }

    return removed;
}

// 0x0048c0c0 (PDB: 0x0048dbd0)
void CameraStackAddCameraDTPID(int dtpID, int signalID, int signalMode, int signalFlags)
{
    if (dtpID != 0) {
        CameraDTPData* cam = reinterpret_cast<CameraDTPData*>(DTPDataSection::GetPointer(static_cast<unsigned int>(dtpID)));
        if (cam != nullptr) {
            cam->DTPID = dtpID;
            CameraStackAddCamera(cam, signalID, signalMode, signalFlags);
        }
    }
}

// 0x0048c0f0 (PDB: 0x0048dc00)
int CameraStackRemoveCameraDTPID(int dtpID, int signalID, int unused)
{
    if (dtpID == 0) {
        return 0;
    }
    CameraDTPData* cam = reinterpret_cast<CameraDTPData*>(DTPDataSection::GetPointer(static_cast<unsigned int>(dtpID)));
    return CameraStackRemoveCamera(cam, signalID, unused);
}

// 0x0048c110 (PDB: 0x0048dc20)
void CameraStackProcessPlayerState(void)
{
    int prevMovementMode = theCameraStack.CurrentMovementMode;
    int refreshNeeded = 0;
    const int* stateInfo = gPlayerSM->stateMachine.activeState->state->stateInfo;

    PlayerMarkupCamInfo info = PLAYER_GetBestMarkupCam();
    MarkUpBox* mub = info.mub;
    MarkUp* markupData = mub ? mub->markup : nullptr;
    int markupCamDTPID = info.DTPCameraDataID;
    bool overrideMovement = (info.OverrideMovementCamera != 0);
    GenericIntroData* markupIntroCamData = nullptr;
    uint32_t markupFlags = 0;

    if ((!markupData || !markupCamDTPID) && mub && mub->instance && mub->instance->intro &&
        mub->instance->intro->data && mub->instance->intro->data->AsGeneric()->DTPCameraID != 0) {
        markupIntroCamData = mub->instance->intro->data->AsGeneric();
        markupFlags = markupData->flags;
    } else if (markupData && markupCamDTPID) {
        markupFlags = markupData->flags;
    }

    int movementCamDTPID = 0;
    CameraDefaults* defaults = globalInfo->cameraDefaults;
    int stateID = stateInfo[1];

    if (stateID == ID_TurretState) {
        theCameraStack.CurrentMovementMode = 1;
        movementCamDTPID = defaults->turretCamera;
        Instance* turretInst = gPlayer->engagedList[23].instance;
        if (turretInst && turretInst->intro && turretInst->intro->data) {
            int turretCamID = turretInst->intro->data->AsGeneric()->DTPCameraID;
            if (turretCamID != 0) {
                movementCamDTPID = turretCamID;
            }
        }
    }

    if (stateID == ID_RopeState) {
        theCameraStack.CurrentMovementMode = 2;
        movementCamDTPID = defaults->ropeCamera;
    }

    if (stateID == ID_HangState || (markupFlags & 0x40000000u) != 0) {
        bool useLedgeCam = true;
        if (IsPlayerPerformingUnsafeLedgeHang() && safeGrabTimer < 10.0f) {
            safeGrabTimer += s_timeScale;
            useLedgeCam = false;
        } else if (stateID == ID_CompressionState && safeGrabTimer != 0.0f) {
            useLedgeCam = false;
        } else if (!IsPlayerPerformingUnsafeLedgeHang()) {
            safeGrabTimer = 0.0f;
        }

        if (gPlayer->groundDetachMode == 1 || gPlayer->groundDetachMode == 2 ||
            gPlayer->jumpSuccess == 2 || IsPlayerDoingSafeGrabAttach() || !useLedgeCam) {
            if ((markupFlags & 0x40000000u) != 0) {
                mub = nullptr;
                markupData = nullptr;
            }
        } else {
            theCameraStack.CurrentMovementMode = 3;
            if (theCameraStack.movementCamUnitOverrides) {
                movementCamDTPID = theCameraStack.movementCamUnitOverrides->ledgeCamID;
            }
            if (!movementCamDTPID) {
                movementCamDTPID = defaults->ledgeCamera;
            }
        }
    } else {
        safeGrabTimer = 0.0f;
    }

    if (stateID == ID_ThrownState) {
        theCameraStack.CurrentMovementMode = 4;
        movementCamDTPID = defaults->thrownCamera;
    }

    if (stateID == ID_HorizPoleState || (markupFlags & 0x02000000u) != 0) {
        theCameraStack.CurrentMovementMode = 5;
        movementCamDTPID = defaults->horizPoleCamera;
    }

    if (stateID == ID_VertPoleState || (markupFlags & 0x04000000u) != 0) {
        theCameraStack.CurrentMovementMode = 6;
        movementCamDTPID = defaults->vertPoleCamera;
    }

    if (stateID == ID_WallVertPoleState || (markupFlags & 0x08000000u) != 0) {
        theCameraStack.CurrentMovementMode = 7;
        movementCamDTPID = defaults->ladderCamera;
    }

    if (stateID == ID_LadderState || (markupFlags & 0x00400000u) != 0) {
        theCameraStack.CurrentMovementMode = 7;
        movementCamDTPID = defaults->ladderCamera;
    }

    if (stateID == ID_VehicleState) {
        theCameraStack.CurrentMovementMode = 8;
        if (theCameraStack.movementCamUnitOverrides) {
            movementCamDTPID = theCameraStack.movementCamUnitOverrides->vehicleCamID;
        }
        if (!movementCamDTPID) {
            movementCamDTPID = defaults->vehicleCamera;
        }
    }

    if (stateID == ID_ZipLineState || (markupFlags & 0x01000000u) != 0) {
        theCameraStack.CurrentMovementMode = 9;
        movementCamDTPID = defaults->ziplineCamera;
    }

    if (stateID == ID_CrawlState || stateID == ID_CrouchState) {
        theCameraStack.CurrentMovementMode = 10;
        if (theCameraStack.movementCamUnitOverrides) {
            movementCamDTPID = theCameraStack.movementCamUnitOverrides->crawlCamID;
        }
        if (!movementCamDTPID) {
            movementCamDTPID = defaults->crawlCamera;
        }
    }

    if (stateID == ID_PushPullState || stateID == ID_PhysicsRollState) {
        theCameraStack.CurrentMovementMode = 11;
        movementCamDTPID = defaults->pushPullCamera;
    }

    if (stateID == ID_SkyDiveState) {
        theCameraStack.CurrentMovementMode = 12;
        movementCamDTPID = defaults->skydiveCamera;
    }

    if (stateID == ID_SwimTreadState) {
        theCameraStack.CurrentMovementMode = 13;
        movementCamDTPID = defaults->swimTreadCamera;
    }

    if (stateID == ID_SwimState || stateID == ID_SwimDiveInState || stateID == ID_SwimInteractState) {
        theCameraStack.CurrentMovementMode = 14;
        movementCamDTPID = defaults->swimCamera;
    }

    if (stateID == ID_DeathState) {
        theCameraStack.CurrentMovementMode = 16;
        if (theCameraStack.movementCamUnitOverrides) {
            movementCamDTPID = theCameraStack.movementCamUnitOverrides->deathCamID;
        }
        if (!movementCamDTPID) {
            movementCamDTPID = defaults->deathCamera;
        }
    }

    if (movementCamDTPID != 0) {
        CameraDTPData* cam = reinterpret_cast<CameraDTPData*>(DTPDataSection::GetPointer(static_cast<unsigned int>(movementCamDTPID)));
        if ((theCameraStack.CurrentMovementMode != prevMovementMode || cam != theCameraStack.CurrentMovementCamera) &&
            cam != nullptr) {
            cam->DTPID = movementCamDTPID;
            theCameraStack.CurrentMovementCamera = cam;
            theCameraStack.LastAddedCamera = cam;
            theCameraStack.LatestAddedCamera = cam;
            theCameraStack.MovementCameraInUse = 1;
            if (!theCameraStack.DisableMovementCameras) {
                refreshNeeded = 1;
            }
        }
    } else if (theCameraStack.MovementCameraInUse) {
        theCameraStack.CurrentMovementCamera = nullptr;
        theCameraStack.CurrentMovementMode = 0;
        theCameraStack.MovementCameraInUse = 0;
        if (!theCameraStack.DisableMovementCameras) {
            refreshNeeded = 1;
        }
    }

    if (mub != nullptr && markupData != nullptr) {
        int chosenMarkupDTPID = markupCamDTPID;
        if (stateID == ID_RopeState) {
            if (mub->instance != nullptr) {
                const uint8_t* ropeQuery = reinterpret_cast<const uint8_t*>(
                    INSTANCE_Query(static_cast<Instance*>(mub->instance), 0xd5));
                if (ropeQuery != nullptr) {
                    int ropeCamID = *reinterpret_cast<const int*>(ropeQuery + 0x60);
                    if (ropeCamID != 0) {
                        chosenMarkupDTPID = ropeCamID;
                        if (ropeQuery[0x64] != 0) {
                            overrideMovement = true;
                        }
                    }
                }
            }
        } else if (markupIntroCamData != nullptr && markupIntroCamData->DTPCameraID != 0) {
            chosenMarkupDTPID = markupIntroCamData->DTPCameraID;
        }

        if (chosenMarkupDTPID != 0) {
            theCameraStack.CurrentMarkupData = markupData;
            if (chosenMarkupDTPID != theCameraStack.CurrentMarkupCameraDTPID) {
                CameraDTPData* cam = reinterpret_cast<CameraDTPData*>(DTPDataSection::GetPointer(static_cast<unsigned int>(chosenMarkupDTPID)));
                cam->DTPID = chosenMarkupDTPID;
                theCameraStack.CurrentMarkupCamera = cam;
                theCameraStack.LastAddedCamera = cam;
                theCameraStack.LatestAddedCamera = cam;
                theCameraStack.MarkupCameraInUse = 1;
                theCameraStack.NewMarkupCamera = 1;
                theCameraStack.CurrentMarkupCameraDTPID = chosenMarkupDTPID;
                theCameraStack.MarkupCameraOverridesMovementCam = overrideMovement ? 1 : 0;
                CameraStackCombineAll();
                return;
            } else {
                if (overrideMovement) {
                    if (!theCameraStack.MarkupCameraOverridesMovementCam) {
                        refreshNeeded = 1;
                    }
                    theCameraStack.MarkupCameraOverridesMovementCam = 1;
                } else {
                    if (theCameraStack.MarkupCameraOverridesMovementCam) {
                        refreshNeeded = 1;
                    }
                    theCameraStack.MarkupCameraOverridesMovementCam = 0;
                }
                if (refreshNeeded) {
                    CameraStackCombineAll();
                }
                return;
            }
        } else if (theCameraStack.MarkupCameraInUse) {
            CameraStackClearMarkupCam();
            CameraStackCombineAll();
            return;
        }
    } else if (theCameraStack.MarkupCameraInUse) {
        CameraStackClearMarkupCam();
        CameraStackCombineAll();
        return;
    }

    if (refreshNeeded) {
        CameraStackCombineAll();
    }
}

// 0x0048c6a0 (PDB: 0x0048e1b0)
void CameraStackMain(void)
{
    if (theCameraStack.CameraStackSystemInUse) {
        CameraStackProcessPlayerState();
        if (theCameraStack.refreshCameraSystem != 0) {
            CameraStackUpdateCameraSystem(&theCameraStack.CurrentCombinedCamera);
        }
    }
}
