#ifndef GAME_CAMERA_CAMERASTACK_H
#define GAME_CAMERA_CAMERASTACK_H

#include <cstdint>

struct MarkUp;

#pragma pack(push, 1)

// Size: 0x74 bytes (116 bytes, TI: 0xbb2e)
struct CameraDTPData {
    /* 0x000 */ int16_t mode;
    /* 0x002 */ int16_t followTilt;
    /* 0x004 */ int16_t followRotation;
    /* 0x006 */ int16_t followDragToRotationSpeed;
    /* 0x008 */ int16_t followRotationRangeSpringAngle;
    /* 0x00a */ int16_t followSmooth;
    /* 0x00c */ int16_t followTransition;
    /* 0x00e */ int16_t followTiltReturnSpeed;
    /* 0x010 */ int16_t followZOffset;
    /* 0x012 */ int16_t autoCenterMultiplier;
    /* 0x014 */ int16_t autoCenterMaxSpeed;
    /* 0x016 */ int16_t minimumDistance;
    /* 0x018 */ int16_t maximumDistance;
    /* 0x01a */ int16_t followHighTiltSpringAngle;
    /* 0x01c */ int16_t followHighTiltDistance;
    /* 0x01e */ int16_t followHighTiltAngle;
    /* 0x020 */ int16_t followHighTiltFOV;
    /* 0x022 */ int16_t followHighTiltZOffset;
    /* 0x024 */ int16_t followHighTiltVerticalBias;
    /* 0x026 */ int16_t followMedTiltDistance;
    /* 0x028 */ int16_t followMedTiltAngle;
    /* 0x02a */ int16_t followMedTiltFOV;
    /* 0x02c */ int16_t followMedTiltZOffset;
    /* 0x02e */ int16_t followMedTiltVerticalBias;
    /* 0x030 */ int16_t followZeroTiltDistance;
    /* 0x032 */ int16_t followZeroTiltAngle;
    /* 0x034 */ int16_t followZeroTiltFOV;
    /* 0x036 */ int16_t followZeroTiltZOffset;
    /* 0x038 */ int16_t followZeroTiltVerticalBias;
    /* 0x03a */ int16_t followLowTiltDistance;
    /* 0x03c */ int16_t followLowTiltAngle;
    /* 0x03e */ int16_t followLowTiltFOV;
    /* 0x040 */ int16_t followLowTiltZOffset;
    /* 0x042 */ int16_t followLowTiltVerticalBias;
    /* 0x044 */ int16_t followLowTiltSpringAngle;
    /* 0x046 */ int16_t followRotationRangeStart;
    /* 0x048 */ int16_t followRotationRangeEnd;
    /* 0x04a */ int16_t followHighTiltLimit;
    /* 0x04c */ int16_t followLowTiltLimit;
    /* 0x04e */ int16_t followVerticalBias;
    /* 0x050 */ int16_t followHorizontalBias;
    /* 0x052 */ int16_t leadAmount;
    /* 0x054 */ int16_t interestInstTune;
    /* 0x056 */ int16_t interestInst;
    /* 0x058 */ int16_t splineTune;
    /* 0x05a */ int16_t splineSource;
    /* 0x05c */ int16_t splineTarget;
    /* 0x05e */ int16_t splineSourceWidth;
    /* 0x060 */ int16_t splineTargetWidth;
    /* 0x062 */ int16_t splineSourceWidthZ;
    /* 0x064 */ int16_t splineTargetWidthZ;
    /* 0x066 */ int16_t combatCamTiltOverride;
    /* 0x068 */ int16_t combatCamDistOverride;
    /* 0x06a */ int16_t crossfade;
    /* 0x06c */ uint16_t rotationRangePlayerRelative : 3;
    /* 0x06c */ uint16_t rotationPlayerRelative : 3;
    /* 0x06c */ uint16_t hybridSplineAdjustTilt : 3;
    /* 0x06c */ uint16_t hybridSplineAdjustDist : 3;
    /* 0x06c */ uint16_t lockInPlace : 3;
    /* 0x06c */ uint16_t DragToNewTilt : 1;
    /* 0x06e */ int8_t cutFlag;
    /* 0x06f */ uint8_t lookaroundDisabled : 2;
    /* 0x06f */ uint8_t grappleCamDisabled : 2;
    /* 0x06f */ uint8_t combatCamDisabled : 4;
    /* 0x070 */ int32_t DTPID;
};

// Size: 0x08 bytes (TI: 0xbb26)
struct CameraStackInfo {
    /* 0x000 */ CameraDTPData* DTPData;
    /* 0x004 */ int16_t signalID;
    /* 0x006 */ uint8_t signalFlags;
    /* 0x007 */ uint8_t pad;
};

// Size: 0x10 bytes (16 bytes, TI: 0xbb28)
struct movementCameraOverrides {
    /* 0x000 */ int32_t ledgeCamID;
    /* 0x004 */ int32_t crawlCamID;
    /* 0x008 */ int32_t vehicleCamID;
    /* 0x00c */ int32_t deathCamID;
};

// Size: 0x290 bytes (656 bytes, TI: 0xbb24)
struct CameraStack {
    /* 0x000 */ int16_t CurrentStackSize;
    /* 0x002 */ int8_t CameraSystemUpdated;
    /* 0x003 */ int8_t CameraStackSystemInUse;
    /* 0x004 */ CameraStackInfo CameraStackList[32];
    /* 0x104 */ CameraDTPData* CurrentBaseCamera;
    /* 0x108 */ CameraDTPData CurrentStoredCombinedBaseCamera;
    /* 0x17c */ CameraDTPData* CurrentMovementCamera;
    /* 0x180 */ CameraDTPData* CurrentMarkupCamera;
    /* 0x184 */ int32_t CurrentMarkupCameraDTPID;
    /* 0x188 */ MarkUp* CurrentMarkupData;
    /* 0x18c */ movementCameraOverrides* movementCamUnitOverrides;
    /* 0x190 */ CameraDTPData CurrentCombinedCamera;
    /* 0x204 */ CameraDTPData* CurrentLockInPlaceCamera;
    /* 0x208 */ CameraDTPData* NewLockInPlaceCamera;
    /* 0x20c */ CameraDTPData* DefaultCamera;
    /* 0x210 */ CameraDTPData* LastAddedCamera;
    /* 0x214 */ CameraDTPData* LatestAddedCamera;
    /* 0x218 */ CameraDTPData LastUpdatedCamera;
    /* 0x28c */ int16_t CurrentMovementMode;
    /* 0x28e */ uint8_t MovementCameraInUse : 1;
    /* 0x28e */ uint8_t MarkupCameraInUse : 1;
    /* 0x28e */ uint8_t MarkupCameraOverridesMovementCam : 1;
    /* 0x28e */ uint8_t DisableMarkupCameras : 1;
    /* 0x28e */ uint8_t DisableMovementCameras : 1;
    /* 0x28e */ uint8_t NewMarkupCamera : 1;
    /* 0x28e */ uint8_t pad : 2;
    /* 0x28f */ uint8_t refreshCameraSystem : 4;
    /* 0x28f */ uint8_t pad2 : 4;
};

#pragma pack(pop)

static_assert(sizeof(CameraDTPData) == 0x74, "CameraDTPData size mismatch");
static_assert(sizeof(CameraStackInfo) == 0x08, "CameraStackInfo size mismatch");
static_assert(sizeof(movementCameraOverrides) == 0x10, "movementCameraOverrides size mismatch");
static_assert(sizeof(CameraStack) == 0x290, "CameraStack size mismatch");

// Global instance
extern CameraStack theCameraStack; // 0x010fda98 (PDB: 0x010fec98)

// Camera stack functions
void CameraStackCombineData(CameraDTPData* dest, CameraDTPData* src);                        // 0x0048a1e0 (PDB: 0x0048bca0)
void CameraStackUpdateCameraSystem(CameraDTPData* data);                                    // 0x0048b240 (PDB: 0x0048cd00)
void CameraStackRefreshCameraSystem(void);                                                  // 0x0048bc40 (PDB: 0x0048d700)
void CameraStackRefreshCameraSystemResetTilt(void);                                         // 0x0048bc50 (PDB: 0x0048d710)
void CameraStackInit(CameraDTPData* defaultCamera);                                         // 0x0048bc60 (PDB: 0x0048d720)
void CameraStackInitDTPID(int dtpID);                                                       // 0x0048bd10 (PDB: 0x0048d7d0)
void CameraStackClearMarkupCam(void);                                                       // 0x0048bd40 (PDB: 0x0048d800)
void CameraStackCombineAll(void);                                                           // 0x0048bd70 (PDB: 0x0048d830)
void CameraStackClear(void);                                                                // 0x0048be70 (PDB: 0x0048d930)
void CameraStackSetBase(CameraDTPData* data, movementCameraOverrides* overrides, int enable); // 0x0048bed0 (PDB: 0x0048d990)
void CameraStackSetBaseDTPID(int dtpID, movementCameraOverrides* overrides, int enable);    // 0x0048bf20 (PDB: 0x0048d9e0)
void CameraStackSetBaseDTPIDRecheck(int dtpID, movementCameraOverrides* overrides, int enable); // 0x0048bf70 (PDB: 0x0048da30)
void CameraStackAddCamera(CameraDTPData* data, int signalID, int signalMode, int signalFlags); // 0x0048bf90 (PDB: 0x0048da50)
int  CameraStackRemoveCamera(CameraDTPData* data, int signalID, int unused);                // 0x0048c020 (PDB: 0x0048db10)
void CameraStackAddCameraDTPID(int dtpID, int signalID, int signalMode, int signalFlags);   // 0x0048c0c0 (PDB: 0x0048dbd0)
int  CameraStackRemoveCameraDTPID(int dtpID, int signalID, int unused);                     // 0x0048c0f0 (PDB: 0x0048dc00)
void CameraStackProcessPlayerState(void);                                                   // 0x0048c110 (PDB: 0x0048dc20)
void CameraStackMain(void);                                                                 // 0x0048c6a0 (PDB: 0x0048e1b0)

#endif // GAME_CAMERA_CAMERASTACK_H
