#pragma once

#ifndef GAME_PHYSICSLEGACY_H
#define GAME_PHYSICSLEGACY_H

#include "common.h"
#include "cdc/runtime/cdcMath/Math.h"
#include "game/instance.h"

// Legacy physics movement & velocity helper routines (Original: game/physicslegacy.c / physicslegacy.cpp)
void PHYSICS_ForceSetWater(Instance* instance, float* waterTime, float waterZLevel, float swimDepth, float maxDelta); // 0x00462270 (PDB: 0x00463c30)
void PHYSICS_SetVelFromZRot(Instance* instance, float angle, float speed);                                            // 0x00462310 (PDB: 0x00463cd0)
float PHYSICS_FindAFromDAndT(float dist, float time);                                                                 // 0x00462340 (PDB: 0x00463d00)
float PHYSICS_FindVFromAAndD(float accl, float dist);                                                                 // 0x00462370 (PDB: 0x00463d30)
void PHYSICS_StopAcceleratingIfCloseToTargetVelocity(Instance* instance, float targetX, float targetY, float targetZ); // 0x004623a0 (PDB: 0x00463d60)
void PhysicsMove(Instance* instance, cdc::Vector3* outOffset, float time);                                            // 0x004624b0 (PDB: 0x00463e70)
void PHYSICS_SetVelFromRot(Instance* instance, cdc::Euler* rot, float magnitude);                                     // 0x00462540 (PDB: 0x00463f00)
void PHYSICS_SetVAndAFromRot(Instance* instance, cdc::Euler* rot, float vMag, float aMag);                            // 0x004625e0 (PDB: 0x00463fa0)
void PhysicsMoveLocalZClamp(Instance* instance, int segment, float time, int clamp);                                  // 0x00462750 (PDB: 0x00464110)
void PhysicsMoveLocalFall(Instance* instance, cdc::Vector3* accl, float time, float maxXYVel, float maxZVel);         // 0x00462840 (PDB: 0x00464200)

#endif // GAME_PHYSICSLEGACY_H
