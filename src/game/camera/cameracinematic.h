#pragma once

#ifndef GAME_CAMERA_CAMERACINEMATIC_H
#define GAME_CAMERA_CAMERACINEMATIC_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/camera.h"

extern cdc::Vector3 Cinematic_lastPosition; // 0x010fd960 (PDB: 0x001eeb60)

// Cinematic camera routines (Original: f:\tr7\game\camera\cameracinematic.cpp)
int  CAMERA_CinematicDone(Camera* camera);    // 0x00483420 (PDB: 0x00484e20)
void CAMERA_CinematicProcess(Camera* camera); // 0x00483440 (PDB: 0x00484e40)

#endif // GAME_CAMERA_CAMERACINEMATIC_H
