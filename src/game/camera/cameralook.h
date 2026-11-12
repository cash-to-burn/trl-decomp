#ifndef GAME_CAMERA_CAMERALOOK_H
#define GAME_CAMERA_CAMERALOOK_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/camera.h"
#include <cstdint>

#pragma pack(push, 1)

// Size: 0x144 bytes (324 bytes, TI: 0x8909)
struct TurretProp {
    /* 0x000 */ uint8_t pad000[0x48];
    /* 0x048 */ int8_t firingSegment;
    /* 0x049 */ int8_t iHorizPivotSegment;
    /* 0x04a */ int8_t iVertPivotSegment;
    /* 0x04b */ int8_t bodySegment;
    /* 0x04c */ uint8_t pad04c[0x108 - 0x04c];
    /* 0x108 */ float fCameraAnchorOffset;
    /* 0x10c */ Vector3f vCameraBoom;
    /* 0x118 */ float fCameraInterp;
    /* 0x11c */ float fCameraProjDistFactor;
    /* 0x120 */ uint16_t uCameraProjInterpIn;
    /* 0x122 */ uint16_t uCameraProjInterpOut;
    /* 0x124 */ uint8_t flags;
    /* 0x125 */ uint8_t turretType;
    /* 0x126 */ uint8_t pad126[0x144 - 0x126];
};

// Size: 0x160 bytes (352 bytes, TI: 0x88cf)
struct TurretData {
    /* 0x000 */ uint8_t pad000[0xf0];
    /* 0x0f0 */ float fInitialZRot;
    /* 0x0f4 */ float fZOffset;
    /* 0x0f8 */ float fXOffset;
    /* 0x0fc */ uint8_t pad0fc[0x160 - 0x0fc];
};

// Size: 0x54 bytes (84 bytes, TI: 0x90fb)
struct TurretState {
    /* 0x00 */ uint8_t basePhysobInteractState[0x48];
    /* 0x48 */ float fPitch;
    /* 0x4c */ float fYaw;
    /* 0x50 */ uint8_t pad50[4];
};

#pragma pack(pop)

static_assert(sizeof(TurretProp) == 0x144, "TurretProp size mismatch");
static_assert(sizeof(TurretData) == 0x160, "TurretData size mismatch");
static_assert(sizeof(TurretState) == 0x54, "TurretState size mismatch");

// Module global (f:\tr7\game\camera\cameraLook.c)
extern float cameraTurretInterp; // 0x010fc57c (PDB: 0x010fd77c)

// Camera look-around, accurate aim, and turret processors (Original: f:\tr7\game\camera\cameraLook.c)
void  CAMERA_SetStickyLookMode(void);                                           // 0x00486450 (PDB: 0x00487e90)
void  CAMERA_SetAccurateAimMode(void);                                          // 0x00486460 (PDB: 0x00487ea0)
void  CAMERA_SetLookFocusAndBase(Instance* instance, cdc::Vector3* outFocus);   // 0x00486470 (PDB: 0x00487eb0)
void  CAMERA_EndLook(Camera* camera);                                           // 0x00486510 (PDB: 0x00487f50)
float CAMERA_AccurateAimInterpTime(Camera* camera);                             // 0x00486780 (PDB: 0x004881c0)
void  CAMERA_InitAimAssist(void);                                               // 0x004867a0 (PDB: 0x004881e0)
void  CAMERA_AimAssist(cdc::Euler* outRot);                                     // 0x00486820 (PDB: 0x00488260)
void  CAMERA_ResetStickyLookMode(void);                                         // 0x00486c90 (PDB: 0x004886d0)
void  CAMERA_ResetAccurateAimMode(void);                                        // 0x00486cb0 (PDB: 0x004886f0)
void  CAMERA_StartLookaroundMode(Camera* camera);                               // 0x00486cd0 (PDB: 0x00488710)
void  CAMERA_StartAccurateAimMode(Camera* camera, Instance* playerInst);        // 0x00486df0 (PDB: 0x00488830)
void  CAMERA_ForceEndLookaroundMode(Camera* camera);                            // 0x00486fb0 (PDB: 0x004889f0)
void  CAMERA_LookProcess(Camera* camera);                                       // 0x00486fd0 (PDB: 0x00488a10)
void  CAMERA_AccurateAimProcessInputs(Camera* camera, Instance* playerInst);    // 0x00487540 (PDB: 0x00488f80)
void  CAMERA_AccurateAimProcess(Camera* camera);                                // 0x004877d0 (PDB: 0x00489210)
void  CAMERA_TurretProcess(Camera* camera);                                     // 0x00487c30 (PDB: 0x00489670)

#endif // GAME_CAMERA_CAMERALOOK_H
