#include "game/camera/camerabehind.h"
#include "game/camera/camera.h"
#include "game/camera/camerafocus.h"
#include "game/camera/camerasupport.h"
#include "game/camera/cameravv.h"
#include "game/globalinfo.h"
#include "game/instance.h"
#include "game/main.h"
#include "game/player/player.h"
#include <algorithm>
#include <cmath>

// External camera, player, input, and math helpers
struct InputSystem {
    float GetAxisValue(int axis); // 0x004e3c50
};
extern InputSystem* INPUT_instances[]; // 0x011015c0

extern float cameraUseTimeMult;  // 0x00f15c78 (PDB: 0x00f15fb0)
extern float camTiltAdjustSpeed; // 0x00f15c88 (PDB: 0x00f15fc0)

extern void         TRANS_SetTransformMatrixf(cdc::Matrix* m);                                      // 0x00402700 (PDB: 0x00402c70)
extern cdc::Vector3* TRANS_RotTransPersVectorf(const cdc::Vector3* vIn, cdc::Vector3* vOut);       // 0x00402710 (PDB: 0x00402c80)
extern float        MATH3D_LengthXYZ(const cdc::Vector3* a, const cdc::Vector3* b);                 // 0x00426b00
extern float        CAMERA_DoVertigo(Camera* camera);                                               // 0x0047cad0 (PDB: 0x0047e470)
extern void         CAMERA_UpdateFocusRoll(Camera* camera);                                         // 0x0047fe90 (PDB: 0x00481830)
extern int          CAMERA_DoCameraCollision(Camera* camera, cdc::Vector3* startPos, int param3);   // 0x004857b0 (PDB: 0x004871e0)
extern void         CAMERA_Unlock_Control_Angle(Camera* camera);                                    // 0x004882a0 (PDB: 0x00489ce0)
extern void         CAMERA_SplineProcess(Camera* camera);                                           // 0x004895a0 (PDB: 0x0048afe0)
extern void         CAMERA_CalcFollowSplinecamPos(Camera* camera, cdc::Vector3* outPos, const cdc::Vector3* focusPos); // 0x00489ff0 (PDB: 0x0048bab0)
extern void         CameraStackMain(void);                                                          // 0x0048c6a0 (PDB: 0x0048e1b0)
extern unsigned int CHUCKABLE_Query(Instance* instance, unsigned int query);                        // 0x00527910 (PDB: 0x00529500)
extern void*        GetGrapplingHookStateTargetInstance(void* extraData);                           // 0x0055a1f0 (PDB: 0x0055c340)
extern bool         PLAYER_IsInWeaponCombatMode(void);                                              // 0x005a51a0 (PDB: 0x005a72f0)
extern void         PLAYER_GetAttachedToVelocity(cdc::Vector3* outVel, const BaseInstance* inst);   // 0x005a9fe0 (PDB: 0x005ac8e0)

// Globals defined in cameraBehind.c
float hybridSplineDist = 0.0f;      // 0x010fd8e4 (PDB: 0x010feae4)
int gBehindCamOverride = 0;         // 0x010fd8e8 (PDB: 0x010feae8)
int cameraCloseLimitingTilt = 0;    // 0x010fd8ec (PDB: 0x010feaec)
float splineLookX = 0.0f;           // 0x010fda7c (PDB: 0x010fec7c)
float splineLookZ = 0.0f;           // 0x010fda80 (PDB: 0x010fec80)

static float recentZTilt[5] = {};       // 0x010fd8d0
static int recentZPosCnt = 0;           // 0x010fd8f0
static int cameraVehicleLock = 0;       // 0x010fd8f4
static float CombatTilt = 0.0f;         // 0x010fd8fc
static cdc::Vector3 recentZPos[5] = {}; // 0x010fd900
static float zmatch_vel = 0.0f;         // 0x010fd950
static float zmatch_acc = 0.0f;         // 0x010fd954
static float lead_value = 0.0f;         // 0x010fd958

namespace {

constexpr float CB_PI = 3.1415927410125732f;
constexpr float CB_TWO_PI = 6.2831854820251465f;
constexpr float CB_HALF_PI = 1.5707963705062866f;
constexpr float CB_DEG_TO_RAD = 0.01745329238474369f;
constexpr float CB_RAD_TO_NEG_DEG = -57.2957763671875f;
constexpr float CB_RAD_TO_ANGLE_UNITS = 651.89862060546875f; // 4096.0f / (2 * pi)

float WrapSignedPi(float angle)
{
    float wrapped = CAMERA_CircleAnglef(angle);
    if (wrapped > CB_PI) {
        wrapped -= CB_TWO_PI;
    }
    return wrapped;
}

// 0x004125e0 (PDB: 0x004119b0)
float VecLen3(const cdc::Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// 0x00427610
float VecLen2(const cdc::Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

// 0x005dcea0 (PDB: 0x005df8d0)
bool VecIsZero3(const cdc::Vector3& v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

// 0x005deb50 (PDB: 0x005e1580)
void VecNormalize2(cdc::Vector3& v)
{
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len > 0.0f) {
        float invLen = 1.0f / len;
        v.x *= invLen;
        v.y *= invLen;
    }
}

// 0x005dd7d0 (PDB: 0x005e0200)
cdc::Vector3 Mul3x3(const cdc::Matrix& m, const cdc::Vector3& v)
{
    cdc::Vector3 out;
    out.x = m.col0.x * v.x + m.col1.x * v.y + m.col2.x * v.z;
    out.y = m.col0.y * v.x + m.col1.y * v.y + m.col2.y * v.z;
    out.z = m.col0.z * v.x + m.col1.z * v.y + m.col2.z * v.z;
    out.w = m.col0.w * v.x + m.col1.w * v.y + m.col2.w * v.z;
    return out;
}

// 0x005decb0 (PDB: 0x005e1c40) & 0x005dd2a0 (PDB: 0x005dfcd0)
// Canonicalize an Euler's Z rotation through XYZ matrix round-trip.
float CanonicalizeEulerZ(const cdc::Euler& rot)
{
    float cx = std::cos(rot.x);
    float sx = std::sin(rot.x);
    float cy = std::cos(rot.y);
    float sy = std::sin(rot.y);
    float cz = std::cos(rot.z);
    float sz = std::sin(rot.z);

    // R = Rz * Ry * Rx column entries needed to extract Z angle
    float m00 = cz * cy;
    float m10 = sz * cy;
    float m01 = cz * sy * sx - sz * cx;
    float m11 = sz * sy * sx + cz * cx;
    if (cy > 0.001f || cy < -0.001f) {
        return std::atan2(m10, m00);
    }
    return std::atan2(-m01, m11);
}

cdc::Vector3 SubVec3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    cdc::Vector3 out;
    out.x = a.x - b.x;
    out.y = a.y - b.y;
    out.z = a.z - b.z;
    out.w = a.w - b.w;
    return out;
}

void ClampHybridSplineDist(const Camera* camera)
{
    if (hybridSplineDist != 0.0f) {
        if (camera->followMinimumDistance != 0.0f && hybridSplineDist < camera->followMinimumDistance) {
            hybridSplineDist = camera->followMinimumDistance;
        }
        if (camera->followMaximumDistance != 0.0f && hybridSplineDist > camera->followMaximumDistance) {
            hybridSplineDist = camera->followMaximumDistance;
        }
    }
}

uint8_t SetSplineInUseLower(uint8_t val, uint8_t lowBits)
{
    return static_cast<uint8_t>((val & 0xf0u) | (lowBits & 0x0fu));
}

} // namespace

// 0x00480470 (PDB: 0x00481e10)
void CAMERA_CalcAutoCenterData(Camera* camera, float* outMult, float* outSpeed)
{
    CameraDefaults* defs = globalInfo->cameraDefaults;
    float closeDeadStart = defs->close_distance_dead;
    float closeDeadEnd = defs->close_min_distance_dead;
    float backMult = defs->dead_to_dead_mult;

    *outMult = camera->followAutoCenterMultiplier;
    *outSpeed = camera->followAutoCenterMaxSpeed;

    float cosFrontMin = std::cos(defs->toward_dead_zone_start * CB_DEG_TO_RAD);
    float cosFrontMax = std::cos(defs->toward_dead_zone_end * CB_DEG_TO_RAD);
    float negCosSideMin = -std::cos(defs->away_dead_zone_start * CB_DEG_TO_RAD);
    float negCosSideMax = -std::cos(defs->away_dead_zone_end * CB_DEG_TO_RAD);
    float negCosBackMax = -std::cos(defs->dead_to_dead_start * CB_DEG_TO_RAD);

    float dot = camera->instanceXYVelCameraDot;
    if (dot > cosFrontMin) {
        *outMult = 0.0f;
        return;
    }

    if (dot > cosFrontMax) {
        float t = (dot - cosFrontMax) / (cosFrontMin - cosFrontMax);
        *outMult *= (1.0f - t) * (backMult + 1.0f);
    } else if (dot < negCosSideMin) {
        *outMult = 0.0f;
        return;
    } else if (dot < negCosSideMax) {
        float t = (negCosSideMax - dot) / (negCosSideMax - negCosSideMin);
        *outMult *= (1.0f - t);
    } else if (dot > negCosBackMax) {
        float t = (dot - negCosBackMax) / (cosFrontMax - negCosBackMax);
        *outMult *= (t * backMult + 1.0f);
    }

    float dist = camera->focusDistance;
    if (dist < closeDeadStart) {
        if (dist < closeDeadEnd) {
            *outMult = 0.0f;
            return;
        }
        *outMult *= (dist - closeDeadEnd) / (closeDeadStart - closeDeadEnd);
    }
}

// 0x00480610 (PDB: 0x00481fb0)
static void CAMERA_CalcFollowPosition(Camera* camera, cdc::Euler* rot)
{
    cdc::Vector3 focus = camera->focusPoint;
    float dist = CAMERA_DoVertigo(camera) * camera->focusDistance;

    CAMERA_CalcPosition(&camera->targetPos, &focus, rot, dist);
    camera->core.position = camera->targetPos;
    camera->core.Rotation = *rot;

    float timeMult = static_cast<float>(INSTANCE_Query(camera->focusInstance, 0x65));
    float focusDist = camera->focusDistance;
    float invFocusDist = 1.0f / focusDist;
    float horizBias = camera->followHorizontalBias;

    float desiredTilt = 0.0f;
    float desiredHBias = 0.0f;

    float hDiv = (timeMult * (1.0f / 384.0f) * invFocusDist) * 512.0f;
    float invScale = invFocusDist * 128.0f;
    float timeScaled43 = timeMult * (4.0f / 3.0f);

    float vBias = camera->followVerticalBias;
    if (camera->mode == 0 || camera->mode == 2) {
        vBias += camera->followShapeVerticalBiasOffset;
    }

    CameraDefaults* defs = globalInfo->cameraDefaults;
    if ((camera->instance_mode & 0x180) != 0 && (camera->instance_mode & 0x400000) == 0) {
        vBias = defs->combat_camera_vbias * 0.001f;
    }

    if (focusDist < timeScaled43) {
        float zAdj = defs->zoomed_in_focus_height;
        if ((camera->instance_mode & 0x2000) != 0) {
            zAdj += 0.08f;
        }
        zAdj -= defs->focus_height;
        camera->core.position.z += zAdj * (timeMult / hDiv) * (hDiv - 1.0f);
    } else {
        float vOffset = (timeMult / hDiv) * vBias;
        float hOffset = (128.0f / invScale) * horizBias;
        if (focusDist < 2000.0f) {
            float t = (focusDist - timeScaled43) / (2000.0f - timeScaled43);
            hOffset *= t;
            vOffset *= t;
        }
        desiredTilt = std::atan2(vOffset, focusDist);
        desiredHBias = std::atan2(hOffset, focusDist);
    }

    if (camera->cutFlag != 0 || (camera->flags & 0x1800) != 0) {
        camera->followDampedVerticalBias = desiredTilt;
        camera->followVBiasVel = 0.0f;
        camera->followVBiasAcc = 0.0f;
        camera->followDampedHorizontalBiasRotation = desiredHBias;
        camera->followHBiasVel = 0.0f;
        camera->followHBiasAcc = 0.0f;
    }

    if (camera->followCameraTimedTransition != 0) {
        float startTilt = camera->followTransitionData.startVerticalBias;
        camera->followDampedVerticalBias =
            CAMERA_SignedAngleDifference(desiredTilt, startTilt) * camera->followTransitionData.transition + startTilt;
    } else {
        CriticalDampAngle(
            9,
            &camera->followDampedVerticalBias,
            desiredTilt,
            &camera->followVBiasVel,
            &camera->followVBiasAcc,
            0.003067961661145091f); // pi / 1024
    }

    rot->x += camera->followDampedVerticalBias;

    if (camera->lock_control_angle == 0) {
        camera->lagZ = rot->z;
    } else {
        Decouple_AngleMoveToward(&camera->lagZ, rot->z, 0.012271846644580364f); // pi / 256
        if (CAMERA_AngleDifference(camera->lagZ, rot->z) <= 0.012271846644580364f ||
            (camera->instance_xyvel < 12.0f && (camera->instance_mode & 0x400) == 0)) {
            CAMERA_Unlock_Control_Angle(camera);
        }
    }

    if (camera->followCameraTimedTransition != 0) {
        float startHBias = camera->followTransitionData.startHorizontalBias;
        camera->followDampedHorizontalBiasRotation =
            CAMERA_SignedAngleDifference(desiredHBias, startHBias) * camera->followTransitionData.transition + startHBias;
    } else {
        CriticalDampAngle(
            9,
            &camera->followDampedHorizontalBiasRotation,
            desiredHBias,
            &camera->followHBiasVel,
            &camera->followHBiasAcc,
            0.003067961661145091f);
    }

    rot->z += camera->followDampedHorizontalBiasRotation;

    float slZ = splineLookZ;
    float slX = splineLookX;
    float curDist = camera->focusDistance;
    if (curDist < 300.0f) {
        if (curDist < 150.0f) {
            slX = 0.0f;
            slZ = 0.0f;
        } else {
            float scale = (curDist - 150.0f) * (1.0f / 150.0f);
            slZ *= scale;
            slX *= scale;
        }
    }

    rot->x += slX;
    rot->z += slZ;

    if ((camera->instance_mode & 0x4000000) != 0 && camera->followSplineData.lockInPlace != 0) {
        camera->core.position = camera->followSplineData.sourceStartPoint;
    }
}

// 0x00480a40 (PDB: 0x004823e0)
static void CAMERA_UpdateFocusDistance(Camera* camera)
{
    float targetDist = camera->targetFocusDistance;
    if (camera->collisionTargetFocusDistance < targetDist) {
        targetDist = camera->collisionTargetFocusDistance;
    }

    if (camera->cutFlag != 0) {
        camera->focusDistance = targetDist;
        return;
    }

    float dampDiv;
    float maxVel;
    if (camera->collisionTargetFocusDistance < camera->focusDistance) {
        dampDiv = 512.0f;
        maxVel = 2.0f;
    } else if (camera->followSplineData.splineInUse != 0 && hybridSplineDist != 0.0f) {
        dampDiv = 256.0f;
        maxVel = 2.0f;
    } else if (camera->distanceState == 1) {
        if (camera->followCameraLookaroundDisabled != 0 || camera->followCameraTimedTransition != 0) {
            dampDiv = 256.0f;
            maxVel = 2.0f;
        } else {
            dampDiv = 4096.0f / (camera->smooth * 0.125f);
            maxVel = 8.0f;
        }
    } else {
        dampDiv = 12.0f;
        maxVel = 8.0f;
    }

    float oldDist = camera->focusDistance;
    CriticalDampValueControl(
        &camera->focusDistance,
        targetDist,
        &camera->focusDistanceVel,
        &camera->focusDistanceAccl,
        maxVel,
        dampDiv);

    if (camera->focusDistance < targetDist && oldDist > targetDist) {
        camera->focusDistance = targetDist;
    }
}

// 0x00480b80 (PDB: 0x00482520)
static void CAMERA_UpdateFocusTilt(Camera* camera)
{
    if (camera->cutFlag != 0 || (camera->flags & 0x1800) != 0) {
        camera->focusRotVel.x = 0.0f;
        camera->focusRotAccl.x = 0.0f;
        camera->FocusRotation.x = camera->tfaceTilt;
        return;
    }

    if (camera->forced_movement == 3 && (camera->instance_mode & 0x10000) == 0) {
        camera->FocusRotation.x = camera->TargetFocusRotation.x;
        return;
    }

    int smoothType = 3;
    float maxVel;
    if ((camera->followSplineData.splineInUse != 0 && (camera->instance_mode & 0x80) == 0 &&
         camera->followSplineData.splineAdjustTilt != 0) ||
        (camera->instance_mode & 0x10000) != 0 ||
        (camera->tiltState == 1 && camera->followCameraTimedTransition != 0)) {
        smoothType = 7;
        maxVel = 0.09817477315664291f; // pi / 32
    } else if (camera->tiltState == 1) {
        maxVel = camera->smooth * -4096.0f;
    } else {
        maxVel = 0.04908738657832146f; // pi / 64
    }

    CriticalDampAngle(
        smoothType,
        &camera->FocusRotation.x,
        camera->tfaceTilt,
        &camera->focusRotVel.x,
        &camera->focusRotAccl.x,
        maxVel);
}

// 0x00480c70 (PDB: 0x00482610)
static void CAMERA_UpdateFocusRotate(Camera* camera)
{
    int smoothType = 3;

    if (camera->forced_movement == 1) {
        CriticalDampAngleControl(
            &camera->FocusRotation.z,
            camera->CollisionTargetFocusRotation.z,
            &camera->focusRotVel.z,
            &camera->focusRotAccl.z,
            0.24543693661689758f, // 5 * pi / 64
            2.0f);
        return;
    }

    float maxVel;
    if (camera->always_rotate_flag == 0 && camera->rotState != 1 && (camera->flags & 0x10000) != 0) {
        maxVel = 0.19634954631328583f; // pi / 16
    } else if (camera->rotState == 1 && camera->followDragToRotateSpeed != 0.0f) {
        smoothType = 7;
        maxVel = 0.24543693661689758f;
    } else if (camera->always_rotate_flag == 0 && camera->rotState != 1 && (camera->flags & 0x10000) == 0) {
        smoothType = 7;
        maxVel = 0.24543693661689758f;
    } else if (((camera->followCameraTimedTransition != 0 && camera->rotState == 1) ||
                camera->followSplineData.splineInUse != 0) &&
               (camera->instance_mode & 0x80) == 0) {
        smoothType = 7;
        maxVel = 0.24543693661689758f;
    } else {
        maxVel = camera->smooth * -4096.0f;
    }

    CriticalDampAngle(
        smoothType,
        &camera->FocusRotation.z,
        camera->CollisionTargetFocusRotation.z,
        &camera->focusRotVel.z,
        &camera->focusRotAccl.z,
        maxVel);

    if (camera->always_rotate_flag != 0 &&
        CAMERA_AngleDifference(camera->TargetFocusRotation.z, camera->FocusRotation.z) < 0.006135923322290182f) {
        camera->always_rotate_flag = 0;
    }
}

// 0x00480dc0 (PDB: 0x00482760)
float CameraGetShapeValue(shapeValueParams* params)
{
    float x = params->xrot;
    if (x >= params->high_stop_degree) {
        return params->high_stop_val;
    }
    if (x > params->high_degree) {
        return ((x - params->high_degree) / (params->high_stop_degree - params->high_degree)) *
                   (params->high_stop_val - params->high_val) +
               params->high_val;
    }
    if (x > params->med_degree) {
        return ((x - params->med_degree) / (params->high_degree - params->med_degree)) *
                   (params->high_val - params->med_val) +
               params->med_val;
    }
    if (x > params->middle_degree) {
        return ((x - params->middle_degree) / (params->med_degree - params->middle_degree)) *
                   (params->med_val - params->zero_val) +
               params->zero_val;
    }
    if (x > params->low_degree) {
        return ((x - params->low_degree) / (params->middle_degree - params->low_degree)) *
                   (params->zero_val - params->low_val) +
               params->low_val;
    }
    if (x >= params->low_stop_degree) {
        return ((x - params->low_stop_degree) / (params->low_degree - params->low_stop_degree)) *
                   (params->low_val - params->low_stop_val) +
               params->low_stop_val;
    }
    return params->low_stop_val;
}

// 0x00480e90 (PDB: 0x00482830)
void CAMERA_ResetFollowTransition(Camera* camera)
{
    // SecuROM stealth at 0x00480e94 restored: edx = camera->FocusRotation.z
    camera->followTransitionData.transition = 0.0f;
    camera->followTransitionData.transitionLinear = 0.0f;
    camera->followTransitionData.transitionVel = 0.0f;
    camera->followTransitionData.transitionAccl = 0.0f;
    camera->followTransitionData.startDistance = camera->targetFocusDistance;
    camera->followTransitionData.startTilt = camera->FocusRotation.x;
    camera->followTransitionData.startRotation = camera->FocusRotation.z;
    camera->followTransitionData.startVerticalBias = camera->followDampedVerticalBias;
    camera->followTransitionData.startHorizontalBias = camera->followDampedHorizontalBiasRotation;
}

// 0x00480ef0 (PDB: 0x00482890)
static cdc::Vector3 CAMERA_CalcFocusOffsetForSwim(Camera* camera)
{
    return Mul3x3(camera->focusInstance->matrix[1], camera->focusOffset);
}

// 0x00480f50 (PDB: 0x004828f0)
static void CAMERA_FollowPlayerSwimTilt(Camera* camera)
{
    float dampDiv = 16.0f;
    float maxVel = 0.024543693706393242f; // pi / 128

    if ((camera->instance_mode & 0x8000) == 0) {
        cdc::Vector3 swimOff = CAMERA_CalcFocusOffsetForSwim(camera);
        (void)swimOff.z;

        float angle = -CB_HALF_PI - gPlayer->extraRot.x;
        if (angle > CB_HALF_PI) {
            angle = -CB_PI;
        } else if (angle > 0.0f) {
            angle = 0.0f;
        } else if (angle < -CB_PI) {
            angle = -CB_PI;
        }

        float tilt = angle + CB_HALF_PI - 0.19634954631328583f; // + pi/2 - pi/16
        camera->targetTilt = tilt;

        float waterLevel = gPlayer->currentWaterZ - 32.0f;
        if (camera->core.position.z > waterLevel) {
            dampDiv = 2.0f;
            maxVel = 0.09817477315664291f; // pi / 32
        }

        float dz = camera->focusPoint.z - waterLevel;
        if (std::fabs(dz) < camera->focusDistance) {
            float absDz = std::fabs(dz);
            float distSqMinusDzSq = camera->focusDistance * camera->focusDistance - absDz * absDz;
            float surfTilt = std::atan2(dz, std::sqrt(distSqMinusDzSq));
            if (tilt < surfTilt) {
                camera->targetTilt = surfTilt;
            }
        }
    }

    if (camera->cutFlag != 0) {
        camera->tilt = camera->targetTilt;
        return;
    }

    CriticalDampAngleControl(
        &camera->tilt,
        camera->targetTilt,
        &camera->tiltVel,
        &camera->tiltAccl,
        maxVel,
        dampDiv);
}

// 0x00481100 (PDB: 0x00482aa0)
static float CAMERA_DesiredCombatCamAngle(Camera* camera, const cdc::Vector3* targetPos)
{
    cdc::Vector3 delta = SubVec3(*targetPos, camera->focusInstance->position);
    float baseAngle = -std::atan2(delta.x, delta.y);
    (void)CAMERA_SignedAngleDifference(baseAngle, camera->FocusRotation.z);

    CameraDefaults* defs = globalInfo->cameraDefaults;
    float angle = baseAngle + defs->combat_camera_rotation_offset * CB_DEG_TO_RAD;
    if ((camera->instance_mode & 0x3000000) != 0) {
        angle += defs->combat_camera_melee_rotation_offset * CB_DEG_TO_RAD;
    }
    return angle;
}

// 0x00481180 (PDB: 0x00482b20)
static void CAMERA_FollowGoBehindPlayer(Camera* camera, float step)
{
    Instance* focusInst = camera->focusInstance;

    if (cameraVehicleLock != 0) {
        if (focusInst->rotation.x > -0.18849556148052216f && focusInst->rotation.x < 0.18849556148052216f &&
            focusInst->rotation.y > -0.12566371262073517f && focusInst->rotation.y < 0.12566371262073517f) {
            cameraVehicleLock = 0;
        }
        return;
    }

    float desiredRotZ;
    if ((camera->instance_mode & 0x8000000) != 0 && !PLAYER_IsInWeaponCombatMode()) {
        if (focusInst->rotation.x >= -CB_HALF_PI && focusInst->rotation.x <= CB_HALF_PI &&
            focusInst->rotation.y >= -CB_HALF_PI && focusInst->rotation.y <= CB_HALF_PI &&
            focusInst->LinkParent != nullptr) {
            desiredRotZ = CAMERA_CircleAnglef(focusInst->rotation.z + CB_PI);
        } else {
            cameraVehicleLock = 1;
            desiredRotZ = camera->CollisionTargetFocusRotation.z;
        }
    } else if ((camera->instance_mode & 0x80) != 0) {
        Instance* victim = playerGetVictim();
        if ((camera->instance_mode & 0x400000) != 0 && victim == nullptr) {
            void* grappleTarget = GetGrapplingHookStateTargetInstance(s_playerInstance->extraData);
            if (grappleTarget != nullptr) {
                victim = reinterpret_cast<Instance*>(
                    CHUCKABLE_Query(reinterpret_cast<Instance*>(grappleTarget), 0xd7));
            }
        }

        CameraDefaults* defs = globalInfo->cameraDefaults;
        float combatStep = defs->combat_camera_rotspeed * CB_DEG_TO_RAD;
        float combatDamp = defs->combat_camera_rotdamp;
        if (victim == nullptr) {
            return;
        }

        float dist = MATH3D_LengthXYZ(&focusInst->position, &gPlayer->LastTargetPos);
        if ((camera->instance_mode & 0x1000000) != 0) {
            desiredRotZ = CAMERA_DesiredCombatCamAngle(camera, &gPlayer->LastTargetPos);
            combatStep += combatStep;
        } else if (dist > 300.0f) {
            if (dist > 1400.0f) {
                desiredRotZ = CAMERA_DesiredCombatCamAngle(camera, &gPlayer->LastTargetPos);
            } else {
                float rawDesired = CAMERA_DesiredCombatCamAngle(camera, &gPlayer->LastTargetPos);
                float signedDiff = CAMERA_SignedAngleDifference(rawDesired, camera->FocusRotation.z);
                desiredRotZ = signedDiff * (dist - 300.0f) * (1.0f / 1100.0f) + camera->FocusRotation.z;
            }
        } else {
            desiredRotZ = camera->FocusRotation.z;
            camera->CollisionTargetFocusRotation.z = camera->FocusRotation.z;
        }

        CriticalDampAngleControl(
            &camera->CollisionTargetFocusRotation.z,
            CAMERA_CircleAnglef(desiredRotZ),
            &zmatch_vel,
            &zmatch_acc,
            combatStep,
            combatDamp);
        camera->TargetFocusRotation.z = camera->CollisionTargetFocusRotation.z;
        return;
    } else {
        float canonZ = CanonicalizeEulerZ(focusInst->rotation);
        desiredRotZ = CAMERA_CircleAnglef(canonZ + CB_PI);
    }

    if (camera->cutFlag != 0) {
        camera->TargetFocusRotation.z = CAMERA_CircleAnglef(desiredRotZ);
        return;
    }

    if (camera->forced_movement != 0) {
        return;
    }

    float dampDiv;
    float rotStep;
    if ((camera->instance_mode & 0x8000000) != 0) {
        dampDiv = 4.0f;
        rotStep = 0.09817477315664291f; // pi / 32
    } else if ((camera->flags & 0x10000) != 0) {
        dampDiv = 2.0f;
        rotStep = 0.19634954631328583f; // pi / 16
    } else {
        dampDiv = 9.0f;
        rotStep = 0.03681553900241852f; // 3 * pi / 256
    }
    (void)step;

    if (camera->followUserControlTimeout > 0.0f) {
        float autoTimeout = globalInfo->cameraDefaults->rightstick_auto_center_disable_return;
        if (camera->followUserControlTimeout >= autoTimeout) {
            return;
        }
        rotStep *= (autoTimeout - camera->followUserControlTimeout) / autoTimeout;
    }

    if (rotStep > 0.0f) {
        CriticalDampAngleControl(
            &camera->TargetFocusRotation.z,
            desiredRotZ,
            &zmatch_vel,
            &zmatch_acc,
            rotStep,
            dampDiv);
    }
}

// 0x00481550 (PDB: 0x00482ef0)
void CAMERA_OffsetRecentZPos(const cdc::Vector3& offset)
{
    for (int i = 0; i < 5; ++i) {
        recentZPos[i].x += offset.x;
        recentZPos[i].y += offset.y;
        recentZPos[i].z += offset.z;
        recentZPos[i].w += offset.w;
    }
}

// 0x00481580 (PDB: 0x00482f20)
static void CAMERA_UpdateFocusRotationX(Camera* camera, Instance* focusInst)
{
    float savedTargetDist = camera->targetFocusDistance;

    if (camera->cutFlag == 0 && (camera->flags & 0x1800) == 0 && (camera->instance_mode & 0x8000) == 0) {
        int idx = recentZPosCnt;
        int moved = 0;
        if (camera->instance_xyvel > 9.0f) {
            recentZPos[idx] = camera->focusPoint;
            moved = 1;
        }

        int prevIdx = idx - 4;
        if (prevIdx < 0) {
            prevIdx += 5;
        }

        cdc::Vector3 delta = SubVec3(recentZPos[prevIdx], recentZPos[idx]);
        float dz = delta.z;
        float xyDist = VecLen2(delta);

        if ((camera->instance_mode & 0x340040) == 0 && camera->instance_zvel < -100.0f) {
            dz = 0.0f;
        }

        float tilt;
        if (std::fabs(dz) > 1.0f && xyDist > 1.0f) {
            float rawAngle = -std::atan2(dz, xyDist);
            VecNormalize2(delta);
            float camHeading = camera->FocusRotation.z - CB_HALF_PI;
            float proj = std::sin(camHeading) * delta.y + std::cos(camHeading) * delta.x;
            float absDz = std::fabs(dz);
            if (absDz < 50.0f) {
                tilt = absDz * proj * 0.02f * rawAngle;
            } else {
                tilt = proj * rawAngle;
            }
            if (tilt > 0.5235987901687622f) { // pi / 6
                tilt = 0.5235987901687622f;
            }
        } else {
            tilt = 0.0f;
        }

        float minTilt = (camera->instance_zvel < -100.0f) ? -1.3962634801864624f : -0.5235987901687622f;
        if (tilt < minTilt) {
            tilt = minTilt;
        }

        recentZTilt[idx] = tilt;

        int i0 = (idx - 1 < 0) ? idx + 4 : idx - 1;
        int i1 = (i0 - 1 < 0) ? i0 + 4 : i0 - 1;
        int i2 = (i1 - 1 < 0) ? i1 + 4 : i1 - 1;
        int i3 = (i2 - 1 < 0) ? i2 + 4 : i2 - 1;
        camera->targetTilt = (recentZTilt[idx] + recentZTilt[i0] + recentZTilt[i1] + recentZTilt[i2] + recentZTilt[i3]) * 0.2f;

        if (moved != 0) {
            recentZPosCnt = (idx + 1 >= 5) ? 0 : idx + 1;
        }
    } else {
        cdc::Vector3 matPos = *reinterpret_cast<const cdc::Vector3*>(&focusInst->matrix[0].col3);
        for (int i = 0; i < 5; ++i) {
            recentZPos[i] = matPos;
            recentZTilt[i] = 0.0f;
        }
        recentZPosCnt = 0;
        camera->targetTilt = 0.0f;
        camera->tilt = 0.0f;
    }

    if (camera->followSplineData.splineInUse != 0) {
        camera->targetTilt = 0.0f;
    }

    if ((camera->instance_mode & 0x10000) != 0) {
        CAMERA_FollowPlayerSwimTilt(camera);
    } else if ((camera->instance_mode & 0x20000) != 0) {
        camera->targetTilt = 0.0f;
        Instance* engaged = gPlayer->engagedList[23].instance;
        if (engaged != nullptr && engaged->extraData != nullptr) {
            camera->targetTilt = *reinterpret_cast<const float*>(
                reinterpret_cast<const uint8_t*>(engaged->extraData) + 0xf8) * -0.9200000166893005f;
        }
        CriticalDampAngleControl(
            &camera->tilt,
            camera->targetTilt,
            &camera->tiltVel,
            &camera->tiltAccl,
            0.012271846644580364f,
            8.0f);
    } else {
        if (camera->targetTilt > 0.0f) {
            camera->targetTilt *= 0.25f;
        } else if (camera->instance_zvel < -100.0f) {
            camera->targetTilt *= 0.625f;
        }

        if (camera->targetTilt > 0.39269909262657166f) { // pi / 8
            camera->targetTilt = 0.39269909262657166f;
        } else if (camera->instance_zvel < -100.0f) {
            if (camera->targetTilt < -1.535889744758606f) { // -88 deg
                camera->targetTilt = -1.535889744758606f;
            }
        } else if (camera->targetTilt < -0.39269909262657166f) {
            camera->targetTilt = -0.39269909262657166f;
        }

        float distMargin = 2912.0f - savedTargetDist;
        if (distMargin < 512.0f) {
            camera->targetTilt *= distMargin * (1.0f / 512.0f);
        }

        CriticalDampAngle(
            3,
            &camera->tilt,
            camera->targetTilt,
            &camera->tiltVel,
            &camera->tiltAccl,
            0.012271846644580364f);
    }

    if (camera->followSplineData.splineInUse != 0 && camera->followSplineData.splineAdjustTilt != 0) {
        camera->tilt = 0.0f;
    }

    camera->tilt = WrapSignedPi(camera->tilt);

    float lookAxis = INPUT_instances[0]->GetAxisValue(3);

    float minLim;
    float maxLim;
    if ((camera->instance_mode & 0x20000) != 0) {
        minLim = -53.0f;
        maxLim = 45.0f;
    } else {
        maxLim = std::min(camera->followHighTiltAngle, camera->followHighTiltLimit);
        minLim = std::max(camera->followLowTiltLimit, camera->followLowTiltAngle);
        if (lookAxis != 0.0f) {
            minLim -= camera->followLowSpringDegree;
            maxLim += camera->followHighSpringDegree;
        }
    }

    camera->TargetFocusRotation.x = WrapSignedPi(camera->TargetFocusRotation.x);
    float baseTiltRot = WrapSignedPi(camera->TargetFocusRotation.x + camera->tilt);
    float totalRotX = WrapSignedPi(baseTiltRot + camera->extraXRot);

    float highBound = CB_HALF_PI - (minLim + 90.0f) * CB_DEG_TO_RAD;
    float lowBound = (90.0f - maxLim) * CB_DEG_TO_RAD - CB_HALF_PI;
    if (totalRotX > highBound) {
        totalRotX = highBound;
        camera->extraXRot = totalRotX - baseTiltRot;
    } else if (totalRotX < lowBound) {
        totalRotX = lowBound;
        camera->extraXRot = totalRotX - baseTiltRot;
    }

    float xrot = CAMERA_CircleAnglef(totalRotX);
    camera->tfaceTilt = xrot;

    if ((camera->instance_mode & 0x80) != 0) {
        Instance* victim = playerGetVictim();
        bool grappleSplineActive = false;
        if ((camera->instance_mode & 0x400000) != 0 &&
            (std::fabs(splineLookX) > 0.01f || std::fabs(splineLookZ) > 0.01f)) {
            grappleSplineActive = true;
        }

        if (grappleSplineActive || victim == nullptr) {
            camera->tfaceTilt = CombatTilt;
        } else {
            TRANS_SetTransformMatrixf(&camera->core.wcTransformf);
            cdc::Vector3 screenPos;
            TRANS_RotTransPersVectorf(&gPlayer->LastTargetPos, &screenPos);

            float t = (camera->focusDistance - 600.0f) * (1.0f / 1400.0f);
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            float bottomBand = (t * 0.13f + 0.09f) * 448.0f;
            float topBand = (t * 0.13f + 0.09f + 0.05f) * 448.0f;
            float highThresh = 0.78f * 448.0f;
            float highRef = (0.78f - 0.05f) * 448.0f;

            if (screenPos.y > highThresh && screenPos.z > 0.0f) {
                float delta = (screenPos.y - highRef) * (1.0f / 448.0f) * 0.5f * 0.13962635397911072f;
                if (delta > 0.05235988274216652f) {
                    delta = 0.05235988274216652f;
                }
                CombatTilt -= delta;
            } else if (screenPos.y < bottomBand && screenPos.z > 0.0f) {
                float delta = (topBand - screenPos.y) * (1.0f / 448.0f) * 0.5f * 0.13962635397911072f;
                if (delta > 0.05235988274216652f) {
                    delta = 0.05235988274216652f;
                }
                CombatTilt += delta;
            } else {
                float desiredCombatTilt =
                    (camera->followCombatCamTiltOverride != 0.0f)
                        ? camera->followCombatCamTiltOverride
                        : globalInfo->cameraDefaults->combat_camera_tilt;
                desiredCombatTilt *= -CB_DEG_TO_RAD;
                if ((camera->instance_mode & 0x1000000) != 0 && camera->focusDistance > 1500.0f) {
                    desiredCombatTilt = 0.0f;
                }

                float relaxRate = 0.5f * 0.00872664712369442f; // 0.5 * (pi / 360)
                if ((screenPos.y > topBand || screenPos.z <= 0.0f) && CombatTilt > desiredCombatTilt) {
                    CombatTilt -= relaxRate;
                    if (CombatTilt < desiredCombatTilt) {
                        CombatTilt = desiredCombatTilt;
                    }
                }
                if ((screenPos.y < highRef || screenPos.z <= 0.0f) && CombatTilt < desiredCombatTilt) {
                    CombatTilt += relaxRate;
                    if (CombatTilt > desiredCombatTilt) {
                        CombatTilt = desiredCombatTilt;
                    }
                }
            }

            if (CombatTilt > 0.7853981852531433f) {
                CombatTilt = 0.7853981852531433f;
                camera->tfaceTilt = 0.7853981852531433f;
            } else if (CombatTilt < -0.7853981852531433f) {
                CombatTilt = -0.7853981852531433f;
                camera->tfaceTilt = -0.7853981852531433f;
            } else {
                camera->tfaceTilt = CombatTilt;
            }
        }

        camera->extraXRot = 0.0f;
        if ((camera->instance_mode & 0x400000) == 0) {
            float combatDist = (camera->followCombatCamDistOverride != 0.0f)
                                   ? camera->followCombatCamDistOverride
                                   : globalInfo->cameraDefaults->combat_camera_distance;
            if (camera->distanceState != 1) {
                camera->targetFocusDistance = combatDist;
            }
            camera->signalFocusDistance = combatDist;
        }
    } else {
        CombatTilt = WrapSignedPi(xrot);

        float shapeAngle = (camera->followCameraTimedTransition != 0)
                               ? camera->TargetFocusRotation.x
                               : camera->tfaceTilt;
        float shapeDeg = WrapSignedPi(shapeAngle) * CB_RAD_TO_NEG_DEG;

        CameraDefaults* defs = globalInfo->cameraDefaults;
        shapeValueParams params;
        params.xrot = shapeDeg;
        params.high_degree = (camera->followHighTiltAngle == -999.0f) ? 45.0f : camera->followHighTiltAngle;
        params.med_degree = (camera->followMedTiltAngle == -999.0f) ? 22.5f : camera->followMedTiltAngle;
        params.middle_degree = (camera->followZeroTiltAngle == -999.0f) ? 0.0f : camera->followZeroTiltAngle;
        params.low_degree = (camera->followLowTiltAngle == -999.0f) ? -30.0f : camera->followLowTiltAngle;
        params.high_stop_degree = params.high_degree + camera->followHighSpringDegree;
        params.low_stop_degree = params.low_degree - camera->followLowSpringDegree;

        params.high_val = (camera->followHighTiltDistance == -999.0f) ? defs->high_distance : camera->followHighTiltDistance;
        params.med_val = (camera->followMedTiltDistance == -999.0f) ? defs->medium_distance : camera->followMedTiltDistance;
        params.zero_val = (camera->followZeroTiltDistance == -999.0f) ? defs->zero_distance : camera->followZeroTiltDistance;
        params.low_val = (camera->followLowTiltDistance == -999.0f) ? defs->low_distance : camera->followLowTiltDistance;
        params.high_stop_val = params.high_val;
        params.low_stop_val = params.low_val;

        float shapeDist = CameraGetShapeValue(&params);

        params.high_stop_val = defs->high_stop_tilt_adjust_speed;
        params.high_val = defs->high_tilt_adjust_speed;
        params.med_val = defs->med_tilt_adjust_speed;
        params.zero_val = defs->zero_tilt_adjust_speed;
        params.low_val = defs->low_tilt_adjust_speed;
        params.low_stop_val = defs->low_stop_tilt_adjust_speed;
        camTiltAdjustSpeed = CameraGetShapeValue(&params) * CB_DEG_TO_RAD;

        params.high_stop_val = camera->followHighTiltFOV;
        params.high_val = camera->followHighTiltFOV;
        params.med_val = camera->followMedTiltFOV;
        params.zero_val = camera->followZeroTiltFOV;
        params.low_val = camera->followLowTiltFOV;
        params.low_stop_val = camera->followLowTiltFOV;
        float fovProj = (CameraGetShapeValue(&params) + 1.0f) * 512.0f;
        CAMERA_SetNewProjDistance(&camera->core, fovProj, 2.0f);

        params.high_stop_val = camera->followHighTiltZOffset;
        params.high_val = camera->followHighTiltZOffset;
        params.med_val = camera->followMedTiltZOffset;
        params.zero_val = camera->followZeroTiltZOffset;
        params.low_val = camera->followLowTiltZOffset;
        params.low_stop_val = camera->followLowTiltZOffset;
        camera->followShapeZOffsetOffset = CameraGetShapeValue(&params);

        params.high_stop_val = camera->followHighTiltVerticalBias;
        params.high_val = camera->followHighTiltVerticalBias;
        params.med_val = camera->followMedTiltVerticalBias;
        params.zero_val = camera->followZeroTiltVerticalBias;
        params.low_val = camera->followLowTiltVerticalBias;
        params.low_stop_val = camera->followLowTiltVerticalBias;
        camera->followShapeVerticalBiasOffset = CameraGetShapeValue(&params);

        float finalDist;
        if (camera->followSplineData.splineInUse != 0 && hybridSplineDist != 0.0f) {
            float t = camera->followTransitionData.transition;
            if (t == 1.0f) {
                finalDist = hybridSplineDist;
            } else {
                camera->distanceState = 0;
                finalDist = (1.0f - t) * camera->followTransitionData.startDistance + t * hybridSplineDist;
            }
            camera->targetFocusDistance = finalDist;
            camera->signalFocusDistance = finalDist;
        } else {
            finalDist = shapeDist;
            if (camera->followMinimumDistance != 0.0f && finalDist < camera->followMinimumDistance) {
                finalDist = camera->followMinimumDistance;
            }
            if (camera->followMaximumDistance != 0.0f && finalDist > camera->followMaximumDistance) {
                finalDist = camera->followMaximumDistance;
            }
            if (camera->distanceState != 1) {
                camera->targetFocusDistance = finalDist;
            }
            camera->signalFocusDistance = finalDist;
        }
    }

    if (camera->cutFlag != 0) {
        camera->focusDistance = camera->targetFocusDistance;
        camera->collisionTargetFocusDistance = camera->targetFocusDistance;
    }
}

// 0x004824b0 (PDB: 0x00483e50)
void CAMERA_GenericCameraProcess(Camera* camera)
{
    CameraStackMain();

    // SecuROM stealth at 0x004824c4 restored: eax = camera->focusInstance
    Instance* focusInst = camera->focusInstance;
    CAMERA_SetMaxVel(camera);

    if (camera->focusInstance == s_playerInstance) {
        cdc::Vector3 attachedVel = {};
        PLAYER_GetAttachedToVelocity(&attachedVel, nullptr);

        float timeScale = (cameraUseTimeMult > 2.0f) ? 2.0f : cameraUseTimeMult;
        attachedVel.x *= timeScale;
        attachedVel.y *= timeScale;
        attachedVel.z *= timeScale;
        attachedVel.w *= timeScale;

        if (!VecIsZero3(attachedVel)) {
            camera->focusPoint.x += attachedVel.x;
            camera->focusPoint.y += attachedVel.y;
            camera->focusPoint.z += attachedVel.z;
            camera->focusPoint.w += attachedVel.w;

            camera->core.position.x += attachedVel.x;
            camera->core.position.y += attachedVel.y;
            camera->core.position.z += attachedVel.z;
            camera->core.position.w += attachedVel.w;
        }
    }

    CAMERA_SetFocus(camera, &camera->targetFocusPoint);

    // Hybrid follow-spline processing
    if (camera->followSplineData.lockInPlace != 0) {
        if (camera->forced_movement == 1) {
            camera->followSplineData.sourceStartPoint = camera->core.position;
        }
        cdc::Vector3 startPt = camera->followSplineData.sourceStartPoint;
        camera->followSplineData.splineInUse = SetSplineInUseLower(camera->followSplineData.splineInUse, 1);

        if (camera->followSplineData.splineAdjustDist != 0) {
            cdc::Vector3 delta = SubVec3(camera->targetFocusPoint, startPt);
            hybridSplineDist = (camera->followSplineData.splineAdjustTilt != 0) ? VecLen3(delta) : VecLen2(delta);
            if (hybridSplineDist < camera->followMinimumDistance) {
                float scale = camera->followMinimumDistance / hybridSplineDist;
                camera->followSplineData.sourceStartPoint.x = camera->targetFocusPoint.x - delta.x * scale;
                camera->followSplineData.sourceStartPoint.y = camera->targetFocusPoint.y - delta.y * scale;
                camera->followSplineData.sourceStartPoint.z = camera->targetFocusPoint.z - delta.z * scale;
                camera->followSplineData.sourceStartPoint.w = camera->targetFocusPoint.w - delta.w * scale;
            }
            ClampHybridSplineDist(camera);
        } else {
            hybridSplineDist = 0.0f;
        }
    } else if (camera->followSplineData.sourceSpline != nullptr) {
        cdc::Vector3 splinePos;
        cdc::Vector3 targetFocus = camera->targetFocusPoint;
        CAMERA_CalcFollowSplinecamPos(camera, &splinePos, &targetFocus);
        camera->followSplineData.splineInUse = SetSplineInUseLower(camera->followSplineData.splineInUse, 1);

        if (camera->followSplineData.splineAdjustDist != 0) {
            cdc::Vector3 delta = SubVec3(camera->targetFocusPoint, splinePos);
            hybridSplineDist = (camera->followSplineData.splineAdjustTilt != 0) ? VecLen3(delta) : VecLen2(delta);
            ClampHybridSplineDist(camera);
        } else {
            hybridSplineDist = 0.0f;
        }
    } else {
        camera->followSplineData.splineInUse = 0;
        hybridSplineDist = 0.0f;
    }

    // Horizontal lead offset based on ACE_amount
    float dist = std::min(camera->targetFocusDistance, camera->collisionTargetFocusDistance);
    float lead = (camera->leadAmount != 0.0f) ? camera->leadAmount : globalInfo->cameraDefaults->lead_amount;
    if (dist < 1000.0f) {
        float t = (1000.0f - dist) * 0.001f;
        lead = (1.0f - t) * lead + t * 200.0f;
    }

    float ace = camera->ACE_amount;
    float aceTerm = 0.0f;
    if (std::fabs(ace) > 6.0f) {
        aceTerm = (ace >= 0.0f) ? (ace - 6.0f) : (ace + 6.0f);
    }

    float desiredLead = -(aceTerm * lead / (0.5f * 45.0f));
    bool decreasing = std::fabs(desiredLead) < std::fabs(lead_value);
    CriticalDampValueControl(
        &lead_value,
        desiredLead,
        &camera->lead_vel,
        &camera->lead_accl,
        decreasing ? 24.0f : 16.0f,
        decreasing ? 2.0f : 4.0f);

    camera->targetFocusPoint.x -= std::cos(camera->core.Rotation.z) * lead_value;
    camera->targetFocusPoint.y -= std::sin(camera->core.Rotation.z) * lead_value;

    // Rotation / Tilt / Auto-center calculation
    if ((camera->flags & 0x10000) != 0) {
        CAMERA_UpdateFocusRotationX(camera, focusInst);
        CAMERA_FollowPlayerSwimTilt(camera);
        if ((camera->flags & 0x40000) != 0) {
            cdc::Euler lookRot;
            CAMERA_CalcRotation(&lookRot, &camera->targetFocusPoint, &camera->core.position);
            camera->TargetFocusRotation.z =
                CAMERA_SignedAngleDifference(lookRot.z, camera->FocusRotation.z) * 1.1f + camera->FocusRotation.z;
        }
    } else {
        if (camera->followTiltReturnSpeed != 0.0f && (camera->instance_mode & 0x80) == 0) {
            float step = camera->instance_vel * camera->followTiltReturnSpeed * cameraUseTimeMult * 0.04f;
            if (camera->extraXRot > 0.0f) {
                camera->extraXRot -= step;
                if (camera->extraXRot <= 0.0f) {
                    camera->extraXRot = 0.0f;
                }
            } else if (camera->extraXRot < 0.0f) {
                camera->extraXRot += step;
                if (camera->extraXRot >= 0.0f) {
                    camera->extraXRot = 0.0f;
                }
            }
        }

        if (camera->followSplineData.splineInUse != 0 && (camera->instance_mode & 0x180) == 0) {
            CriticalDampValue(
                3,
                &camera->followTransitionData.transition,
                1.0f,
                &camera->followTransitionData.transitionVel,
                &camera->followTransitionData.transitionAccl,
                camera->followSplineData.splineTune);

            if (std::fabs(1.0f - camera->followTransitionData.transition) <= 0.01f) {
                camera->followTransitionData.transition = 1.0f;
            }

            cdc::Euler splineRot;
            CAMERA_CalcRotation(&splineRot, &camera->targetFocusPoint, &camera->followSplineData.targetStartPoint);
            camera->markupCenter = splineRot.z;

            if (camera->followTransitionData.transition == 1.0f) {
                if (camera->forced_movement != 1 && camera->CenterFlag == -1000.0f) {
                    float diff = CAMERA_SignedAngleDifference(splineRot.z, camera->FocusRotation.z);
                    float maxStep = camera->instance_vel * 0.02f;
                    if (maxStep > 1.0f || (camera->instance_mode & 0x4000000) != 0) {
                        maxStep = 1.0f;
                    }
                    float nextZ = camera->FocusRotation.z - diff * maxStep;
                    camera->TargetFocusRotation.z = nextZ;
                    camera->CollisionTargetFocusRotation.z = nextZ;
                }
            } else {
                float diff = CAMERA_SignedAngleDifference(
                    splineRot.z, camera->followTransitionData.startRotation);
                float nextZ = diff * camera->followTransitionData.transition +
                              camera->followTransitionData.startRotation;
                camera->TargetFocusRotation.z = nextZ;
                camera->CollisionTargetFocusRotation.z = nextZ;
            }

            if (camera->followSplineData.splineAdjustTilt != 0) {
                if (camera->followTransitionData.transition == 1.0f) {
                    camera->TargetFocusRotation.x = splineRot.x;
                    if (camera->followSplineData.lockInPlace != 0 && camera->extraXRot != 0.0f) {
                        camera->followSplineData.sourceStartPoint = camera->core.position;
                        camera->extraXRot = 0.0f;
                        camera->always_rotate_flag = 1;
                    }
                } else {
                    float diffX = CAMERA_SignedAngleDifference(
                        splineRot.x, camera->followTransitionData.startTilt);
                    camera->TargetFocusRotation.x = diffX * camera->followTransitionData.transition +
                                                    camera->followTransitionData.startTilt;
                }
            }

            camera->always_rotate_flag = 1;
        } else {
            if (camera->mode == 0 && camera->instance_xyvel > 0.5f &&
                (camera->instance_mode & 0x1a0) == 0 && camera->always_rotate_flag == 0 &&
                camera->rotState != 1 && camera->forced_movement != 1 &&
                g_gameTracker.cheatMode != 1) {
                cdc::Euler moveRot;
                CAMERA_CalcRotation(&moveRot, &camera->targetFocusPoint, &camera->focusPoint);
                float signedDiff = CAMERA_SignedAngleDifference(moveRot.z, camera->FocusRotation.z);

                float mult = 0.0f;
                float speed = 0.0f;
                CAMERA_CalcAutoCenterData(camera, &mult, &speed);

                float step = signedDiff * mult;
                float maxStep = speed * cameraUseTimeMult * CB_DEG_TO_RAD;
                if (step > maxStep) {
                    step = maxStep;
                } else if (step < -maxStep) {
                    step = -maxStep;
                }

                if (camera->followUserControlTimeout > 0.0f) {
                    float autoTimeout = globalInfo->cameraDefaults->rightstick_auto_center_disable_return;
                    if (camera->followUserControlTimeout < autoTimeout) {
                        step *= (autoTimeout - camera->followUserControlTimeout) / autoTimeout;
                    } else {
                        step = 0.0f;
                    }
                }

                float desiredZ;
                if (step != 0.0f && (camera->instance_mode & 0x20002) == 0 &&
                    (camera->instance_mode & 0x44000) != 0 &&
                    camera->always_rotate_flag == 0 && camera->rotState != 1) {
                    float curZ = CAMERA_CircleAnglef(camera->TargetFocusRotation.z);
                    float limCurZ = CAMERA_LimitRotation(camera, curZ);
                    float rawNextZ = step + camera->FocusRotation.z;
                    if (limCurZ == curZ) {
                        desiredZ = CAMERA_LimitRotation(camera, rawNextZ);
                    } else {
                        float limNextZ = CAMERA_LimitRotation(camera, rawNextZ);
                        camera->TargetFocusRotation.z = limNextZ;
                        camera->signalRot.z = limNextZ;
                        camera->rotDirection = static_cast<uint8_t>(static_cast<int>(step * CB_RAD_TO_ANGLE_UNITS));
                        camera->rotState = 1;
                        desiredZ = limNextZ;
                    }
                } else {
                    desiredZ = step + camera->FocusRotation.z;
                }

                camera->TargetFocusRotation.z = desiredZ;
                camera->rotDirection = static_cast<uint8_t>(static_cast<int>(step * CB_RAD_TO_ANGLE_UNITS));
            }
        }

        CAMERA_UpdateFocusRotationX(camera, focusInst);
    }

    if (camera->cutFlag != 0) {
        CAMERA_SetFocus(camera, &camera->targetFocusPoint);
    }

    // Lock-behind / cardinal snap handling
    if (((camera->flags & 0x10000) == 0 || (camera->flags & 0x40000) != 0) &&
        (camera->flags & 0x2000) == 0 && camera->LockBehindMode == 0 &&
        (camera->instance_mode & 0x80) == 0) {
        if ((camera->instance_mode & 0x20) != 0 && (camera->prev_instance_mode & 0x20) == 0) {
            float modAngle = std::fmod(camera->core.Rotation.z, CB_HALF_PI);
            float snapOffset = (modAngle > 0.7853981852531433f) ? (CB_HALF_PI - modAngle) : -modAngle;
            if (std::fabs(snapOffset) > 0.19634954631328583f) { // pi / 16
                camera->TargetFocusRotation.z = CAMERA_CircleAnglef(snapOffset + camera->core.Rotation.z);
            }
        }
    } else {
        CAMERA_FollowGoBehindPlayer(camera, 0.09817477315664291f); // pi / 32
    }

    if (camera->cutFlag != 0) {
        camera->focusDistance = camera->targetFocusDistance;
        camera->FocusRotation.x = camera->TargetFocusRotation.x;
        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        camera->FocusRotation.z = camera->TargetFocusRotation.z;
        camera->focusPoint = camera->targetFocusPoint;
    }

    if ((camera->flags & 0x1800) == 0) {
        float colDist = (camera->focusDistance > camera->signalFocusDistance)
                            ? camera->focusDistance
                            : camera->signalFocusDistance;
        camera->combat_cam_distance = camera->signalFocusDistance;

        cdc::Vector3 tmpPos1;
        CAMERA_CalcPosition(&tmpPos1, &camera->focusPoint, &camera->FocusRotation, colDist);

        cdc::Vector3 realFocus = camera->real_focuspoint;
        realFocus.z += currentZOffset;
        cdc::Vector3 camStart;
        CAMERA_CalcPosition(&camStart, &realFocus, &camera->FocusRotation, colDist);

        if ((camera->instance_mode & 0x4000000) != 0 && camera->followSplineData.lockInPlace != 0) {
            camera->data.Follow.hit = 0;
        } else {
            camera->data.Follow.hit = CAMERA_DoCameraCollision(camera, &camStart, 0);
        }
    }

    CAMERA_UpdateFocusDistance(camera);
    CAMERA_UpdateFocusTilt(camera);
    CAMERA_UpdateFocusRotate(camera);

    if ((camera->flags & 0x1800) == 0) {
        if (camera->cutFlag != 0) {
            camera->focusPoint = camera->targetFocusPoint;
        } else if ((camera->instance_mode & 0x80) != 0) {
            CriticalDampPosition(
                3,
                &camera->focusPoint,
                &camera->targetFocusPoint,
                &camera->focusPointVel,
                &camera->focusPointAccl,
                camera->maxVel);
        } else {
            cdc::Vector3 focusDiff = SubVec3(camera->focusPoint, camera->targetFocusPoint);
            float dampDiv = 9.0f - (VecLen3(focusDiff) / cameraUseTimeMult) * 0.02f;
            if (dampDiv > 8.0f) {
                dampDiv = 8.0f;
            } else if (dampDiv < 4.0f) {
                dampDiv = 4.0f;
            }
            if (camera->focusDistance < 200.0f) {
                dampDiv = 2.0f;
            }
            CriticalDampPositionControl(
                &camera->focusPoint,
                &camera->targetFocusPoint,
                &camera->focusPointVel,
                &camera->focusPointAccl,
                camera->maxVel,
                dampDiv);
        }
    } else {
        camera->focusPoint = camera->targetFocusPoint;
        camera->focusDistance = camera->targetFocusDistance;
        if ((camera->flags & 0x1000) != 0 && (camera->flags & 0x10000) == 0) {
            CAMERA_SetZRotation(camera, camera->teleportZRot);
        }
        cdc::Vector3 zeroVec = {};
        camera->focusPointVel = zeroVec;
        camera->focusPointAccl = zeroVec;
    }

    cdc::Euler finalRot = camera->FocusRotation;
    float focusDist = camera->focusDistance;
    float signedPitch = WrapSignedPi(finalRot.x);
    finalRot.x = signedPitch;

    cameraCloseLimitingTilt = 0;
    if (focusDist < 400.0f) {
        float limitTilt = 0.1745329350233078f; // 10 * pi / 180
        if (focusDist >= 150.0f) {
            float t = (focusDist - 150.0f) * 0.004f; // 1 / 250
            limitTilt = (1.0f - t) * limitTilt + t * signedPitch;
        }
        if (signedPitch > limitTilt) {
            finalRot.x = limitTilt;
            cameraCloseLimitingTilt = 1;
        }
    }

    CAMERA_CalcFollowPosition(camera, &finalRot);
    CAMERA_UpdateFocusRoll(camera);

    if (s_playerInstance != nullptr) {
        int16_t playerUnit = *reinterpret_cast<const int16_t*>(&s_playerInstance->currentStreamUnitID);
        if (g_gameTracker.StreamUnitID == static_cast<int>(playerUnit) && camera->cutFlag != 0) {
            --camera->cutFlag;
        }
    } else if (camera->cutFlag != 0) {
        --camera->cutFlag;
    }

    camera->rotDirection = 0;
    if ((camera->flags & 0x10000) == 0 && gBehindCamOverride == 0 && camera->restoreToSplineFlag != 0) {
        cdc::Vector3 savedPos = camera->core.position;
        cdc::Vector3 savedTarget = camera->targetPos;
        camera->restoreToSplineFlag = 0;
        camera->cutFlag = 1;
        camera->mode = 5;
        CAMERA_RestoreMode(camera);
        camera->followSplineData.targetStartPoint = savedPos;
        camera->focusPoint = savedTarget;
        camera->smooth = 40.0f;
        CAMERA_SplineProcess(camera);
    }
}
