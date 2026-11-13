#include "game/anim/anim.h"
#include "game/g2/instncg2.h"
#include "game/math3d.h"
#include "game/obtable.h"
#include "game/player/player.h"
#include <cmath>
#include <cstdint>

AnimPattern_RunAndJump gAnimPattern_RunAndJump;                   // 0x00f127e8 (PDB seg 3:0x00002208)
AnimPattern_RunToSlide gAnimPattern_RunToSlide;                   // 0x00f127ec (PDB seg 3:0x0000220c)
AnimPattern_SlopeJumpToLedge gAnimPattern_SlopeJumpToLedge;       // 0x00f127f0 (PDB seg 3:0x00002210)
AnimPattern_SlopeBackJumpToLedge gAnimPattern_SlopeBackJumpToLedge; // 0x00f127f4 (PDB seg 3:0x00002214)

namespace {

inline const AnimFragmentTable& GetPlayerAnimTable()
{
    return static_cast<PlayerProp*>(s_playerInstance->data)->oldData.animFragmentTable;
}

inline Anim MakePatternAnim(
    int16_t animId,
    uint16_t flags,
    uint16_t targetFrame = 0xffffu,
    uint16_t interpFrames = 0xffffu,
    float speed = 1.0f)
{
    Anim anim = {};
    anim.mAnimId = static_cast<uint16_t>(animId);
    anim.mFlags = flags;
    anim.mRepeatCount = 1;
    anim.mTargetFrame = targetFrame;
    anim.mInterpFrames = interpFrames;
    anim.mSpeed = speed;
    return anim;
}

bool PlaySlopeJumpToLedge(AnimProcessor* ap, int16_t blendId)
{
    const float leadTime = ap->GetFloat(0);
    Instance* inst = ap->mAnimComponent->mInstance;
    const float speed2D = std::sqrt(inst->vel.x * inst->vel.x + inst->vel.y * inst->vel.y);
    float distRemaining = leadTime * speed2D - 150.0f;
    if (distRemaining < 1.0f) {
        distRemaining = 1.0f;
    }

    const int16_t keylistId = inst->object->baseData->animBlend[blendId].animFragment[0];
    AnimFragment* keylist = G2Instance_GetKeylist(inst, keylistId);
    const int duration = static_cast<int>(keylist->mKeyCount) * static_cast<int>(keylist->mTimePerKey);
    const float ticksRemaining = (distRemaining / speed2D) * 100.0f;

    AnimSequence seq;
    seq << MakePatternAnim(blendId, 0, 0, 1, static_cast<float>(duration) / ticksRemaining);
    ap->PushBack(seq);
    ap->TakeControl();
    return true;
}

} // namespace

// 0x0041cdd0 (PDB: 0x0041bda0)
bool AnimPattern_RunAndJump::RunToEdge(AnimProcessor* ap, const cdc::Vector3& pos, const cdc::Vector3& rot)
{
    const AnimFragmentTable& table = GetPlayerAnimTable();
    AnimSequenceList seqList;
    const cdc::Euler targetRot = MATH3D_EulerFromPosToPos(&pos, &rot);

    AnimSequence seq;
    seq << MakePatternAnim(table.step, 3)
        << MakePatternAnim(table.shortCompress, 0)
        << MakePatternAnim(table.extend, 0);

    if (seq.SolveTranslationRotation(ap->mAnimComponent->mInstance, pos, targetRot)) {
        seqList.Add(&seq);
    }

    if (seqList.Size() == 0) {
        return false;
    }

    ap->PushBack(*ANIM_MinimumErrorSequence(seqList));
    return true;
}

// 0x0041cf60 (PDB: 0x0041bf30)
bool AnimPattern_RunAndJump::JumpToEdge(AnimProcessor* ap, const cdc::Vector3& pos, float zRot)
{
    (void)pos;
    (void)zRot;

    const AnimFragmentTable& table = GetPlayerAnimTable();
    AnimSequence seq;
    seq << MakePatternAnim(table.flail, 1)
        << MakePatternAnim(table.absorb, 0);
    ap->PushBack(seq);
    return true;
}

// 0x0041d020 (PDB: 0x0041bff0)
bool AnimPattern_RunAndJump::Entry(AnimProcessor* ap)
{
    ap->SetMarker();
    if (RunToEdge(ap, ap->GetVector(0), ap->GetVector(1))) {
        ap->CriticalOn();
        if (JumpToEdge(ap, ap->GetVector(1), ap->GetFloat(0))) {
            ap->CriticalOff();
            ap->TakeControl();
            return true;
        }
    }
    ap->PopBackToMarker();
    return false;
}

// 0x0041d0c0 (PDB: 0x0041c090)
bool AnimPattern_RunToSlide::Entry(AnimProcessor* ap)
{
    const AnimFragmentTable& table = GetPlayerAnimTable();
    AnimFragment* keylist = ap->mSection->mKeylist;

    ap->SetMarker();
    AnimSequence seq;

    const int duration = static_cast<int>(keylist->mKeyCount) * static_cast<int>(keylist->mTimePerKey);
    int stepIdx = static_cast<int>((ap->mSection->mElapsedTime * 20.0f) / static_cast<float>(duration) + 0.5f);
    if (stepIdx < 0) {
        stepIdx = 0;
    }

    if (stepIdx < 3) {
        seq << MakePatternAnim(table.slide, 0, static_cast<uint16_t>(stepIdx), 1);
    } else if (stepIdx < 9) {
        seq << MakePatternAnim(table.shortRightStep, 0, 0, static_cast<uint16_t>(9 - stepIdx))
            << MakePatternAnim(table.slide, 0);
    } else if (stepIdx < 12) {
        const uint16_t startFrame = static_cast<uint16_t>(static_cast<float>(stepIdx - 9) * 0.5f);
        seq << MakePatternAnim(table.shortRightStep, 0, startFrame, 1)
            << MakePatternAnim(table.slide, 0);
    } else {
        if (stepIdx >= 18) {
            stepIdx = 18;
        }
        seq << MakePatternAnim(table.slide, 0, 0, static_cast<uint16_t>(19 - stepIdx));
    }

    ap->PushBack(seq);
    ap->TakeControl();
    return true;
}

// 0x0041d2f0 (PDB: 0x0041c2c0)
bool AnimPattern_SlopeJumpToLedge::Entry(AnimProcessor* ap)
{
    return PlaySlopeJumpToLedge(ap, GetPlayerAnimTable().slopeJump2);
}

// 0x0041d470 (PDB: 0x0041c440)
bool AnimPattern_SlopeBackJumpToLedge::Entry(AnimProcessor* ap)
{
    return PlaySlopeJumpToLedge(ap, GetPlayerAnimTable().slopeBackJump2);
}
