#include "game/anim/footlockcontroller.h"
#include "game/camera/cameracollide.h"
#include "game/instance.h"
#include "game/math3d.h"
#include "game/player/player.h"
#include <cmath>

extern void MULTIBODY_LegacyLineProbe(unsigned int flags, PCollideInfo* info, BaseInstance* player); // 0x00556060 (PDB: 0x00558150)
extern void FX_LeaveFootprint(cdc::Vector3* pos, cdc::Vector3* normal, float speedZ, IndexedFace* face, int bone); // 0x00443100 (PDB: 0x00444390)

bool g_useSecondaryIK = false; // 0x010fc358 (PDB: 0x010ff558)
bool g_useToePitch = false;    // 0x010fc359 (PDB: 0x010ff559)
static bool g_useIK = true;    // 0x00f15b64 (PDB: seg3:0x00005e9c)

namespace {

constexpr float kTwoPi          = 6.28318548f;   // 0x00f01444
constexpr float kInvTwoPi       = 0.1591549367f; // 0x00efe4dc
constexpr float kDegToRad       = 0.0174532924f; // 0x00f01468
constexpr float kQuarterPi      = 0.785398185f;  // 0x3f490fdb
constexpr float kStraightLegDot = 0.995000005f;  // 0x00f01478
constexpr float kAxisEpsilonSq  = 1.0e-4f;       // 0x00f02adc
constexpr uint32_t kLineProbeMask = 0x002c19e7;

inline float WrapAngle0ToTwoPi(float angle)
{
    const float turns = angle * kInvTwoPi;
    return (turns - std::floor(turns)) * kTwoPi;
}

inline cdc::Vector3 MatrixTrans(const cdc::Matrix& m)
{
    return cdc::Vector3::Make(m.col3.x, m.col3.y, m.col3.z, m.col3.w);
}

inline cdc::Vector3 MatrixCol1(const cdc::Matrix& m)
{
    return cdc::Vector3::Make(m.col1.x, m.col1.y, m.col1.z, m.col1.w);
}

inline cdc::Vector3 MatrixCol2(const cdc::Matrix& m)
{
    return cdc::Vector3::Make(m.col2.x, m.col2.y, m.col2.z, m.col2.w);
}

} // namespace

// 0x00477f00 (PDB: 0x00479870 / ?Build@Matrix@cdc@@QAEXABVVector3@2@00@Z)
void cdc::Matrix::Build(const cdc::Vector3& c0, const cdc::Vector3& c1, const cdc::Vector3& c2)
{
    col0 = c0;
    col1 = c1;
    col2 = c2;
    col0.w = 0.0f;
    col1.w = 0.0f;
    col2.w = 0.0f;
    col3 = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 1.0f);
}

// 0x00477bf0 (PDB: 0x00479530 / ?Deactivate@FootLockController@@UAEXXZ)
void FootLockController::Deactivate()
{
    m_active = false;
    m_slide_lock = false;
}

// 0x00477c00 (PDB: 0x00479540 / ?IsActive@FootLockController@@UBE_NXZ)
bool FootLockController::IsActive() const
{
    return m_active;
}

// 0x00477c10 (PDB: 0x00479580 / ?SetSlideLock@FootLockController@@QAEX_N@Z)
void FootLockController::SetSlideLock(bool on)
{
    m_slide_lock = on;
    if (on) {
        m_active = true;
    }
}

// 0x00477c30 (PDB: 0x004795a0 / ?Reposition@FootLockController@@QAE_NABVVector3@cdc@@0@Z)
bool FootLockController::Reposition(const cdc::Vector3& p0, const cdc::Vector3& p1)
{
    (void)p1;

    PCollideInfo colInfo;
    colInfo.startPoint = p0;
    colInfo.startPoint.z += m_zProbeOffsetUp * 2.0f;
    colInfo.endPoint = p0;
    colInfo.endPoint.z -= m_zProbeOffsetDown;

    MULTIBODY_LegacyLineProbe(kLineProbeMask, &colInfo, m_instance);

    if ((colInfo.type == 3 || colInfo.type == 5) && colInfo.worldNormal.z > m_minimumSlopeNormalZ) {
        m_lockNormal = colInfo.worldNormal;
        m_lockPoint = colInfo.endPoint + cdc::Vector3::Make(0.0f, 0.0f, m_toeHeight, 0.0f);
        m_active = true;
        return true;
    }
    return false;
}

// 0x00477d80 (PDB: 0x004796f0 / ?SetHipLimits@FootLockController@@QAEXPBM0@Z)
void FootLockController::SetHipLimits(const float* min, const float* max)
{
    m_minHipZAnglePtr = min;
    m_maxHipZAnglePtr = max;
}

// 0x00477da0 (PDB: 0x00479710 / ?DeactivateWithoutInterp@FootLockController@@QAEXXZ)
void FootLockController::DeactivateWithoutInterp()
{
    Deactivate();
    m_interpWeight = 0.0f;
}

// 0x00477dc0 (PDB: 0x00479730 / ?Deactivate@HandLockController@@UAEXXZ)
void HandLockController::Deactivate()
{
    if (IsActive()) {
        m_active = false;
    }
}

// 0x00477de0 (PDB: 0x00479750 / ?IsActive@HandLockController@@UBE_NXZ)
bool HandLockController::IsActive() const
{
    return m_active;
}

// 0x00477df0 (PDB: 0x00479760 / ?SetLockPoint@HandLockController@@QAEXABVVector3@cdc@@@Z)
void HandLockController::SetLockPoint(const cdc::Vector3& lockPoint)
{
    m_lockPoint = lockPoint;
}

// 0x00477d80 (PDB: 0x004796f0 / ?SetLockPointCallback@HandLockController@@QAEXP6A?AVVector3@cdc@@PAX@Z0@Z)
void HandLockController::SetLockPointCallback(LockPointCallback function, void* data)
{
    m_lockPointCallback = function;
    m_lockPointCallbackData = data;
}

// 0x00477e10 (PDB: 0x00479780 / ?SkipSecondaryIkThisFrame@FootLockController@@QAEXXZ)
void FootLockController::SkipSecondaryIkThisFrame()
{
    m_skipSecondaryIkThisFrame = true;
}

// 0x00477e20 (PDB: 0x00479790 / ?RelocateLockPoint@FootLockController@@QAEXABVVector3@cdc@@@Z)
void FootLockController::RelocateLockPoint(const cdc::Vector3& offset)
{
    m_lockPoint += offset;
}

// 0x00477e40 (PDB: 0x004797b0 / ?AdvanceInterp@FootLockController@@QAEXXZ)
void FootLockController::AdvanceInterp()
{
    m_interpSpeed = m_active ? 0.5f : 0.2f;
    if (m_active) {
        if (m_interpWeight < 1.0f) {
            const float next = m_interpWeight + s_timeScale * m_interpSpeed;
            m_interpWeight = (next < 1.0f) ? next : 1.0f;
        }
    } else {
        const float next = m_interpWeight - s_timeScale * m_interpSpeed;
        m_interpWeight = (next > 0.0f) ? next : 0.0f;
    }
}

// 0x00477fd0 (PDB: 0x00479940 / ??0FootLockController@@QAE@PAUInstance@@IIIII@Z)
FootLockController::FootLockController(
    Instance* instance,
    unsigned int waistJoint,
    unsigned int hipJoint,
    unsigned int kneeJoint,
    unsigned int ankleJoint,
    unsigned int toeJoint)
{
    m_instance = instance;
    m_waistJoint = static_cast<uint8_t>(waistJoint);
    m_hipJoint = static_cast<uint8_t>(hipJoint);
    m_kneeJoint = static_cast<uint8_t>(kneeJoint);
    m_ankleJoint = static_cast<uint8_t>(ankleJoint);
    m_toeJoint = static_cast<uint8_t>(toeJoint);

    m_interpSpeed = 100.0f;
    m_autoUnlockTolerance = 100.0f;
    m_minimumSlopeNormalZ = 0.0f;
    m_zProbeOffsetUp = 200.0f;
    m_zProbeOffsetDown = 200.0f;
    m_minHipZAngle = -kQuarterPi;
    m_maxHipZAngle = kQuarterPi;

    m_lockOrientation.Identity();
    m_lockPoint = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_lastBrokenLockPoint = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    const cdc::Quat qId = cdc::Quat::Identity();
    m_active = false;
    m_slide_lock = false;
    m_interpWeight = 0.0f;
    m_minHipZAnglePtr = nullptr;
    m_maxHipZAnglePtr = nullptr;
    m_skipSecondaryIkThisFrame = false;
    m_lastHipQuat = qId;
    m_lastKneeQuat = qId;
    m_toeHeight = 4.8f;
}

// 0x004780f0 (PDB: 0x00479a60 / ?FindLockPoint@FootLockController@@QAE_NPAVVector3@cdc@@MM@Z)
bool FootLockController::FindLockPoint(cdc::Vector3* lockPoint, float zOffsetUp, float zOffsetDown)
{
    const cdc::Matrix* matrices = m_instance->matrix;
    const cdc::Matrix& toeMatrix = matrices[m_toeJoint];

    PCollideInfo colInfo;
    colInfo.startPoint = MatrixTrans(toeMatrix);
    colInfo.startPoint.z += zOffsetUp;
    colInfo.endPoint = MatrixTrans(toeMatrix);
    colInfo.endPoint.z -= zOffsetDown;

    MULTIBODY_LegacyLineProbe(kLineProbeMask, &colInfo, m_instance);

    if (colInfo.type == 3 && colInfo.inst != nullptr) {
        const cdc::Vector3* vel = reinterpret_cast<const cdc::Vector3*>(
            static_cast<uintptr_t>(INSTANCE_Query(m_instance, 7)));
        FX_LeaveFootprint(
            &colInfo.endPoint,
            &colInfo.worldNormal,
            vel->z,
            static_cast<IndexedFace*>(colInfo.prim),
            m_ankleJoint);
    }

    if ((colInfo.type != 3 && colInfo.type != 5) || colInfo.worldNormal.z <= m_minimumSlopeNormalZ) {
        return false;
    }

    const cdc::Vector3 candidate = colInfo.endPoint + cdc::Vector3::Make(0.0f, 0.0f, m_toeHeight, 0.0f);
    m_lockNormal = colInfo.worldNormal;
    if ((candidate - m_lastBrokenLockPoint).Len3Squared() <= m_autoUnlockTolerance) {
        return false;
    }

    const cdc::Vector3 toeCol1 = MatrixCol1(toeMatrix);
    cdc::Vector3 forward = toeCol1 - m_lockNormal * cdc::Dot3(toeCol1, m_lockNormal);
    if (forward.Len3Squared() <= kAxisEpsilonSq) {
        return false;
    }
    forward.Normalize3();

    cdc::Vector3 right;
    cdc::Cross3(&right, forward, cdc::Vector3::Make(0.0f, 0.0f, 1.0f, 0.0f));
    if (right.Len3Squared() <= kAxisEpsilonSq) {
        return false;
    }
    right.Normalize3();

    cdc::Vector3 up;
    cdc::Cross3(&up, right, forward);

    m_lockOrientation.Build(forward, right, up);

    const cdc::Vector3 ankleCol1 = MatrixCol1(matrices[m_ankleJoint]);
    m_lockAngle = MATH3D_AngleFromVector(&ankleCol1);
    *lockPoint = candidate;
    return true;
}

// 0x004783f0 (PDB: 0x00479d60 / ?Activate@FootLockController@@UAEXXZ)
void FootLockController::Activate()
{
    if (!IsActive()) {
        if (m_instance == s_playerInstance && !gPlayer->isStuckToGround && gPlayer->standingUpon == nullptr) {
            return;
        }
        if (FindLockPoint(&m_lockPoint, m_zProbeOffsetUp, m_zProbeOffsetDown)) {
            m_active = true;
            m_slide_lock = false;
        }
    }
}

// 0x00478450 (PDB: 0x00479dc0 / ??0HandLockController@@QAE@PAUInstance@@IIIII@Z)
HandLockController::HandLockController(
    Instance* instance,
    unsigned int shoulderHelperJoint,
    unsigned int shoulderJoint,
    unsigned int elbowJoint,
    unsigned int wristJoint,
    unsigned int grabJoint)
{
    m_instance = instance;
    m_shoulderHelperJoint = static_cast<uint8_t>(shoulderHelperJoint);
    m_shoulderJoint = static_cast<uint8_t>(shoulderJoint);
    m_elbowJoint = static_cast<uint8_t>(elbowJoint);
    m_wristJoint = static_cast<uint8_t>(wristJoint);
    m_grabJoint = static_cast<uint8_t>(grabJoint);

    m_interpSpeed = 100.0f;
    m_autoUnlockTolerance = 100.0f;
    m_lockOrientation.Identity();
    m_lockPoint = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    const cdc::Quat qId = cdc::Quat::Identity();
    m_active = false;
    m_interpWeight = 0.0f;
    m_minHipZAnglePtr = nullptr;
    m_maxHipZAnglePtr = nullptr;
    m_lockPointCallback = nullptr;
    m_lockPointCallbackData = nullptr;
    m_lastShoulderQuat = qId;
    m_lastElbowQuat = qId;
    m_toeHeight = 4.8f;
}

// 0x00478540 (PDB: 0x00479eb0 / ?CallbackImpl@HandLockController@@QAEIPAVAnimController@@PAUG2AnimSegValue@@PAVMatrix@cdc@@2@Z)
unsigned int HandLockController::CallbackImpl(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix)
{
    (void)ac;
    (void)segMatrix;

    m_interpSpeed = m_active ? 0.5f : (1.0f / 18.0f);
    if (m_active) {
        if (m_interpWeight < 1.0f) {
            const float next = m_interpWeight + s_timeScale * m_interpSpeed;
            m_interpWeight = (next < 1.0f) ? next : 1.0f;
        }
    } else {
        const float next = m_interpWeight - s_timeScale * m_interpSpeed;
        m_interpWeight = (next > 0.0f) ? next : 0.0f;
    }

    if (m_interpWeight == 0.0f) {
        return 7;
    }
    if ((m_instance->iflags & 0x200) == 0) {
        return 0;
    }

    cdc::Matrix* matrices = m_instance->matrix;
    const cdc::Vector3 shoulderPos = MatrixTrans(matrices[m_shoulderJoint]);
    const cdc::Vector3 elbowPos = MatrixTrans(matrices[m_elbowJoint]);
    const cdc::Vector3 wristPos = cdc::Mul3x4(*parentMatrix, segValue->trans);

    cdc::Quat upperQuat;
    cdc::Quat lowerQuat;
    if (m_active) {
        if (m_lockPointCallback != nullptr) {
            m_lockPoint = m_lockPointCallback(m_lockPointCallbackData);
        }
        const cdc::Vector3 targetPos = m_lockPoint - cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        const cdc::Vector3 bendFallback = MatrixCol2(matrices[m_wristJoint]);
        MATH3D_Solve2BoneIK(&shoulderPos, &elbowPos, &wristPos, &targetPos, &bendFallback, &upperQuat, &lowerQuat);
        m_lastShoulderQuat = upperQuat;
        m_lastElbowQuat = lowerQuat;
    } else {
        upperQuat = m_lastShoulderQuat;
        lowerQuat = m_lastElbowQuat;
    }

    if (m_interpWeight < 1.0f) {
        const cdc::Quat qId = cdc::Quat::Identity();
        upperQuat = cdc::Slerp(qId, upperQuat, m_interpWeight);
        lowerQuat = cdc::Slerp(qId, lowerQuat, m_interpWeight);
    }

    cdc::Matrix upperMat;
    cdc::Matrix lowerMat;
    upperMat.Build(upperQuat);
    lowerMat.Build(lowerQuat);

    const cdc::Vector3 upperArmVec = elbowPos - shoulderPos;
    const cdc::Vector3 forearmVec = wristPos - elbowPos;

    cdc::Matrix& shoulderMat = matrices[m_shoulderJoint];
    cdc::Mul3x3(&shoulderMat, upperMat, shoulderMat);

    cdc::Matrix& elbowMat = matrices[m_elbowJoint];
    elbowMat.SetTrans(shoulderPos + cdc::Mul3x3(upperMat, upperArmVec));
    cdc::Mul3x3(&elbowMat, lowerMat, elbowMat);
    cdc::Mul3x3(&elbowMat, upperMat, elbowMat);

    const cdc::Vector3 rotatedForearm = cdc::Mul3x3(upperMat, cdc::Mul3x3(lowerMat, forearmVec));
    matrices[m_wristJoint].SetTrans(MatrixTrans(elbowMat) + rotatedForearm);

    return 7;
}

// 0x00478a00 (PDB: 0x0047a370 / ?HelperCallbackImpl@HandLockController@@QAEIPAVAnimController@@PAUG2AnimSegValue@@PAVMatrix@cdc@@2@Z)
unsigned int HandLockController::HelperCallbackImpl(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix)
{
    (void)ac;

    if (m_interpWeight <= 0.0f) {
        return 7;
    }

    *segMatrix = m_instance->matrix[m_shoulderJoint];
    segMatrix->SetTrans(cdc::Mul3x4(*parentMatrix, segValue->trans));
    return 8;
}

// 0x00478ab0 (PDB: 0x0047a420 / ?SolveIK@FootLockController@@QAEXXZ)
void FootLockController::SolveIK()
{
    if (!m_active) {
        return;
    }

    const cdc::Matrix* matrices = m_instance->matrix;
    const cdc::Vector3 hipPos = MatrixTrans(matrices[m_hipJoint]);
    const cdc::Vector3 kneePos = MatrixTrans(matrices[m_kneeJoint]);
    const cdc::Vector3 anklePos = MatrixTrans(matrices[m_ankleJoint]);
    const cdc::Vector3 toePos = MatrixTrans(matrices[m_toeJoint]);

    cdc::Quat origAnkleQuat;
    origAnkleQuat.Build(matrices[m_ankleJoint]);
    cdc::Quat origToeQuat;
    origToeQuat.Build(matrices[m_toeJoint]);

    const cdc::Vector3 localToeVec = origAnkleQuat.Inverse().Rotate(toePos - anklePos);

    if (hipPos.z - 100.0f <= m_lockPoint.z) {
        m_active = false;
        return;
    }

    if (m_slide_lock) {
        cdc::Matrix invAnkleMat;
        cdc::Matrix relMat;
        cdc::OrthonormalInverse3x4(&invAnkleMat, matrices[m_ankleJoint]);
        cdc::Mul4x4(&relMat, matrices[m_toeJoint], invAnkleMat);
        if (!Reposition(toePos, MatrixCol1(relMat))) {
            m_lockPoint = toePos;
        }
    }

    if (m_minHipZAnglePtr != nullptr) {
        m_minHipZAngle = (*m_minHipZAnglePtr) * kDegToRad;
    }
    if (m_maxHipZAnglePtr != nullptr) {
        m_maxHipZAngle = (*m_maxHipZAnglePtr) * kDegToRad;
    }

    const cdc::Vector3 waistCol1 = MatrixCol1(matrices[m_waistJoint]);
    const float waistAngle = MATH3D_AngleFromVector(&waistCol1);
    const float deltaAngle = WrapAngle0ToTwoPi(m_lockAngle - waistAngle);
    const float clampedDelta = MATH3D_ClampAngleInRange(m_minHipZAngle, m_maxHipZAngle, deltaAngle);
    m_lockAngle = WrapAngle0ToTwoPi(clampedDelta + waistAngle);

    cdc::Vector3 footForward = MATH3D_ConvertAngleToVector(m_lockAngle);
    if (g_useToePitch) {
        footForward = footForward - m_lockNormal * cdc::Dot3(footForward, m_lockNormal);
    }
    footForward.Normalize3();

    cdc::Vector3 footRight;
    cdc::Cross3(&footRight, footForward, cdc::Vector3::Make(0.0f, 0.0f, 1.0f, 0.0f));
    footRight.Normalize3();

    cdc::Vector3 footUp;
    cdc::Cross3(&footUp, footRight, footForward);

    cdc::Matrix targetFootMat;
    targetFootMat.Identity();
    targetFootMat.col0 = cdc::Vector3::Make(footRight.x, footRight.y, footRight.z, targetFootMat.col0.w);
    targetFootMat.SetCol1(footForward);
    targetFootMat.SetCol2(footUp);

    cdc::Quat targetFootQuat;
    targetFootQuat.Build(targetFootMat);

    const cdc::Quat deltaFootQuat = targetFootQuat * origToeQuat.Inverse();
    const cdc::Vector3 desiredAnklePos = m_lockPoint - deltaFootQuat.Rotate(toePos - anklePos);
    const cdc::Quat targetAnkleQuat = deltaFootQuat * origAnkleQuat;

    cdc::Quat outHipQuat;
    cdc::Quat outKneeQuat;
    const cdc::Vector3 bendFallback = MatrixCol2(matrices[m_ankleJoint]);
    MATH3D_Solve2BoneIK(&hipPos, &kneePos, &anklePos, &desiredAnklePos, &bendFallback, &outHipQuat, &outKneeQuat);
    outHipQuat.Normalize();
    outKneeQuat.Normalize();

    const cdc::Vector3 newThighVec = outHipQuat.Rotate(kneePos - hipPos);
    const cdc::Vector3 newShinVec = outHipQuat.Rotate(outKneeQuat.Rotate(anklePos - kneePos));
    const cdc::Vector3 newKneePos = hipPos + newThighVec;
    const cdc::Vector3 newFootVec = targetAnkleQuat.Rotate(localToeVec);

    const float straightDot = cdc::Dot3(newThighVec.UnitVec3(), newShinVec.UnitVec3());
    const cdc::Vector3 solvedToePos = newKneePos + newShinVec + newFootVec;
    const float errDistSq = (solvedToePos - m_lockPoint).Len3Squared();

    if (errDistSq * m_interpWeight > m_autoUnlockTolerance || straightDot > kStraightLegDot) {
        m_slide_lock = false;
        m_active = false;
        return;
    }

    m_lastHipQuat = outHipQuat;
    m_lastKneeQuat = outKneeQuat;
    m_lastAnkleQuat = targetAnkleQuat * origAnkleQuat.Inverse();
    m_lastToeQuat = targetFootQuat * origToeQuat.Inverse();
}

// 0x00479250 (PDB: 0x0047abc0 / ?SetBones@FootLockController@@QAEXXZ)
void FootLockController::SetBones()
{
    if (m_interpWeight <= 0.0f) {
        return;
    }

    cdc::Matrix* matrices = m_instance->matrix;
    const cdc::Vector3 hipPos = MatrixTrans(matrices[m_hipJoint]);
    const cdc::Vector3 kneePos = MatrixTrans(matrices[m_kneeJoint]);
    const cdc::Vector3 anklePos = MatrixTrans(matrices[m_ankleJoint]);
    const cdc::Vector3 toePos = MatrixTrans(matrices[m_toeJoint]);

    cdc::Quat origHipQuat;
    origHipQuat.Build(matrices[m_hipJoint]);
    cdc::Quat origKneeQuat;
    origKneeQuat.Build(matrices[m_kneeJoint]);
    cdc::Quat origAnkleQuat;
    origAnkleQuat.Build(matrices[m_ankleJoint]);
    cdc::Quat origToeQuat;
    origToeQuat.Build(matrices[m_toeJoint]);

    cdc::Quat hipDelta = m_lastHipQuat;
    cdc::Quat kneeDelta = m_lastKneeQuat;
    cdc::Quat ankleDelta = m_lastAnkleQuat;
    cdc::Quat toeDelta = m_lastToeQuat;

    if (m_interpWeight < 1.0f) {
        const cdc::Quat qId = cdc::Quat::Identity();
        hipDelta = cdc::Slerp(qId, m_lastHipQuat, m_interpWeight);
        kneeDelta = cdc::Slerp(qId, m_lastKneeQuat, m_interpWeight);
        ankleDelta = cdc::Slerp(qId, m_lastAnkleQuat, m_interpWeight);
        toeDelta = cdc::Slerp(qId, m_lastToeQuat, m_interpWeight);
    }

    const cdc::Vector3 thighVec = kneePos - hipPos;
    const cdc::Vector3 shinVec = anklePos - kneePos;
    const cdc::Vector3 footVec = toePos - anklePos;

    const cdc::Quat finalHipQuat = hipDelta * origHipQuat;
    const cdc::Quat finalKneeQuat = (hipDelta * kneeDelta) * origKneeQuat;
    const cdc::Quat finalAnkleQuat = ankleDelta * origAnkleQuat;
    const cdc::Quat finalToeQuat = toeDelta * origToeQuat;

    const cdc::Vector3 newThighVec = (finalHipQuat * origHipQuat.Inverse()).Rotate(thighVec);
    const cdc::Vector3 newShinVec = (finalKneeQuat * origKneeQuat.Inverse()).Rotate(shinVec);
    const cdc::Vector3 newFootVec = (finalAnkleQuat * origAnkleQuat.Inverse()).Rotate(footVec);

    const cdc::Vector3 outKneePos = hipPos + newThighVec;
    const cdc::Vector3 outAnklePos = outKneePos + newShinVec;
    const cdc::Vector3 outToePos = outAnklePos + newFootVec;

    matrices[m_hipJoint].Build(finalHipQuat);
    matrices[m_hipJoint].SetTrans(hipPos);

    matrices[m_kneeJoint].Build(finalKneeQuat);
    matrices[m_kneeJoint].SetTrans(outKneePos);

    matrices[m_ankleJoint].Build(finalAnkleQuat);
    matrices[m_ankleJoint].SetTrans(outAnklePos);

    matrices[m_toeJoint].Build(finalToeQuat);
    matrices[m_toeJoint].SetTrans(outToePos);
}

// 0x00479810 (PDB: 0x0047b180 / ?Callback@HandLockController@@SAIPAVAnimController@@PAUG2AnimSegValue@@PAVMatrix@cdc@@2PAX@Z)
unsigned int HandLockController::Callback(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix,
    void* voiddata)
{
    return static_cast<HandLockController*>(voiddata)->CallbackImpl(ac, segValue, parentMatrix, segMatrix);
}

// 0x00479830 (PDB: 0x0047b1a0 / ?HelperCallback@HandLockController@@SAIPAVAnimController@@PAUG2AnimSegValue@@PAVMatrix@cdc@@2PAX@Z)
unsigned int HandLockController::HelperCallback(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix,
    void* voiddata)
{
    return static_cast<HandLockController*>(voiddata)->HelperCallbackImpl(ac, segValue, parentMatrix, segMatrix);
}

// 0x00479850 (PDB: 0x0047b1c0 / ?Apply@FootLockController@@QAEXXZ)
void FootLockController::Apply()
{
    if (m_slide_lock) {
        m_interpWeight = 0.99f;
    }
    if (m_active || m_interpWeight > 0.0f) {
        if (g_useIK) {
            SolveIK();
            SetBones();
        }
        AdvanceInterp();
    }
    m_skipSecondaryIkThisFrame = false;
}

// 0x004798b0 (PDB: 0x0047b220 / ?Activate@HandLockController@@UAEXXZ)
void HandLockController::Activate()
{
    if (IsActive()) {
        return;
    }

    m_active = true;
    AnimProcessor* animProc = m_instance->animComponent->mAnimProcessor;

    if (!animProc->IsControllerActive(m_wristJoint, 0x80)) {
        animProc->EnableController(m_wristJoint, 0x80);
        animProc->SetController_Function(m_wristJoint, 0x80, &HandLockController::Callback, this);
    }

    if (!animProc->IsControllerActive(m_shoulderHelperJoint, 0x80)) {
        animProc->EnableController(m_shoulderHelperJoint, 0x80);
        animProc->SetController_Function(m_shoulderHelperJoint, 0x80, &HandLockController::HelperCallback, this);
    }
}
