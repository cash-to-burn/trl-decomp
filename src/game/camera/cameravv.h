#ifndef GAME_CAMERA_CAMERAVV_H
#define GAME_CAMERA_CAMERAVV_H

#include "cdc/runtime/cdcMath/Math.h"

struct CameraCore;

// View volume & transform functions (Original: f:\tr7\game\camera\cameravv.c)
void CAMERA_CalculateViewVolumeNormals(CameraCore* camera);                      // 0x0048d650 (PDB: 0x0048f160)
void CAMERA_AspectAdjustMatrixf(cdc::Matrix* matrix);                             // 0x0048d970 (PDB: 0x0048f480)
void CAMERA_SetNewProjDistance(CameraCore* camera, float projDist, float speed);  // 0x0048da40 (PDB: 0x0048f550)
int  CAMERA_SphereIntersectsViewVolume(cdc::Vector3* sphere, CameraCore* camera); // 0x0048da60 (PDB: 0x0048f570)
void CAMERA_CalcVVClipInfo(CameraCore* camera);                                   // 0x0048db70 (PDB: 0x0048f680)
void CAMERA_CalculateWCTransform(CameraCore* camera);                             // 0x0048ddd0 (PDB: 0x0048f8e0)
void CAMERA_SetViewVolume(CameraCore* camera);                                    // 0x0048e100 (PDB: 0x0048fc10)
void CAMERA_SetDefaultViewVolume(CameraCore* camera);                             // 0x0048e120 (PDB: 0x0048fc30)
void CAMERA_SetProjDistance2(CameraCore* camera, float projDist);                 // 0x0048e150 (PDB: 0x0048fc60)
void CAMERA_SetProjDistance(CameraCore* camera, float projDist);                  // 0x0048e210 (PDB: 0x0048fd20)

#endif // GAME_CAMERA_CAMERAVV_H
