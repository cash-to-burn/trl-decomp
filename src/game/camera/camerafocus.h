#ifndef GAME_CAMERA_CAMERAFOCUS_H
#define GAME_CAMERA_CAMERAFOCUS_H

#include "cdc/runtime/cdcMath/Math.h"

struct Camera;
struct Instance;

// Camera focus globals
extern float currentZOffset; // 0x010fda34 (PDB: 0x010fec34)

// Camera focus point calculation routines (Original: f:\tr7\game\camera\cameraFocus.c)
void  CAMERA_CreateNewFocuspoint(Camera* camera);                        // 0x00485c50 (PDB: 0x00487680)
void  CAMERA_SetMaxVel(Camera* camera);                                  // 0x00485cc0 (PDB: 0x004876f0)
float CAMERA_FocusHeight(Camera* camera);                                // 0x00485cd0 (PDB: 0x00487700)
void  CAMERA_SetInstanceFocus(Camera* camera, Instance* instance);       // 0x00485d50 (PDB: 0x00487780)
void  CAMERA_SetFocus(Camera* camera, cdc::Vector3* focusPosition);      // 0x004860e0 (PDB: 0x00487b10)

#endif // GAME_CAMERA_CAMERAFOCUS_H
