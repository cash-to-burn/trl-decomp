#include "game/anim/rnodecontroller.h"

#include "game/anim/footlockcontroller.h"
#include "game/instance.h"
#include "game/player/player.h"
#include <algorithm>

// 0x004799d0 (PDB: 0x0047b340)
RNodeController::RNodeController(Instance* instance, unsigned int rNodeJoint, unsigned int tNodeJoint)
{
    m_instance = instance;
    m_rNodeJoint = static_cast<uint8_t>(rNodeJoint);
    m_tNodeJoint = static_cast<uint8_t>(tNodeJoint);
    m_translationTotal = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_transSlewing = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_transLeaning = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_leanAmount = 0.0f;
    m_slewAmount = 0.0f;
    m_interpSpeed = 100.0f;
    m_active = false;
    m_locked = false;
    m_slewingActive = false;
    m_recently_started = true;
    m_slewDirection = 0;
    m_slewTargetPos = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_interpWeight = 0.0f;
    m_interpSlew = 0.0f;
    m_lastRNodeQuat = cdc::Quat::Identity();
    m_lastRNodePos = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
}

// 0x00479a80 (PDB: 0x0047b3f0)
void RNodeController::SetSlewTarget(bool left, bool right)
{
    if (left && !right) {
        m_slewDirection = 1;
    } else if (!left && right) {
        m_slewDirection = 2;
    } else {
        m_slewDirection = 0;
    }
}

// 0x00479ab0 (PDB: 0x0047b420)
void RNodeController::DeactivateSlewing()
{
    m_slewingActive = false;
    m_slewDirection = 0;
}

// 0x00479af0 (PDB: 0x0047b460)
void RNodeController::Deactivate(bool lock)
{
    if (m_active) {
        m_active = false;
        m_locked = lock;
        m_slewingActive = false;
    }
}

// 0x00479b10 (PDB: 0x0047b480)
void RNodeController::Relocate(const cdc::Vector3& offset)
{
    m_lastRNodePos += offset;
}

// 0x00479b40 (PDB: 0x0047b4b0)
unsigned int RNodeController::CallbackImpl(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix)
{
    (void)ac;

    m_interpSpeed = 0.2f;
    if (m_active) {
        if (m_interpWeight < 1.0f) {
            m_interpWeight = std::min(1.0f, m_interpWeight + s_timeScale * 0.2f);
        }
    } else {
        m_interpWeight = std::max(0.0f, m_interpWeight - s_timeScale * 0.2f);
    }

    if (m_interpWeight == 0.0f) {
        return 7;
    }

    const cdc::Vector3 tNodePos = static_cast<const cdc::Vector3&>(m_instance->matrix[m_tNodeJoint].col3);
    cdc::Quat curRNodeQuat;
    curRNodeQuat.Build(*segMatrix);

    const cdc::Vector3 baseDeltaPos = cdc::Mul3x3(m_instance->matrix[m_tNodeJoint], segValue->trans);
    cdc::Vector3 deltaPos = baseDeltaPos;
    const cdc::Vector3 curWorldPos = tNodePos + deltaPos;

    if (m_recently_started) {
        m_lastRNodePos = curWorldPos;
        m_interpWeight = 0.0f;
        m_lastRNodeQuat = curRNodeQuat;
        m_translationTotal = deltaPos;
        m_recently_started = false;
    }

    if (m_active) {
        m_translationTotal = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

        if (m_leanAmount < -0.01f || m_leanAmount > 0.01f) {
            const cdc::Vector3 yAxis = cdc::Vector3::Make(0.0f, 1.0f, 0.0f, 0.0f);
            cdc::Quat leanQuat;
            leanQuat.Build(yAxis, m_leanAmount);

            const cdc::Vector3 rotatedDelta = leanQuat.Rotate(deltaPos);
            m_transLeaning = rotatedDelta - deltaPos;
            m_transLeaning.x = 0.0f;
            m_transLeaning.y = 0.0f;
            deltaPos = deltaPos + m_transLeaning;

            curRNodeQuat = cdc::QuatMul(curRNodeQuat, leanQuat);
        } else {
            m_transLeaning = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        }

        if (m_slewingActive && m_slewDirection > 0) {
            const cdc::Vector3 rightAxis = static_cast<const cdc::Vector3&>(m_instance->matrix[0].col0);
            m_interpSlew = std::min(1.0f, s_timeScale * playerData->oldData.InterpoBlendSlewAdjustRate);

            FootLockController* footLock = nullptr;
            if (m_slewDirection == 1) {
                footLock = gPlayer->footLockControllerLeft;
            } else if (m_slewDirection == 2) {
                footLock = gPlayer->footLockControllerRight;
            }

            float targetSlew = 0.0f;
            if (footLock != nullptr && footLock->IsActive()) {
                m_slewTargetPos = footLock->GetLockPoint();
                const float dot = cdc::Dot3(m_slewTargetPos - tNodePos, rightAxis);
                const float radius = playerData->oldData.InterpoBlendSlewRadius;
                const float exag = playerData->oldData.InterpoBlendSlewExaggeration;
                if (m_slewDirection == 1) {
                    targetSlew = std::min(0.0f, (dot - radius) * exag);
                } else {
                    targetSlew = std::max(0.0f, (dot + radius) * exag);
                }
            } else {
                m_slewDirection = 0;
            }

            m_slewAmount = targetSlew * m_interpSlew + m_slewAmount * (1.0f - m_interpSlew);
            m_transSlewing = rightAxis * m_slewAmount;
        } else {
            if (m_slewAmount > -1.0f && m_slewAmount < 1.0f) {
                m_interpSlew = 0.0f;
                m_slewAmount = 0.0f;
                m_transSlewing = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            } else {
                const cdc::Vector3 rightAxis = static_cast<const cdc::Vector3&>(m_instance->matrix[0].col0);
                m_interpSlew = std::min(0.33f, s_timeScale * m_interpSpeed);
                m_slewAmount *= (1.0f - m_interpSlew);
                m_transSlewing = rightAxis * m_slewAmount;
            }
        }

        deltaPos = deltaPos + m_transSlewing;
        curRNodeQuat.Normalize();
        m_lastRNodeQuat = curRNodeQuat;
        m_lastRNodeMatrix.Build(curRNodeQuat);
        m_translationTotal = deltaPos;
        m_lastRNodePos = tNodePos + deltaPos;
    } else {
        m_translationTotal = deltaPos;
        m_lastRNodePos = curWorldPos;
        m_lastRNodeQuat = curRNodeQuat;
    }

    if (m_interpWeight < 1.0f) {
        cdc::Quat segQuat;
        segQuat.Build(*segMatrix);

        const cdc::Quat lastQuat = m_lastRNodeQuat;
        const float dot4 =
            segQuat.x * lastQuat.x +
            segQuat.y * lastQuat.y +
            segQuat.z * lastQuat.z +
            segQuat.w * lastQuat.w;
        if (dot4 < 0.0f) {
            segQuat.x = -segQuat.x;
            segQuat.y = -segQuat.y;
            segQuat.z = -segQuat.z;
            segQuat.w = -segQuat.w;
        }

        const float w = m_interpWeight;
        const float invW = 1.0f - w;
        cdc::Quat blendedQuat;
        blendedQuat.x = lastQuat.x * w + segQuat.x * invW;
        blendedQuat.y = lastQuat.y * w + segQuat.y * invW;
        blendedQuat.z = lastQuat.z * w + segQuat.z * invW;
        blendedQuat.w = lastQuat.w * w + segQuat.w * invW;
        blendedQuat.Normalize();

        const cdc::Vector3 blendedPos = tNodePos + baseDeltaPos * invW + m_translationTotal * w;

        m_lastRNodeMatrix.Build(blendedQuat);
        cdc::Mul3x3(segMatrix, *parentMatrix, m_lastRNodeMatrix);
        segMatrix->SetTrans(blendedPos);
    } else {
        m_lastRNodeMatrix.Build(curRNodeQuat);
        cdc::Mul3x3(segMatrix, *parentMatrix, m_lastRNodeMatrix);
        segMatrix->SetTrans(tNodePos + m_translationTotal);
    }

    m_leanAmount = 0.0f;
    m_transLeaning = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_transSlewing = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    return 8;
}

// 0x0047a360 (PDB: 0x0047bcd0)
unsigned int RNodeController::Callback(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix,
    void* voiddata)
{
    return static_cast<RNodeController*>(voiddata)->CallbackImpl(ac, segValue, parentMatrix, segMatrix);
}

// 0x0047a380 (PDB: 0x0047bcf0)
void RNodeController::Activate(bool unlock)
{
    if (m_active) {
        return;
    }
    if (unlock) {
        m_locked = false;
    }
    if (m_locked) {
        return;
    }

    m_translationTotal = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_transSlewing = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_transLeaning = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_leanAmount = 0.0f;
    m_slewAmount = 0.0f;
    m_active = true;
    m_slewingActive = false;
    m_recently_started = true;
    m_slewDirection = 0;
    m_slewTargetPos = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    m_interpWeight = 0.0f;
    m_interpSlew = 0.0f;
    m_lastRNodeQuat = cdc::Quat::Identity();
    m_lastRNodePos = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    AnimProcessor* animProcessor = m_instance->animComponent->animProcessor;
    if (!animProcessor->IsControllerActive(m_rNodeJoint, 0x80)) {
        animProcessor->EnableController(m_rNodeJoint, 0x80);
    }
    animProcessor->SetController_Function(m_rNodeJoint, 0x80, Callback, this);
}

// 0x0047a460 (PDB: 0x0047bdd0)
void RNodeController::SetLeaning(float amount)
{
    if (!m_active && amount != 0.0f) {
        Activate(false);
    }
    m_leanAmount = amount;
}

// 0x0047a4b0 (PDB: 0x0047be20)
void RNodeController::ActivateSlewing()
{
    if (!m_active) {
        Activate(false);
    }
    m_slewingActive = true;
}
