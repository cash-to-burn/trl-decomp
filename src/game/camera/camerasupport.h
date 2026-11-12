#ifndef GAME_CAMERA_CAMERASUPPORT_H
#define GAME_CAMERA_CAMERASUPPORT_H

#include "cdc/runtime/cdcMath/Math.h"

struct Camera;

// Critical damping routines (Original: f:\tr7\game\camera\camerasupport.c)
void CriticalDampValue(int smoothType, float* current, float target, float* vel, float* acc, float maxVel); // 0x0048c6d0 (PDB: 0x0048e1e0)
void CriticalDampValueControl(float* current, float target, float* vel, float* acc, float maxVel, float dampDivisor); // 0x0048c840 (PDB: 0x0048e350)
void CriticalDampPosition(int smoothType, cdc::Vector3* current, const cdc::Vector3* target, cdc::Vector3* vel, cdc::Vector3* acc, float maxVel); // 0x0048ccc0 (PDB: 0x0048e7d0)
void CriticalDampPositionControl(cdc::Vector3* current, const cdc::Vector3* target, cdc::Vector3* vel, cdc::Vector3* acc, float maxVel, float dampDivisor); // 0x0048d100 (PDB: 0x0048ec10)
void CriticalDampAngle(int smoothType, float* current, float target, float* vel, float* acc, float maxVel); // 0x0048d200 (PDB: 0x0048ed10)
void CriticalDampAngleControl(float* current, float target, float* vel, float* acc, float maxVel, float dampDivisor); // 0x0048d2a0 (PDB: 0x0048edb0)

// Camera angle and position support routines
void CAMERA_CalcRotation(cdc::Euler* outRot, const cdc::Vector3* target, const cdc::Vector3* source); // 0x0048c8d0 (PDB: 0x0048e3e0)
float CAMERA_CircleAnglef(float angle); // 0x0048ca00 (PDB: 0x0048e510)
void CAMERA_SetZRotation(Camera* camera, float rotZ); // 0x0048ca60 (PDB: 0x0048e570)
float CAMERA_SignedAngleDifference(float a, float b); // 0x0048ca90 (PDB: 0x0048e5a0)
bool CAMERA_FocusInstanceMoved(Camera* camera); // 0x0048caf0 (PDB: 0x0048e600)
void CAMERA_AngleMoveTowardf(float* current, float target, float step); // 0x0048cb80 (PDB: 0x0048e690)
void AngleMoveToward(float* current, float target, float step); // 0x0048cc50 (PDB: 0x0048e760)
void Decouple_AngleMoveToward(float* current, float target, float step); // 0x0048cc70 (PDB: 0x0048e780)
float CAMERA_CalcZRotation(const cdc::Vector3* target, const cdc::Vector3* source); // 0x0048d340 (PDB: 0x0048ee50)
float CAMERA_AngleDifference(float a, float b); // 0x0048d380 (PDB: 0x0048ee90)
float CAMERA_LimitRotation(Camera* camera, float rotZ); // 0x0048d410 (PDB: 0x0048ef20)
void CAMERA_CalcPosition(cdc::Vector3* outPos, const cdc::Vector3* focusPos, const cdc::Euler* rot, float dist); // 0x0048d580 (PDB: 0x0048f090)

#endif // GAME_CAMERA_CAMERASUPPORT_H
