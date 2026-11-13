#pragma once

#ifndef GAME_MATH3D_H
#define GAME_MATH3D_H

#include "common.h"
#include "cdc/runtime/cdcMath/Math.h"

void MATH3D_QuatLogToMatrix(cdc::Matrix* mat, const cdc::Quat* q);                                                                         // 0x0045f690 (PDB: 0x00460e40)
float MATH3D_AngleFromVector(const cdc::Vector3* vec);                                                                                       // 0x0045f800 (PDB: 0x00460fb0)
float MATH3D_AngleFromPosToPosf(const cdc::Vector3* from, const cdc::Vector3* to);                                                           // 0x0045f840 (PDB: 0x00460ff0)
float MATH3D_ElevationFromPosToPosf(const cdc::Vector3* from, const cdc::Vector3* to);                                                       // 0x0045f880 (PDB: 0x00461030)
float MATH3D_ElevationFromVector(const cdc::Vector3* vec);                                                                                   // 0x0045f900 (PDB: 0x004610b0)
void MATH3D_RotationFromPosToPosf(const cdc::Vector3* from, const cdc::Vector3* to, cdc::Euler* rot);                                        // 0x0045f970 (PDB: 0x00461120)
void MATH3D_ZYXtoXYZf(cdc::Euler* euler);                                                                                                    // 0x0045f9d0 (PDB: 0x00461180)
float MATH3D_AngleDifff(float current, float destination);                                                                                     // 0x0045fa20 (PDB: 0x004611d0)
int MATH3D_CheckAngleInRange(float zfacemin, float zfacemax, float ztest);                                                                   // 0x0045fa60 (PDB: 0x00461210)
void MATH3D_NearestPointOnPlane(cdc::Vector3* planept, const cdc::Vector3* planenorm, float d, const cdc::Vector3* position);                // 0x0045fb40 (PDB: 0x004612f0)
int MATH3D_IntersectLineAndPlane(cdc::Vector3* intersection, const cdc::Vector3* pos0, const cdc::Vector3* pos1, const cdc::Vector3* planeNormal, float planeD); // 0x0045fba0 (PDB: 0x00461350)
float MATH3D_NearestPointOnLine(cdc::Vector3* outPoint, const cdc::Vector3* pos0, const cdc::Vector3* pos1, const cdc::Vector3* point);      // 0x0045fc90 (PDB: 0x00461440)
float MATH3D_DistanceFromPointToLineSegment(const cdc::Vector3* pos0, const cdc::Vector3* pos1, const cdc::Vector3* point);                  // 0x0045fdb0 (PDB: 0x00461560)
float MATH3D_AngleBetweenVectorsf(const cdc::Vector3* v0, const cdc::Vector3* v1);                                                           // 0x0045fe20 (PDB: 0x004615d0)
cdc::Vector3 MATH3D_ConvertAngleToVector(float angle);                                                                                       // 0x0045feb0 (PDB: 0x00461660)
cdc::Euler MATH3D_EulerFromPosToPos(const cdc::Vector3* from, const cdc::Vector3* to);                                                       // 0x0045ff00 (PDB: 0x004616b0)
float MATH3D_ClampAngleInRange(float minAngle, float maxAngle, float inputAngle);                                                            // 0x0045ffc0 (PDB: 0x00461770)
void MATH3D_AngleMoveTowardf(float* current_ptr, float destination, float step);                                                             // 0x00460060 (PDB: 0x00461810)
void MATH3D_RelativePositionf(const cdc::Matrix* mat, const cdc::Vector3* origin, const cdc::Vector3* target, cdc::Vector3* out);            // 0x00460120 (PDB: 0x004618d0)
int MATH3D_ConeDetectf(const cdc::Vector3* vec, float xcone, float ycone);                                                                   // 0x00460190 (PDB: 0x00461940)
void MATH3D_Solve2BoneIK(const cdc::Vector3* rootPos, const cdc::Vector3* midPos, const cdc::Vector3* endPos, const cdc::Vector3* targetPos, const cdc::Vector3* bendAxisFallback, cdc::Quat* outUpperRot, cdc::Quat* outLowerRot); // 0x00460270 (PDB: 0x00461a20)
float MATH3D_CheckMatrixAxisAlignment(const cdc::Matrix* mat0, const cdc::Matrix* mat1, const cdc::Vector3* axis);                           // 0x00460500 (PDB: 0x00461cb0)

#endif // GAME_MATH3D_H
