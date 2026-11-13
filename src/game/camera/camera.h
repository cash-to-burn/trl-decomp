#ifndef GAME_CAMERA_CAMERA_H
#define GAME_CAMERA_CAMERA_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/instance.h"
#include "game/gameloop.h"
#include <cstdint>

// Forward declarations
struct MultiSpline;
struct IndexedFace;
struct Terrain;
struct Level;

#pragma pack(push, 1)

// Size: 0x300 bytes (768 bytes, TI: 0x68e2)
struct CameraCore {
    /* 0x000 */ cdc::Vector3 position;
    /* 0x010 */ cdc::Vector3 effectPosition;
    /* 0x020 */ cdc::Matrix vvNormalWorVecMatf[2];
    /* 0x0a0 */ cdc::Matrix defaultvvNormalWorVecMatf[2];
    /* 0x120 */ cdc::Matrix wcTransformf;
    /* 0x160 */ cdc::Matrix wcTransformNoShakef;
    /* 0x1a0 */ cdc::Matrix wcTransform2f;
    /* 0x1e0 */ cdc::Matrix cwTransform2f;
    /* 0x220 */ cdc::Vector3 viewVolumeNormal[5];
    /* 0x270 */ cdc::Euler Rotation;
    /* 0x280 */ cdc::Vector3 debugPos;
    /* 0x290 */ cdc::Vector3 nondebugPos;
    /* 0x2a0 */ float leftX;
    /* 0x2a4 */ float rightX;
    /* 0x2a8 */ float topY;
    /* 0x2ac */ float bottomY;
    /* 0x2b0 */ float farPlane;
    /* 0x2b4 */ float projDistance;
    /* 0x2b8 */ float newProjDistance;
    /* 0x2bc */ float projDistanceSpd;
    /* 0x2c0 */ float screenXRatio;
    /* 0x2c4 */ float screenYRatio;
    /* 0x2c8 */ float screenFullXRatio;
    /* 0x2cc */ float screenFullYRatio;
    /* 0x2d0 */ float nearPlane;
    /* 0x2d4 */ int waterBlurFlag;
    /* 0x2d8 */ uint8_t pad2d8[8];
    /* 0x2e0 */ cdc::Euler debugRot;
    /* 0x2f0 */ IndexedFace* tiltTface;
    /* 0x2f4 */ Terrain* tiltTfaceTerrain;
    /* 0x2f8 */ uint8_t pad2f8[8];
};

// Size: 0x18 bytes (24 bytes, TI: 0x5d2d)
struct CameraKey {
    /* 0x000 */ uint8_t x;
    /* 0x001 */ uint8_t pad001;
    /* 0x002 */ uint8_t y;
    /* 0x003 */ uint8_t pad003;
    /* 0x004 */ uint8_t z;
    /* 0x005 */ uint8_t pad005;
    /* 0x006 */ uint8_t id;
    /* 0x007 */ uint8_t pad007;
    /* 0x008 */ uint8_t rx;
    /* 0x009 */ uint8_t pad009;
    /* 0x00a */ uint8_t ry;
    /* 0x00b */ uint8_t pad00b;
    /* 0x00c */ uint8_t rz;
    /* 0x00d */ uint8_t pad00d;
    /* 0x00e */ uint8_t flags;
    /* 0x00f */ uint8_t pad00f;
    /* 0x010 */ uint8_t tx;
    /* 0x011 */ uint8_t pad011;
    /* 0x012 */ uint8_t ty;
    /* 0x013 */ uint8_t pad013;
    /* 0x014 */ uint8_t tz;
    /* 0x015 */ uint8_t pad015;
    /* 0x016 */ uint8_t pad[2];
};

// Size: 0x24 bytes (36 bytes, TI: 0x6572)
struct FollowTransitionData {
    /* 0x000 */ float startTilt;
    /* 0x004 */ float startRotation;
    /* 0x008 */ float startDistance;
    /* 0x00c */ float startVerticalBias;
    /* 0x010 */ float startHorizontalBias;
    /* 0x014 */ float transition;
    /* 0x018 */ float transitionLinear;
    /* 0x01c */ float transitionVel;
    /* 0x020 */ float transitionAccl;
};

// Size: 0x50 bytes (80 bytes, TI: 0x69c6)
struct FollowSplinecamData {
    /* 0x000 */ cdc::Vector3 sourceStartPoint;
    /* 0x010 */ cdc::Vector3 targetStartPoint;
    /* 0x020 */ MultiSpline* sourceSpline;
    /* 0x024 */ MultiSpline* targetSpline;
    /* 0x028 */ uint8_t sourceNumber;
    /* 0x029 */ uint8_t pad029;
    /* 0x02a */ uint8_t targetNumber;
    /* 0x02b */ uint8_t pad02b;
    /* 0x02c */ uint8_t splinecam_currkey;
    /* 0x02d */ uint8_t pad02d;
    /* 0x02e */ uint8_t splineInUse : 4;
    /* 0x02e */ uint8_t lockInPlace : 4;
    /* 0x02f */ uint8_t splineAdjustDist : 4;
    /* 0x02f */ uint8_t splineAdjustTilt : 2;
    /* 0x02f */ uint8_t pad : 2;
    /* 0x030 */ float sourceSplineWidth;
    /* 0x034 */ float targetSplineWidth;
    /* 0x038 */ float sourceSplineWidthZ;
    /* 0x03c */ float targetSplineWidthZ;
    /* 0x040 */ float splineTune;
    /* 0x044 */ uint8_t pad044[12];
};

// Size: 0x50 bytes (80 bytes, TI: 0x651a)
struct SplinecamData {
    /* 0x000 */ cdc::Vector3 sourceStartPoint;
    /* 0x010 */ cdc::Vector3 targetStartPoint;
    /* 0x020 */ cdc::Vector3 thirdStartPoint;
    /* 0x030 */ MultiSpline* sourceSpline;
    /* 0x034 */ MultiSpline* targetSpline;
    /* 0x038 */ MultiSpline* thirdSpline;
    /* 0x03c */ uint8_t useStartPoint;
    /* 0x03d */ uint8_t pad03d;
    /* 0x03e */ uint8_t sourceNumber;
    /* 0x03f */ uint8_t pad03f;
    /* 0x040 */ uint8_t targetNumber;
    /* 0x041 */ uint8_t pad041;
    /* 0x042 */ uint8_t thirdNumber;
    /* 0x043 */ uint8_t pad043[13];
};

// Size: 0x7c bytes (124 bytes, TI: 0x6810)
struct SplinecamParams {
    /* 0x000 */ float sourceSplineWidth;
    /* 0x004 */ float targetSplineWidth;
    /* 0x008 */ float sourceSplineWidthZ;
    /* 0x00c */ float targetSplineWidthZ;
    /* 0x010 */ float combatCamDist;
    /* 0x014 */ float combatCamMinDist;
    /* 0x018 */ float combatCamZOffset;
    /* 0x01c */ float zoomDist;
    /* 0x020 */ float cameraVelocity;
    /* 0x024 */ float cameraDampening;
    /* 0x028 */ float targetVelocity;
    /* 0x02c */ float targetDampening;
    /* 0x030 */ float combatFraming;
    /* 0x034 */ float followDistance;
    /* 0x038 */ float followTilt;
    /* 0x03c */ float followRotation;
    /* 0x040 */ float followSmooth;
    /* 0x044 */ float followZOffset;
    /* 0x048 */ float switchToFollowCamDist;
    /* 0x04c */ float followHighTiltDistance;
    /* 0x050 */ float followHighTiltAngle;
    /* 0x054 */ float followMedTiltDistance;
    /* 0x058 */ float followMedTiltAngle;
    /* 0x05c */ float followZeroTiltDistance;
    /* 0x060 */ float followZeroTiltAngle;
    /* 0x064 */ float followLowTiltDistance;
    /* 0x068 */ float followLowTiltAngle;
    /* 0x06c */ float followVerticalBias;
    /* 0x070 */ int8_t followCamOverrideEnabled;
    /* 0x071 */ int8_t combatCamLockedOut;
    /* 0x072 */ uint8_t pad[2];
    /* 0x074 */ float interestInstTune;
    /* 0x078 */ Instance* interestInst;
};

// Size: 0x230 bytes (560 bytes, TI: 0x6431)
struct SavedCinematic {
    /* 0x000 */ cdc::Vector3 position;
    /* 0x010 */ cdc::Vector3 focusPoint;
    /* 0x020 */ cdc::Vector3 base;
    /* 0x030 */ cdc::Vector3 targetPos;
    /* 0x040 */ cdc::Vector3 targetFocusPoint;
    /* 0x050 */ cdc::Vector3 targetBase;
    /* 0x060 */ cdc::Vector3 baseRot;
    /* 0x070 */ cdc::Euler focusRotation;
    /* 0x080 */ cdc::Vector3 focusPointVel;
    /* 0x090 */ cdc::Vector3 focusPointAccl;
    /* 0x0a0 */ cdc::Euler targetFocusRotation;
    /* 0x0b0 */ cdc::Vector3 baseVel;
    /* 0x0c0 */ cdc::Vector3 baseAccl;
    /* 0x0d0 */ float targetFocusDistance;
    /* 0x0d4 */ float focusDistance;
    /* 0x0d8 */ float maxVel;
    /* 0x0dc */ float velocityLag;
    /* 0x0e0 */ float velocityLagVel;
    /* 0x0e4 */ float velocityLagAccl;
    /* 0x0e8 */ uint8_t pad0e8[8];
    /* 0x0f0 */ SplinecamData materialSplinecamData;
    /* 0x140 */ SplinecamData alternateSplinecamData;
    /* 0x190 */ SplinecamParams splinecamParams;
    /* 0x20c */ int Cinematic_Camera_StreamID;
    /* 0x210 */ int8_t lookat_spline;
    /* 0x211 */ uint8_t pad211[3];
    /* 0x214 */ Level* level;
    /* 0x218 */ float smooth;
    /* 0x21c */ float leadAmount;
    /* 0x220 */ int splinecam_currkey;
    /* 0x224 */ uint8_t pad224[12];
};

// Size: 0x140 bytes (320 bytes, TI: 0x67a5)
struct NextCamData {
    /* 0x000 */ SplinecamData materialSplinecamData;
    /* 0x050 */ SplinecamData alternateSplinecamData;
    /* 0x0a0 */ SplinecamParams splinecamParams;
    /* 0x11c */ float followCamDistance;
    /* 0x120 */ float followCamTilt;
    /* 0x124 */ float followCamRotation;
    /* 0x128 */ float followCamSmooth;
    /* 0x12c */ float ZOffset;
    /* 0x130 */ float cutAngle;
    /* 0x134 */ float leadAmount;
    /* 0x138 */ int8_t cutFlag;
    /* 0x139 */ int8_t lookaroundDisabled;
    /* 0x13a */ uint8_t crossfade;
    /* 0x13b */ uint8_t pad13b;
    /* 0x13c */ int Cinematic_Camera_StreamID;
};

struct CameraDataCinematic {
    /* 0x000 */ cdc::Vector3 lastSplinePos;
    /* 0x010 */ SplinecamData materialSplinecamData;
    /* 0x060 */ SplinecamData alternateSplinecamData;
    /* 0x0b0 */ SplinecamParams splinecamParams;
    /* 0x12c */ int8_t splinePointAhead;
    /* 0x12d */ int8_t lookat_spline;
    /* 0x12e */ int8_t cinema_done;
    /* 0x12f */ int8_t pad;
    /* 0x130 */ int splinecam_currkey;
    /* 0x134 */ int splinecam_helpkey;
    /* 0x138 */ int Cinematic_Camera_StreamID;
    /* 0x13c */ uint8_t pad13c[4];
};

struct CameraDataFollow {
    /* 0x000 */ float stopTimer;
    /* 0x004 */ int stopHit;
    /* 0x008 */ IndexedFace* tface;
    /* 0x00c */ IndexedFace* oldTFace;
    /* 0x010 */ int hit;
};

// Size: 0x160 bytes (352 bytes, TI: 0x6a97)
struct CameraDataUnion {
    /* 0x000 */ CameraDataCinematic Cinematic;
    /* 0x140 */ CameraDataFollow Follow;
    /* 0x154 */ uint8_t pad154[12];
};

// Size: 0x1320 bytes (4896 bytes, TI: 0x6a90)
struct Camera {
    /* 0x000 */ CameraCore core;
    /* 0x300 */ uint8_t shake;
    /* 0x301 */ uint8_t pad301;
    /* 0x302 */ uint8_t shakeFrame;
    /* 0x303 */ uint8_t pad303;
    /* 0x304 */ float shakeScale;
    /* 0x308 */ uint8_t shakeFadeIn;
    /* 0x309 */ uint8_t pad309;
    /* 0x30a */ uint8_t shakeFadeOut;
    /* 0x30b */ uint8_t pad30b;
    /* 0x30c */ int flags;
    /* 0x310 */ int lock;
    /* 0x314 */ uint8_t mode;
    /* 0x315 */ uint8_t pad315;
    /* 0x316 */ uint16_t specialModeBits;
    /* 0x318 */ int8_t posState;
    /* 0x319 */ int8_t rotState;
    /* 0x31a */ int8_t tiltState;
    /* 0x31b */ int8_t distanceState;
    /* 0x31c */ uint8_t rotationRangePlayerRelative : 4;
    /* 0x31c */ uint8_t cameraUnderwater : 4;
    /* 0x31d */ uint8_t followCameraCombatCamDisable : 2;
    /* 0x31d */ uint8_t followCameraGrappleCamDisable : 2;
    /* 0x31d */ uint8_t followCameraLookaroundDisabled : 2;
    /* 0x31d */ uint8_t followCameraTimedTransition : 2;
    /* 0x31e */ uint8_t pad2;
    /* 0x31f */ uint8_t pad31f;
    /* 0x320 */ float lagZ;
    /* 0x324 */ float tilt;
    /* 0x328 */ float ZOffset;
    /* 0x32c */ uint8_t pad32c[4];
    /* 0x330 */ cdc::Vector3 focusPoint;
    /* 0x340 */ float focusDistance;
    /* 0x344 */ float followCombatCamTiltOverride;
    /* 0x348 */ float followCombatCamDistOverride;
    /* 0x34c */ float followTiltReturnSpeed;
    /* 0x350 */ float followDragToRotateSpeed;
    /* 0x354 */ float followUserControlTimeout;
    /* 0x358 */ float followHighTiltDistance;
    /* 0x35c */ float followHighTiltAngle;
    /* 0x360 */ float followMedTiltDistance;
    /* 0x364 */ float followMedTiltAngle;
    /* 0x368 */ float followZeroTiltDistance;
    /* 0x36c */ float followZeroTiltAngle;
    /* 0x370 */ float followLowTiltDistance;
    /* 0x374 */ float followLowTiltAngle;
    /* 0x378 */ float followHighTiltLimit;
    /* 0x37c */ float followLowTiltLimit;
    /* 0x380 */ float followHighTiltFOV;
    /* 0x384 */ float followHighTiltZOffset;
    /* 0x388 */ float followHighTiltVerticalBias;
    /* 0x38c */ float followMedTiltFOV;
    /* 0x390 */ float followMedTiltZOffset;
    /* 0x394 */ float followMedTiltVerticalBias;
    /* 0x398 */ float followZeroTiltFOV;
    /* 0x39c */ float followZeroTiltZOffset;
    /* 0x3a0 */ float followZeroTiltVerticalBias;
    /* 0x3a4 */ float followLowTiltFOV;
    /* 0x3a8 */ float followLowTiltZOffset;
    /* 0x3ac */ float followLowTiltVerticalBias;
    /* 0x3b0 */ float followShapeVerticalBiasOffset;
    /* 0x3b4 */ float followShapeFOVOffset;
    /* 0x3b8 */ float followShapeZOffsetOffset;
    /* 0x3bc */ float followRotationRangeStart;
    /* 0x3c0 */ float followRotationRangeEnd;
    /* 0x3c4 */ float followRotationRangeSoftRange;
    /* 0x3c8 */ float followMinimumDistance;
    /* 0x3cc */ float followMaximumDistance;
    /* 0x3d0 */ float followHighSpringDegree;
    /* 0x3d4 */ float followLowSpringDegree;
    /* 0x3d8 */ float followAutoCenterMultiplier;
    /* 0x3dc */ float followAutoCenterMaxSpeed;
    /* 0x3e0 */ float followVerticalBias;
    /* 0x3e4 */ float followDampedVerticalBias;
    /* 0x3e8 */ float followVBiasVel;
    /* 0x3ec */ float followVBiasAcc;
    /* 0x3f0 */ float followHorizontalBias;
    /* 0x3f4 */ float followDampedHorizontalBiasRotation;
    /* 0x3f8 */ float followHBiasVel;
    /* 0x3fc */ float followHBiasAcc;
    /* 0x400 */ FollowTransitionData followTransitionData;
    /* 0x424 */ uint8_t pad424[12];
    /* 0x430 */ FollowSplinecamData followSplineData;
    /* 0x480 */ Instance* followInterestInst;
    /* 0x484 */ float followInterestInstTune;
    /* 0x488 */ Instance* focusInstance;
    /* 0x48c */ uint8_t pad48c[4];
    /* 0x490 */ cdc::Vector3 oldFocusInstancePos;
    /* 0x4a0 */ cdc::Vector3 newFocusInstancePos;
    /* 0x4b0 */ cdc::Euler OldFocusInstanceRot;
    /* 0x4c0 */ cdc::Euler NewFocusInstanceRot;
    /* 0x4d0 */ cdc::Vector3 focusInstanceVelVec;
    /* 0x4e0 */ cdc::Vector3 focusLeadPivot;
    /* 0x4f0 */ cdc::Vector3 focusOffset;
    /* 0x500 */ cdc::Euler FocusRotation;
    /* 0x510 */ float tiltVel;
    /* 0x514 */ float tiltAccl;
    /* 0x518 */ uint8_t pad518[8];
    /* 0x520 */ cdc::Vector3 positionVel;
    /* 0x530 */ cdc::Vector3 positionAccl;
    /* 0x540 */ cdc::Vector3 focusPointVel;
    /* 0x550 */ cdc::Vector3 focusPointAccl;
    /* 0x560 */ cdc::Vector3 targetPointVel;
    /* 0x570 */ cdc::Vector3 targetPointAccl;
    /* 0x580 */ float focusDistanceVel;
    /* 0x584 */ float focusDistanceAccl;
    /* 0x588 */ uint8_t pad588[8];
    /* 0x590 */ cdc::Vector3 focusRotVel;
    /* 0x5a0 */ cdc::Vector3 focusRotAccl;
    /* 0x5b0 */ float maxVel;
    /* 0x5b4 */ float velocityLag;
    /* 0x5b8 */ float velocityLagVel;
    /* 0x5bc */ float velocityLagAccl;
    /* 0x5c0 */ cdc::Vector3 targetPos;
    /* 0x5d0 */ float targetTilt;
    /* 0x5d4 */ float targetFocusDistance;
    /* 0x5d8 */ uint8_t pad5d8[8];
    /* 0x5e0 */ cdc::Vector3 targetFocusPoint;
    /* 0x5f0 */ float collisionTargetFocusDistance;
    /* 0x5f4 */ uint8_t pad5f4[12];
    /* 0x600 */ cdc::Euler TargetFocusRotation;
    /* 0x610 */ float tfaceTilt;
    /* 0x614 */ uint8_t pad614[12];
    /* 0x620 */ cdc::Euler CollisionTargetFocusRotation;
    /* 0x630 */ float smooth;
    /* 0x634 */ CameraKey* cameraKey;
    /* 0x638 */ uint8_t lookTimer;
    /* 0x639 */ uint8_t pad639;
    /* 0x63a */ uint8_t lookSavedMode;
    /* 0x63b */ uint8_t pad63b;
    /* 0x63c */ uint8_t CameraLookStickyFlag;
    /* 0x63d */ uint8_t pad63d;
    /* 0x63e */ uint8_t panic_count;
    /* 0x63f */ uint8_t pad63f;
    /* 0x640 */ bool bAccAimFlag;
    /* 0x641 */ uint8_t pad641;
    /* 0x642 */ uint8_t stack;
    /* 0x643 */ uint8_t pad643;
    /* 0x644 */ uint8_t targetStack;
    /* 0x645 */ uint8_t pad645[3];
    /* 0x648 */ int savedMode[3];
    /* 0x654 */ float savedTargetFocusDistance[3];
    /* 0x660 */ cdc::Euler savedFocusRotation;
    /* 0x670 */ float savedFocusDistance;
    /* 0x674 */ float signalFocusDistance;
    /* 0x678 */ uint8_t pad678[8];
    /* 0x680 */ SavedCinematic savedCinematic[3];
    /* 0xd10 */ cdc::Vector3 signalPos;
    /* 0xd20 */ cdc::Vector3 signalRot;
    /* 0xd30 */ int lookThrowMode;
    /* 0xd34 */ int centerTimer;
    /* 0xd38 */ uint8_t forced_movement;
    /* 0xd39 */ uint8_t padd39;
    /* 0xd3a */ uint8_t last_forced_movement;
    /* 0xd3b */ uint8_t padd3b;
    /* 0xd3c */ uint8_t rotDirection;
    /* 0xd3d */ uint8_t padd3d[3];
    /* 0xd40 */ cdc::Euler LookRot;
    /* 0xd50 */ NextCamData nextCamDeferred;
    /* 0xe90 */ NextCamData nextCam;
    /* 0xfd0 */ int8_t deferredNextCamMode;
    /* 0xfd1 */ int8_t restoreToSplineFlag;
    /* 0xfd2 */ int8_t lookaroundDisabled;
    /* 0xfd3 */ int8_t cutFlag;
    /* 0xfd4 */ float cutAngle;
    /* 0xfd8 */ uint8_t padfd8[8];
    /* 0xfe0 */ cdc::Euler splinecam_helprot;
    /* 0xff0 */ CameraDataUnion data;
    /* 0x1150 */ float teleportZRot;
    /* 0x1154 */ float extraZRot;
    /* 0x1158 */ float extraXRot;
    /* 0x115c */ uint8_t leftTimer;
    /* 0x115d */ uint8_t pad115d;
    /* 0x115e */ uint8_t rightTimer;
    /* 0x115f */ uint8_t pad115f;
    /* 0x1160 */ uint8_t collision_lastPush;
    /* 0x1161 */ uint8_t pad1161;
    /* 0x1162 */ uint8_t collision_timer;
    /* 0x1163 */ uint8_t pad1163;
    /* 0x1164 */ unsigned int prev_instance_mode;
    /* 0x1168 */ unsigned int instance_mode;
    /* 0x116c */ float instance_vel;
    /* 0x1170 */ float instance_xyvel;
    /* 0x1174 */ float instance_prev_xyvel;
    /* 0x1178 */ float instance_zvel;
    /* 0x117c */ float instanceXYVelCameraDot;
    /* 0x1180 */ cdc::Vector3 real_focuspoint;
    /* 0x1190 */ uint8_t always_rotate_flag;
    /* 0x1191 */ uint8_t pad1191;
    /* 0x1192 */ uint8_t lock_control_angle;
    /* 0x1193 */ uint8_t pad1193;
    /* 0x1194 */ float actual_vel_x;
    /* 0x1198 */ float actual_acc_x;
    /* 0x119c */ float focuspoint_fallz;
    /* 0x11a0 */ float leadAmount;
    /* 0x11a4 */ float lead_angle;
    /* 0x11a8 */ float lead_vel;
    /* 0x11ac */ float lead_accl;
    /* 0x11b0 */ float lead_timer;
    /* 0x11b4 */ uint8_t pad11b4[12];
    /* 0x11c0 */ cdc::Vector3 camera_plane;
    /* 0x11d0 */ cdc::Vector3 camera_plane_vert;
    /* 0x11e0 */ cdc::Vector3 left_point;
    /* 0x11f0 */ cdc::Vector3 right_point;
    /* 0x1200 */ cdc::Vector3 left_point2;
    /* 0x1210 */ cdc::Vector3 right_point2;
    /* 0x1220 */ cdc::Vector3 bottom_point;
    /* 0x1230 */ cdc::Vector3 bottom_point2;
    /* 0x1240 */ cdc::Vector3 top_point;
    /* 0x1250 */ cdc::Vector3 top_point2;
    /* 0x1260 */ float ACE_amount;
    /* 0x1264 */ uint8_t shorten_count;
    /* 0x1265 */ uint8_t pad1265;
    /* 0x1266 */ int8_t shorten_flag;
    /* 0x1267 */ int8_t camera_still;
    /* 0x1268 */ int8_t globalCameraMode;
    /* 0x1269 */ int8_t alternateCameraFlag;
    /* 0x126a */ uint8_t pad126a[2];
    /* 0x126c */ float Camera_lookDist;
    /* 0x1270 */ float combat_cam_distance;
    /* 0x1274 */ float combat_cam_angle;
    /* 0x1278 */ float combat_cam_weight;
    /* 0x127c */ int combat_cam_debounce;
    /* 0x1280 */ float newCombatCamDist;
    /* 0x1284 */ float newCombatCamSpeed;
    /* 0x1288 */ uint16_t newLookMaxRot;
    /* 0x128a */ uint16_t newLookMaxTilt;
    /* 0x128c */ uint16_t newLookMaxSpeed;
    /* 0x128e */ uint16_t newLookMaxAccl;
    /* 0x1290 */ float CameraCenterDelay;
    /* 0x1294 */ float CenterFlag;
    /* 0x1298 */ float CenterVel;
    /* 0x129c */ float CenterAccl;
    /* 0x12a0 */ float markupCenter;
    /* 0x12a4 */ float forcedRotation;
    /* 0x12a8 */ uint16_t LockBehindMode;
    /* 0x12aa */ uint16_t LastLockBehindMode;
    /* 0x12ac */ float roll_target;
    /* 0x12b0 */ float current_roll_amount;
    /* 0x12b4 */ float roll_inc;
    /* 0x12b8 */ uint8_t pad12b8[8];
    /* 0x12c0 */ cdc::Vector3 ThrowLookPoint;
    /* 0x12d0 */ cdc::Vector3 lookaccl;
    /* 0x12e0 */ cdc::Vector3 lookvel;
    /* 0x12f0 */ int matrixmode;
    /* 0x12f4 */ int splineCamLead;
    /* 0x12f8 */ uint8_t pad12f8[8];
    /* 0x1300 */ cdc::Vector3 lastCombatPos;
    /* 0x1310 */ float targetTimeMult;
    /* 0x1314 */ uint8_t pad1314[12];
};

#pragma pack(pop)

// Static layout and size assertions
static_assert(sizeof(CameraCore) == 0x300, "CameraCore size mismatch");
static_assert(sizeof(CameraKey) == 0x18, "CameraKey size mismatch");
static_assert(sizeof(FollowTransitionData) == 0x24, "FollowTransitionData size mismatch");
static_assert(sizeof(FollowSplinecamData) == 0x50, "FollowSplinecamData size mismatch");
static_assert(sizeof(SplinecamData) == 0x50, "SplinecamData size mismatch");
static_assert(sizeof(SplinecamParams) == 0x7c, "SplinecamParams size mismatch");
static_assert(sizeof(SavedCinematic) == 0x230, "SavedCinematic size mismatch");
static_assert(sizeof(NextCamData) == 0x140, "NextCamData size mismatch");
static_assert(sizeof(CameraDataUnion) == 0x160, "CameraDataUnion size mismatch");
static_assert(sizeof(Camera) == 0x1320, "Camera size mismatch");

// Subsystem globals
extern Camera theCamera;                                   // 0x010fc5a0 (PDB: 0x010fd7a0)
extern SplineCameraLevelData* emergencyLoadingCamDataPtr;  // 0x010fc590 (PDB: 0x010fd790)
extern uint32_t g_cameraAxisFlags;                         // 0x01075ac8
extern float zOffsetUndo;                                  // 0x010fc578 (PDB: 0x010fd778)

// Camera lifecycle & configuration routines (Original: f:\tr7\game\camera\camera.c)
void CAMERA_Initialize(Camera* camera);                                     // 0x0047c030 (PDB: 0x0047d9d0)
void CAMERA_Lock(Camera* camera, int lockBits);                             // 0x0047c570 (PDB: 0x0047df10)
void CAMERA_Unlock(Camera* camera, int lockBits);                           // 0x0047c590 (PDB: 0x0047df30)
void CAMERA_SetSmoothValue(Camera* camera, int smooth);                     // 0x0047c5b0 (PDB: 0x0047df50)
void CAMERA_Adjust_tilt_stay_in_place(Camera* camera, int tilt);            // 0x0047c610 (PDB: 0x0047dfb0)
void CAMERA_Adjust_tilt(Camera* camera, int tilt);                          // 0x0047c6a0 (PDB: 0x0047e040)
void CAMERA_Adjust_distance(Camera* camera, int dist);                      // 0x0047c720 (PDB: 0x0047e0c0)
void CAMERA_Adjust_ZOffset(Camera* camera, int offset);                     // 0x0047c770 (PDB: 0x0047e110)
void CAMERA_Adjust_rotation(Camera* camera, int rot);                       // 0x0047c780 (PDB: 0x0047e120)
void CAMERA_Adjust_roll(int roll, int duration);                            // 0x0047c820 (PDB: 0x0047e1c0)
void CAMERA_ChangeToUnderWater(Camera* camera, Instance* instance);         // 0x0047c990 (PDB: 0x0047e330)
void CAMERA_ChangeOutOfWade(Camera* camera, Instance* instance);            // 0x0047cab0 (PDB: 0x0047e450)
void CAMERA_ChangeToOutOfWater(Camera* camera, Instance* instance);         // 0x004803e0 (PDB: 0x00481d80)
void CAMERA_SetMode(Camera* camera, int mode);                              // 0x0047b240 (PDB: 0x0047cbe0)
void CAMERA_ClearNextCamData(NextCamData* nextCam);                         // 0x0047b100 (PDB: 0x0047caa0)
void CAMERA_Save(Camera* camera, int saveBits);                             // 0x0047b060 (PDB: 0x0047ca00)
void CAMERA_SaveMode(Camera* camera, int mode);                             // 0x0047acc0 (PDB: 0x0047c660)
void CAMERA_RestoreMode(Camera* camera);                                    // 0x0047ff50 (PDB: 0x004818f0)
void CAMERA_Restore(Camera* camera, int restoreBits);                       // 0x0047dd60 (PDB: 0x0047f700)
int  CAMERA_QueryMode(Camera* camera);                                      // 0x0047de30 (PDB: 0x0047f7d0)
void CAMERA_Relocate(Camera* camera, cdc::Vector3* offset, int param3);     // 0x0047e1e0 (PDB: 0x0047fb80)
void CAMERA_SetShakeNoPadshock(Camera* camera, int shake, int scale);       // 0x0047e0e0 (PDB: 0x0047fa80)
void CAMERA_SetShake(Camera* camera, int shake, int scale);                 // 0x0047e180 (PDB: 0x0047fb20)
void CAMERA_SetShakeRamp(int fadeIn, int sustain, int fadeOut, int scale);  // 0x0047e860 (PDB: 0x00480200)
void CAMERA_SetVertigo(int mode, int param2, int param3);                   // 0x0047d760 (PDB: 0x0047f100)
void CAMERA_SetValueByEvent(Camera* camera, int eventId, int value);        // 0x0047d860 (PDB: 0x0047f200)

// Axis & Aim inversion routines
void CAMERA_SetAxisInversionX(bool inverted);                               // 0x0047dc00 (PDB: 0x0047f5a0)
void CAMERA_SetAxisInversionY(bool inverted);                               // 0x0047dc30 (PDB: 0x0047f5d0)
bool CAMERA_GetAxisInversionX(void);                                        // 0x0047dc80 (PDB: 0x0047f620)
bool CAMERA_GetAxisInversionY(void);                                        // 0x0047dca0 (PDB: 0x0047f640)
void CAMERA_SetAimAxisInversionX(bool inverted);                            // 0x0047dcc0 (PDB: 0x0047f660)
void CAMERA_SetAimAxisInversionY(bool inverted);                            // 0x0047dcf0 (PDB: 0x0047f690)
bool CAMERA_GetAimAxisInversionX(void);                                     // 0x0047dd20 (PDB: 0x0047f6c0)
bool CAMERA_GetAimAxisInversionY(void);                                     // 0x0047dd40 (PDB: 0x0047f6e0)

#endif // GAME_CAMERA_CAMERA_H
