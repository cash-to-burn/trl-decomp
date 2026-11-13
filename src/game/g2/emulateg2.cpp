#include "game/g2/emulateg2.h"
#include "game/g2/instncg2.h"
#include "game/instance.h"
#include "game/obtable.h"
#include "game/player/player.h"

#include <cstdint>

extern int INSTANCE_DefaultAnimCallback(
    AnimProcessor* ap,
    int sectionID,
    G2AnimCallbackMsg msg,
    int messageDataA,
    int messageDataB,
    void* callbackData); // 0x00456cc0 (PDB: 0x00458340)

G2AnimAlphaTable G2AlphaTables[8] = {}; // 0x010fe7c0 (PDB: 0x011019c0)

namespace {

AnimProcessor* GetAnimProcessor(BaseInstance* instance)
{
    return instance->animComponent->mAnimProcessor;
}

AnimFragmentProcessor* GetAnimSection(BaseInstance* instance, int sectionIdx)
{
    return &instance->animComponent->mAnimProcessor[0].mSection[sectionIdx];
}

int ComputeAnimLengthFrames(const AnimFragment* keylist)
{
    const int timePerKey = keylist->mTimePerKey;
    return (static_cast<int>(keylist->mKeyCount) * (timePerKey + 1) - 1) / timePerKey;
}

void ApplySectionMode(AnimFragmentProcessor* sec, int mode)
{
    if (mode == 0) {
        sec->mFlags |= 0x1;
        return;
    }

    sec->mFlags &= ~0x1;
    if (mode == 2) {
        sec->SetLooping();
    } else {
        sec->mFlags &= ~0x2;
    }
}

void SwitchSectionAnimation(
    Instance* instance,
    Instance* host,
    int newAnim,
    int newFrame,
    int frames,
    int currentMode,
    bool hosted)
{
    AnimFragmentProcessor* sec = GetAnimSection(instance, 0);
    AnimFragment* keylist = G2Instance_GetKeylist(host, newAnim);
    AnimFxHeader* fx = host->object->animFXList[newAnim];

    sec->SetAlphaTable(nullptr);
    const float duration = static_cast<float>(static_cast<int>(keylist->mTimePerKey) * frames);
    sec->InterpToKeylistFrame(keylist, newAnim, newFrame, duration, host->object, fx);

    ApplySectionMode(sec, currentMode);
    if (hosted) {
        sec->mFlags |= 0x20;
    } else {
        sec->mFlags &= ~0x20;
    }
}

} // namespace

// 0x004de560 (PDB: 0x004e0090)
void G2EmulationInit()
{
    G2AlphaTables[1].size = 3;
    G2AlphaTables[1].data[0] = 0.0f;
    G2AlphaTables[1].data[1] = 0.0625f;
    G2AlphaTables[1].data[2] = 0.25f;
    G2AlphaTables[1].data[3] = 1.0f;

    G2AlphaTables[2].size = 3;
    G2AlphaTables[2].data[0] = 0.0f;
    G2AlphaTables[2].data[1] = 0.75f;
    G2AlphaTables[2].data[2] = 0.9375f;
    G2AlphaTables[2].data[3] = 1.0f;

    G2AlphaTables[3].size = 1;
    G2AlphaTables[3].data[0] = 1.0f;

    G2AlphaTables[4].size = 6;
    G2AlphaTables[4].data[0] = 0.0f;
    G2AlphaTables[4].data[1] = 0.1f;
    G2AlphaTables[4].data[2] = 0.25f;
    G2AlphaTables[4].data[3] = 0.75f;
    G2AlphaTables[4].data[4] = 0.9f;
    G2AlphaTables[4].data[5] = 1.0f;

    G2AlphaTables[5].size = 3;
    G2AlphaTables[5].data[0] = 0.0f;
    G2AlphaTables[5].data[1] = 0.03125f;
    G2AlphaTables[5].data[2] = 0.125f;
    G2AlphaTables[5].data[3] = 1.0f;

    G2AlphaTables[6].size = 3;
    G2AlphaTables[6].data[0] = 0.0f;
    G2AlphaTables[6].data[1] = 0.875f;
    G2AlphaTables[6].data[2] = 0.96875f;
    G2AlphaTables[6].data[3] = 1.0f;
}

// 0x004de670 (PDB: 0x004e01b0)
void G2EmulationInstanceToInstanceSwitchAnimation(
    Instance* instance,
    Instance* host,
    int NewAnim,
    int NewFrame,
    int Frames,
    int CurrentMode)
{
    SwitchSectionAnimation(instance, host, NewAnim, NewFrame, Frames, CurrentMode, true);
}

// 0x004de730 (PDB: 0x004e0270)
void G2EmulationInstanceSwitchAnimation(
    Instance* instance,
    int NewAnim,
    int NewFrame,
    int Frames,
    int CurrentMode)
{
    SwitchSectionAnimation(instance, instance, NewAnim, NewFrame, Frames, CurrentMode, false);
}

// 0x004de7e0 (PDB: 0x004e0320)
void G2EmulationInstanceToInstanceSwitchAnimationCharacter(
    Instance* instance,
    Instance* host,
    int NewAnim,
    int NewFrame,
    int Frames,
    int CurrentMode)
{
    AnimFragment* keylist = G2Instance_GetKeylist(host, NewAnim);
    AnimFxHeader* fx = host->object->animFXList[NewAnim];
    AnimProcessor* ap = GetAnimProcessor(instance);
    AnimFragmentProcessor* sec = &ap->mSection[0];

    sec->mFlags |= 0x20;
    sec->mAlarmFlags &= ~0x200u;
    ap->SetAlphaTable(nullptr);

    const float duration = static_cast<float>(static_cast<int>(keylist->mTimePerKey) * Frames);
    ap->InterpToKeylistFrame(keylist, NewAnim, NewFrame, duration, host->object, fx);

    if (CurrentMode == 0) {
        ap->SetPaused();
    } else {
        ap->SetUnpaused();
        if (CurrentMode == 2) {
            ap->SetLooping();
        } else {
            ap->SetNoLooping();
        }
    }

    ap->mLayerBlendMode = 0;
}

// 0x004de8b0 (PDB: 0x004e03f0)
void G2EmulationInstanceSetAnimation(
    BaseInstance* instance,
    int CurrentSection,
    int NewAnim,
    int NewFrame,
    int Frames)
{
    AnimProcessor* ap = GetAnimProcessor(instance);
    AnimFragmentProcessor* sec = &ap->mSection[CurrentSection];
    AnimFragment* keylist = G2Instance_GetKeylist(instance, NewAnim);

    sec->SetAlphaTable(nullptr);
    AnimFxHeader* fx = instance->object->animFXList[NewAnim];

    if (static_cast<float>(Frames) == 0.0f) {
        ap->SwitchToKeylistFrame(keylist, NewAnim, NewFrame, instance->object, fx);
    } else {
        const float duration = static_cast<float>(static_cast<int>(keylist->mTimePerKey) * Frames);
        sec->InterpToKeylistFrame(keylist, NewAnim, NewFrame, duration, instance->object, fx);
    }

    sec->mAlarmFlags &= ~0x200u;
    sec->mFlags &= ~0x20;
}

// 0x004de960 (PDB: 0x004e04a0)
void G2EmulationInstanceSetAnimationLastFrame(Instance* instance, int NewAnim)
{
    AnimProcessor* ap = GetAnimProcessor(instance);
    AnimFragmentProcessor* sec = &ap->mSection[0];
    AnimFragment* keylist = G2Instance_GetKeylist(instance, NewAnim);

    sec->SetAlphaTable(nullptr);
    AnimFxHeader* fx = instance->object->animFXList[NewAnim];
    const int lastFrame = ComputeAnimLengthFrames(keylist) - 1;
    ap->SwitchToKeylistFrame(keylist, NewAnim, lastFrame, instance->object, fx);
    sec->mFlags &= ~0x20;
}

// 0x004de9d0 (PDB: 0x004e0510)
void G2EmulationSetAnimation(CharacterState* state, int NewAnim, int NewFrame, int Frames)
{
    if (NewAnim < 0) {
        NewAnim = 0;
    }
    G2EmulationInstanceSetAnimation(state->instance, 0, NewAnim, NewFrame, Frames);
    state->playerState.vanimIndex = -1;
    state->playerState.vanim = nullptr;
}

// 0x004dea10 (PDB: 0x004e0550)
void G2EmulationInstanceSetMode(BaseInstance* instance, int CurrentSection, int Mode)
{
    ApplySectionMode(GetAnimSection(instance, CurrentSection), Mode);
}

// 0x004dea60 (PDB: 0x004e05a0)
void G2EmulationInstanceSetAnimSpeed(Instance* instance, int CurrentSection, float speed)
{
    GetAnimSection(instance, CurrentSection)->mSpeedAdjustment = speed;
}

// 0x004dea90 (PDB: 0x004e05d0)
void G2EmulationSetControllerAlpha(Instance* instance, int seg, int type, int alphaTableIndex)
{
    G2AnimAlphaTable* table = (alphaTableIndex != 0) ? &G2AlphaTables[alphaTableIndex] : nullptr;
    GetAnimProcessor(instance)->SetControllerAlphaTable(seg, type, table);
}

// 0x004dead0 (PDB: 0x004e0610)
int G2EmulationInstanceQueryAnimation(Instance* instance, int CurrentSection)
{
    return GetAnimSection(instance, CurrentSection)->mKeylistID;
}

// 0x004deaf0 (PDB: 0x004e0630)
int G2EmulationInstanceQueryAnimLength(Instance* instance, int CurrentSection)
{
    return ComputeAnimLengthFrames(GetAnimSection(instance, CurrentSection)->mKeylist);
}

// 0x004deb30 (PDB: 0x004e0670)
int G2EmulationInstanceEventAnimPlaying(Instance* instance, int CurrentSection)
{
    return (~(GetAnimSection(instance, CurrentSection)->mAlarmFlags >> 9)) & 0x1;
}

// 0x004deb60 (PDB: 0x004e06a0)
void G2EmulationInstanceSetEventAnimPlaying(Instance* instance, int CurrentSection)
{
    GetAnimSection(instance, CurrentSection)->mAlarmFlags &= ~0x200u;
}

// 0x004deb90 (PDB: 0x004e06d0)
int G2EmulationInstanceQueryMode(Instance* instance, int CurrentSection)
{
    const uint16_t flags = GetAnimSection(instance, CurrentSection)->mFlags;
    if ((flags & 0x1) != 0) {
        return 0;
    }
    return ((flags & 0x2) != 0) ? 2 : 1;
}

// 0x004debd0 (PDB: 0x004e0710)
void G2EmulationInstanceSetStartAndEndSegment(BaseInstance* instance, int CurrentSection, short StartSeg, short EndSeg)
{
    AnimFragmentProcessor* sec = GetAnimSection(instance, CurrentSection);
    sec->mFirstSeg = static_cast<uint8_t>(StartSeg);
    sec->mSegCount = static_cast<uint8_t>(EndSeg - StartSeg + 1);
}

// 0x004dec00 (PDB: 0x004e0740)
void G2EmulationInstanceSetTotalSections(BaseInstance* instance, short Total)
{
    AnimProcessor* ap = GetAnimProcessor(instance);
    while (static_cast<int>(ap->mSectionCount) < static_cast<int>(Total)) {
        ap->AddSection(0, 0);
    }
    ap->SetCallback(INSTANCE_DefaultAnimCallback, instance);
}

// 0x004dec50 (PDB: 0x004e0790)
void G2EmulationInstanceInitSection(Instance* instance, int CurrentSection, AnimCallbackFn callback, void* callbackData)
{
    AnimFragmentProcessor* sec = GetAnimSection(instance, CurrentSection);
    sec->mCallback = callback;
    sec->mCallbackData = callbackData;
}

// 0x004dec80 (PDB: 0x004e07c0)
void G2EmulationSetInterpController_Vector(
    Instance* instance,
    int segment,
    int type,
    cdc::Vector3* vec,
    int Frames,
    int Data)
{
    AnimProcessor* ap = GetAnimProcessor(instance);
    ap->SetInterpController_Vector(segment, type, vec, static_cast<float>(Frames) * 100.0f);
    ap->SetControllerCallbackData(segment, type, reinterpret_cast<void*>(static_cast<intptr_t>(Data)));
}

// 0x004decd0 (PDB: 0x004e0810)
void G2EmulationSetMode(CharacterState* state, int CurrentSection, int Mode)
{
    G2EmulationInstanceSetMode(state->instance, CurrentSection, Mode);
}

// 0x004ded20 (PDB: 0x004e0860)
int G2EmulationQueryAnimation(CharacterState* state, int CurrentSection)
{
    return G2EmulationInstanceQueryAnimation(state->instance, CurrentSection);
}

// 0x004ded50 (PDB: 0x004e0890)
void G2EmulationSetStartAndEndSegment(CharacterState* state, int CurrentSection, short StartSeg, short EndSeg)
{
    G2EmulationInstanceSetStartAndEndSegment(state->instance, CurrentSection, StartSeg, EndSeg);
}

// 0x004ded90 (PDB: 0x004e08d0)
void G2EmulationSetTotalSections(CharacterState* state, short Total)
{
    G2EmulationInstanceSetTotalSections(state->instance, Total);
}

// 0x004deda0 (PDB: 0x004e08e0)
int G2EmulationQueryInstanceAnimation(Instance* instance)
{
    return GetAnimProcessor(instance)->GetKeylistID();
}

// 0x004dedc0 (PDB: 0x004e0900)
void G2EmulationInstancePlayAnimation(BaseInstance* instance)
{
    if (instance->animComponent != nullptr) {
        instance->animComponent->mBlendProcessor.NextKeyframe();
    } else {
        G2Instance_NextKeyframe(instance);
    }
}

// 0x004dede0 (PDB: 0x004e0920)
void G2EmulatePlayAnimation(CharacterState* state)
{
    G2EmulationInstancePlayAnimation(state->instance);
}

// 0x004dee00 (PDB: 0x004e0940)
int G2EmulationInstanceQueryFrame(Instance* instance, int CurrentSection)
{
    return GetAnimSection(instance, CurrentSection)->GetKeyframeNumber();
}

// 0x004dee30 (PDB: 0x004e0970)
int G2EmulationQueryFrame(CharacterState* state, int CurrentSection)
{
    return G2EmulationInstanceQueryFrame(state->instance, CurrentSection);
}
