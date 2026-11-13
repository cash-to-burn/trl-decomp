#pragma once

#ifndef GAME_ANIM_RNODECONTROLLER_H
#define GAME_ANIM_RNODECONTROLLER_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/anim/anim.h"
#include <cstddef>
#include <cstdint>

struct Instance;

// Size: 0xd0 (208 bytes, TI: 0x6a5f)
class alignas(16) RNodeController {
public:
    RNodeController(Instance* instance, unsigned int rNodeJoint, unsigned int tNodeJoint); // 0x004799d0 (PDB: 0x0047b340)

    void SetSlewTarget(bool left, bool right);        // 0x00479a80 (PDB: 0x0047b3f0)
    void DeactivateSlewing();                         // 0x00479ab0 (PDB: 0x0047b420)
    void Deactivate(bool lock);                       // 0x00479af0 (PDB: 0x0047b460)
    void Relocate(const cdc::Vector3& offset);        // 0x00479b10 (PDB: 0x0047b480)
    unsigned int CallbackImpl(
        AnimController* ac,
        G2AnimSegValue* segValue,
        cdc::Matrix* parentMatrix,
        cdc::Matrix* segMatrix);                      // 0x00479b40 (PDB: 0x0047b4b0)
    static unsigned int Callback(
        AnimController* ac,
        G2AnimSegValue* segValue,
        cdc::Matrix* parentMatrix,
        cdc::Matrix* segMatrix,
        void* voiddata);                              // 0x0047a360 (PDB: 0x0047bcd0)
    void Activate(bool unlock);                       // 0x0047a380 (PDB: 0x0047bcf0)
    void SetLeaning(float amount);                    // 0x0047a460 (PDB: 0x0047bdd0)
    void ActivateSlewing();                           // 0x0047a4b0 (PDB: 0x0047be20)

    bool IsActive() const { return m_active; }
    bool IsSlewing() const { return m_slewingActive; }
    cdc::Vector3 GetTranslation() const { return m_translationTotal; }

public:
    /* 0x00 */ bool m_active;
    /* 0x01 */ bool m_locked;
    /* 0x02 */ bool m_slewingActive;
    /* 0x03 */ bool m_recently_started;
    /* 0x04 */ uint8_t m_rNodeJoint;
    /* 0x05 */ uint8_t m_tNodeJoint;
    /* 0x06 */ uint8_t m_slewDirection;
    /* 0x07 */ uint8_t pad07;
    /* 0x08 */ Instance* m_instance;
    /* 0x0c */ uint32_t pad0c;
    /* 0x10 */ cdc::Vector3 m_translationTotal;
    /* 0x20 */ cdc::Vector3 m_transSlewing;
    /* 0x30 */ cdc::Vector3 m_transLeaning;
    /* 0x40 */ float m_leanAmount;
    /* 0x44 */ float m_slewAmount;
    /* 0x48 */ float m_interpWeight;
    /* 0x4c */ float m_interpSpeed;
    /* 0x50 */ float m_interpSlew;
    /* 0x54 */ uint8_t pad54[0x0c];
    /* 0x60 */ cdc::Quat m_lastRNodeQuat;
    /* 0x70 */ cdc::Matrix m_lastRNodeMatrix;
    /* 0xb0 */ cdc::Vector3 m_lastRNodePos;
    /* 0xc0 */ cdc::Vector3 m_slewTargetPos;
};
static_assert(sizeof(RNodeController) == 0xd0, "RNodeController size mismatch");
static_assert(offsetof(RNodeController, m_instance) == 0x08, "RNodeController::m_instance offset mismatch");
static_assert(offsetof(RNodeController, m_translationTotal) == 0x10, "RNodeController::m_translationTotal offset mismatch");
static_assert(offsetof(RNodeController, m_leanAmount) == 0x40, "RNodeController::m_leanAmount offset mismatch");
static_assert(offsetof(RNodeController, m_lastRNodeQuat) == 0x60, "RNodeController::m_lastRNodeQuat offset mismatch");
static_assert(offsetof(RNodeController, m_lastRNodeMatrix) == 0x70, "RNodeController::m_lastRNodeMatrix offset mismatch");
static_assert(offsetof(RNodeController, m_lastRNodePos) == 0xb0, "RNodeController::m_lastRNodePos offset mismatch");
static_assert(offsetof(RNodeController, m_slewTargetPos) == 0xc0, "RNodeController::m_slewTargetPos offset mismatch");

#endif // GAME_ANIM_RNODECONTROLLER_H
