#pragma once

#ifndef GAME_CAMERA_CAMERASPLINE_H
#define GAME_CAMERA_CAMERASPLINE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/spline.h"
#include <cstdint>

struct Camera;
struct Intro;
struct Level;

// Spline camera configuration block from level data
// size: 0x58 (88 bytes)
struct SplineCameraLevelData {
    /* 0x00 */ int16_t CameraFollowSmooth;
    /* 0x02 */ int16_t CameraFollowTilt;
    /* 0x04 */ int16_t CameraFollowDistance;
    /* 0x06 */ int16_t CameraFollowRotation;
    /* 0x08 */ int16_t CameraZOffset;
    /* 0x0a */ int16_t CameraCutAngle;
    /* 0x0c */ int16_t CameraCombatMinDistance;
    /* 0x0e */ int16_t CameraCombatDistance;
    /* 0x10 */ int16_t CameraCombatZOffset;
    /* 0x12 */ int8_t CameraCombatLockedOut;
    /* 0x13 */ int8_t CameraDisableLookaroundFlag;
    /* 0x14 */ int16_t CameraSpline0;
    /* 0x16 */ int16_t CameraSpline1;
    /* 0x18 */ int16_t CameraSpline2;
    /* 0x1a */ int16_t CameraAlternateSpline0;
    /* 0x1c */ int16_t CameraAlternateSpline1;
    /* 0x1e */ int16_t CameraAlternateSpline2;
    /* 0x20 */ int16_t CameraMode;
    /* 0x22 */ int16_t CameraLeadAmount;
    /* 0x24 */ int16_t CameraSpline0Width;
    /* 0x26 */ int16_t CameraSpline1Width;
    /* 0x28 */ int16_t CameraSpline0WidthZ;
    /* 0x2a */ int16_t CameraSpline1WidthZ;
    /* 0x2c */ int16_t CameraZoomDist;
    /* 0x2e */ int16_t CameraVelocity;
    /* 0x30 */ int32_t CameraDampening;
    /* 0x34 */ int16_t CameraTargetVelocity;
    /* 0x36 */ int16_t CameraCombatFraming;
    /* 0x38 */ int32_t CameraTargetDampening;
    /* 0x3c */ int8_t CameraCutFlag;
    /* 0x3d */ int8_t CameraFollowCamOverrideEnabled;
    /* 0x3e */ int16_t CameraCrossfade;
    /* 0x40 */ int16_t CameraInterestInstId;
    /* 0x42 */ int16_t CameraInterestTune;
    /* 0x44 */ int16_t CameraSwitchToFollowDist;
    /* 0x46 */ int16_t CameraFollowVerticalBias;
    /* 0x48 */ int16_t CameraFollowHighTiltDistance;
    /* 0x4a */ int16_t CameraFollowHighTiltAngle;
    /* 0x4c */ int16_t CameraFollowMedTiltDistance;
    /* 0x4e */ int16_t CameraFollowMedTiltAngle;
    /* 0x50 */ int16_t CameraFollowZeroTiltDistance;
    /* 0x52 */ int16_t CameraFollowZeroTiltAngle;
    /* 0x54 */ int16_t CameraFollowLowTiltDistance;
    /* 0x56 */ int16_t CameraFollowLowTiltAngle;
};
static_assert(sizeof(SplineCameraLevelData) == 0x58, "SplineCameraLevelData size mismatch");

// Extern globals defined in f:\tr7\game\camera\cameraSpline.c
extern float starting_lock_control_zrot; // 0x010fda6c (PDB: 0x010fec6c)
extern float splineLookX;                // 0x010fda7c (PDB: 0x010fec7c)
extern float splineLookZ;                // 0x010fda80 (PDB: 0x010fec80)
extern float splineLookVelX;             // 0x010fda84 (PDB: 0x010fec84)
extern float splineLookVelZ;             // 0x010fda88 (PDB: 0x010fec88)
extern float splineLookAcclX;            // 0x010fda8c (PDB: 0x010fec8c)
extern float splineLookAcclZ;            // 0x010fda90 (PDB: 0x010fec90)

// Spline camera functions (Original: f:\tr7\game\camera\cameraSpline.c)
void CAMERA_ClearSplineKeyValues(MultiSpline* spline);                                                                     // 0x00488130 (PDB: 0x00489b70)
void CAMERA_ResetSplineCombatSettings(Camera* camera);                                                                     // 0x00488140 (PDB: 0x00489b80)
void CAMERA_Lock_Control_Angle(Camera* camera);                                                                            // 0x00488190 (PDB: 0x00489bd0)
void CAMERA_Unlock_Control_Angle(Camera* camera);                                                                          // 0x004882a0 (PDB: 0x00489ce0)
void CAMERA_SetSpline0(Camera* camera, Intro* spline, int streamID, int alternate);                                        // 0x004882b0 (PDB: 0x00489cf0)
void CAMERA_SetSpline1(Camera* camera, Intro* spline, int streamID, int alternate);                                        // 0x00488350 (PDB: 0x00489d90)
void CAMERA_SetSpline2(Camera* camera, Intro* spline, int streamID, int alternate);                                        // 0x004883f0 (PDB: 0x00489e30)
void CAMERA_SetSplineWidths(int srcW, int tgtW, int srcWZ, int tgtWZ);                                                     // 0x00488490 (PDB: 0x00489ed0)
int CAMERA_SourceNumber(Camera* camera);                                                                                   // 0x004884c0 (PDB: 0x00489f00)
int CAMERA_ProcessSplineCamLevelData(Camera* camera, SplineCameraLevelData* data);                                         // 0x00488500 (PDB: 0x00489f40)
void CAMERA_SetSplineCameraFromLevelData(Camera* camera, Level* level, int index);                                         // 0x00488b50 (PDB: 0x0048a590)
float CAMERA_NearestPointOnLine(cdc::Vector3* outPt, cdc::Vector3* a, cdc::Vector3* b, cdc::Vector3* c);                   // 0x00488b70 (PDB: 0x0048a5b0)
void CAMERA_DoWideSplineCalc(Camera* camera, cdc::Vector3* pos, Spline* spline, int key, float t, cdc::Vector3* outOffset, float width1, float width2); // 0x00488c60 (PDB: 0x0048a6a0)
void CAMERA_DoWideSplineCalcZ(Camera* camera, cdc::Vector3* outOffset, Spline* spline, cdc::Vector3* refPos, float widthZ1, float widthZ2); // 0x00488f70 (PDB: 0x0048a9b0)
float CAMERA_SplineGetNearestPointTarget(Camera* camera, Spline* spline, cdc::Vector3* targetPt, int* outKey, cdc::Vector3* outPt, float* outT); // 0x004890e0 (PDB: 0x0048ab20)
void CAMERA_SplineProcess(Camera* camera);                                                                                 // 0x004895a0 (PDB: 0x0048afe0)
void CAMERA_CalcFollowSplinecamPos(Camera* camera, cdc::Vector3* camPos, cdc::Vector3* targetPos);                         // 0x00489ff0 (PDB: 0x0048bab0)

#endif // GAME_CAMERA_CAMERASPLINE_H
