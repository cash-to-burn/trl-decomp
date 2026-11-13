#include "game/camera/cameracollide.h"
#include "game/camera/camerasupport.h"
#include "game/Stream/stream.h"
#include "game/globalinfo.h"
#include "game/instance.h"
#include "game/main.h"
#include "game/player/player.h"
#include <cmath>

namespace {

constexpr float kPi        = 3.14159274f;
constexpr float kTwoPi     = 6.28318548f;
constexpr float kHalfPi    = 1.57079637f;
constexpr float kDegToRad  = 0.0174532924f;
constexpr float kSentinelDist = 32767.0f; // 0x46fffe00

inline cdc::Vector3 MakeVec3(float x, float y, float z, float w = 0.0f)
{
    cdc::Vector3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

inline cdc::Vector3 VecAdd(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return MakeVec3(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline cdc::Vector3 VecSub(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return MakeVec3(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline cdc::Vector3 VecScale(const cdc::Vector3& v, float s)
{
    return MakeVec3(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline float VecDot3(const cdc::Vector3& a, const cdc::Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline bool VecIsZero3(const cdc::Vector3& v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

inline void VecNormalize3(cdc::Vector3& v)
{
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        float invLen = 1.0f / len;
        v.x *= invLen;
        v.y *= invLen;
        v.z *= invLen;
    }
}

void MatrixBuildIdentity(cdc::Matrix& m)
{
    m.col0 = MakeVec3(1.0f, 0.0f, 0.0f, 0.0f);
    m.col1 = MakeVec3(0.0f, 1.0f, 0.0f, 0.0f);
    m.col2 = MakeVec3(0.0f, 0.0f, 1.0f, 0.0f);
    m.col3 = MakeVec3(0.0f, 0.0f, 0.0f, 1.0f);
}

// 0x005dd8d0 (PDB: 0x005e0300)
void MatrixRotX(cdc::Matrix& m, float sinA, float cosA)
{
    cdc::Vector* cols[4] = { &m.col0, &m.col1, &m.col2, &m.col3 };
    for (int i = 0; i < 4; ++i) {
        float y = cols[i]->y;
        float z = cols[i]->z;
        cols[i]->y = y * cosA - z * sinA;
        cols[i]->z = z * cosA + y * sinA;
    }
}

// 0x005dda20 (PDB: 0x005e0450)
void MatrixRotZ(cdc::Matrix& m, float sinA, float cosA)
{
    cdc::Vector* cols[4] = { &m.col0, &m.col1, &m.col2, &m.col3 };
    for (int i = 0; i < 4; ++i) {
        float x = cols[i]->x;
        float y = cols[i]->y;
        cols[i]->x = x * cosA - y * sinA;
        cols[i]->y = y * cosA + x * sinA;
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

} // namespace

// External subsystems used by cameraCollide
struct MultibodySystem {
    void SetDrawLineProbes(bool enable, const cdc::Vector3& color); // 0x00515830
};
extern MultibodySystem* gMultibodySystem; // 0x01116f50 (PDB: 0x0111a100)
extern cdc::Vector3 gLineProbeColor;      // 0x01116f60 (PDB: 0x0111a110)

extern int  MATH3D_IntersectLineAndPlane(cdc::Vector3* intersection, const cdc::Vector3* pos0, const cdc::Vector3* pos1, const cdc::Vector3* planeNormal, float planeD); // 0x0045fba0 (PDB: 0x00461350)
extern void MULTIBODY_LegacyLineProbe(unsigned int flags, PCollideInfo* info, BaseInstance* player); // 0x00556060 (PDB: 0x00558150)

// Globals defined in cameraCollide.cpp
int previousCollision = 0;                // 0x010fd970 (PDB: 0x01100b70)
cdc::Vector3 CameraLine_startPt[5] = {};  // 0x010fd980 (PDB: 0x01100b80)
cdc::Vector3 CameraLine_endPt[5] = {};    // 0x010fd9d0 (PDB: 0x01100bd0)

// Forward declarations for internal helpers
static int   CAMERA_CalcIntersectAngle(const cdc::Vector3* planePt, const cdc::Vector3* basePos, const cdc::Vector3* p0, const cdc::Vector3* p1, float hitlineRot, float* low, float* high);
static void  CAMERA_SetupColInfo(Camera* camera, CameraCollisionInfo* colInfo, const cdc::Vector3* targetCamPos);
static float CAMERA_GetLineAngle(Camera* camera, CameraCollisionInfo* colInfo, const cdc::Vector3* linept, const cdc::Vector3* linept2, int line);
static int   CAMERA_ACNoForcedMovement(Camera* camera, CameraCollisionInfo* colInfo);
static int   CAMERA_AbsoluteCollision(Camera* camera, CameraCollisionInfo* colInfo);
static void  CAMERA_DoPanicCheck(Camera* camera, CameraCollisionInfo* colInfo, cdc::Euler* rotation, float* bestZ, float* maxDist);
static void  CAMERA_Panic(Camera* camera, float minDist);

// 0x00483c50 (PDB: 0x00485680)
void CAMERA_update_dist_debounced(Camera* camera, float dist)
{
    int16_t* shortenCount = reinterpret_cast<int16_t*>(&camera->shorten_count);
    ++(*shortenCount);
    camera->shorten_flag = 1;
    if (*shortenCount > 1) {
        camera->collisionTargetFocusDistance = dist;
    }
}

// 0x00483c80 (PDB: 0x004856b0)
static int CAMERA_CalcIntersectAngle(
    const cdc::Vector3* planePt,
    const cdc::Vector3* basePos,
    const cdc::Vector3* p0,
    const cdc::Vector3* p1,
    float hitlineRot,
    float* low,
    float* high)
{
    cdc::Vector3 intersection;
    float planeD = VecDot3(*planePt, theCamera.camera_plane);
    if (static_cast<float>(MATH3D_IntersectLineAndPlane(&intersection, p0, p1, &theCamera.camera_plane, planeD)) != 0.0f) {
        float angle = CAMERA_CalcZRotation(basePos, &intersection);
        float diff = CAMERA_SignedAngleDifference(angle, hitlineRot);
        if (diff < *low) {
            *low = diff;
        }
        if (diff > *high) {
            *high = diff;
        }
        return 1;
    }
    return 0;
}

// 0x00483d40 (PDB: 0x00485770)
static void CAMERA_SetupColInfo(Camera* camera, CameraCollisionInfo* colInfo, const cdc::Vector3* targetCamPos)
{
    static int16_t toggle = 0; // 0x010fda20 (PDB: 0x01100c20)

    const int16_t mode = *reinterpret_cast<const int16_t*>(&camera->mode);
    if (mode == 5 || mode == 13 || (camera->instance_mode & 0x2180) != 0) {
        colInfo->start = camera->targetFocusPoint;
    } else {
        Instance* focusInst = camera->focusInstance;
        colInfo->start = MakeVec3(
            focusInst->matrix[focusInst->rootMatrix].col3.x,
            focusInst->matrix[focusInst->rootMatrix].col3.y,
            focusInst->matrix[focusInst->rootMatrix].col3.z,
            focusInst->matrix[focusInst->rootMatrix].col3.w);

        if ((camera->instance_mode & 0x18000) == 0) {
            colInfo->start.z += (camera->instance_mode & 0x400) != 0 ? 60.0f : 170.0f;
        }
        if ((camera->instance_mode & 0x40) != 0) {
            colInfo->start.x = camera->targetFocusPoint.x;
            colInfo->start.y = camera->targetFocusPoint.y;
        }
    }

    colInfo->end = *targetCamPos;

    if (camera->data.Follow.tface != nullptr || previousCollision != 0) {
        colInfo->cldLines = 0x1e;
        toggle = (toggle == 0) ? 1 : 0;
    } else if (toggle != 0) {
        colInfo->cldLines = 0x0e;
        toggle = 0;
    } else {
        colInfo->cldLines = 0x16;
        toggle = 1;
    }
}

// 0x00483ec0 (PDB: 0x004858f0)
float CAMERA_dampgetline(float target)
{
    static float target_angle = 0.0f; // 0x010fda24 (PDB: 0x01100c24)
    static float angle_vel = 0.0f;    // 0x010fda28 (PDB: 0x01100c28)
    static float angle_accl = 0.0f;   // 0x010fda2c (PDB: 0x01100c2c)

    CriticalDampAngle(3, &target_angle, target, &angle_vel, &angle_accl, 1024.0f);

    if (target_angle > kPi) {
        target_angle -= kTwoPi;
    }

    if (std::fabs(target_angle) < kPi / 256.0f && std::fabs(target) > kPi / 64.0f) {
        target_angle = (target >= 0.0f) ? (kPi / 256.0f) : (-kPi / 256.0f);
    }

    return target_angle;
}

// 0x00483f90 (PDB: 0x004859c0)
static float CAMERA_GetLineAngle(
    Camera* camera,
    CameraCollisionInfo* colInfo,
    const cdc::Vector3* linept,
    const cdc::Vector3* linept2,
    int line)
{
    (void)camera;
    TerrainGroup* tg = colInfo->terrainGroups[line];
    MTriangle tri;
    tg->mesh->GetTriangle(&tri, colInfo->tfaceList[line]);

    const cdc::Vector3& meshPos = tg->mesh->m_position;
    cdc::Vector3 basePt = VecSub(*linept, meshPos);
    cdc::Vector3 hitEndLocal = VecSub(CameraLine_endPt[line], meshPos);

    float hitlineRot = CAMERA_CalcZRotation(&CameraLine_startPt[line], &CameraLine_endPt[line]);
    float high = -5000.0f;
    float low = 5000.0f;

    const cdc::Vector3* edgeStarts[3] = { &tri.v0, &tri.v1, &tri.v2 };
    const cdc::Vector3* edgeEnds[3]   = { &tri.v1, &tri.v2, &tri.v0 };

    int hitCount = 0;
    for (int i = 0; i < 3; ++i) {
        hitCount |= CAMERA_CalcIntersectAngle(&hitEndLocal, &basePt, edgeStarts[i], edgeEnds[i], hitlineRot, &low, &high);
    }

    if (hitCount == 0) {
        cdc::Vector3 basePt2 = VecSub(*linept2, meshPos);
        for (int i = 0; i < 3; ++i) {
            CAMERA_CalcIntersectAngle(&hitEndLocal, &basePt2, edgeStarts[i], edgeEnds[i], hitlineRot, &low, &high);
        }
    }

    if (high == -5000.0f) {
        high = 0.0f;
    }
    if (low == 5000.0f) {
        low = 0.0f;
    }
    if (high > kHalfPi) {
        high = 0.0f;
    }
    if (low < -kHalfPi) {
        low = 0.0f;
    }

    return (line == 2) ? high : low;
}

// Helper unifying symmetric right (side == 1, right_point) and left (side == 2, left_point) collision push logic
static int CAMERA_ApplySidePush(
    Camera* camera,
    CameraCollisionInfo* colInfo,
    int side,
    float targetDiff)
{
    const bool isRight = (side == 1);
    const int pushValue = isRight ? 2 : 1;
    *reinterpret_cast<int16_t*>(&camera->collision_lastPush) = static_cast<int16_t>(pushValue);

    if (*reinterpret_cast<const int16_t*>(&camera->mode) == 0 &&
        (isRight ? (camera->ACE_amount < 0.0f) : (camera->ACE_amount > 0.0f))) {
        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
        return 1;
    }

    const cdc::Vector3* pt1 = isRight ? &camera->right_point : &camera->left_point;
    const cdc::Vector3* pt2 = isRight ? &camera->right_point2 : &camera->left_point2;
    const float lineAngle = CAMERA_GetLineAngle(camera, colInfo, pt1, pt2, side);
    const float dampedAngle = CAMERA_dampgetline(lineAngle);

    if (isRight) {
        if (targetDiff < 0.0f && std::fabs(-targetDiff) > std::fabs(dampedAngle - kPi / 512.0f)) {
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
            return 1;
        }
        if (std::fabs(camera->ACE_amount) < 6.0f) {
            if (dampedAngle <= -kPi / 32.0f) {
                camera->CollisionTargetFocusRotation.z -= kPi / 32.0f;
            } else {
                camera->CollisionTargetFocusRotation.z = camera->FocusRotation.z + dampedAngle - kPi / 512.0f;
            }
        } else if (dampedAngle <= 0.0f) {
            camera->CollisionTargetFocusRotation.z = camera->FocusRotation.z + dampedAngle - kPi / 512.0f;
        }
    } else {
        if (targetDiff > 0.0f && std::fabs(targetDiff) > std::fabs(dampedAngle + kPi / 512.0f)) {
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
            return 1;
        }
        if (std::fabs(camera->ACE_amount) < 6.0f) {
            if (dampedAngle >= kPi / 32.0f) {
                camera->CollisionTargetFocusRotation.z += kPi / 32.0f;
            } else {
                camera->CollisionTargetFocusRotation.z = camera->FocusRotation.z + dampedAngle + kPi / 512.0f;
            }
        } else if (dampedAngle >= 0.0f) {
            camera->CollisionTargetFocusRotation.z = camera->FocusRotation.z + dampedAngle + kPi / 512.0f;
        }
    }

    float limitedRot = CAMERA_LimitRotation(camera, camera->CollisionTargetFocusRotation.z);
    if (limitedRot != camera->CollisionTargetFocusRotation.z) {
        camera->CollisionTargetFocusRotation.z = limitedRot;
        CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
    }

    return 1;
}

// 0x00484200 (PDB: 0x00485c30)
static int CAMERA_ACNoForcedMovement(Camera* camera, CameraCollisionInfo* colInfo)
{
    int ret = 0;
    const int16_t notMoved = CAMERA_FocusInstanceMoved(camera) ? 0 : 1;

    if (*reinterpret_cast<const int16_t*>(&camera->always_rotate_flag) == 0 &&
        camera->rotState != 1 &&
        colInfo->numCollided > 1 &&
        (colInfo->line == 1 || colInfo->line == 2)) {
        if ((colInfo->flags & 4) != 0 &&
            (colInfo->flags & 2) != 0 &&
            std::fabs(colInfo->lengthList[2] - colInfo->lengthList[1]) <= 1000.0f) {
            CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
            CAMERA_dampgetline(0.0f);
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
            return 1;
        }

        float minLength = kSentinelDist;
        bool allOtherLinesFar = true;
        for (int i = 1; i < 5; ++i) {
            if (i == colInfo->line) {
                continue;
            }
            if (colInfo->lengthList[i] < 800.0f) {
                allOtherLinesFar = false;
                break;
            }
            if (colInfo->lengthList[i] < minLength) {
                minLength = colInfo->lengthList[i];
            }
        }

        if (allOtherLinesFar) {
            colInfo->numCollided = 1;
            colInfo->flags = 1 << colInfo->line;
            if (colInfo->lenCenterToExtend <= 800.0f) {
                CAMERA_update_dist_debounced(camera, minLength);
            } else {
                CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
            }
        } else if (colInfo->numCollided == 4 &&
                   *reinterpret_cast<const int16_t*>(&camera->collision_lastPush) != 0 &&
                   colInfo->lenCenterToExtend < 400.0f) {
            colInfo->numCollided = 1;
            if (*reinterpret_cast<const int16_t*>(&camera->collision_lastPush) == 2) {
                colInfo->flags = 2;
                colInfo->line = 1;
            } else {
                colInfo->line = 2;
                colInfo->flags = 4;
            }
        }
    }

    const float targetDiff = CAMERA_SignedAngleDifference(
        camera->TargetFocusRotation.z,
        camera->CollisionTargetFocusRotation.z);

    if (colInfo->numCollided == 4) {
        camera->collisionTargetFocusDistance = colInfo->lenCenterToExtend;
        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        CAMERA_dampgetline(0.0f);
        return 1;
    }

    const int16_t lastPush = *reinterpret_cast<const int16_t*>(&camera->collision_lastPush);
    const bool rightActive = (colInfo->flags & 2) != 0 && (notMoved == 0 || lastPush != 1);
    const bool leftActive  = (colInfo->flags & 4) != 0 && (notMoved == 0 || lastPush != 2);

    if (rightActive) {
        if ((colInfo->flags & 4) != 0) {
            CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
            return 1;
        }
        return CAMERA_ApplySidePush(camera, colInfo, 1, targetDiff);
    }

    if (leftActive) {
        if ((colInfo->flags & 2) != 0) {
            CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
            return 1;
        }
        return CAMERA_ApplySidePush(camera, colInfo, 2, targetDiff);
    }

    camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;

    if ((colInfo->flags & 8) != 0) {
        CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        if (colInfo->numCollided == 1 || ((colInfo->flags & 0x10) != 0 && colInfo->numCollided == 2)) {
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        }
    } else if ((colInfo->flags & 0x10) != 0) {
        CAMERA_update_dist_debounced(camera, colInfo->lenCenterToExtend);
        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
    }

    CAMERA_dampgetline(0.0f);
    return ret;
}

// 0x00484830 (PDB: 0x00486260)
static int CAMERA_AbsoluteCollision(Camera* camera, CameraCollisionInfo* colInfo)
{
    int camFlags = camera->flags;
    if ((camFlags & 0x10000) != 0) {
        return 0;
    }

    camera->FocusRotation.z = CAMERA_CircleAnglef(camera->FocusRotation.z);
    camFlags = camera->flags;

    if ((camFlags & 0x10000) == 0 &&
        (camera->instance_mode & 0x180) == 0 &&
        (camFlags & 0x2000) == 0 &&
        camera->LockBehindMode == 0 &&
        camera->rotState == 0 &&
        *reinterpret_cast<const int16_t*>(&camera->always_rotate_flag) == 0) {
        if (*reinterpret_cast<const int16_t*>(&camera->forced_movement) == 0 &&
            *reinterpret_cast<const int16_t*>(&camera->last_forced_movement) == 0) {
            return CAMERA_ACNoForcedMovement(camera, colInfo);
        }
        camera->collisionTargetFocusDistance = colInfo->lenCenterToExtend;
        return 0;
    }

    if ((camFlags & 0x10000) != 0) {
        if ((colInfo->flags & 0x18) == 0) {
            return CAMERA_ACNoForcedMovement(camera, colInfo);
        }
        float dist = colInfo->lenCenterToExtend - 150.0f;
        if (dist < 400.0f) {
            dist = 400.0f;
        }
        camera->collisionTargetFocusDistance = dist;
        return 1;
    }

    camera->collisionTargetFocusDistance = colInfo->lenCenterToExtend;
    return 0;
}

// 0x00484910 (PDB: 0x00486340)
IndexedFace* CAMERA_SphereToSphereWithLines(
    Camera* camera,
    CameraCollisionInfo* colInfo,
    int secondcheck_flag,
    int* objectFlag)
{
    Level* level = STREAM_GetLevelWithID(
        *reinterpret_cast<const int16_t*>(&camera->focusInstance->currentStreamUnitID));

    IndexedFace* minTface = nullptr;
    float minDist = 0.0f;
    int minLine = 0;

    if (level == nullptr) {
        return (*objectFlag == 0) ? minTface : nullptr;
    }

    colInfo->flags = 0;
    colInfo->numCollided = 0;
    colInfo->line = -1;

    cdc::Euler rotation;
    CAMERA_CalcRotation(&rotation, &colInfo->end, &colInfo->start);

    if ((camera->flags & 0x10000) != 0) {
        cdc::Vector3 delta = VecSub(colInfo->end, colInfo->start);
        colInfo->start = VecAdd(colInfo->start, VecScale(delta, 1.0f / 32.0f));
    }

    cdc::Matrix matrix;
    MatrixBuildIdentity(matrix);
    const float rotZPlusHalfPi = rotation.z + kHalfPi;
    MatrixRotZ(matrix, std::sin(rotZPlusHalfPi), std::cos(rotZPlusHalfPi));

    if ((camera->flags & 0x10000) == 0 && *reinterpret_cast<const int16_t*>(&camera->mode) != 5) {
        cdc::Vector3 rightVec = Mul3x3(matrix, MakeVec3(0.0f, 1.0f, 0.0f));
        camera->ACE_amount = VecDot3(rightVec, camera->focusInstanceVelVec);
    } else {
        camera->ACE_amount = 0.0f;
    }

    const float lineY = ((camera->instance_mode & 0x10) != 0) ? 36.0f : 72.0f;
    float line3 = 0.0f;

    if (*reinterpret_cast<const int16_t*>(&camera->always_rotate_flag) == 0 &&
        *reinterpret_cast<const int16_t*>(&camera->forced_movement) == 0) {
        line3 = static_cast<float>(*reinterpret_cast<const int16_t*>(&camera->rotDirection)) * lineY * (45.0f / 4096.0f);
        if (line3 > 80.0f) {
            line3 = 80.0f;
        } else if (line3 < -80.0f) {
            line3 = -80.0f;
        }
    } else if (*reinterpret_cast<const int16_t*>(&camera->forced_movement) != 0) {
        line3 = static_cast<float>(*reinterpret_cast<const int16_t*>(&camera->rotDirection)) * lineY * (1.0f / 4096.0f);
    } else {
        float diff = CAMERA_SignedAngleDifference(camera->FocusRotation.z, camera->TargetFocusRotation.z);
        line3 = (diff < 0.0f) ? lineY : -lineY;
    }

    if (camera->ACE_amount > 0.0f) {
        if (line3 > 0.0f) {
            line3 -= camera->ACE_amount;
            if (line3 < 0.0f) {
                line3 = 0.0f;
            }
        }
    } else if (line3 < 0.0f) {
        line3 -= camera->ACE_amount;
        if (line3 > 0.0f) {
            line3 = 0.0f;
        }
    }

    float aceAbs = camera->ACE_amount;
    float aceOffset = ((camera->instance_mode & 0x10) != 0) ? 0.0f : aceAbs;
    if ((camera->instance_mode & 0x2000) != 0) {
        aceAbs = 0.0f;
        aceOffset = 0.0f;
    }

    // Line 1 (right)
    float startDistY = 45.0f;
    float endDistY = 32.0f;
    if (camera->ACE_amount > 0.0f) {
        endDistY = aceAbs * 5.5f + 32.0f;
        startDistY = aceOffset + 45.0f;
    }
    if (line3 > 0.0f) {
        endDistY += line3 * 5.0f;
    }
    CameraLine_startPt[1] = VecAdd(colInfo->start, Mul3x3(matrix, MakeVec3(0.0f, startDistY, 0.0f)));
    CameraLine_endPt[1]   = VecAdd(colInfo->end,   Mul3x3(matrix, MakeVec3(0.0f, endDistY, 0.0f)));

    // Line 2 (left)
    startDistY = 45.0f;
    endDistY = 32.0f;
    if (camera->ACE_amount < 0.0f) {
        endDistY = 32.0f - aceAbs * 5.5f;
        startDistY = 45.0f - aceOffset;
    }
    if (line3 < 0.0f) {
        endDistY -= line3 * 5.0f;
    }
    CameraLine_startPt[2] = VecSub(colInfo->start, Mul3x3(matrix, MakeVec3(0.0f, startDistY, 0.0f)));
    CameraLine_endPt[2]   = VecSub(colInfo->end,   Mul3x3(matrix, MakeVec3(0.0f, endDistY, 0.0f)));

    camera->right_point  = VecAdd(CameraLine_startPt[1], camera->focusInstanceVelVec);
    camera->right_point2 = CameraLine_startPt[1];
    camera->left_point   = VecAdd(CameraLine_startPt[2], camera->focusInstanceVelVec);
    camera->left_point2  = CameraLine_startPt[2];

    // Vertical orientation matrix for top/bottom lines and camera plane
    MatrixBuildIdentity(matrix);
    const float rotXPlusHalfPi = rotation.x + kHalfPi;
    MatrixRotX(matrix, std::sin(rotXPlusHalfPi), std::cos(rotXPlusHalfPi));
    MatrixRotZ(matrix, std::sin(rotation.z), std::cos(rotation.z));

    // Line 3 (top) & Line 4 (bottom)
    CameraLine_startPt[3] = VecAdd(colInfo->start, Mul3x3(matrix, MakeVec3(0.0f, 16.0f, 0.0f)));
    CameraLine_endPt[3]   = VecAdd(colInfo->end,   Mul3x3(matrix, MakeVec3(0.0f, 32.0f, 0.0f)));
    CameraLine_startPt[4] = VecSub(colInfo->start, Mul3x3(matrix, MakeVec3(0.0f, 32.0f, 0.0f)));
    CameraLine_endPt[4]   = VecSub(colInfo->end,   Mul3x3(matrix, MakeVec3(0.0f, 180.0f, 0.0f)));

    cdc::Vector3 camPlane = Mul3x3(matrix, MakeVec3(0.0f, 1.0f, 0.0f));
    camera->camera_plane.x = camPlane.x;
    camera->camera_plane.y = camPlane.y;
    camera->camera_plane.z = camPlane.z;

    cdc::Vector3 rayDir = Mul3x3(matrix, MakeVec3(0.0f, 0.0f, 1.0f));

    CameraLine_startPt[0] = colInfo->start;
    CameraLine_endPt[0]   = colInfo->end;
    colInfo->lenCenterToExtend = camera->targetFocusDistance;

    STREAM_GetStreamUnitWithID(level->streamUnitID);

    for (int lineIdx = 0; lineIdx < 5; ++lineIdx) {
        if ((colInfo->cldLines & (1 << lineIdx)) == 0) {
            continue;
        }

        cdc::Vector3 rayVec = VecSub(CameraLine_endPt[lineIdx], CameraLine_startPt[lineIdx]);
        if (VecIsZero3(rayVec)) {
            rayVec = MakeVec3(0.0f, -1.0f, 0.0f);
        } else {
            VecNormalize3(rayVec);
        }

        PCollideInfo pcollide = {};
        pcollide.startPoint = VecAdd(CameraLine_startPt[lineIdx], VecScale(rayVec, 32.0f));
        pcollide.endPoint = CameraLine_endPt[lineIdx];
        pcollide.instance = nullptr;
        pcollide.collideType = 1;

        gMultibodySystem->SetDrawLineProbes(false, gLineProbeColor);
        MULTIBODY_LegacyLineProbe(0x302c5981, &pcollide, s_playerInstance);
        gMultibodySystem->SetDrawLineProbes((g_gameTracker.debugFlags4 & 0x800000) != 0, gLineProbeColor);

        IndexedFace* hitFace = static_cast<IndexedFace*>(pcollide.prim);
        colInfo->tfaceList[lineIdx] = hitFace;
        colInfo->terrainGroups[lineIdx] = pcollide.terrainGroup;
        CameraLine_endPt[lineIdx] = pcollide.endPoint;

        const float rawDist = VecDot3(VecSub(CameraLine_startPt[lineIdx], pcollide.endPoint), rayDir);
        colInfo->lengthList[lineIdx] = rawDist;

        if (hitFace == nullptr) {
            continue;
        }

        if ((camera->instance_mode & 0x10) != 0 && rawDist < 64.0f) {
            colInfo->tfaceList[lineIdx] = nullptr;
            continue;
        }

        previousCollision = 1;
        if (pcollide.type == 5 || pcollide.type == 1 || pcollide.type == 10) {
            *objectFlag = 1;
        }

        if (secondcheck_flag != 0) {
            return (*objectFlag == 0) ? colInfo->tfaceList[lineIdx] : nullptr;
        }

        ++colInfo->numCollided;

        float adjDist = rawDist - 100.0f;
        if ((camera->instance_mode & 0x400000) != 0) {
            if (adjDist < 110.0f) {
                adjDist = 110.0f;
            }
        } else if ((camera->instance_mode & 0x400) != 0) {
            if (adjDist < 250.0f) {
                adjDist = 250.0f;
            }
        } else if (adjDist < 150.0f) {
            adjDist = 150.0f;
        }

        colInfo->lengthList[lineIdx] = adjDist;
        if (minLine == 0 || adjDist < minDist) {
            minLine = lineIdx;
            colInfo->line = lineIdx;
            minDist = adjDist;
            colInfo->lenCenterToExtend = adjDist;
            minTface = colInfo->tfaceList[lineIdx];
        }

        colInfo->flags |= (1 << lineIdx);
    }

    return (*objectFlag == 0) ? minTface : nullptr;
}

// 0x00485570 (PDB: 0x00486fa0)
static void CAMERA_DoPanicCheck(
    Camera* camera,
    CameraCollisionInfo* colInfo,
    cdc::Euler* rotation,
    float* bestZ,
    float* maxDist)
{
    int objFlag = 0;
    cdc::Vector3 targetPos;
    CAMERA_CalcPosition(&targetPos, &camera->focusPoint, rotation, camera->targetFocusDistance);
    colInfo->end = targetPos;

    CAMERA_SphereToSphereWithLines(camera, colInfo, 0, &objFlag);
    if (colInfo->numCollided == 0 || colInfo->lenCenterToExtend >= *maxDist) {
        *bestZ = rotation->z;
        *maxDist = colInfo->lenCenterToExtend;
    }
}

// 0x00485600 (PDB: 0x00487030)
static void CAMERA_Panic(Camera* camera, float minDist)
{
    CameraCollisionInfo colInfo = {};
    cdc::Vector3 dummyTarget = {};
    CAMERA_SetupColInfo(camera, &colInfo, &dummyTarget);

    cdc::Euler rotation = camera->FocusRotation;
    float bestZ = rotation.z;
    float maxDist = minDist;
    int16_t countRight = 0;
    int16_t countLeft = 0;

    float angle = 3.23976755f; // 0x404f585a (~PI + PI/32)
    while (angle > 0.0f) {
        rotation.z = camera->FocusRotation.z + angle;
        CAMERA_DoPanicCheck(camera, &colInfo, &rotation, &bestZ, &maxDist);
        if (colInfo.numCollided == 0) {
            if (++countRight > 2) {
                bestZ += kPi / 32.0f;
                break;
            }
        }

        rotation.z = camera->FocusRotation.z - angle;
        CAMERA_DoPanicCheck(camera, &colInfo, &rotation, &bestZ, &maxDist);
        if (colInfo.numCollided == 0) {
            if (++countLeft > 2) {
                bestZ -= kPi / 32.0f;
                break;
            }
        }

        angle -= kPi / 16.0f;
    }

    if (countRight == 0 && countLeft == 0 && std::fabs(maxDist - minDist) < 400.0f) {
        *reinterpret_cast<uint16_t*>(&camera->panic_count) = 0x8001;
        return;
    }

    camera->TargetFocusRotation.z = bestZ;
    camera->CollisionTargetFocusRotation.z = bestZ;
    camera->signalRot.z = bestZ;
    camera->collisionTargetFocusDistance = maxDist;
    camera->targetFocusDistance = maxDist;
}

// 0x004857b0 (PDB: 0x004871e0)
int CAMERA_DoCameraCollision(Camera* camera, cdc::Vector3* targetCamPos, int simpleflag)
{
    CameraCollisionInfo colInfo = {};
    int objFlag = 0;

    CAMERA_SetupColInfo(camera, &colInfo, targetCamPos);
    previousCollision = 0;
    camera->data.Follow.tface = CAMERA_SphereToSphereWithLines(camera, &colInfo, 0, &objFlag);

    if (colInfo.lengthList[0] >= 800.0f &&
        (camera->instance_mode & 0x10) != 0 &&
        camera->instance_zvel < -10.0f &&
        *reinterpret_cast<const int16_t*>(&camera->forced_movement) == 0 &&
        (camera->flags & 0x2000) == 0 &&
        camera->LockBehindMode == 0) {
        CAMERA_Panic(camera, colInfo.lenCenterToExtend);
        if (*reinterpret_cast<const uint16_t*>(&camera->panic_count) != 0x8001) {
            const float zrot = camera->signalRot.z;
            camera->TargetFocusRotation.z = zrot;
            camera->FocusRotation.z = zrot;
            camera->teleportZRot = zrot;
            camera->CollisionTargetFocusRotation.z = zrot;
            camera->cutFlag = 2;
            camera->rotState = 0;
            return 0;
        }
    } else {
        *reinterpret_cast<int16_t*>(&camera->panic_count) = 0;
    }

    if (objFlag != 0) {
        camera->data.Follow.tface = nullptr;
        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        camera->collisionTargetFocusDistance = colInfo.lengthList[colInfo.line];
        return 1;
    }

    if (camera->data.Follow.tface != nullptr) {
        const float deadZoneCos = std::cos(globalInfo->cameraDefaults->toward_dead_zone_end * kDegToRad);
        if (deadZoneCos < camera->instanceXYVelCameraDot) {
            camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
            camera->collisionTargetFocusDistance = colInfo.lengthList[colInfo.line];
            return 1;
        }
    }

    if (simpleflag != 0) {
        if (camera->data.Follow.tface == nullptr) {
            return 0;
        }
        const int16_t mode = *reinterpret_cast<const int16_t*>(&camera->mode);
        if (mode == 3 || mode == 5 || mode == 13) {
            camera->collisionTargetFocusDistance = colInfo.lengthList[colInfo.line];
        }
        return 1;
    }

    if (*reinterpret_cast<const int16_t*>(&camera->always_rotate_flag) != 0) {
        colInfo.line = 4;
    }

    int ret = 0;
    const int16_t alwaysRotate = *reinterpret_cast<const int16_t*>(&camera->always_rotate_flag);

    if ((camera->flags & 0x10000) != 0) {
        if (camera->data.Follow.tface != nullptr) {
            ret = CAMERA_AbsoluteCollision(camera, &colInfo);
        } else {
            camera->collisionTargetFocusDistance = kSentinelDist;
        }

        const float absDiff = std::fabs(
            CAMERA_SignedAngleDifference(camera->CollisionTargetFocusRotation.z, camera->TargetFocusRotation.z));
        if ((colInfo.flags & 6) == 0) {
            CAMERA_dampgetline(0.0f);
        }
        if (absDiff > kHalfPi && (colInfo.flags & 6) == 0) {
            Decouple_AngleMoveToward(&camera->CollisionTargetFocusRotation.z, camera->TargetFocusRotation.z, kPi / 32.0f);
            return ret;
        }

        float step = absDiff * 0.0625f;
        if (step < kPi / 256.0f) {
            step = kPi / 256.0f;
        } else if (step > kPi / 64.0f) {
            step = kPi / 64.0f;
        }
        Decouple_AngleMoveToward(&camera->CollisionTargetFocusRotation.z, camera->TargetFocusRotation.z, step);
        return ret;
    }

    if ((camera->instance_mode & 0x180) == 0 &&
        (camera->flags & 0x2000) == 0 &&
        camera->LockBehindMode == 0 &&
        alwaysRotate == 0) {
        if (camera->data.Follow.tface != nullptr) {
            ret = CAMERA_AbsoluteCollision(camera, &colInfo);
            camera->TargetFocusRotation.z = camera->CollisionTargetFocusRotation.z;
            return ret;
        }

        CAMERA_dampgetline(0.0f);
        if (*reinterpret_cast<const int16_t*>(&camera->mode) == 0 && camera->instance_xyvel > 0.0f) {
            if (CAMERA_AngleDifference(camera->CollisionTargetFocusRotation.z, camera->FocusRotation.z) > kPi / 512.0f) {
                const float curDiff = CAMERA_SignedAngleDifference(
                    camera->CollisionTargetFocusRotation.z,
                    camera->FocusRotation.z);
                const float targetDiff = CAMERA_SignedAngleDifference(
                    camera->TargetFocusRotation.z,
                    camera->FocusRotation.z);

                if (((curDiff < 0.0f && targetDiff < 0.0f) || (curDiff > 0.0f && targetDiff > 0.0f)) &&
                    std::fabs(curDiff) < std::fabs(targetDiff)) {
                    camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
                    camera->collisionTargetFocusDistance = kSentinelDist;
                    return ret;
                }

                *reinterpret_cast<int16_t*>(&camera->collision_lastPush) = 0;
                camera->collisionTargetFocusDistance = kSentinelDist;
                return ret;
            }
        }

        camera->CollisionTargetFocusRotation.z = camera->TargetFocusRotation.z;
        *reinterpret_cast<int16_t*>(&camera->collision_lastPush) = 0;
        camera->collisionTargetFocusDistance = kSentinelDist;
        return ret;
    }

    if (camera->data.Follow.tface != nullptr) {
        ret = CAMERA_AbsoluteCollision(camera, &colInfo);
    } else {
        camera->collisionTargetFocusDistance = kSentinelDist;
    }
    Decouple_AngleMoveToward(
        &camera->CollisionTargetFocusRotation.z,
        camera->TargetFocusRotation.z,
        (3.0f * kPi) / 64.0f);
    return ret;
}
