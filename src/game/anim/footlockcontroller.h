#pragma once

#ifndef GAME_ANIM_FOOTLOCKCONTROLLER_H
#define GAME_ANIM_FOOTLOCKCONTROLLER_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/anim/anim.h"
#include <cstddef>
#include <cstdint>

struct Instance;

// Size: 0x04 (4 bytes, TI: 0x63cf)
class LimbLockController {
public:
    virtual void Activate() = 0;
    virtual void Deactivate() = 0;
    virtual bool IsActive() const = 0;
};
static_assert(sizeof(LimbLockController) == 0x04, "LimbLockController size mismatch");

// Size: 0x110 (272 bytes, TI: 0x69bd)
class FootLockController : public LimbLockController {
public:
    FootLockController(
        Instance* instance,
        unsigned int waistJoint,
        unsigned int hipJoint,
        unsigned int kneeJoint,
        unsigned int ankleJoint,
        unsigned int toeJoint); // 0x00477fd0 (PDB: 0x00479940)

    virtual void Activate() override;      // 0x004783f0 (PDB: 0x00479d60)
    virtual void Deactivate() override;    // 0x00477bf0 (PDB: 0x00479530)
    virtual bool IsActive() const override; // 0x00477c00 (PDB: 0x00479540)

    // 0x00479550 PDB (inlined in retail)
    cdc::Vector3 GetLockPoint() const { return m_lockPoint; }

    void SetSlideLock(bool on);                                              // 0x00477c10 (PDB: 0x00479580)
    bool Reposition(const cdc::Vector3& p0, const cdc::Vector3& p1);         // 0x00477c30 (PDB: 0x004795a0)
    void SetHipLimits(const float* min, const float* max);                   // 0x00477d80 (PDB: 0x004796f0)
    void DeactivateWithoutInterp();                                          // 0x00477da0 (PDB: 0x00479710)
    void SkipSecondaryIkThisFrame();                                         // 0x00477e10 (PDB: 0x00479780)
    void RelocateLockPoint(const cdc::Vector3& offset);                      // 0x00477e20 (PDB: 0x00479790)
    void AdvanceInterp();                                                    // 0x00477e40 (PDB: 0x004797b0)
    bool FindLockPoint(cdc::Vector3* lockPoint, float zOffsetUp, float zOffsetDown); // 0x004780f0 (PDB: 0x00479a60)
    void SolveIK();                                                          // 0x00478ab0 (PDB: 0x0047a420)
    void SetBones();                                                         // 0x00479250 (PDB: 0x0047abc0)
    void Apply();                                                            // 0x00479850 (PDB: 0x0047b1c0)

public:
    /* 0x010 */ cdc::Matrix m_lockOrientation;
    /* 0x050 */ cdc::Vector3 m_lockPoint;
    /* 0x060 */ cdc::Vector3 m_lockNormal;
    /* 0x070 */ float m_lockAngle;
    /* 0x080 */ cdc::Vector3 m_lastBrokenLockPoint;
    /* 0x090 */ cdc::Quat m_lastHipQuat;
    /* 0x0a0 */ cdc::Quat m_lastKneeQuat;
    /* 0x0b0 */ cdc::Quat m_lastAnkleQuat;
    /* 0x0c0 */ cdc::Quat m_lastToeQuat;
    /* 0x0d0 */ bool m_active;
    /* 0x0d1 */ bool m_slide_lock;
    /* 0x0d4 */ float m_interpWeight;
    /* 0x0d8 */ Instance* m_instance;
    /* 0x0dc */ uint8_t m_waistJoint;
    /* 0x0dd */ uint8_t m_hipJoint;
    /* 0x0de */ uint8_t m_kneeJoint;
    /* 0x0df */ uint8_t m_ankleJoint;
    /* 0x0e0 */ uint8_t m_toeJoint;
    /* 0x0e4 */ float m_minHipZAngle;
    /* 0x0e8 */ float m_maxHipZAngle;
    /* 0x0ec */ const float* m_minHipZAnglePtr;
    /* 0x0f0 */ const float* m_maxHipZAnglePtr;
    /* 0x0f4 */ float m_interpSpeed;
    /* 0x0f8 */ float m_autoUnlockTolerance;
    /* 0x0fc */ float m_minimumSlopeNormalZ;
    /* 0x100 */ float m_zProbeOffsetUp;
    /* 0x104 */ float m_zProbeOffsetDown;
    /* 0x108 */ float m_toeHeight;
    /* 0x10c */ bool m_useIK;
    /* 0x10d */ bool m_useSecondaryIK;
    /* 0x10e */ bool m_useToePitch;
    /* 0x10f */ bool m_skipSecondaryIkThisFrame;
};
static_assert(sizeof(FootLockController) == 0x110, "FootLockController size mismatch");
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(FootLockController, m_lockOrientation) == 0x010, "FootLockController::m_lockOrientation offset mismatch");
static_assert(offsetof(FootLockController, m_lastBrokenLockPoint) == 0x080, "FootLockController::m_lastBrokenLockPoint offset mismatch");
static_assert(offsetof(FootLockController, m_active) == 0x0d0, "FootLockController::m_active offset mismatch");
static_assert(offsetof(FootLockController, m_instance) == 0x0d8, "FootLockController::m_instance offset mismatch");
static_assert(offsetof(FootLockController, m_skipSecondaryIkThisFrame) == 0x10f, "FootLockController::m_skipSecondaryIkThisFrame offset mismatch");
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// Size: 0x100 (256 bytes, TI: 0x63ed)
class HandLockController : public LimbLockController {
public:
    typedef cdc::Vector3 (*LockPointCallback)(void* data);

    HandLockController(
        Instance* instance,
        unsigned int shoulderHelperJoint,
        unsigned int shoulderJoint,
        unsigned int elbowJoint,
        unsigned int wristJoint,
        unsigned int grabJoint); // 0x00478450 (PDB: 0x00479dc0)

    virtual void Activate() override;      // 0x004798b0 (PDB: 0x0047b220)
    virtual void Deactivate() override;    // 0x00477dc0 (PDB: 0x00479730)
    virtual bool IsActive() const override; // 0x00477de0 (PDB: 0x00479750)

    void SetLockPointCallback(LockPointCallback function, void* data); // 0x00477d80 (PDB: 0x004796f0)
    void SetLockPoint(const cdc::Vector3& lockPoint);                  // 0x00477df0 (PDB: 0x00479760)

    unsigned int CallbackImpl(AnimController* ac, G2AnimSegValue* segValue, cdc::Matrix* parentMatrix, cdc::Matrix* segMatrix);       // 0x00478540 (PDB: 0x00479eb0)
    unsigned int HelperCallbackImpl(AnimController* ac, G2AnimSegValue* segValue, cdc::Matrix* parentMatrix, cdc::Matrix* segMatrix); // 0x00478a00 (PDB: 0x0047a370)

    static unsigned int Callback(AnimController* ac, G2AnimSegValue* segValue, cdc::Matrix* parentMatrix, cdc::Matrix* segMatrix, void* voiddata);       // 0x00479810 (PDB: 0x0047b180)
    static unsigned int HelperCallback(AnimController* ac, G2AnimSegValue* segValue, cdc::Matrix* parentMatrix, cdc::Matrix* segMatrix, void* voiddata); // 0x00479830 (PDB: 0x0047b1a0)

public:
    /* 0x010 */ cdc::Matrix m_lockOrientation;
    /* 0x050 */ cdc::Vector3 m_lockPoint;
    /* 0x060 */ cdc::Vector3 m_lockNormal;
    /* 0x070 */ float m_lockAngle;
    /* 0x080 */ cdc::Quat m_lastShoulderQuat;
    /* 0x090 */ cdc::Quat m_lastElbowQuat;
    /* 0x0a0 */ cdc::Quat m_lastWristQuat;
    /* 0x0b0 */ bool m_active;
    /* 0x0b4 */ float m_interpWeight;
    /* 0x0b8 */ Instance* m_instance;
    /* 0x0bc */ uint8_t m_shoulderHelperJoint;
    /* 0x0bd */ uint8_t m_shoulderJoint;
    /* 0x0be */ uint8_t m_elbowJoint;
    /* 0x0bf */ uint8_t m_wristJoint;
    /* 0x0c0 */ uint8_t m_grabJoint;
    /* 0x0c4 */ float m_minHipZAngle;
    /* 0x0c8 */ float m_maxHipZAngle;
    /* 0x0cc */ const float* m_minHipZAnglePtr;
    /* 0x0d0 */ const float* m_maxHipZAnglePtr;
    /* 0x0d4 */ float m_interpSpeed;
    /* 0x0d8 */ float m_autoUnlockTolerance;
    /* 0x0dc */ float m_minimumSlopeNormalZ;
    /* 0x0e0 */ float m_zProbeOffsetUp;
    /* 0x0e4 */ float m_zProbeOffsetDown;
    /* 0x0e8 */ float m_toeHeight;
    /* 0x0ec */ LockPointCallback m_lockPointCallback;
    /* 0x0f0 */ void* m_lockPointCallbackData;
};
static_assert(sizeof(HandLockController) == 0x100, "HandLockController size mismatch");
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(HandLockController, m_lockOrientation) == 0x010, "HandLockController::m_lockOrientation offset mismatch");
static_assert(offsetof(HandLockController, m_active) == 0x0b0, "HandLockController::m_active offset mismatch");
static_assert(offsetof(HandLockController, m_instance) == 0x0b8, "HandLockController::m_instance offset mismatch");
static_assert(offsetof(HandLockController, m_lockPointCallback) == 0x0ec, "HandLockController::m_lockPointCallback offset mismatch");
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern bool g_useSecondaryIK; // 0x010fc358 (PDB: 0x010ff558 / seg3:0x1ed158)
extern bool g_useToePitch;    // 0x010fc359 (PDB: 0x010ff559 / seg3:0x1ed159)

#endif // GAME_ANIM_FOOTLOCKCONTROLLER_H
