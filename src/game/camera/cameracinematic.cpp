#include "game/camera/cameracinematic.h"
#include "game/camera/camerafocus.h"
#include "game/camera/camerasupport.h"
#include "game/spline.h"
#include <cmath>
#include <cstdint>

// 0x0048e1a8.. (G2 / UI / GameValue / Camera helpers declared externally until their modules are decompiled)
struct G2EulerAngles {
    float x;
    float y;
    float z;
    int order;
};

extern void G2Quat_ToEuler(cdc::Quat* quat, G2EulerAngles* euler, int order); // 0x004e01e0 (PDB: 0x004e1d10)
extern float CAMERA_DoVertigo(Camera* camera);                                 // 0x0047cad0 (PDB: 0x0047e470)
extern void CAMERA_UpdateFocusRoll(Camera* camera);                            // 0x0047fe90 (PDB: 0x00481830)
extern bool UIScreenManager_HasStackedScreen(int screenId);                  // 0x004f7340 (PDB: 0x004f8b70 / UIScreenManager::HasStackedScreen)
extern void SetGameValue(int valueType, float value, bool apply);              // 0x00452570 (PDB: 0x00453ad0)

extern float cameraUseTimeMult;   // 0x00f15c78 (PDB: 0x00f15fb0)
extern float transition_camera;   // 0x010fc570 (PDB: 0x010fd770)
extern float transition_target;   // 0x010fc574 (PDB: 0x010fd774)

// 0x010fd960 (PDB: 0x001eeb60)
cdc::Vector3 Cinematic_lastPosition = {};

static const float CC_RAD_TO_DEG = 57.2957763671875f; // 0x00f0168c (__180_PI)

// 0x00483420 (PDB: 0x00484e20)
int CAMERA_CinematicDone(Camera* camera)
{
    return (camera->data.Cinematic.cinema_done != 0) ? 1 : 0;
}

// 0x00483440 (PDB: 0x00484e40)
void CAMERA_CinematicProcess(Camera* camera)
{
    MultiSpline* targetSpline = camera->data.Cinematic.materialSplinecamData.targetSpline;
    MultiSpline* sourceSpline = camera->data.Cinematic.materialSplinecamData.sourceSpline;
    SplinecamData* matSplineData = &camera->data.Cinematic.materialSplinecamData;

    cdc::Vector3* camPosHold = nullptr;
    cdc::Vector3* camTargetHold = nullptr;
    int rollFlag = 0;
    cdc::Euler targetRotation = {};
    cdc::Vector3 camPos = {};
    cdc::Vector3 camTarget = {};

    if (camera->posState == 0) {
        if (targetSpline == nullptr && (matSplineData->useStartPoint & 0x2) == 0) {
            CAMERA_SetFocus(camera, &camera->targetFocusPoint);
        }

        if (sourceSpline != nullptr) {
            sourceSpline->Step(cameraUseTimeMult);

            if (sourceSpline->rotational != nullptr) {
                cdc::Quat quat = sourceSpline->GetQuaternion();
                G2EulerAngles g2Euler;
                G2Quat_ToEuler(&quat, &g2Euler, 0x15);
                targetRotation.x = g2Euler.x;
                targetRotation.y = g2Euler.y;
                targetRotation.z = g2Euler.z;
                targetRotation.w = 0.0f;
                rollFlag = 1;
            }

            if (sourceSpline->positional != nullptr) {
                camPos = sourceSpline->GetPoint();
                camPosHold = &camPos;
                if (camera->data.Cinematic.splinePointAhead == 0) {
                    camera->core.position = camPos;
                } else {
                    camera->core.position = camera->data.Cinematic.lastSplinePos;
                }
            } else if ((matSplineData->useStartPoint & 0x1) != 0) {
                camera->core.position = matSplineData->sourceStartPoint;
            } else {
                camera->core.position = Cinematic_lastPosition;
            }

            if (camera->data.Cinematic.splinePointAhead != 0) {
                CAMERA_SetTarget(camera, &camPos);
                camera->data.Cinematic.lastSplinePos = camPos;
            } else {
                if (targetSpline != nullptr) {
                    targetSpline->Step(cameraUseTimeMult);
                    if (targetSpline->positional != nullptr) {
                        camTarget = targetSpline->GetPoint();
                        camTargetHold = &camTarget;
                        camera->targetFocusPoint = camTarget;
                        CAMERA_SetTarget(camera, &camTarget);
                    } else if ((matSplineData->useStartPoint & 0x2) != 0) {
                        camera->targetFocusPoint = matSplineData->targetStartPoint;
                        CAMERA_SetTarget(camera, &camera->targetFocusPoint);
                    }
                } else if ((matSplineData->useStartPoint & 0x2) != 0) {
                    camera->targetFocusPoint = matSplineData->targetStartPoint;
                    CAMERA_SetTarget(camera, &camera->targetFocusPoint);
                } else {
                    CAMERA_SetMaxVel(camera);
                    CriticalDampPosition(
                        3,
                        &camera->focusPoint,
                        &camera->targetFocusPoint,
                        &camera->focusPointVel,
                        &camera->focusPointAccl,
                        0.0f);
                }
            }

            camera->targetPos = camera->core.position;
            CAMERA_SetMaxVel(camera);
            CriticalDampPosition(
                3,
                &camera->focusPoint,
                &camera->targetFocusPoint,
                &camera->focusPointVel,
                &camera->focusPointAccl,
                0.0f);
        } else if ((matSplineData->useStartPoint & 0x1) != 0) {
            camera->core.position = matSplineData->sourceStartPoint;
            if ((matSplineData->useStartPoint & 0x2) != 0) {
                camera->targetFocusPoint = matSplineData->targetStartPoint;
                CAMERA_SetTarget(camera, &camera->targetFocusPoint);
            }
            if ((matSplineData->useStartPoint & 0x1) != 0) {
                CAMERA_SetMaxVel(camera);
                CriticalDampPosition(
                    3,
                    &camera->focusPoint,
                    &camera->targetFocusPoint,
                    &camera->focusPointVel,
                    &camera->focusPointAccl,
                    0.0f);
            }
        }

        CAMERA_CalcRotation(&camera->FocusRotation, &camera->focusPoint, &camera->core.position);
        if (camPosHold == nullptr && camTargetHold == nullptr) {
            camera->data.Cinematic.cinema_done = 1;
        }
    } else {
        if (camera->cutFlag != 0) {
            camera->core.position = camera->targetPos;
            if (targetSpline == nullptr && (matSplineData->useStartPoint & 0x2) == 0) {
                CAMERA_SetFocus(camera, &camera->targetFocusPoint);
            }
            camera->focusPoint = camera->targetFocusPoint;
            camera->cutFlag = 0;
            camera->posState = 0;
            transition_camera = 0.0f;
            transition_target = 0.0f;

            cdc::Vector3 zeroVec = {};
            camera->targetPointVel = zeroVec;
            camera->targetPointAccl = zeroVec;
            camera->focusPointVel = zeroVec;
            camera->focusPointAccl = zeroVec;
        } else {
            SplinecamParams& params = camera->data.Cinematic.splinecamParams;
            CriticalDampPositionControl(
                &camera->core.position,
                &camera->targetPos,
                &camera->targetPointVel,
                &camera->targetPointAccl,
                camera->instance_vel * (1.0f / 3.0f) + params.cameraVelocity,
                params.cameraDampening * (1.0f / 256.0f),
                &transition_camera);

            if (targetSpline == nullptr && (matSplineData->useStartPoint & 0x2) == 0) {
                float maxTargetVel = camera->instance_vel * 0.5f + params.targetVelocity;
                CAMERA_SetFocus(camera, &camera->targetFocusPoint);
                CriticalDampPositionControl(
                    &camera->focusPoint,
                    &camera->targetFocusPoint,
                    &camera->focusPointVel,
                    &camera->focusPointAccl,
                    maxTargetVel,
                    params.targetDampening * (1.0f / 256.0f),
                    &transition_target);
            } else {
                CriticalDampPositionControl(
                    &camera->focusPoint,
                    &camera->targetFocusPoint,
                    &camera->focusPointVel,
                    &camera->focusPointAccl,
                    params.targetVelocity,
                    params.targetDampening * (1.0f / 256.0f),
                    &transition_target);
            }

            CAMERA_SetTarget(camera, &camera->focusPoint);

            if (std::fabs(camera->core.position.x - camera->targetPos.x) <= 8.0f &&
                std::fabs(camera->core.position.y - camera->targetPos.y) <= 8.0f &&
                std::fabs(camera->core.position.z - camera->targetPos.z) <= 8.0f &&
                std::fabs(camera->focusPoint.x - camera->targetFocusPoint.x) <= 8.0f &&
                std::fabs(camera->focusPoint.y - camera->targetFocusPoint.y) <= 8.0f &&
                std::fabs(camera->focusPoint.z - camera->targetFocusPoint.z) <= 8.0f) {
                camera->posState = 0;
            }
        }
    }

    cdc::Euler newRot;
    CAMERA_CalcRotation(&newRot, &camera->focusPoint, &camera->core.position);
    camera->core.Rotation = newRot;

    if (rollFlag != 0) {
        camera->core.Rotation.y = targetRotation.y;
    } else {
        CAMERA_UpdateFocusRoll(camera);
    }

    camera->lagZ = camera->core.Rotation.z;
    Cinematic_lastPosition = camera->core.position;

    float vertigoPct = CAMERA_DoVertigo(camera);
    if (vertigoPct != 1.0f) {
        float dx = camera->core.position.x - camera->focusPoint.x;
        float dy = camera->core.position.y - camera->focusPoint.y;
        float dz = camera->core.position.z - camera->focusPoint.z;
        float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len != 0.0f) {
            float invLen = 1.0f / len;
            dx *= invLen;
            dy *= invLen;
            dz *= invLen;
        }
        float scaledLen = len * vertigoPct;
        camera->core.position.x = camera->focusPoint.x + dx * scaledLen;
        camera->core.position.y = camera->focusPoint.y + dy * scaledLen;
        camera->core.position.z = camera->focusPoint.z + dz * scaledLen;
    }

    if (UIScreenManager_HasStackedScreen(0x1389)) {
        SetGameValue(0x47, camera->TargetFocusRotation.z * CC_RAD_TO_DEG - 180.0f, true);
        SetGameValue(0x46, camera->TargetFocusRotation.x * CC_RAD_TO_DEG, true);
    }
}
