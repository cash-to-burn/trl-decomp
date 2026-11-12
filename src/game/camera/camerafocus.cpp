#include "game/camera/camerafocus.h"
#include "game/camera/camera.h"
#include "game/camera/camerasupport.h"
#include "game/globalinfo.h"
#include "game/instance.h"
#include "game/player/player.h"
#include <cmath>

// External rendering/transform & debug print helpers
extern void TRANS_SetTransformMatrixf(cdc::Matrix* m);                                      // 0x00402700 (PDB: 0x00402c70)
extern cdc::Vector3* TRANS_RotTransPersVectorf(const cdc::Vector3* vIn, cdc::Vector3* vOut); // 0x00402710 (PDB: 0x00402c80)
extern void FONT_Print(const char* fmt, ...);                                               // 0x00432510 (PDB: 0x00433850)

// Camera focus globals
float zOffsetUndo = 0.0f;                  // 0x010fc578 (PDB: 0x010fd778)
static float combatZPosition = 0.0f;       // 0x010fda30 (PDB: 0x010fec30)
static float currentZOffset = 0.0f;        // 0x010fda34 (PDB: 0x010fec34)
static float zOffsetVel = 0.0f;            // 0x010fda38 (PDB: 0x010fec38)
static float zOffsetAccl = 0.0f;           // 0x010fda3c (PDB: 0x010fec3c)

namespace {

cdc::Vector3 ToVector3(const cdc::Vector& v)
{
    cdc::Vector3 out;
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
    out.w = v.w;
    return out;
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

// 0x00485df0 (PDB: 0x00487820)
cdc::Vector3 CAMERA_GetInterestInstFocus(Camera* camera)
{
    cdc::Vector3 dummyScreen;
    cdc::Vector3 playerMatrix1Pos = ToVector3(s_playerInstance->matrix[1].col3);
    TRANS_SetTransformMatrixf(&camera->core.wcTransformf);
    TRANS_RotTransPersVectorf(&playerMatrix1Pos, &dummyScreen);

    float maxDist = camera->followInterestInstTune;
    FONT_Print("max dist is %d\n", static_cast<int>(maxDist));

    cdc::Vector3 basePos = ToVector3(s_playerInstance->matrix[0].col3);
    basePos.z += static_cast<float>(INSTANCE_Query(s_playerInstance, 0x65)) *
                 globalInfo->cameraDefaults->focus_height_combat;

    cdc::Vector3 outPos = basePos;
    int count = 1;

    Instance* interestInst = camera->followInterestInst;
    if (interestInst == nullptr) {
        return outPos;
    }

    cdc::Vector3 interestPos = ToVector3(interestInst->matrix[interestInst->rootMatrix].col3);
    float dx = interestPos.x - basePos.x;
    float dy = interestPos.y - basePos.y;
    float dz = interestPos.z - basePos.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    cdc::Vector3 screenInterest;
    TRANS_RotTransPersVectorf(&interestPos, &screenInterest);

    if (dist <= maxDist && screenInterest.z > 160.0f) {
        count = 2;
        float halfMax = maxDist * 0.5f;
        float weight = (dist <= halfMax) ? 1.0f : ((maxDist - dist) / halfMax);

        if (interestInst->fadeValue != 0.0f) {
            weight *= (1.0f - interestInst->fadeValue);
        }

        float invWeight = 1.0f - weight;
        outPos.x = basePos.x + (interestPos.x * weight + basePos.x * invWeight);
        outPos.y = basePos.y + (interestPos.y * weight + basePos.y * invWeight);
        outPos.z = basePos.z + (interestPos.z * weight + basePos.z * invWeight);
        outPos.w = basePos.w + (interestPos.w * weight + basePos.w * invWeight);
    }

    float divisor = static_cast<float>(count);
    outPos.x /= divisor;
    outPos.y /= divisor;
    outPos.z /= divisor;
    outPos.w /= divisor;
    return outPos;
}

// 0x00486030 (PDB: 0x00487a60)
cdc::Vector3 CAMERA_CalcFocusOffset(Camera* camera)
{
    Instance* inst = camera->focusInstance;
    camera->focusOffset.z = CAMERA_FocusHeight(camera);

    cdc::Vector3 offset = camera->focusOffset;
    if (inst->matrix == nullptr) {
        return offset;
    }

    if ((camera->instance_mode & 0x10) != 0) {
        offset.y = -80.0f;
    }
    return Mul3x3(inst->matrix[0], offset);
}

} // namespace

// 0x00485c50 (PDB: 0x00487680)
void CAMERA_CreateNewFocuspoint(Camera* camera)
{
    CAMERA_CalcPosition(&camera->targetPos, &camera->core.position, &camera->FocusRotation, camera->focusDistance);

    camera->focusPoint.x = camera->core.position.x + (camera->core.position.x - camera->targetPos.x);
    camera->focusPoint.y = camera->core.position.y + (camera->core.position.y - camera->targetPos.y);
    camera->focusPoint.z = camera->core.position.z + (camera->core.position.z - camera->targetPos.z);
    camera->focusPoint.w = camera->core.position.w + (camera->core.position.w - camera->targetPos.w);
}

// 0x00485cc0 (PDB: 0x004876f0)
void CAMERA_SetMaxVel(Camera* camera)
{
    camera->maxVel = 768.0f;
}

// 0x00485cd0 (PDB: 0x00487700)
float CAMERA_FocusHeight(Camera* camera)
{
    if (camera->focusInstance == nullptr) {
        return 0.0f;
    }

    float height = static_cast<float>(INSTANCE_Query(camera->focusInstance, 0x65)) *
                   globalInfo->cameraDefaults->focus_height;
    if ((camera->instance_mode & 0x400) != 0) {
        return height * 0.32f;
    }
    return height;
}

// 0x00485d50 (PDB: 0x00487780)
void CAMERA_SetInstanceFocus(Camera* camera, Instance* instance)
{
    camera->focusInstance = instance;
    camera->flags |= 0x800;
    camera->newFocusInstancePos = instance->position;

    if (instance->object == nullptr) {
        return;
    }

    camera->focusOffset.x = 0.0f;
    camera->focusOffset.y = 0.0f;
    if (instance == s_playerInstance) {
        camera->focusOffset.z = CAMERA_FocusHeight(camera);
    } else {
        camera->focusOffset.z = static_cast<float>(INSTANCE_Query(instance, 0x65));
    }
}

// 0x004860e0 (PDB: 0x00487b10)
void CAMERA_SetFocus(Camera* camera, cdc::Vector3* focusPosition)
{
    Instance* focusInst = camera->focusInstance; // Restored across 0x004860ef stealth jump
    zOffsetUndo = 0.0f;

    if ((camera->instance_mode & 0x18000) != 0) {
        *focusPosition = ToVector3(focusInst->matrix[3].col3);
        camera->real_focuspoint = *focusPosition;
        camera->focuspoint_fallz = focusPosition->z;
        return;
    }

    float desiredZOffset = camera->ZOffset;
    float maxVel = 8.0f;
    int smoothType = 9;

    *focusPosition = ToVector3(focusInst->matrix[0].col3);
    focusPosition->z += static_cast<float>(static_cast<int>(INSTANCE_Query(focusInst, 0x42)));

    if ((camera->instance_mode & 0x400000) != 0 && playerGetVictim() != nullptr) {
        desiredZOffset = 0.0f;
    } else if ((camera->instance_mode & 0x180) != 0 && (camera->instance_mode & 0x400000) == 0) {
        smoothType = 3;
        maxVel = 32.0f;
        desiredZOffset = globalInfo->cameraDefaults->combat_camera_zoffset;

        if ((camera->instance_mode & 0x82000a) != 0) {
            if (focusPosition->z < combatZPosition) {
                combatZPosition = focusPosition->z;
            }
            float dz = combatZPosition - focusPosition->z;
            float dist = camera->focusDistance;
            if (dist >= 1000.0f) {
                focusPosition->z = combatZPosition;
            } else if (dist > 400.0f) {
                focusPosition->z += (dist - 400.0f) * dz * (1.0f / 600.0f);
            }
        } else {
            combatZPosition = focusPosition->z;
        }
    } else if (camera->mode == 2 || camera->mode == 0) {
        desiredZOffset += camera->followShapeZOffsetOffset;
    }

    if ((camera->instance_mode & 0x400000) != 0 || (camera->instance_mode & 0x180) == 0) {
        combatZPosition = focusPosition->z;
    }

    if (camera->cutFlag != 0) {
        currentZOffset = desiredZOffset;
        zOffsetVel = 0.0f;
        zOffsetAccl = 0.0f;
    } else {
        CriticalDampValue(smoothType, &currentZOffset, desiredZOffset, &zOffsetVel, &zOffsetAccl, maxVel);
    }

    cdc::Vector3 rotatedOffset = CAMERA_CalcFocusOffset(camera);
    focusPosition->x += rotatedOffset.x;
    focusPosition->y += rotatedOffset.y;
    focusPosition->z += rotatedOffset.z;
    focusPosition->w += rotatedOffset.w;

    camera->real_focuspoint = *focusPosition;

    if (camera->followInterestInst != nullptr) {
        *focusPosition = CAMERA_GetInterestInstFocus(camera);
        return;
    }

    zOffsetUndo = currentZOffset;
    float limit = camera->focusDistance * 0.25f - 48.0f;
    if (limit < 0.0f) {
        limit = 0.0f;
    }
    if (std::fabs(currentZOffset) > limit) {
        zOffsetUndo = (currentZOffset < 0.0f) ? -limit : limit;
    }

    focusPosition->z += zOffsetUndo;
    camera->focuspoint_fallz = focusPosition->z;
}
