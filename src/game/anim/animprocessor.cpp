#include "game/anim/anim.h"
#include "game/anim/animpatternvm.h"
#include "game/g2/instncg2.h"
#include "game/g2/poolmmg2.h"
#include "game/g2/quatg2.h"
#include "game/g2/timerg2.h"
#include "game/main.h"
#include "game/math3d.h"
#include "game/obtable.h"
#include "game/player/player.h"
#include <cmath>
#include <cstring>

// G2 / game external helpers
extern void playerInvProcessAimBlend(Instance* inst, bool force); // 0x005a8310 (PDB: 0x005aac10)
extern uint32_t debugRazielFlags1;                                        // 0x01075ac8 (PDB: 0x01077e04)
extern bool g_interpoBlendMirrorHack;                                     // 0x011187c0 (PDB: 0x0111ba20)

static const BoneSet allBones[2] = { {1.0f, 0, 10000}, {0.0f, 0xffff, 0xffff} }; // 0x00f013fc

// Globals declared in anim.h
G2AnimInterpStateBlockPool _interpStateBlockPool = {}; // 0x010fa300 (PDB: 0x010fd100)
AnimControllerPool _controllerPool = {};               // 0x010fa310 (PDB: 0x010fd110)
cdc::Vector3 gAnimMorphVelocityDir = {};               // 0x010fa320 (PDB: 0x010fd120)
cdc::Vector3 AnimProcessor::sUserTransformTranslation = {}; // 0x010fa330 (PDB: 0x010fd130)
cdc::Quat AnimProcessor::sUserTransformRotation = cdc::Quat::Identity(); // 0x010fa340 (PDB: 0x010fd140), $E9 (PDB: 0x00efa9f0)
G2AnimSegValue* _segValues = _segValuesStatic;         // 0x00f1658c (PDB: 0x00f188c4)
AnimProcessor* _segValuesAnim = nullptr;               // 0x010fe8a0 (PDB: 0x01101aa0)
G2AnimSegValue _segValuesStatic[180] = {};             // 0x010fe8c0 / 0x01032820 (PDB: 0x01101ac0 / 0x01034b60)
AnimPattern* AnimProcessor::sNativePattern[4] = {
    &gAnimPattern_SlopeJumpToLedge,
    &gAnimPattern_SlopeBackJumpToLedge,
    &gAnimPattern_RunAndJump,
    nullptr,
}; // 0x00f15b40 (PDB: 0x00f17e78)

// 0x0046fad0 (PDB: 0x00471450)
void MATH3D_InterpOnePlusAlpha(cdc::Vector3* dst, cdc::Vector3* a, cdc::Vector3* b, float alpha)
{
    dst->x = a->x * alpha + b->x;
    dst->y = a->y * alpha + b->y;
    dst->z = a->z * alpha + b->z;
}

// 0x0046fb00 (PDB: 0x00471480)
void AnimProcessor::UpdateMirrorMode()
{
    if (mFlags & 0x2000) {
        mFlags |= 0x1000;
    } else {
        mFlags &= ~0x1000;
    }
}

// 0x0046fb30 (PDB: 0x004714b0)
void ANIM_SetMorphVelocityDirection(const cdc::Vector3& dir)
{
    gAnimMorphVelocityDir = dir;
}

// 0x0046fb60 (PDB: 0x004714e0)
void AnimProcessor::CopyVectorWithOrder(cdc::Vector* vec, _G2EulerAngles_Type* euler, int order)
{
    if (order == 1) {
        euler->x = vec->z;
        euler->y = vec->y;
        euler->z = vec->x;
        euler->order = 1;
    } else if (order == 0x15) {
        euler->x = vec->x;
        euler->y = vec->y;
        euler->z = vec->z;
        euler->order = 0x15;
    }
}

// 0x0046fbb0 (PDB: 0x00471530)
void AnimController::GetVector(cdc::Vector3* vector)
{
    *vector = mData.vector.base;
    if (mFlags & 0x4000) {
        float scale = mElapsedTime * (1.0f / 4096.0f);
        vector->x += mData.vector.offset.x * scale;
        vector->y += mData.vector.offset.y * scale;
        vector->z += mData.vector.offset.z * scale;
    }
}

// 0x0046fc10 (PDB: 0x00471590)
AnimController* AnimController::Destroy()
{
    AnimController* controller = this;
    AnimController* pool = _controllerPool.blockPool;
    uint16_t next;
    do {
        next = controller->mNext;
        G2PoolMem_Free(&_controllerPool, controller);
        controller = &pool[next];
    } while (controller > pool && controller->mType == 0);
    return controller;
}

// 0x0046fc40 (PDB: 0x004715c0)
void AnimController::InsertIntoList(uint16_t* listHead)
{
    AnimController* pool = _controllerPool.blockPool;
    uint16_t* prevNext = listHead;
    AnimController* cur = &pool[*prevNext];
    while (cur > pool) {
        if (cur->mSegNumber > mSegNumber) {
            break;
        }
        if (cur->mSegNumber == mSegNumber && cur->mType > mType) {
            break;
        }
        prevNext = &cur->mNext;
        cur = &pool[*prevNext];
    }

    *prevNext = static_cast<uint16_t>(this - pool);

    AnimController* tail = this;
    AnimController* next = &pool[tail->mNext];
    while (next > pool) {
        tail = next;
        next = &pool[tail->mNext];
    }
    tail->mNext = static_cast<uint16_t>(cur - pool);
}

// 0x0046fcd0 (PDB: 0x00471650)
AnimController* AnimController::FindInList(int seg, int type, uint16_t* listHead)
{
    AnimController* pool = _controllerPool.blockPool;
    AnimController* cur = &pool[*listHead];
    while (cur > pool) {
        if (cur->mSegNumber == seg && cur->mType == type) {
            return cur;
        }
        cur = &pool[cur->mNext];
    }
    return nullptr;
}

// 0x0046fd20 (PDB: 0x004716a0)
AnimController* AnimController::FindPtrInList(int seg, int type, uint16_t** listPtr)
{
    AnimController* pool = _controllerPool.blockPool;
    AnimController* cur = &pool[**listPtr];
    while (cur > pool) {
        if (cur->mSegNumber == seg && cur->mType == type) {
            return cur;
        }
        *listPtr = &cur->mNext;
        cur = &pool[**listPtr];
    }
    *listPtr = nullptr;
    return nullptr;
}

// 0x0046fd80 (PDB: 0x00471700)
AnimController* AnimController::RemoveFromList(int seg, int type, uint16_t* listHead)
{
    AnimController* pool = _controllerPool.blockPool;
    uint16_t* prevNext = listHead;
    AnimController* cur = &pool[*prevNext];
    while (cur > pool) {
        if (cur->mSegNumber == seg && cur->mType == type) {
            break;
        }
        prevNext = &cur->mNext;
        cur = &pool[*prevNext];
    }
    if (cur <= pool) {
        return nullptr;
    }

    AnimController* iter = cur;
    AnimController* lastRemoved;
    do {
        lastRemoved = iter;
        iter = &pool[iter->mNext];
        *prevNext = lastRemoved->mNext;
    } while (iter > pool && iter->mType == 0);

    lastRemoved->mNext = 0;
    return cur;
}

// 0x0046fe00 (PDB: 0x00471780)
void AnimProcessor::FreeInterpStateBlockList(G2AnimInterpStateBlock* list)
{
    while (list != nullptr) {
        G2AnimInterpStateBlock* next = list->next;
        G2PoolMem_Free(&_interpStateBlockPool, list);
        list = next;
    }
}

// 0x0046fe30 (PDB: 0x004717b0)
bool AnimProcessor::Install()
{
    G2PoolMem_InitPool(&_interpStateBlockPool, 0x100, sizeof(G2AnimInterpStateBlock));
    G2PoolMem_InitPool(&_controllerPool, 0x300, sizeof(AnimController));
    AnimController* dummy = static_cast<AnimController*>(G2PoolMem_Allocate(&_controllerPool));
    dummy->mNext = 0;
    dummy->mSegNumber = 0;
    dummy->mType = 0;
    return true;
}

// 0x0046fe70 (PDB: 0x004717f0)
void AnimProcessor::ResetInternalState()
{
    G2PoolMem_ResetPool(&_interpStateBlockPool);
    G2PoolMem_ResetPool(&_controllerPool);
    AnimController* dummy = static_cast<AnimController*>(G2PoolMem_Allocate(&_controllerPool));
    dummy->mNext = 0;
    dummy->mSegNumber = 0;
    dummy->mType = 0;
}

// 0x0046fea0 (PDB: 0x00471820)
AnimFragmentProcessor* AnimProcessor::GetSectionWithSeg(int seg)
{
    AnimFragmentProcessor* end = mSection + mSectionCount;
    for (AnimFragmentProcessor* sec = mSection; sec < end; ++sec) {
        if (seg >= sec->mFirstSeg && seg < sec->mFirstSeg + sec->mSegCount) {
            return sec;
        }
    }
    return nullptr;
}

// 0x0046fef0 (PDB: 0x00471870)
void AnimFragmentProcessor::JumpToTime(float targetTime)
{
    if (targetTime < mElapsedTime) {
        mStoredTime = static_cast<float>(-static_cast<int>(mKeylist->mTimePerKey));
    }
    mFlags &= ~0x80;
    mElapsedTime = targetTime;
    mAlarmFlags &= ~0x3u;
    if (mFirstSeg == 0) {
        mAnimProcessor->mRootTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// 0x0046ff80 (PDB: 0x00471900)
void AnimProcessor::ResetCache()
{
    mFlags &= ~0x0c;
    _segValuesAnim = nullptr;
}

// 0x0046ffa0 (PDB: 0x00471920)
AnimController* AnimProcessor::GetControllerList()
{
    return &_controllerPool.blockPool[mControllerList];
}

// 0x0046ffc0 (PDB: 0x00471940)
void AnimFragmentProcessor::InitializeSegValue(Segment* segment, G2AnimSegValue* segValue)
{
    segValue->trans = segment->pivot;
    segValue->quat.x = 0.0f;
    segValue->quat.y = 0.0f;
    segValue->quat.z = 0.0f;
    segValue->quat.w = 1.0f;
    segValue->rot = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    segValue->scale = cdc::Vector3::Make(1.0f, 1.0f, 1.0f, 0.0f);
}

// 0x00470050 (PDB: 0x004719d0)
void AnimFragmentProcessor::InitializeKeyflagInfo(
    G2AnimBitStream* bs1,
    G2AnimBitStream* bs2,
    G2AnimBitStream* bs3)
{
    AnimFragment* keylist = mKeylist;
    const int sectionCount = keylist->mSectionCount;
    const uint32_t* base = reinterpret_cast<const uint32_t*>(keylist->SectionDataOffsets() + sectionCount);
    const uint8_t flags = *reinterpret_cast<const uint8_t*>(base);
    const int segBits = ((keylist->mSegmentCount * 3) + 7) & ~7;

    bs1->stream = nullptr;
    bs1->flags = 0;
    bs1->bitCount = 0;
    bs2->stream = nullptr;
    bs2->flags = 0;
    bs2->bitCount = 0;
    bs3->stream = nullptr;
    bs3->flags = 0;
    bs3->bitCount = 0;

    int bitOffset = mFirstSeg * 3 + 8;

    if (flags & 0x1) {
        int wordIdx = bitOffset >> 5;
        uint32_t* ptr = const_cast<uint32_t*>(&base[wordIdx]);
        int bitInWord = bitOffset & 0x1f;
        bs1->stream = ptr;
        bs1->flags = *ptr >> bitInWord;
        bs1->bitCount = 32 - bitInWord;
        bitOffset += segBits;
    }

    if (flags & 0x2) {
        int wordIdx = bitOffset >> 5;
        uint32_t* ptr = const_cast<uint32_t*>(&base[wordIdx]);
        int bitInWord = bitOffset & 0x1f;
        bs2->stream = ptr;
        bs2->flags = *ptr >> bitInWord;
        bs2->bitCount = 32 - bitInWord;
        bitOffset += segBits;
    }

    if (flags & 0x4) {
        int wordIdx = bitOffset >> 5;
        uint32_t* ptr = const_cast<uint32_t*>(&base[wordIdx]);
        int bitInWord = bitOffset & 0x1f;
        bs3->stream = ptr;
        bs3->flags = *ptr >> bitInWord;
        bs3->bitCount = 32 - bitInWord;
    }
}

// 0x00470140 (PDB: 0x00471ac0)
float AnimFragmentProcessor::DecompressChannel_Linear(int keyframe, float* channelData)
{
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(channelData);
    int16_t count = *reinterpret_cast<const int16_t*>(raw + 2);
    const uint8_t* runs = raw + 4;
    const float* values = reinterpret_cast<const float*>(runs + (((count + 2) >> 2) * 4));

    int run = runs[0];
    float accum = values[0];
    float step = values[1];
    ++values;

    while (keyframe > run) {
        keyframe -= run;
        accum += step;
        ++runs;
        run = runs[0];
        ++values;
        step = *values;
    }

    return accum + (step * static_cast<float>(keyframe)) / static_cast<float>(run);
}

// 0x004701a0 (PDB: 0x00471b20)
void AnimFragmentProcessor::TriggerEffects(float startTime, float endTime, int masked)
{
    if (!mFxList || !mCallback) {
        return;
    }

    const int sectionID = mSectionID;
    const int timePerKey = mKeylist->mTimePerKey;
    AnimFxHeader* fx = mFxList;

    while (fx->info != 0xffff) {
        const uint16_t info = fx->info;
        const int fxSection = (info >> 11) & 1;
        const uint32_t byteSize = (static_cast<uint32_t>(info) >> 10) & 0x3ffffc;

        if (fxSection == sectionID && ((masked ^ info) & 0x600) == 0) {
            bool inWindow = true;
            if (masked == 0) {
                float fxTime = static_cast<float>(fx->keyframeID * timePerKey);
                if (fxTime < startTime || fxTime >= endTime) {
                    inWindow = false;
                }
            }

            if (inWindow) {
                bool skip = false;
                if ((info & 0x8) && (mAlarmFlags & 0x4)) {
                    skip = true;
                }
                if (info & 0x4) {
                    skip = true;
                }

                int extra = 0;
                const int triggerMode = info & 0x3;
                if (triggerMode == 0 || triggerMode == 2) {
                    if (mAlarmFlags & 0x40) {
                        skip = true;
                    }
                } else if (triggerMode == 1) {
                    if (mAlarmFlags & 0x80) {
                        skip = true;
                    }
                    extra = 0xff;
                }

                if (!skip) {
                    ModeAndType modeAndType(*reinterpret_cast<FxaInfo*>(fx), static_cast<signed char>(extra), fx->fxID);
                    mCallback(
                        mAnimProcessor,
                        sectionID,
                        G2ANIM_MSG_PLAYEFFECT,
                        static_cast<int>(modeAndType.raw),
                        static_cast<int>(reinterpret_cast<uintptr_t>(fx + 1)),
                        mCallbackData);
                }
            }
        }

        fx = reinterpret_cast<AnimFxHeader*>(reinterpret_cast<uint8_t*>(fx) + byteSize);
    }
}

// 0x004702f0 (PDB: 0x00471c70)
float G2AnimAlphaTable::GetValue(float alpha) const
{
    const G2AnimAlphaTable* volatile self = this;
    if (self == nullptr) {
        return alpha;
    }

    float scaled = static_cast<float>(size - 1) * alpha;
    int idx = static_cast<int>(scaled);
    float v0 = data[idx];
    float v1 = data[idx + 1];
    return v0 + (v1 - v0) * (scaled - static_cast<float>(idx));
}

// 0x00470340 (PDB: 0x00471cc0)
void AnimProcessor::SwapBones()
{
    G2AnimSegValue temp[128];

    const uint8_t segCount = mSection[0].mSegCount;
    for (uint8_t i = 0; i < segCount; ++i) {
        G2AnimSegValue& sv = _segValues[i];
        sv.rot.y = -sv.rot.y;
        sv.rot.z = -sv.rot.z;
        sv.rot.w = 0.0f;
        sv.trans.x = -sv.trans.x;
        sv.trans.w = 0.0f;
    }

    BoneMirrorData* bmd = mModelData->boneMirrorData;
    while (bmd->count > 0) {
        int b1 = bmd->bone1;
        int b2 = bmd->bone2;
        int count = bmd->count;
        const int totalSegs = mSection[0].mSegCount;
        if (b2 + count > totalSegs) {
            count = totalSegs - b2;
        }
        if (count > 0) {
            std::memcpy(temp, &_segValues[b1], static_cast<size_t>(count) * sizeof(G2AnimSegValue));
            std::memcpy(&_segValues[b1], &_segValues[b2], static_cast<size_t>(count) * sizeof(G2AnimSegValue));
            std::memcpy(&_segValues[b2], temp, static_cast<size_t>(count) * sizeof(G2AnimSegValue));
        }
        ++bmd;
    }
}

// 0x004704f0 (PDB: 0x00471e70)
void AnimProcessor::AllocateSections(int count)
{
    bool wasInInterp = false;
    if (mSectionsAllocated >= 1) {
        wasInInterp = (mSection[0].mFlags & 0x200) != 0;
    }

    if (mSectionsAllocated < count) {
        delete[] mSection;
        delete[] mWeights;
        mSectionCount = static_cast<uint8_t>(count);
        mSectionsAllocated = static_cast<uint8_t>(count);
        mSection = new AnimFragmentProcessor[static_cast<uint8_t>(count)];
        mWeights = new float[mSectionCount];
    } else {
        mSectionCount = static_cast<uint8_t>(count);
    }

    std::memset(mSection, 0, static_cast<size_t>(mSectionCount) * sizeof(AnimFragmentProcessor));
    if (wasInInterp) {
        mSection[0].mFlags |= 0x200;
    }

    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mStoredTime = -1.0f;
        mSection[i].mSwAlarmTable = nullptr;
        mSection[i].mSpeedAdjustment = 1.0f;
        mSection[i].mAnimProcessor = this;
        mWeights[i] = 1.0f;
        mSection[i].mSectionID = 0;
    }
}

// 0x00470620 (PDB: 0x00471fa0)
void AnimProcessor::SetMarker()
{
    mMarker = mBackIndex;
}

// 0x00470630 (PDB: 0x00471fb0)
void AnimProcessor::PopToNextAnim()
{
    if (mFrontIndex == mBackIndex) {
        return;
    }
    while (static_cast<int16_t>(mAnimQueue[mFrontIndex].mAnimId) < 0) {
        ++mFrontIndex;
        if (mFrontIndex == mBackIndex) {
            break;
        }
    }
}

// 0x00470670 (PDB: 0x00471ff0)
cdc::Vector3 AnimProcessor::GetVector(int index)
{
    return mVectorVar[index];
}

// 0x004706a0 (PDB: 0x00472020)
float AnimProcessor::GetFloat(int index)
{
    return mFloatVar[index];
}

// 0x004706b0 (PDB: 0x00472030)
void AnimProcessor::ReleaseControl()
{
    if (!mHasControl) {
        return;
    }
    if (mOldCallback != nullptr) {
        mSection[0].mCallback = mOldCallback;
        mSection[0].mCallbackData = mOldCallbackData;
        mOldCallback = nullptr;
        mOldCallbackData = nullptr;
    }
    mUseRateRamp = false;
    mBackIndex = mFrontIndex;
    mReadyForNextAnim = false;
    mHasControl = false;
}

// 0x00470710 (PDB: 0x00472090)
cdc::Vector3 AnimProcessor::GetTranslation()
{
    if (!mHasControl) {
        return mAnimComponent->mInstance->position;
    }
    return cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
}

// 0x004707a0 (PDB: 0x00472120)
cdc::Matrix AnimProcessor::GetRotation()
{
    cdc::Matrix mat;
    if (!mHasControl) {
        mat.Build(mAnimComponent->mInstance->rotation);
    }
    return mat;
}

// 0x004707d0 (PDB: 0x00472150)
void AnimProcessor::SetAnimComponent(AnimComponent* comp)
{
    mAnimComponent = comp;
}

// 0x004707e0 (PDB: 0x00472160)
int AnimProcessor::CurrentPatternId() const
{
    return mCurrentPatternId;
}

// 0x004707f0 (PDB: 0x00472170)
int AnimProcessor::CurrentBlendId() const
{
    return mCurrentBlendId;
}

// 0x00470800 (PDB: 0x00472180)
void AnimProcessor::SetCurrentBlendId(int id)
{
    mCurrentBlendId = id;
}

// 0x00470810 (PDB: 0x00472190)
bool AnimProcessor::GetPatternMirrorMode() const
{
    return mMirror;
}

// 0x00470820 (PDB: 0x004721a0)
void AnimProcessor::DisableRootTranslation()
{
    mSection[0].mFlags |= 0x8;
}

// 0x00470830 (PDB: 0x004721b0)
void AnimProcessor::EnableRootTranslation()
{
    mSection[0].mFlags &= ~0x8;
}

// 0x00470840 (PDB: 0x004721c0)
float AnimProcessor::GetElapsedTime()
{
    return mSection[mMasterSection].mElapsedTime;
}

// 0x00470860 (PDB: 0x004721e0)
AnimFragment* AnimProcessor::GetKeylist()
{
    return mSection[mMasterSection].mKeylist;
}

// 0x00470880 (PDB: 0x00472200)
int AnimProcessor::GetKeylistID()
{
    return mSection[mMasterSection].mKeylistID;
}

// 0x004708a0 (PDB: 0x00472220)
bool AnimProcessor::IsAlarmSet(unsigned int mask)
{
    return (mSection[mMasterSection].mAlarmFlags & mask) != 0;
}

// 0x004708d0 (PDB: 0x00472250)
void AnimProcessor::JumpToTime(float targetTime)
{
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].JumpToTime(targetTime);
    }
}

// 0x00470920 (PDB: 0x004722a0)
void AnimProcessor::SetCallback(AnimCallbackFn callback, void* data)
{
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mCallback = callback;
        mSection[i].mCallbackData = data;
    }
}

// 0x00470970 (PDB: 0x004722f0)
void AnimProcessor::SetNoLooping()
{
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mFlags &= ~0x2;
    }
}

// 0x004709a0 (PDB: 0x00472320)
void AnimProcessor::SetPaused()
{
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mFlags |= 0x1;
    }
}

// 0x004709d0 (PDB: 0x00472350)
void AnimProcessor::SetSpeedAdjustment(float speed)
{
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mSpeedAdjustment = speed;
    }
}

// 0x00470a10 (PDB: 0x00472390)
void AnimProcessor::SetUnpaused()
{
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mFlags &= ~0x1;
    }
}

// 0x00470a40 (PDB: 0x004723c0)
float AnimProcessor::GetAnimDuration(const Anim& anim)
{
    Instance* inst = mAnimComponent->mInstance;
    AnimBlend* blend = ANIM_AnimBlendFromIndex(inst, anim.mAnimId);
    AnimFragment* frag = G2Instance_GetKeylistFromObject(inst->object, blend->animFragment[0]);
    return static_cast<float>(frag->mTimePerKey * frag->mKeyCount);
}

// 0x00470a90 (PDB: 0x00472410)
VirtualMachine* AnimProcessor::GetVirtualMachine()
{
    if (mVirtualMachine == nullptr) {
        mVirtualMachine = new VirtualMachine(this);
    }
    return mVirtualMachine;
}

// 0x00470ad0 (PDB: 0x00472450)
bool AnimFragmentProcessor::IsInInterpolation() const
{
    if ((mFlags & 0x200) && mAnimProcessor != nullptr) {
        return mAnimProcessor->mInterpInfo.stateBlockList != nullptr;
    }
    return false;
}

// 0x00470b00 (PDB: 0x00472480)
void AnimFragmentProcessor::SetAlphaTable(G2AnimAlphaTable* alphaTable)
{
    if ((mFlags & 0x200) && mAnimProcessor != nullptr) {
        mAnimProcessor->mInterpInfo.alphaTable = alphaTable;
    }
}

// 0x00470b20 (PDB: 0x004724a0)
void AnimProcessor::SetUserTransform(const cdc::Quat& rot, const cdc::Vector3& trans)
{
    sUserTransformRotation = rot;
    sUserTransformTranslation = trans;
}

// 0x00470b60 (PDB: 0x004724e0)
void AnimProcessor::AttachControllerToSeg(int seg, int type)
{
    AnimController* controller = static_cast<AnimController*>(G2PoolMem_Allocate(&_controllerPool));
    std::memset(controller, 0, sizeof(AnimController));
    controller->mNext = 0;
    controller->mType = static_cast<uint8_t>(type);
    controller->mSegNumber = static_cast<uint8_t>(seg);
    controller->mFlags = 0x15;
    controller->InsertIntoList(&mControllerList);
}

// 0x00470bb0 (PDB: 0x00472530)
void AnimProcessor::DetachControllerFromSeg(int seg, int type)
{
    uint16_t* listPtr = &mControllerList;
    AnimController* controller = AnimController::FindPtrInList(seg, type, &listPtr);
    if (controller == nullptr) {
        listPtr = &mDisabledControllerList;
        controller = AnimController::FindPtrInList(seg, type, &listPtr);
        if (controller == nullptr) {
            return;
        }
    }

    AnimController* next = controller->Destroy();
    *listPtr = static_cast<uint16_t>(next - _controllerPool.blockPool);
}

// 0x00470c30 (PDB: 0x004725b0)
void AnimProcessor::EnableController(int seg, int type)
{
    AnimController* controller = AnimController::RemoveFromList(seg, type, &mDisabledControllerList);
    if (controller == nullptr) {
        return;
    }

    controller->mFlags &= 0x1cff;
    controller->mDuration = 0.0f;
    controller->mElapsedTime = 0.0f;

    switch (controller->mType) {
    case 8:
        G2Quat_FromMatrixf(&controller->mData.quat.dest, &mSegMatrices[seg]);
        break;

    case 0x20:
        controller->mData.vector.base.x = mSegMatrices[seg].col3.x;
        controller->mData.vector.base.y = mSegMatrices[seg].col3.y;
        controller->mData.vector.base.z = mSegMatrices[seg].col3.z;
        break;

    case 0x0a: {
        int parentSeg = mModelData->segmentList[seg].parent;
        cdc::Quat parentQuat;
        parentQuat.Build(mSegMatrices[parentSeg]);
        cdc::Quat segQuat;
        segQuat.Build(mSegMatrices[seg]);
        cdc::Quat relQuat = parentQuat.Inverse() * segQuat;
        relQuat.Normalize();
        controller->mData.quat.src = relQuat;
        controller->mData.quat.dest = relQuat;

        cdc::Matrix mat;
        mat.Build(relQuat);
        _G2EulerAngles_Type euler;
        G2EulerAngles_FromMatrixf(&euler, &mat, 0x15);
        controller->mData.vector.base = cdc::Vector3::Make(
            static_cast<float>(euler.x),
            static_cast<float>(euler.y),
            static_cast<float>(euler.z),
            0.0f);
        break;
    }

    case 0x80:
        break;

    default:
        std::memset(&controller->mData.quat.src, 0, sizeof(cdc::Quat) * 2);
        break;
    }

    controller->InsertIntoList(&mControllerList);
}

// 0x00470dd0 (PDB: 0x00472750)
void AnimProcessor::DisableController(int seg, int type)
{
    AnimController* controller = AnimController::RemoveFromList(seg, type, &mControllerList);
    if (controller != nullptr) {
        controller->InsertIntoList(&mDisabledControllerList);
    }
}

// 0x00470e10 (PDB: 0x00472790)
bool AnimProcessor::IsControllerActive(int seg, int type)
{
    AnimController* controller = AnimController::FindInList(seg, type, &mControllerList);
    if (controller == nullptr) {
        return false;
    }
    return (controller->mFlags & 0x8000) == 0;
}

// 0x00470e40 (PDB: 0x004727c0)
bool AnimProcessor::IsControllerInterpolating(int seg, int type)
{
    AnimController* controller = AnimController::FindInList(seg, type, &mControllerList);
    if (controller != nullptr && (controller->mFlags & 0x4000)) {
        return (controller->mFlags & 0x2000) == 0;
    }
    return false;
}

// 0x00470e80 (PDB: 0x00472800)
void AnimProcessor::UpdateControllers()
{
    AnimController* pool = _controllerPool.blockPool;
    AnimController* cur = &pool[mControllerList];
    if (cur <= pool) {
        return;
    }

    float frameTime = G2Timer_GetFrameTime();
    while (cur > pool) {
        if (cur->mFlags & 0x4000) {
            if (cur->mElapsedTime >= cur->mDuration) {
                if (cur->mFlags & 0x8000) {
                    cur->mFlags &= ~0x8000;
                    AnimController::RemoveFromList(cur->mSegNumber, cur->mType, &mControllerList);
                    cur->InsertIntoList(&mDisabledControllerList);
                } else if ((cur->mFlags & 0x2000) == 0) {
                    cur->mFlags |= 0x2000;
                    int seg = cur->mSegNumber;
                    AnimFragmentProcessor* sec = GetSectionWithSeg(seg);
                    if (sec != nullptr && sec->mCallback != nullptr) {
                        sec->mCallback(
                            this,
                            sec->mSectionID,
                            G2ANIM_MSG_SEGCTRLR_INTERPDONE,
                            cur->mType,
                            seg,
                            cur->mCallbackData);
                    }
                }
            } else {
                cur->mElapsedTime += frameTime;
                if (cur->mElapsedTime > cur->mDuration) {
                    cur->mElapsedTime = cur->mDuration;
                }
            }
        }
        cur = &_controllerPool.blockPool[cur->mNext];
    }
}

// 0x00470f90 (PDB: 0x00472910)
void AnimController::GetMatrix(cdc::Matrix* matrix)
{
    if (mFlags & 0x4000) {
        if (mFlags & 0x2000) {
            G2Quat_ToMatrixf(&mData.quat.dest, matrix);
        } else {
            cdc::Quat quat;
            float alpha = mAlphaTable->GetValue(mElapsedTime / mDuration);
            G2Quat_Slerp(alpha, &mData.quat.src, &mData.quat.dest, &quat, 0);
            G2Quat_ToMatrixf(&quat, matrix);
        }
        return;
    }

    cdc::Euler euler;
    euler.x = mData.vector.base.x;
    euler.y = mData.vector.base.y;
    euler.z = mData.vector.base.z;
    euler.w = 0.0f;

    switch (mFlags & 0xff) {
    case 1:
        matrix->Build(euler);
        break;
    case 0x15:
        matrix->Build_XYZOrder(euler);
        break;
    }
}

// 0x00471050 (PDB: 0x004729d0)
void WrapQuatLog(cdc::Quat* out, cdc::Quat& in)
{
    constexpr float twoPi = 6.2831855f;
    float dx = (in.x - out->x) / twoPi;
    float dy = (in.y - out->y) / twoPi;
    float dz = (in.z - out->z) / twoPi;
    float sqDist = dx * dx + dy * dy + dz * dz;
    if (sqDist > 0.25f) {
        cdc::Vector3 unit = out->UnitVec3();
        float wraps = static_cast<float>(static_cast<int>(std::sqrt(sqDist) + 0.5f));
        float scale = wraps * twoPi;
        out->x -= unit.x * scale;
        out->y -= unit.y * scale;
        out->z -= unit.z * scale;
    }
}

// 0x00471120 (PDB: 0x00472aa0)
void AnimFragmentProcessor::SegValueToQuat(G2AnimSegValue* segValues, int dest)
{
    G2AnimSegValue* sv = &segValues[mFirstSeg];
    G2AnimInterpInfo* interpInfo = ((mFlags & 0x200) && mAnimProcessor != nullptr)
        ? &mAnimProcessor->mInterpInfo
        : nullptr;
    int segCount = mSegCount;
    G2AnimInterpStateBlock* block = interpInfo->stateBlockList;
    G2AnimQuatInfo* quatInfo = block->quatInfo;
    int slot = 4;

    while (segCount > 0) {
        if (dest == 0) {
            quatInfo->srcQuat.x = sv->rot.x;
            quatInfo->srcQuat.y = sv->rot.y;
            quatInfo->srcQuat.z = sv->rot.z;
            quatInfo->srcQuat.w = sv->rot.w;
            quatInfo->srcScale = sv->scale;
            quatInfo->srcTrans = sv->trans;
        } else {
            quatInfo->destQuat.x = sv->rot.x;
            quatInfo->destQuat.y = sv->rot.y;
            quatInfo->destQuat.z = sv->rot.z;
            quatInfo->destQuat.w = sv->rot.w;
            quatInfo->destScale = sv->scale;
            quatInfo->destTrans = sv->trans;
        }
        ++sv;
        ++quatInfo;
        --slot;
        if (slot == 0) {
            block = block->next;
            quatInfo = block->quatInfo;
            slot = 4;
        }
        --segCount;
    }

    if (dest == 0) {
        G2AnimQuatInfo* first = &interpInfo->stateBlockList->quatInfo[0];
        first->srcQuat.x = 0.0f;
        first->srcQuat.y = 0.0f;
        first->srcQuat.z = 0.0f;
    }
}

// 0x00471200 (PDB: 0x00472b80)
G2AnimInterpStateBlock* AnimProcessor::AllocateInterpStateBlockList(AnimFragmentProcessor* section)
{
    G2AnimInterpInfo* interpInfo = ((section->mFlags & 0x200) && section->mAnimProcessor != nullptr)
        ? &section->mAnimProcessor->mInterpInfo
        : nullptr;
    int segCount = section->mSegCount;

    interpInfo->stateBlockList = static_cast<G2AnimInterpStateBlock*>(G2PoolMem_Allocate(&_interpStateBlockPool));
    G2AnimInterpStateBlock* cur = interpInfo->stateBlockList;
    if (cur != nullptr) {
        segCount -= 4;
        while (segCount > 0) {
            G2AnimInterpStateBlock* next = static_cast<G2AnimInterpStateBlock*>(G2PoolMem_Allocate(&_interpStateBlockPool));
            cur->next = next;
            if (next == nullptr) {
                FreeInterpStateBlockList(interpInfo->stateBlockList);
                interpInfo->stateBlockList = nullptr;
                return nullptr;
            }
            segCount -= 4;
            cur = next;
        }
        cur->next = nullptr;
    }
    return interpInfo->stateBlockList;
}

// 0x00471290 (PDB: 0x00472c10)
void AnimProcessor::SetModel(Model* model)
{
    if (mModelData != nullptr && mModelData->numSegments != model->numSegments) {
        for (int i = 0; i < mSectionCount; ++i) {
            AnimFragmentProcessor* sec = &mSection[i];
            bool inInterp = (sec->mFlags & 0x200) != 0;
            G2AnimInterpInfo* interpInfo = (inInterp && sec->mAnimProcessor != nullptr)
                ? &sec->mAnimProcessor->mInterpInfo
                : nullptr;
            if (interpInfo != nullptr && interpInfo->stateBlockList != nullptr) {
                sec->mElapsedTime = interpInfo->targetTime;
            }

            if (sec->mSectionID == 0) {
                G2AnimInterpInfo* rootInterp = &sec->mAnimProcessor->mInterpInfo;
                if (inInterp) {
                    FreeInterpStateBlockList(rootInterp->stateBlockList);
                    rootInterp->stateBlockList = nullptr;
                }
                sec->mFlags |= 0x200;
                rootInterp->duration = 0.0f;
                rootInterp->targetTime = 0.0f;
                rootInterp->alphaTable = nullptr;
                rootInterp->stateBlockList = nullptr;
            } else {
                sec->mFlags &= ~0x200;
            }
        }
    }

    mModelData = model;
    AllocateSections(1);
    mSection[0].mSegCount = static_cast<uint8_t>(model->numSegments);
}

// 0x00471380 (PDB: 0x00472d00)
AnimFragmentProcessor* AnimProcessor::AddSection(int firstSeg, int segCount)
{
    const int newCount = mSectionCount + 1;
    if (mSectionsAllocated < newCount) {
        AnimFragmentProcessor* newSections = new AnimFragmentProcessor[newCount];
        float* newWeights = new float[newCount];
        std::memcpy(newSections, mSection, static_cast<size_t>(mSectionCount) * sizeof(AnimFragmentProcessor));
        std::memcpy(newWeights, mWeights, static_cast<size_t>(mSectionCount) * sizeof(float));
        delete[] mSection;
        delete[] mWeights;
        mSection = newSections;
        mWeights = newWeights;
        mSectionsAllocated = static_cast<uint8_t>(newCount);
    }

    mWeights[mSectionCount] = 1.0f;
    AnimFragmentProcessor* sec = &mSection[mSectionCount];
    std::memset(sec, 0, sizeof(AnimFragmentProcessor));
    sec->mSectionID = 0;
    sec->mSegCount = static_cast<uint8_t>(segCount);
    sec->mAnimProcessor = this;
    sec->mFirstSeg = static_cast<uint8_t>(firstSeg);
    sec->mStoredTime = -1.0f;
    sec->mSwAlarmTable = nullptr;
    sec->mSpeedAdjustment = 1.0f;
    ++mSectionCount;
    return sec;
}

static cdc::Quat MakeQuat(float x, float y, float z, float w)
{
    cdc::Quat q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.w = w;
    return q;
}

static cdc::Vector3 QuatVec3(const cdc::Quat& q)
{
    return cdc::Vector3::Make(q.x, q.y, q.z, 0.0f);
}

static uint32_t ReadBitStream3(G2AnimBitStream* bs)
{
    if (!bs->stream) {
        return 0;
    }
    if (bs->bitCount >= 3) {
        const uint32_t code = bs->flags & 7;
        bs->flags >>= 3;
        bs->bitCount -= 3;
        return code;
    }
    const uint32_t oldFlags = bs->flags;
    ++bs->stream;
    const uint32_t nextWord = *bs->stream;
    const uint32_t code = (oldFlags | (nextWord << bs->bitCount)) & 7;
    bs->flags = nextWord >> (3 - bs->bitCount);
    bs->bitCount += 29;
    return code;
}

static void SetSectionSpeedsByKeyCount(AnimFragmentProcessor* sections, const float* weights, int count)
{
    float totalKeys = 0.0f;
    for (int i = 0; i < count; ++i) {
        totalKeys += static_cast<float>(sections[i].mKeylist->mKeyCount) * weights[i];
    }
    const float invTotal = 1.0f / totalKeys;
    for (int i = 0; i < count; ++i) {
        sections[i].mSpeedAdjustment = static_cast<float>(sections[i].mKeylist->mKeyCount) * invTotal;
    }
}

// 0x004714e0 (PDB: 0x00472e60)
void AnimFragmentProcessor::GetRootMotionFromTimeForDuration(float startTime, float duration, cdc::Vector3* trans, cdc::Quat* rot)
{
    const float targetTime = startTime + duration;
    float interpStartTime = 0.0f;
    float interpEndTime = 0.0f;
    cdc::Quat rotOut = cdc::Quat::Identity();
    cdc::Vector3 transOut = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    if (mFlags & 0x200) {
        G2AnimInterpInfo* info = mAnimProcessor ? &mAnimProcessor->mInterpInfo : nullptr;
        if (info && info->stateBlockList && startTime < info->targetTime) {
            interpStartTime = startTime;
            if (targetTime >= info->targetTime) {
                interpEndTime = info->targetTime;
                startTime = info->targetTime;
            } else {
                interpEndTime = targetTime;
                startTime = targetTime;
            }

            const RootInterpMode mode = mRootInterpMode;
            if (mode == kRootInterpMode_UserTransform) {
                mFlags &= static_cast<uint16_t>(~8);
            }

            G2AnimQuatInfo* qi = info->stateBlockList->quatInfo;
            const float invDuration = 1.0f / info->duration;

            switch (mode) {
            case kRootInterpMode_ConserveMotion: {
                const float alphaEnd = info->alphaTable->GetValue(1.0f - (info->targetTime - interpEndTime) * invDuration);
                const float alphaStart = info->alphaTable->GetValue(1.0f - (info->targetTime - interpStartTime) * invDuration);
                const cdc::Vector3 srcT = cdc::Vector3::Make(qi->srcTrans.x, qi->srcTrans.y, qi->srcTrans.z, 0.0f);
                const cdc::Vector3 destT = cdc::Vector3::Make(qi->destTrans.x, qi->destTrans.y, qi->destTrans.z, 0.0f);
                const cdc::Quat qDelta = qi->srcQuat.Inverse() * qi->destQuat;
                const cdc::Vector3 vDelta = destT - srcT;
                const cdc::Quat qRotated = qi->srcQuat.Inverse() * MakeQuat(vDelta.x, vDelta.y, vDelta.z, 0.0f) * qi->srcQuat;
                const cdc::Quat logQ = QuatLog(qDelta);
                const float deltaAlpha = alphaEnd - alphaStart;
                transOut = qRotated * deltaAlpha;
                rotOut = QuatExp(logQ * deltaAlpha);
                break;
            }
            case kRootInterpMode_MorphSpeed: {
                const float midTime = (interpStartTime + interpEndTime) * 0.5f;
                const float alpha = info->alphaTable->GetValue(1.0f - (info->targetTime - midTime) * invDuration);
                rotOut = cdc::Quat::Identity();
                const cdc::Vector3 v = (qi->destTrans * alpha + qi->srcTrans * (1.0f - alpha)) * (interpEndTime - interpStartTime);
                transOut = gAnimMorphVelocityDir * v.x;
                transOut.w = 0.0f;
                break;
            }
            case kRootInterpMode_MorphVelocity: {
                const float midTime = (interpStartTime + interpEndTime) * 0.5f;
                const float alpha = info->alphaTable->GetValue(1.0f - (info->targetTime - midTime) * invDuration);
                rotOut = cdc::Quat::Identity();
                transOut = (qi->destTrans * alpha + qi->srcTrans * (1.0f - alpha)) * (interpEndTime - interpStartTime);
                transOut.w = 0.0f;
                break;
            }
            case kRootInterpMode_UserTransform: {
                const float alphaEnd = info->alphaTable->GetValue(1.0f - (info->targetTime - interpEndTime) * invDuration);
                const float alphaStart = info->alphaTable->GetValue(1.0f - (info->targetTime - interpStartTime) * invDuration);
                const cdc::Vector3 srcT = cdc::Vector3::Make(qi->srcTrans.x, qi->srcTrans.y, qi->srcTrans.z, 0.0f);
                const cdc::Vector3 destT = cdc::Vector3::Make(qi->destTrans.x, qi->destTrans.y, qi->destTrans.z, 0.0f);
                const cdc::Quat logQ = QuatLog(qi->destQuat);
                const cdc::Quat qStart = QuatExp(logQ * alphaStart);
                const float deltaAlpha = alphaEnd - alphaStart;
                const cdc::Vector3 deltaT = (destT - srcT) * deltaAlpha;
                rotOut = QuatExp(logQ * deltaAlpha);
                transOut = QuatVec3(qStart.Inverse() * MakeQuat(deltaT.x, deltaT.y, deltaT.z, 0.0f) * qStart);
                break;
            }
            default:
                break;
            }
        }
    }

    if (targetTime > startTime) {
        cdc::Quat startRot, endRot;
        cdc::Vector3 startTrans, endTrans;
        ANIM_GetTransformAtTime(&startRot, &startTrans, mKeylist, 0, startTime);
        ANIM_GetTransformAtTime(&endRot, &endTrans, mKeylist, 0, targetTime);
        const cdc::Quat animRot = startRot.Inverse() * endRot;
        const cdc::Vector3 diff = cdc::Vector3::Make(
            endTrans.x - startTrans.x,
            endTrans.y - startTrans.y,
            endTrans.z - startTrans.z,
            0.0f);
        const cdc::Quat animTrans = startRot.Inverse() * MakeQuat(diff.x, diff.y, diff.z, 0.0f) * startRot;
        *rot = animRot;
        *trans = QuatVec3(animTrans);

        if (interpEndTime > interpStartTime) {
            *rot = rotOut * (*rot);
            const cdc::Quat tq = MakeQuat(trans->x, trans->y, trans->z, 0.0f);
            *trans = transOut + QuatVec3(rotOut * tq * rotOut.Inverse());
        }
    } else if (interpEndTime > interpStartTime) {
        *rot = rotOut;
        *trans = transOut;
    }
}

// 0x00471d00 (PDB: 0x00473680)
void AnimFragmentProcessor::GetRootMotionOverInterval(float startTime, float endTime, cdc::Vector3* trans, cdc::Quat* rot)
{
    G2AnimInterpInfo* info = (mFlags & 0x200) ? &mAnimProcessor->mInterpInfo : nullptr;
    if (endTime > startTime && ((mFlags & 0x40) || (info && info->stateBlockList))) {
        GetRootMotionFromTimeForDuration(startTime, endTime - startTime, trans, rot);
    } else {
        *trans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        *rot = cdc::Quat::Identity();
    }
}

// 0x00471e00 (PDB: 0x00473780)
void AnimFragmentProcessor::SwitchToKeylistAtTimeWithoutAdvancing(AnimFragment* keylist, int keylistID, float time, void* fxListHost, AnimFxHeader* fxList)
{
    if (mFlags & 0x200) {
        G2AnimInterpInfo* info = &mAnimProcessor->mInterpInfo;
        if (info && info->stateBlockList) {
            AnimProcessor::FreeInterpStateBlockList(info->stateBlockList);
            info->stateBlockList = nullptr;
        }
    }

    mAlarmFlags &= ~3u;
    if (keylist != mKeylist || fxList != mFxList || fxListHost != mFxListHost) {
        if (mKeylist) {
            TriggerEffects(0.0f, 0.0f, 0x400);
        }
        mKeylistID = static_cast<uint16_t>(keylistID);
        mKeylist = keylist;
        mFxListHost = fxListHost;
        mFxList = fxList;
        mStoredTime = -static_cast<float>(keylist->mTimePerKey);
        TriggerEffects(0.0f, 0.0f, 0x200);
    }

    mAlarmFlags &= ~0x104u;
    JumpToTime(time);

    if (mFlags & 2) {
        mLoopStartTime = 0.0f;
        mLoopEndTime = static_cast<float>(mKeylist->mTimePerKey) * static_cast<float>(mKeylist->mKeyCount);
    }

    mFlags &= static_cast<uint16_t>(~1);
    mSwAlarmTable = nullptr;
    if (ANIM_HasRootMotion(keylist)) {
        mFlags |= 0x40;
    } else {
        mFlags &= static_cast<uint16_t>(~0x40);
    }

    if (mCallback) {
        mCallback(mAnimProcessor, mSectionID, G2ANIM_MSG_SWITCH_KEYLIST, static_cast<int>(time), 0, mCallbackData);
    }
}

// 0x00471f40 (PDB: 0x004738c0)
void AnimFragmentProcessor::AdvanceOverAccumulatedTime()
{
    if (mAccumulatedTime <= 0.0f) {
        return;
    }
    AnimFragment* keylist = mKeylist;
    if (!keylist || (mFlags & 1) || (mAlarmFlags & 1)) {
        mAccumulatedTime = 0.0f;
        return;
    }

    mAnimProcessor->mFlags |= 1;
    mAlarmFlags &= ~3u;
    float oldTime = mElapsedTime;
    float loopEndTime = (mFlags & 2) ? mLoopEndTime
        : static_cast<float>(keylist->mTimePerKey) * static_cast<float>(keylist->mKeyCount);

    float targetTime = oldTime + mSpeedAdjustment * mAccumulatedTime;
    AnimFragmentProcessor* master = &mAnimProcessor->mSection[0];
    if (this != master) {
        const float masterDuration = static_cast<float>(master->mKeylist->mTimePerKey) * static_cast<float>(master->mKeylist->mKeyCount);
        const float secDuration = static_cast<float>(mKeylist->mTimePerKey) * static_cast<float>(mKeylist->mKeyCount);
        targetTime = (master->mElapsedTime / masterDuration) * secDuration;
        if (targetTime < oldTime) {
            targetTime += secDuration;
        }
        while (targetTime < oldTime) {
            targetTime += secDuration;
        }
    }

    if (mSwAlarmTable) {
        const float* alarmTime = static_cast<const float*>(mSwAlarmTable);
        while (*alarmTime != -1.0f) {
            if ((oldTime <= *alarmTime && targetTime > *alarmTime) ||
                (mStoredTime <= 0.0f && *alarmTime == oldTime)) {
                mAlarmFlags |= 0x20;
                if (mCallback) {
                    mCallback(mAnimProcessor, mSectionID, G2ANIM_MSG_SWALARMSET,
                              static_cast<int>(oldTime), static_cast<int>(targetTime), mCallbackData);
                }
            }
            ++alarmTime;
        }
    } else {
        mAlarmFlags &= ~0x20u;
    }

    TriggerEffects(oldTime, targetTime, 0);

    if (targetTime > loopEndTime) {
        mAccumulatedTime = (targetTime - loopEndTime) / mSpeedAdjustment;
        targetTime = loopEndTime;
    } else {
        mAccumulatedTime = 0.0f;
    }

    cdc::Vector3 stepTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    cdc::Quat stepRot = cdc::Quat::Identity();

    while (true) {
        if (targetTime < oldTime) {
            GetRootMotionOverInterval(targetTime, oldTime, &stepTrans, &stepRot);
        } else if (targetTime >= oldTime) {
            GetRootMotionOverInterval(oldTime, targetTime, &stepTrans, &stepRot);
        }

        if (mAnimProcessor->mFlags & 0x10) {
            stepTrans.x = -stepTrans.x;
            stepRot.y = -stepRot.y;
            stepRot.z = -stepRot.z;
        }

        const cdc::Quat rotatedStep = mRootRot * MakeQuat(stepTrans.x, stepTrans.y, stepTrans.z, 0.0f) * mRootRot.Inverse();
        mRootTrans = mRootTrans + rotatedStep;
        mRootRot = mRootRot * stepRot;

        if (mFlags & 8) {
            mRootTrans = stepTrans;
        }
        if (mFlags & 0x100) {
            mRootRot = stepRot;
        }

        if (mFlags & 0x200) {
            G2AnimInterpInfo* info = &mAnimProcessor->mInterpInfo;
            if (info && info->stateBlockList && targetTime >= info->targetTime) {
                AnimProcessor::FreeInterpStateBlockList(info->stateBlockList);
                info->stateBlockList = nullptr;
            }
        }

        if (mAccumulatedTime <= 0.0f) {
            if (mFlags & 0x200) {
                G2AnimInterpInfo* info = &mAnimProcessor->mInterpInfo;
                if (info && info->stateBlockList) {
                    mAccumulatedTime = 0.0f;
                    break;
                }
            }

            G2AnimCallbackMsg msg;
            if (mFlags & 2) {
                mAlarmFlags |= 4;
                JumpToTime(mLoopStartTime);
                targetTime = mAccumulatedTime + mLoopStartTime;
                oldTime = mLoopStartTime;
                if (targetTime > mLoopEndTime) {
                    targetTime = mLoopEndTime;
                }
                const float clampedEnd = targetTime;
                TriggerEffects(mLoopStartTime - 1.0f, clampedEnd, 0);
                if (targetTime >= mLoopEndTime) {
                    targetTime = mLoopEndTime - 1.0f;
                }
                msg = G2ANIM_MSG_LOOPPOINT;
            } else {
                targetTime = oldTime - 1.0f;
                mAlarmFlags |= 0x201;
                if (targetTime < 0.0f) {
                    targetTime = 0.0f;
                }
                msg = G2ANIM_MSG_DONE;
            }

            if (mCallback) {
                mCallback(mAnimProcessor, mSectionID, msg, static_cast<int>(targetTime),
                          static_cast<int>(mAccumulatedTime), mCallbackData);
            }

            if (mAccumulatedTime <= 0.0f) {
                break;
            }

            if (mFlags & 2) {
                if (targetTime > mLoopEndTime) {
                    targetTime = mLoopEndTime - 1.0f;
                }
                JumpToTime(targetTime);
                mStoredTime = mLoopStartTime;
            }
        }

        if (mFirstSeg == 0) {
            mAnimProcessor->mRootTrans = stepTrans;
            mFlags |= 0x80;
        }

        if (!(mFlags & 2)) {
            break;
        }

        mAccumulatedTime = (oldTime - mLoopStartTime) / mSpeedAdjustment;
        oldTime = mLoopStartTime;
        loopEndTime = mLoopEndTime;
        targetTime = oldTime;
    }

    mAccumulatedTime = 0.0f;
    mElapsedTime = targetTime;
    mAlarmFlags |= 0x100;
}

// 0x004725e0 (PDB: 0x00473f60)
void AnimFragmentProcessor::DecompressFrameWithBoneSet(Model* model, int keyframe, float interpAlpha, float weight, float blendFrac, bool b0, bool b1, bool b2, const BoneSet* boneSet, bool b3)
{
    if (!boneSet) {
        boneSet = allBones;
    }

    G2AnimBitStream bs1, bs2, bs3;
    InitializeKeyflagInfo(&bs1, &bs2, &bs3);

    uint8_t* animData = reinterpret_cast<uint8_t*>(&mKeylist->SectionDataOffsets()[mSectionID])
        + mKeylist->SectionDataOffsets()[mSectionID];
    const int keyCount = mKeylist->mKeyCount;

    int key1 = keyframe - 1;
    int key2 = keyframe;
    if (key1 < 0) {
        if (mFlags & 2) {
            key1 = keyCount - 1;
        } else {
            interpAlpha = 1.0f;
            key1 = 0;
        }
    }

    int s = mFirstSeg;
    const int endAll = s + mSegCount;
    const int endSeg = (endAll < mKeylist->mSegmentCount) ? endAll : mKeylist->mSegmentCount;

    for (; s < endSeg && boneSet->firstBone != 0xffff && boneSet->lastBone != 0xffff; ++boneSet) {
        if (boneSet->weight == 0.0f && b3) {
            for (; s <= boneSet->lastBone && s < endSeg; ++s) {
                uint32_t mask = ReadBitStream3(&bs1) | (ReadBitStream3(&bs2) << 4) | (ReadBitStream3(&bs3) << 8);
                while (mask != 0) {
                    if (mask & 1) {
                        const int16_t type = *reinterpret_cast<const int16_t*>(animData);
                        if (type == 0) {
                            animData += 4 + keyCount * 4;
                        } else if (type == 1) {
                            animData += 8;
                        } else if (type == 2) {
                            const int16_t n = *reinterpret_cast<const int16_t*>(animData + 2);
                            animData += 4 + (((n + 2) >> 2) + n) * 4;
                        }
                    }
                    mask >>= 1;
                }
            }
            continue;
        }

        const float svWeight = 1.0f - (1.0f - boneSet->weight) * blendFrac;
        const float scaleWeight = boneSet->weight * weight;

        for (; s <= boneSet->lastBone && s < endSeg; ++s) {
            G2AnimSegValue sv1, sv2;
            InitializeSegValue(&model->segmentList[s], &sv1);
            InitializeSegValue(&model->segmentList[s], &sv2);

            uint32_t mask = ReadBitStream3(&bs1) | (ReadBitStream3(&bs2) << 4) | (ReadBitStream3(&bs3) << 8);
            const bool zeroMask = (mask == 0);

            float* p1 = &sv1.rot.x;
            float* p2 = &sv2.rot.x;
            while (mask != 0) {
                if (mask & 1) {
                    const int16_t type = *reinterpret_cast<const int16_t*>(animData);
                    float v1 = 0.0f;
                    float v2 = 0.0f;
                    if (type == 0) {
                        const float* arr = reinterpret_cast<const float*>(animData + 4);
                        v1 = arr[key1];
                        v2 = arr[key2];
                        animData += 4 + keyCount * 4;
                    } else if (type == 1) {
                        v1 = *reinterpret_cast<const float*>(animData + 4);
                        v2 = v1;
                        animData += 8;
                    } else if (type == 2) {
                        v1 = DecompressChannel_Linear(key1, reinterpret_cast<float*>(animData));
                        v2 = DecompressChannel_Linear(key2, reinterpret_cast<float*>(animData));
                        const int16_t n = *reinterpret_cast<const int16_t*>(animData + 2);
                        animData += 4 + (((n + 2) >> 2) + n) * 4;
                    }
                    *p1 = v1;
                    *p2 = v2;
                }
                mask >>= 1;
                ++p1;
                ++p2;
            }

            if (!zeroMask && interpAlpha != 0.0f) {
                if (s != 0) {
                    sv1.scale = sv1.scale + (sv2.scale - sv1.scale) * interpAlpha;
                    sv1.trans = sv1.trans + (sv2.trans - sv1.trans) * interpAlpha;
                }
                constexpr float kTwoPi = 6.283185482f;
                const cdc::Vector3 diff = cdc::Vector3::Make(
                    (sv2.rot.x - sv1.rot.x) / kTwoPi,
                    (sv2.rot.y - sv1.rot.y) / kTwoPi,
                    (sv2.rot.z - sv1.rot.z) / kTwoPi,
                    0.0f);
                const float sqDist = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (sqDist > 0.25f) {
                    const cdc::Vector3 unit = sv1.rot.UnitVec3();
                    const float wraps = static_cast<float>(static_cast<int>(std::sqrt(sqDist) + 0.5f));
                    sv1.rot = sv1.rot - unit * (wraps * kTwoPi);
                }
                sv1.rot = sv1.rot + (sv2.rot - sv1.rot) * interpAlpha;
            }

            if (b2) {
                sv1.scale = sv1.scale - cdc::Vector3::Make(1.0f, 1.0f, 1.0f, 0.0f);
                sv1.trans = sv1.trans - model->segmentList[s].pivot;
            }

            sv1.rot = sv1.rot * scaleWeight;
            sv1.rot.w = 0.0f;
            sv1.scale = sv1.scale * scaleWeight;
            sv1.trans = sv1.trans * scaleWeight;

            if (b0) {
                InitializeSegValue(&model->segmentList[s], &_segValues[s]);
                _segValues[s].rot = _segValues[s].rot * svWeight + sv1.rot;
                _segValues[s].scale = _segValues[s].scale * svWeight + sv1.scale;
                _segValues[s].trans = _segValues[s].trans * svWeight + sv1.trans;
            } else if (b1) {
                _segValues[s].rot = _segValues[s].rot * svWeight + sv1.rot;
                _segValues[s].scale = _segValues[s].scale * svWeight + sv1.scale;
                _segValues[s].trans = _segValues[s].trans * svWeight + sv1.trans;
            } else {
                _segValues[s].rot = _segValues[s].rot + sv1.rot;
                _segValues[s].scale = _segValues[s].scale + sv1.scale;
                _segValues[s].trans = _segValues[s].trans + sv1.trans;
            }
            _segValues[s].quat = cdc::Quat::Identity();
        }
    }

    if (b0 && !b3) {
        for (; s < endAll; ++s) {
            InitializeSegValue(&model->segmentList[s], &_segValues[s]);
        }
    }

    _segValuesAnim = reinterpret_cast<AnimProcessor*>(static_cast<intptr_t>(-1));
}

// 0x00472f50 (PDB: 0x004748d0)
void AnimProcessor::Update(bool updateRoot, bool force)
{
    if (mSectionCount == 0) {
        return;
    }

    if (mAnimComponent) {
        Instance* playerInst = static_cast<Instance*>(g_gameTracker.playerInstance);
        if (mAnimComponent->mInstance == playerInst &&
            this == reinterpret_cast<AnimProcessor*>(reinterpret_cast<uint8_t*>(mAnimComponent->mAnimProcessor) + sizeof(AnimProcessor))) {
            playerInvProcessAimBlend(playerInst, force);
        }
    }

    switch (mBlendType) {
    case 1: {
        Instance* playerInst = static_cast<Instance*>(g_gameTracker.playerInstance);
        const cdc::Vector& fwd = playerInst->matrix->col1;
        float dot = -cdc::Dot3(fwd, gPlayer->floorNormal);
        if (dot < -1.0f) dot = -1.0f;
        else if (dot > 1.0f) dot = 1.0f;
        const float angle = std::acos(dot) - 1.5707963705f;
        float slopeFrac = angle + mBlendValue1 * 0.436332315f;
        if (slopeFrac < -0.436332315f) slopeFrac = -0.436332315f;
        else if (slopeFrac > 0.436332315f) slopeFrac = 0.436332315f;
        slopeFrac *= 2.291831255f;

        float segSpeed[6];
        for (int i = 0; i < 6; ++i) {
            AnimFragment* kl = mSection[i].mKeylist;
            const cdc::Vector3 disp = cdc::Vector3::Make(kl->mTransX, kl->mTransY, kl->mTransZ, 0.0f);
            segSpeed[i] = std::sqrt(disp.Len3Squared()) / static_cast<float>(kl->mKeyCount);
        }

        const float speedBlend = gPlayer->speedBlend;
        if (slopeFrac <= 0.0f) {
            const float invSlope = 1.0f - (-slopeFrac);
            mWeights[0] = 0.0f;
            mWeights[1] = 0.0f;
            mWeights[2] = invSlope * (1.0f - speedBlend);
            mWeights[3] = invSlope * speedBlend;
            mWeights[4] = (-slopeFrac) * (1.0f - speedBlend);
            mWeights[5] = (-slopeFrac) * speedBlend;
        } else {
            const float invSlope = 1.0f - slopeFrac;
            mWeights[0] = slopeFrac * (1.0f - speedBlend);
            mWeights[1] = slopeFrac * speedBlend;
            mWeights[2] = invSlope * (1.0f - speedBlend);
            mWeights[3] = invSlope * speedBlend;
            mWeights[4] = 0.0f;
            mWeights[5] = 0.0f;
        }
        for (int i = 6; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        SetSectionSpeedsByKeyCount(mSection, mWeights, mSectionCount);

        float avgSpeed = 0.0f;
        for (int i = 0; i < 6; ++i) {
            avgSpeed += static_cast<float>(mSection[i].mKeylist->mKeyCount) * mWeights[i] * segSpeed[i];
        }
        const float scale = gPlayer->speed / avgSpeed;
        for (int i = 0; i < 6; ++i) {
            mSection[i].mSpeedAdjustment = static_cast<float>(mSection[i].mKeylist->mKeyCount) * scale;
        }
        break;
    }
    case 2:
        mWeights[0] = 1.0f - mBlendValue1;
        mWeights[1] = mBlendValue1;
        for (int i = 2; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        SetSectionSpeedsByKeyCount(mSection, mWeights, 2);
        break;
    case 3:
        mWeights[0] = (1.0f - mBlendValue2) * (1.0f - mBlendValue1);
        mWeights[1] = (1.0f - mBlendValue2) * mBlendValue1;
        mWeights[2] = mBlendValue2;
        for (int i = 3; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        SetSectionSpeedsByKeyCount(mSection, mWeights, 3);
        break;
    case 4:
        mWeights[0] = (1.0f - mBlendValue2) * (1.0f - mBlendValue1);
        mWeights[1] = (1.0f - mBlendValue2) * mBlendValue1;
        mWeights[2] = (1.0f - mBlendValue1) * mBlendValue2;
        mWeights[3] = mBlendValue1 * mBlendValue2;
        for (int i = 4; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        SetSectionSpeedsByKeyCount(mSection, mWeights, 4);
        break;
    case 5:
    case 8: {
        if (mBlendValue1 <= 0.0f) {
            const float t = mBlendValue1 + 1.0f;
            const float invU = 1.0f - mBlendValue2;
            const float invT = 1.0f - t;
            mWeights[0] = invT * invU;
            mWeights[1] = t * invU;
            mWeights[2] = 0.0f;
            mWeights[3] = invT * mBlendValue2;
            mWeights[4] = t * mBlendValue2;
            mWeights[5] = 0.0f;
        } else {
            const float invU = 1.0f - mBlendValue2;
            const float invT = 1.0f - mBlendValue1;
            mWeights[0] = 0.0f;
            mWeights[1] = invT * invU;
            mWeights[2] = mBlendValue1 * invU;
            mWeights[3] = 0.0f;
            mWeights[4] = invT * mBlendValue2;
            mWeights[5] = mBlendValue1 * mBlendValue2;
        }
        for (int i = 6; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    }
    case 6:
        if (mBlendValue1 <= 0.0f) {
            mWeights[0] = -mBlendValue1;
            mWeights[1] = mBlendValue1 + 1.0f;
            mWeights[2] = 0.0f;
        } else {
            mWeights[0] = 0.0f;
            mWeights[1] = 1.0f - mBlendValue1;
            mWeights[2] = mBlendValue1;
        }
        for (int i = 3; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    case 7:
        if (mBlendValue1 <= 0.0f) {
            mWeights[0] = mBlendValue1 + 1.0f;
            mWeights[1] = -mBlendValue1;
            mWeights[2] = 0.0f;
        } else {
            mWeights[0] = 1.0f - mBlendValue1;
            mWeights[1] = 0.0f;
            mWeights[2] = mBlendValue1;
        }
        for (int i = 3; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    case 9:
        mWeights[0] = 1.0f - mBlendValue1;
        mWeights[1] = mBlendValue1;
        for (int i = 2; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    case 10: {
        float a = mBlendValue1;
        float b = mBlendValue2;
        float absSum = std::fabs(a) + std::fabs(b);
        if (absSum > 1.0f) {
            const float inv = 1.0f / absSum;
            a *= inv;
            b *= inv;
            absSum = 1.0f;
        }
        mWeights[0] = 1.0f - absSum;
        mWeights[1] = (-a > 0.0f) ? -a : 0.0f;
        mWeights[2] = (a > 0.0f) ? a : 0.0f;
        mWeights[3] = (-b > 0.0f) ? -b : 0.0f;
        mWeights[4] = (b > 0.0f) ? b : 0.0f;
        for (int i = 5; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        SetSectionSpeedsByKeyCount(mSection, mWeights, 5);
        break;
    }
    case 11: {
        if (mBlendValue1 <= 0.0f) {
            const float t = mBlendValue1 + 1.0f;
            if (mBlendValue2 * 2.0f <= 1.0f) {
                const float u = mBlendValue2 * 2.0f;
                const float invU = 1.0f - u;
                const float invT = 1.0f - t;
                mWeights[0] = invT * invU;
                mWeights[1] = t * invU;
                mWeights[2] = 0.0f;
                mWeights[3] = invT * u;
                mWeights[4] = t * u;
                mWeights[5] = 0.0f;
                mWeights[6] = 0.0f;
                mWeights[7] = 0.0f;
                mWeights[8] = 0.0f;
            } else {
                const float u = mBlendValue2 * 2.0f - 1.0f;
                const float invU = 1.0f - u;
                const float invT = 1.0f - t;
                mWeights[0] = 0.0f;
                mWeights[1] = 0.0f;
                mWeights[2] = 0.0f;
                mWeights[3] = invT * invU;
                mWeights[4] = t * invU;
                mWeights[5] = 0.0f;
                mWeights[6] = invT * u;
                mWeights[7] = t * u;
                mWeights[8] = 0.0f;
            }
        } else {
            if (mBlendValue2 * 2.0f <= 1.0f) {
                const float u = mBlendValue2 * 2.0f;
                const float invU = 1.0f - u;
                const float invT = 1.0f - mBlendValue1;
                mWeights[0] = 0.0f;
                mWeights[1] = invT * invU;
                mWeights[2] = mBlendValue1 * invU;
                mWeights[3] = 0.0f;
                mWeights[4] = invT * u;
                mWeights[5] = mBlendValue1 * u;
                mWeights[6] = 0.0f;
                mWeights[7] = 0.0f;
                mWeights[8] = 0.0f;
            } else {
                const float u = mBlendValue2 * 2.0f - 1.0f;
                const float invU = 1.0f - u;
                const float invT = 1.0f - mBlendValue1;
                mWeights[0] = 0.0f;
                mWeights[1] = 0.0f;
                mWeights[2] = 0.0f;
                mWeights[3] = 0.0f;
                mWeights[4] = invT * invU;
                mWeights[5] = mBlendValue1 * invU;
                mWeights[6] = 0.0f;
                mWeights[7] = invT * u;
                mWeights[8] = mBlendValue1 * u;
            }
        }
        for (int i = 9; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    }
    case 12: {
        if (mBlendValue1 <= 0.0f) {
            mWeights[0] = 1.0f - mBlendValue2;
            mWeights[1] = 0.0f;
            mWeights[2] = 0.0f;
            mWeights[3] = mBlendValue2;
        }
        if (mBlendValue1 <= 0.5f) {
            const float t = mBlendValue1 * 2.0f;
            mWeights[0] = (1.0f - t) * (1.0f - mBlendValue2);
            mWeights[1] = (1.0f - mBlendValue2) * t;
            mWeights[2] = 0.0f;
            mWeights[3] = mBlendValue2;
        } else if (mBlendValue1 <= 1.0f) {
            const float t = (mBlendValue1 - 0.5f) * 2.0f;
            mWeights[0] = 0.0f;
            mWeights[1] = (1.0f - mBlendValue2) * (1.0f - t);
            mWeights[2] = (1.0f - mBlendValue2) * t;
            mWeights[3] = mBlendValue2;
        } else {
            mWeights[0] = 0.0f;
            mWeights[1] = 0.0f;
            mWeights[2] = 1.0f - mBlendValue2;
            mWeights[3] = mBlendValue2;
        }
        for (int i = 4; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    }
    default:
        mWeights[0] = 1.0f;
        for (int i = 1; i < mSectionCount; ++i) {
            mWeights[i] = 0.0f;
        }
        break;
    }

    mLastBlendValue1 = mBlendValue1;
    mLastBlendValue2 = mBlendValue2;

    float weightSum = 0.0f;
    for (int i = 0; i < mSectionCount; ++i) {
        weightSum += mWeights[i];
    }
    const float invWeightSum = 1.0f / weightSum;

    if (mClearBeforeUpdate && (mFlags & 0x10)) {
        SwapBones();
    }

    bool first = true;
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor* sec = &mSection[i];
        if (mWeights[i] != 0.0f || (first && i == mSectionCount - 1)) {
            AnimFragment* kl = sec->mKeylist;
            const float timePerKey = static_cast<float>(kl->mTimePerKey);
            const int storedKey = static_cast<int>(sec->mStoredTime / timePerKey);
            int key = static_cast<int>(sec->mElapsedTime / timePerKey);
            if (key >= kl->mKeyCount) {
                key = kl->mKeyCount - 1;
            }
            if (storedKey < 0 || storedKey > key) {
                sec->mStoredTime = -timePerKey;
                _segValuesAnim = reinterpret_cast<AnimProcessor*>(static_cast<intptr_t>(-1));
                mFlags &= static_cast<uint16_t>(~4);
            }
            const float interpAlpha = (sec->mElapsedTime <= 0.0f)
                ? 0.0f
                : (sec->mElapsedTime - static_cast<float>(key) * timePerKey) / timePerKey;

            bool b0 = false;
            bool b1 = false;
            bool b2 = false;
            float weight = 0.0f;
            float blendFrac = 0.0f;

            switch (mLayerBlendMode) {
            case 0:
                b0 = first;
                weight = mWeights[i] * mLayerWeight * invWeightSum;
                blendFrac = 1.0f - mLayerWeight;
                break;
            case 1:
                b1 = first;
                weight = mWeights[i] * mLayerWeight * invWeightSum;
                blendFrac = 1.0f - mLayerWeight;
                break;
            case 2:
                b2 = true;
                weight = mWeights[i] * mLayerWeight * invWeightSum;
                blendFrac = 1.0f - mLayerWeight;
                break;
            default:
                break;
            }

            if (force && first) {
                b0 = true;
            }

            sec->DecompressFrameWithBoneSet(mModelData, key, interpAlpha, weight, blendFrac, b0, b1, b2, mBoneSet, updateRoot);
            first = false;
        }
        sec->mFlags |= 0x80;
        sec->mStoredTime = sec->mElapsedTime;
    }

    if (mFlags & 0x10) {
        SwapBones();
    }
}

// 0x00474330 (PDB: 0x00475cb0)
int AnimProcessor::AnimCallback(AnimProcessor* ap, int sectionID, G2AnimCallbackMsg msg, int messageDataA, int messageDataB, void* callbackData)
{
    AnimProcessor* parent = static_cast<AnimProcessor*>(callbackData);
    if (reinterpret_cast<uintptr_t>(parent) < 0x1000) {
        return 1;
    }

    if (msg == G2ANIM_MSG_DONE) {
        parent->PopToNextAnim();
        if (parent->mBackIndex == parent->mFrontIndex) {
            if (parent->mOldCallback) {
                parent->mOldCallback(ap, sectionID, G2ANIM_MSG_SECTION_INTERPDONE, messageDataA, messageDataB, parent);
            }
            parent->ReleaseControl();
            return 1;
        }
        parent->mReadyForNextAnim = true;
        return 1;
    }

    if (msg == G2ANIM_MSG_LOOPPOINT) {
        if (parent->mBackIndex != parent->mFrontIndex) {
            parent->mReadyForNextAnim = true;
        }
        if (parent->mOldCallback) {
            parent->mOldCallback(ap, sectionID, G2ANIM_MSG_LOOPPOINT, messageDataA, messageDataB, parent);
        }
        return 1;
    }

    if (parent->mOldCallback) {
        return parent->mOldCallback(ap, sectionID, msg, messageDataA, messageDataB, parent->mOldCallbackData);
    }

    return 1;
}

static AnimController* FindControllerAny(AnimProcessor* ap, int seg, int type)
{
    AnimController* ac = AnimController::FindInList(seg, type, &ap->mControllerList);
    if (ac == nullptr) {
        ac = AnimController::FindInList(seg, type, &ap->mDisabledControllerList);
    }
    return ac;
}

// 0x00474400 (PDB: 0x00475d80)
AnimProcessor::~AnimProcessor()
{
    if (mVirtualMachine != nullptr) {
        delete mVirtualMachine;
    }

    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        G2AnimInterpInfo* info = (sec.mFlags & 0x200) ? &mInterpInfo : nullptr;
        if (info != nullptr && info->stateBlockList != nullptr) {
            sec.mElapsedTime = info->targetTime;
        }
        if (sec.mSectionID == 0) {
            if (sec.mFlags & 0x200) {
                FreeInterpStateBlockList(info->stateBlockList);
                info->stateBlockList = nullptr;
            }
            sec.mFlags |= 0x200;
            info->duration = 0.0f;
            info->targetTime = 0.0f;
            info->alphaTable = nullptr;
            info->stateBlockList = nullptr;
        } else {
            sec.mFlags &= ~0x200;
        }
    }

    delete[] mWeights;
    mWeights = nullptr;
    delete[] mSection;
    mSection = nullptr;
    mSectionCount = 0;
}

// 0x00474520 (PDB: 0x00475ea0)
void AnimProcessor::PopBackToMarker()
{
    if (mBackIndex != mMarker) {
        while (mBackIndex != mFrontIndex) {
            --mBackIndex;
            if (mBackIndex < 0) {
                mBackIndex = 15;
            }
            if (mBackIndex == mMarker) {
                break;
            }
        }
    }
}

// 0x00474560 (PDB: 0x00475ee0)
bool AnimProcessor::StopPattern()
{
    if (mCurrentPatternIndex != -1) {
        if (mCurrentPatternIsNative) {
            sNativePattern[mCurrentPatternIndex]->Exit(this);
        } else {
            mVirtualMachine->CallVirtualMethod(
                mAnimComponent->mInstance->object->animPatternList[mCurrentPatternIndex],
                2);
        }
        mCurrentPatternIndex = -1;
        mCurrentPatternId = -1;
        ReleaseControl();
    }
    return true;
}

// 0x004745d0 (PDB: 0x00475f50)
bool AnimProcessor::StartPattern(int patternId)
{
    mCurrentPatternIsNative = (patternId & 0xffff0000) == 0x10000;
    mCurrentPatternId = patternId;
    mAnimWasPlayed = false;
    mCurrentPatternIndex = patternId & 0xffff;
    if (mCurrentPatternIndex != -1) {
        if (mVirtualMachine == nullptr) {
            mVirtualMachine = new VirtualMachine(this);
        }
        if (mCurrentPatternIsNative) {
            sNativePattern[mCurrentPatternIndex]->Entry(this);
        } else {
            mVirtualMachine->CallVirtualMethod(
                mAnimComponent->mInstance->object->animPatternList[mCurrentPatternIndex],
                0);
        }
    }
    return mAnimWasPlayed;
}

// 0x00474690 (PDB: 0x00476010)
void AnimProcessor::SetAlphaTable(G2AnimAlphaTable* alphaTable)
{
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        G2AnimInterpInfo* info = (sec.mFlags & 0x200) ? &mInterpInfo : nullptr;
        if (info != nullptr) {
            info->alphaTable = alphaTable;
        }
    }
}

// 0x004746e0 (PDB: 0x00476060)
void AnimProcessor::SetInterpInfo()
{
    AnimFragmentProcessor& sec = mSection[mMasterSection];
    G2AnimInterpInfo* info = &mInterpInfo;
    if (sec.mSectionID == 0) {
        if (sec.mFlags & 0x200) {
            FreeInterpStateBlockList(info->stateBlockList);
            info->stateBlockList = nullptr;
        }
        sec.mFlags |= 0x200;
        info->duration = 0.0f;
        info->targetTime = 0.0f;
        info->alphaTable = nullptr;
        info->stateBlockList = nullptr;
    } else {
        sec.mFlags &= ~0x200;
    }
}

// 0x00474740 (PDB: 0x004760c0)
int AnimFragmentProcessor::GetKeyframeNumber() const
{
    G2AnimInterpInfo* info = ((mFlags & 0x200) && mAnimProcessor != nullptr)
        ? &mAnimProcessor->mInterpInfo
        : nullptr;
    if (info != nullptr && info->stateBlockList != nullptr) {
        return static_cast<int>(info->targetTime / static_cast<float>(mKeylist->mTimePerKey));
    }
    return static_cast<int>(mElapsedTime / static_cast<float>(mKeylist->mTimePerKey));
}

// 0x00474790 (PDB: 0x00476110)
void AnimFragmentProcessor::SetLooping()
{
    mAlarmFlags &= ~0x3u;
    mFlags |= 0x2;
    mLoopStartTime = 0.0f;
    mLoopEndTime = static_cast<float>(mKeylist->mTimePerKey * mKeylist->mKeyCount);
}

// 0x004747c0 (PDB: 0x00476140)
AnimTweaker::AnimTweaker()
{
    mOriginal.Identity();
    mCurrentCorrection.Identity();
    mModified.Identity();
    mTotalCorrection.q = cdc::Quat::Identity();
    mTotalCorrection.t = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    mFrames = 0.0f;
    mIsRunning = false;
}

// 0x00474850 (PDB: 0x004761d0)
void AnimTweaker::Start(const cdc::Vector3& translation, float frames)
{
    mOriginal.Identity();
    mCurrentCorrection.Identity();
    mModified.Identity();
    mTotalCorrection.q = cdc::Quat::Identity();
    mTotalCorrection.t.x = translation.x;
    mTotalCorrection.t.y = translation.y;
    mTotalCorrection.t.z = translation.z;
    mTotalCorrection.t.w = translation.w;
    mFrames = frames;
    mIsRunning = true;
}

// 0x004748f0 (PDB: 0x00476270)
void AnimProcessor::SetSectionCount(int count)
{
    while (mSectionCount < count) {
        AddSection(0, 0);
    }
    mSectionCount = static_cast<uint8_t>(count);
}

// 0x00474930 (PDB: 0x004762b0)
bool AnimProcessor::DoesControllerExist(int seg, int type)
{
    return FindControllerAny(this, seg, type) != nullptr;
}

// 0x00474980 (PDB: 0x00476300)
void AnimProcessor::SetControllerAlphaTable(int seg, int type, G2AnimAlphaTable* alphaTable)
{
    FindControllerAny(this, seg, type)->mAlphaTable = alphaTable;
}

// 0x004749d0 (PDB: 0x00476350)
void AnimProcessor::SetControllerCallbackData(int seg, int type, void* data)
{
    FindControllerAny(this, seg, type)->mCallbackData = data;
}

// 0x00474a20 (PDB: 0x004763a0)
void AnimProcessor::SetControllerAngleOrder(int seg, int type, int order)
{
    AnimController* ac = FindControllerAny(this, seg, type);
    ac->mFlags = static_cast<uint16_t>((ac->mFlags & 0xff00) | order);
}

// 0x00474a70 (PDB: 0x004763f0)
void AnimProcessor::SetController_Vector(int seg, int type, cdc::Vector* vector)
{
    AnimController* ac = FindControllerAny(this, seg, type);
    ac->mData.vector.base.x = vector->x;
    ac->mData.vector.base.y = vector->y;
    ac->mData.vector.base.z = vector->z;
    *(reinterpret_cast<uint8_t*>(&ac->mFlags) + 1) = 0;
}

// 0x00474ad0 (PDB: 0x00476450)
void AnimProcessor::SetController_Function(int seg, int type, AnimControllerCallback func, void* data)
{
    AnimController* ac = FindControllerAny(this, seg, type);
    ac->mData.callback.fn = func;
    ac->mData.callback.fnData = data;
}

// 0x00474b20 (PDB: 0x004764a0)
unsigned int AnimController::ApplyWorldToParentMatrix(cdc::Matrix* parentMatrix)
{
    switch (mType) {
    case 0x08:
        GetMatrix(parentMatrix);
        break;
    case 0x20: {
        cdc::Vector3 v;
        GetVector(&v);
        parentMatrix->SetTrans(v);
        break;
    }
    case 0x4c: {
        cdc::Matrix m;
        GetMatrix(&m);
        cdc::Mul3x3(parentMatrix, *parentMatrix, m);
        break;
    }
    case 0x54: {
        cdc::Vector3 v;
        GetVector(&v);
        parentMatrix->Scale(v);
        break;
    }
    case 0x64: {
        cdc::Vector3 v;
        GetVector(&v);
        parentMatrix->col3.x += v.x;
        parentMatrix->col3.y += v.y;
        parentMatrix->col3.z += v.z;
        break;
    }
    default:
        break;
    }
    return 7;
}

// 0x00474c80 (PDB: 0x00476600)
void AnimFragmentProcessor::UpdateStoredFrameFromQuat()
{
    if (this != &mAnimProcessor->mSection[0]) {
        return;
    }

    mAnimProcessor->Update(false, true);

    int segCount = mSegCount;
    G2AnimInterpInfo* info = (mFlags & 0x200) ? &mAnimProcessor->mInterpInfo : nullptr;
    float alpha = info->alphaTable->GetValue(
        1.0f - (info->targetTime - mElapsedTime) / info->duration);

    G2AnimSegValue* sv = &_segValues[mFirstSeg];
    G2AnimInterpStateBlock* stateBlock = info->stateBlockList;
    G2AnimQuatInfo* qi = stateBlock->quatInfo;
    int blockRem = 4;

    if (segCount > 0) {
        float w1 = alpha;
        float w0 = 1.0f - alpha;
        while (segCount > 0) {
            cdc::Quat destQuat = MakeQuat(sv->rot.x, sv->rot.y, sv->rot.z, sv->rot.w);
            cdc::Quat srcQuat = qi->srcQuat;
            if (segCount == 1) {
                WrapQuatLog(&srcQuat, destQuat);
            }
            cdc::Quat blendedRot = MakeQuat(
                w0 * srcQuat.x + w1 * destQuat.x,
                w0 * srcQuat.y + w1 * destQuat.y,
                w0 * srcQuat.z + w1 * destQuat.z,
                w0 * srcQuat.w + w1 * destQuat.w);
            sv->quat = blendedRot;
            sv->rot = QuatVec3(blendedRot);
            sv->scale = qi->srcScale * w0 + sv->scale * w1;
            sv->trans = qi->srcTrans * w0 + sv->trans * w1;

            --segCount;
            ++qi;
            --blockRem;
            if (blockRem == 0) {
                stateBlock = stateBlock->next;
                qi = stateBlock->quatInfo;
                blockRem = 4;
            }
            sv->rot.w = 0.0f;
            ++sv;
        }
    }

    mStoredTime = mElapsedTime;
    mFlags |= 0x80;
    _segValuesAnim = reinterpret_cast<AnimProcessor*>(static_cast<intptr_t>(-1));
}

// 0x00474e30 (PDB: 0x004767b0)
void AnimProcessor::Stop()
{
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        if (sec.mKeylist != nullptr) {
            sec.TriggerEffects(0.0f, 0.0f, 0x400);
        }
        sec.mKeylist = nullptr;
        sec.mKeylistID = 0xffff;
        sec.mFxListHost = nullptr;
        sec.mFxList = nullptr;
    }
    mSectionCount = 0;
}

// 0x00474eb0 (PDB: 0x00476830)
void AnimProcessor::UpdateStoredFrame()
{
    if (mSectionCount == 0 || mLayerWeight <= 0.0f) {
        return;
    }

    AnimFragmentProcessor* master = mSection;
    G2AnimInterpInfo* info = (master->mFlags & 0x200) ? &mInterpInfo : nullptr;
    if (info == nullptr || info->stateBlockList == nullptr) {
        Update(false, false);
        mFlags |= 4;
        return;
    }

    if (mLayerBlendMode == 0 && mLayerWeight == 1.0f) {
        master->UpdateStoredFrameFromQuat();
        mFlags |= 4;
        return;
    }

    int numSegs = mModelData->numSegments;
    G2AnimSegValue* oldSegValues = _segValues;
    _segValues = new G2AnimSegValue[numSegs];
    std::memcpy(_segValues, oldSegValues, static_cast<size_t>(numSegs) * sizeof(G2AnimSegValue));

    master->UpdateStoredFrameFromQuat();

    float wNew = mLayerWeight;
    float wOld = 1.0f - mLayerWeight;
    if (mLayerBlendMode == 2) {
        wNew = 1.0f;
        wOld = 1.0f;
    }

    const BoneSet* bs = (mBoneSet != nullptr) ? mBoneSet : allBones;
    int seg = 0;
    while (seg < numSegs && bs->firstBone != 0xffff) {
        float segWeight = bs->weight;
        float scaleOld = 1.0f - (1.0f - wOld) * segWeight;
        float scaleNew = wNew * segWeight;

        while (seg < numSegs && seg <= static_cast<int>(static_cast<int16_t>(bs->lastBone))) {
            G2AnimSegValue& newSv = _segValues[seg];
            G2AnimSegValue& oldSv = oldSegValues[seg];

            if (mLayerBlendMode == 0) {
                cdc::Quat blended = MakeQuat(
                    newSv.rot.x * scaleNew,
                    newSv.rot.y * scaleNew,
                    newSv.rot.z * scaleNew,
                    newSv.rot.w * scaleNew);
                oldSv.quat = blended;
                oldSv.rot = QuatVec3(blended);
                oldSv.scale = newSv.scale * scaleNew;
                oldSv.trans = newSv.trans * scaleNew;
            } else if (mLayerBlendMode == 1) {
                cdc::Quat blended = MakeQuat(
                    oldSv.rot.x * scaleOld + newSv.rot.x * scaleNew,
                    oldSv.rot.y * scaleOld + newSv.rot.y * scaleNew,
                    oldSv.rot.z * scaleOld + newSv.rot.z * scaleNew,
                    oldSv.rot.w * scaleOld + newSv.rot.w * scaleNew);
                oldSv.quat = blended;
                oldSv.rot = QuatVec3(blended);
                oldSv.scale = oldSv.scale * scaleOld + newSv.scale * scaleNew;
                oldSv.trans = oldSv.trans * scaleOld + newSv.trans * scaleNew;
            } else {
                oldSv.rot = oldSv.rot + newSv.rot * scaleNew;
                oldSv.scale = oldSv.scale + newSv.scale * scaleNew;
                oldSv.trans = oldSv.trans + newSv.trans * scaleNew;
            }
            oldSv.rot.w = 0.0f;
            ++seg;
        }
        ++bs;
    }

    delete[] _segValues;
    _segValues = oldSegValues;
    mFlags |= 4;
}

// 0x00475220 (PDB: 0x00476ba0)
void AnimProcessor::BuildSegLocalRotMatrix(G2AnimSegValue* segValue, cdc::Matrix* matrix)
{
    matrix->Identity();
    if (segValue->rot.w == 0.0f) {
        G2Quat_ToMatrixf(&segValue->quat, matrix);
    } else {
        cdc::Quat q = MakeQuat(segValue->rot.x, segValue->rot.y, segValue->rot.z, segValue->rot.w);
        MATH3D_QuatLogToMatrix(matrix, &q);
    }
}

// 0x004752a0 (PDB: 0x00476c20)
AnimProcessor::AnimProcessor()
    : mAnimTweaker()
{
    for (int i = 0; i < 16; ++i) {
        mAnimQueue[i].mSpeed = 1.0f;
        mAnimQueue[i].mAnimId = 0xffff;
        mAnimQueue[i].mFlags = 0;
        mAnimQueue[i].mRepeatCount = 1;
        mAnimQueue[i].mTargetFrame = 0xffff;
        mAnimQueue[i].mInterpFrames = 0xffff;
    }

    mRootTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    mRootRot = cdc::Quat::Identity();

    mModelData = nullptr;
    mSection = nullptr;
    mWeights = nullptr;
    mSectionsAllocated = 0;
    mSectionCount = 0;

    AllocateSections(1);

    mMasterSection = 0;
    mBlendType = 0;
    mLastBlendValue1 = 0.0f;
    mLastBlendValue2 = 0.0f;
    mSection[0].mSegCount = 0;
    mBlendValue1 = 0.5f;
    mBlendValue2 = 0.5f;

    mCurrentPatternIndex = -1;
    mCurrentPatternId = -1;
    mCurrentBlendId = -1;
    mLayerWeight = 1.0f;
    mStartRate = 1.0f;
    mEndRate = 1.0f;

    mControllerList = 0;
    mDisabledControllerList = 0;
    mSegMatrices = nullptr;
    mFlags = 0;
    mBoneSet = nullptr;
    mLayerBlendMode = 0;
    mFrontIndex = 0;
    mBackIndex = 0;
    mMarker = 0;
    mHasControl = false;
    mReadyForNextAnim = false;
    mMirror = false;
    mUseRateRamp = false;
    mAnimComponent = nullptr;
    mCurrentPatternIsNative = false;
    mVirtualMachine = nullptr;
    mAnimWasPlayed = false;

    mInterpInfo.targetTime = 0.0f;
    mInterpInfo.duration = 0.0f;
    mInterpInfo.stateBlockList = nullptr;
    mInterpInfo.alphaTable = nullptr;

    mClearBeforeUpdate = true;
    mVectorVar[0] = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    mVectorVar[1] = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
}

// 0x00475460 (PDB: 0x00476de0)
bool AnimProcessor::ExecutePattern(int patternId, bool mirror)
{
    StopPattern();
    mMirror = mirror;
    return StartPattern(patternId);
}

// 0x00475490 (PDB: 0x00476e10)
int AnimProcessor::GetKeyframeNumber()
{
    return mSection[mMasterSection].GetKeyframeNumber();
}

// 0x004754b0 (PDB: 0x00476e30)
void AnimProcessor::NextKeyframe()
{
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        if (!(sec.mFlags & 1)) {
            sec.mAccumulatedTime += G2Timer_GetFrameTime();
            sec.AdvanceOverAccumulatedTime();
        }
    }
}

// 0x00475500 (PDB: 0x00476e80)
void AnimProcessor::NextKeyframeTimeDelta(float delta)
{
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        if (!(sec.mFlags & 1)) {
            sec.mAccumulatedTime += delta;
            sec.AdvanceOverAccumulatedTime();
        }
    }
}

static inline void SyncMirroredTransformFlag(AnimProcessor* ap)
{
    if (ap->mFlags & 0x20) {
        ap->mFlags |= 0x10;
    } else {
        ap->mFlags &= ~0x10u;
    }
}

static inline void ApplyMaskedVector(cdc::Vector3& dst, const cdc::Vector3& src, uint16_t flags, bool additive)
{
    if (!(flags & 0x1000)) dst.x = additive ? (dst.x + src.x) : src.x;
    if (!(flags & 0x0800)) dst.y = additive ? (dst.y + src.y) : src.y;
    if (!(flags & 0x0400)) dst.z = additive ? (dst.z + src.z) : src.z;
}

static inline void EvaluateStoredFrameWithControllers(AnimProcessor* ap)
{
    ap->UpdateStoredFrame();
    _segValues[0].trans = ap->mRootTrans;
    ap->ApplyControllersToStoredFrame();
}

static inline void CopySegValueBitmask(void* dstVoid, const G2AnimSegValue* sv, uint16_t mask)
{
    uint32_t* dst = reinterpret_cast<uint32_t*>(dstVoid);
    const uint32_t* src = reinterpret_cast<const uint32_t*>(sv);
    while (mask != 0) {
        if (mask & 1u) {
            *dst++ = *src;
        }
        mask >>= 1;
        ++src;
    }
}

// 0x00475550 (PDB: 0x00476ed0)
void AnimProcessor::SetLooping()
{
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        sec.mAlarmFlags &= ~3u;
        sec.mFlags |= 2;
        sec.mLoopStartTime = 0.0f;
        sec.mLoopEndTime = static_cast<float>(static_cast<int>(sec.mKeylist->mKeyCount) * static_cast<int>(sec.mKeylist->mTimePerKey));
    }
}

// 0x004755b0 (PDB: 0x00476f30)
unsigned int AnimController::ApplyToSegValue(G2AnimSegValue* segValue, cdc::Matrix* parentMatrix, cdc::Matrix* segMatrix)
{
    if (mSegNumber == 0 && !(mType & 2)) {
        return ApplyWorldToParentMatrix(parentMatrix);
    }

    switch (mType) {
    case 0x08:
        GetMatrix(segMatrix);
        return 6;
    case 0x0a:
        if ((mFlags & 0xc000) == 0xc000) {
            mData.quat.dest.Build(*segMatrix);
        }
        GetMatrix(segMatrix);
        return 7;
    case 0x0e: {
        cdc::Matrix m;
        GetMatrix(&m);
        cdc::Mul3x3(segMatrix, m, *segMatrix);
        return 7;
    }
    case 0x10:
    case 0x12: {
        cdc::Vector3 vec;
        GetVector(&vec);
        ApplyMaskedVector(segValue->scale, vec, mFlags, false);
        return (mType == 0x10) ? 5u : 7u;
    }
    case 0x16: {
        cdc::Vector3 vec;
        GetVector(&vec);
        ApplyMaskedVector(segValue->scale, vec, mFlags, true);
        return 7;
    }
    case 0x20: {
        cdc::Vector3 vec;
        GetVector(&vec);
        cdc::Vector3 rel = vec - parentMatrix->col3;
        ApplyMaskedVector(segValue->trans, rel, mFlags, false);
        return 3;
    }
    case 0x22: {
        cdc::Vector3 vec;
        GetVector(&vec);
        ApplyMaskedVector(segValue->trans, vec, mFlags, false);
        return 7;
    }
    case 0x26: {
        cdc::Vector3 vec;
        GetVector(&vec);
        float prevTime = mElapsedTime - G2Timer_GetFrameTime();
        if (mSegNumber == 0 && prevTime > 0.0f) {
            cdc::Vector3 prev = mData.vector.base;
            if (mFlags & 0x4000) {
                MATH3D_InterpOnePlusAlpha(&prev, &mData.vector.offset, &prev, prevTime * (1.0f / 4096.0f));
            }
            vec.x -= prev.x;
            vec.y -= prev.y;
            vec.z -= prev.z;
        }
        ApplyMaskedVector(segValue->trans, vec, mFlags, true);
        return 7;
    }
    case 0x4c: {
        cdc::Mul3x3(segMatrix, *parentMatrix, *segMatrix);
        cdc::Matrix m;
        GetMatrix(&m);
        cdc::Mul3x3(segMatrix, m, *segMatrix);
        return 6;
    }
    case 0x80:
        return mData.callback.fn(this, segValue, parentMatrix, segMatrix, mData.callback.fnData);
    default:
        return 7;
    }
}

// 0x00475950 (PDB: 0x004772d0)
void AnimFragmentProcessor::ApplyControllersToStoredFrame()
{
    AnimProcessor* ap = mAnimProcessor;
    if (ap->mControllerList == 0) {
        return;
    }

    AnimController* ac = &_controllerPool.blockPool[ap->mControllerList];
    Segment* segments = ap->mModelData->segmentList;
    const int endSeg = mFirstSeg + mSegCount;
    for (int seg = mFirstSeg; seg < endSeg; ++seg) {
        cdc::Matrix localMat;
        cdc::Matrix parentMat;
        if (ac->mSegNumber == seg) {
            AnimProcessor::BuildSegLocalRotMatrix(&_segValues[seg], &localMat);
            if (ap->mSegMatrices != nullptr) {
                parentMat = ap->mSegMatrices[segments[seg].parent];
            } else {
                parentMat.Identity();
            }
        }

        while (ac > &_controllerPool.blockPool[0] && ac->mSegNumber == seg) {
            if (seg != 0 || (ac->mType & 2)) {
                if ((ac->mType & 0x38) == 8) {
                    if (ac->mType != 0x80) {
                        ac->ApplyToSegValue(&_segValues[seg], &parentMat, &localMat);
                    }
                    if (!(ac->mType & 2)) {
                        cdc::Matrix invParent;
                        cdc::OrthonormalInverse3x3(&invParent, parentMat);
                        cdc::Mul3x3(&localMat, invParent, localMat);
                    }
                    if (ac->mDuration == 0.0f) {
                        _G2EulerAngles_Type euler;
                        G2EulerAngles_FromMatrixf(&euler, &localMat, 0);
                        _segValues[seg].rot.x = euler.x;
                        _segValues[seg].rot.y = euler.y;
                        _segValues[seg].rot.z = euler.z;
                    } else {
                        G2Quat_FromMatrixf(&_segValues[seg].quat, &localMat);
                    }
                } else if (ac->mType != 0x80) {
                    ac->ApplyToSegValue(&_segValues[seg], &parentMat, &localMat);
                }
            }
            ac = &_controllerPool.blockPool[ac->mNext];
        }
    }
}

// 0x00475b50 (PDB: 0x004774d0)
void AnimFragmentProcessor::InterpToKeylistAtTime(AnimFragment* keylist, int keylistID, float targetTime, float duration, void* fxListHost, AnimFxHeader* fxList)
{
    AnimProcessor* ap = mAnimProcessor;
    if (this != &ap->mSection[0]) {
        targetTime -= duration;
    } else if (duration != 0.0f) {
        G2AnimInterpInfo* info = (mFlags & 0x200) ? &ap->mInterpInfo : nullptr;
        cdc::Vector3 oldDelta = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

        if (info->stateBlockList != nullptr) {
            float startT = mElapsedTime - 1.0f;
            float remT = info->targetTime - info->duration;
            if (startT < remT) {
                startT = remT;
            }
            cdc::Quat dummyRot;
            GetRootMotionFromTimeForDuration(startT, 1.0f, &oldDelta, &dummyRot);
            UpdateStoredFrameFromQuat();
            SegValueToQuat(_segValues, 0);
        } else {
            if (!AnimProcessor::AllocateInterpStateBlockList(this)) {
                SwitchToKeylistAtTimeWithoutAdvancing(keylist, keylistID, targetTime, fxListHost, fxList);
                AdvanceOverAccumulatedTime();
                return;
            }
            mFlags &= ~0x200u;
            const float savedB1 = ap->mBlendValue1;
            const float savedB2 = ap->mBlendValue2;
            ap->mBlendValue1 = ap->mLastBlendValue1;
            ap->mBlendValue2 = ap->mLastBlendValue2;
            const BoneSet* savedBoneSet = ap->mBoneSet;
            ap->mBoneSet = nullptr;
            ap->Update(false, true);
            ap->mBoneSet = savedBoneSet;
            ap->mBlendValue1 = savedB1;
            ap->mBlendValue2 = savedB2;
            mFlags |= 0x200u;
            SegValueToQuat(_segValues, 0);

            const float maxTime = static_cast<float>(static_cast<int>(mKeylist->mKeyCount) * static_cast<int>(mKeylist->mTimePerKey));
            const float clampTime = (mElapsedTime < maxTime) ? mElapsedTime : maxTime;
            if (mNewRootInterpMode == kRootInterpMode_MorphVelocity || mNewRootInterpMode == kRootInterpMode_MorphSpeed) {
                cdc::Quat rootRot;
                cdc::Vector3 p1;
                ANIM_GetTransformAtTime(&rootRot, &p1, mKeylist, 0, clampTime);
                cdc::Vector3 p0 = ANIM_GetTranslationAtTime(mKeylist, 0, clampTime - 1.0f);
                oldDelta = p1 - p0;
                if (mNewRootInterpMode == kRootInterpMode_MorphVelocity) {
                    rootRot.Normalize();
                    oldDelta = rootRot.Inverse().Rotate(oldDelta);
                }
            }
        }

        const float oldSpeed = oldDelta.Len3();
        const uint32_t savedAlarmFlags = mAlarmFlags;
        mFlags &= ~0x200u;
        SwitchToKeylistAtTimeWithoutAdvancing(keylist, keylistID, targetTime, fxListHost, fxList);
        SyncMirroredTransformFlag(ap);
        ap->Update(false, true);
        mFlags |= 0x200u;
        mAlarmFlags = savedAlarmFlags;
        SegValueToQuat(_segValues, 1);

        G2AnimInterpStateBlock* sb = info->stateBlockList;
        G2AnimQuatInfo* qi = &sb->quatInfo[0];
        int qiIdx = 4;
        mRootInterpMode = mNewRootInterpMode;

        switch (mNewRootInterpMode) {
        case kRootInterpMode_ConserveMotion:
            ANIM_GetTransformAtTime(&qi->srcQuat, &qi->srcTrans, keylist, 0, targetTime - duration);
            ANIM_GetTransformAtTime(&qi->destQuat, &qi->destTrans, keylist, 0, targetTime);
            break;
        case kRootInterpMode_MorphSpeed: {
            cdc::Vector3 curP = ANIM_GetTranslationAtTime(keylist, 0, mElapsedTime);
            cdc::Vector3 prevP = ANIM_GetTranslationAtTime(keylist, 0, mElapsedTime - 1.0f);
            cdc::Vector3 newDelta = curP - prevP;
            qi->srcQuat.Identity();
            qi->srcTrans = cdc::Vector3::Make(oldSpeed, 0.0f, 0.0f, 0.0f);
            qi->destQuat.Identity();
            qi->destTrans = cdc::Vector3::Make(newDelta.Len3(), 0.0f, 0.0f, 0.0f);
            break;
        }
        case kRootInterpMode_MorphVelocity: {
            cdc::Vector3 curP = ANIM_GetTranslationAtTime(keylist, 0, mElapsedTime);
            cdc::Vector3 prevP = ANIM_GetTranslationAtTime(keylist, 0, mElapsedTime - 1.0f);
            qi->srcQuat.Identity();
            qi->srcTrans = oldDelta;
            qi->destQuat.Identity();
            qi->destTrans = curP - prevP;
            break;
        }
        case kRootInterpMode_UserTransform:
            qi->srcQuat.Identity();
            qi->srcTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            qi->destQuat = AnimProcessor::sUserTransformRotation;
            qi->destTrans = AnimProcessor::sUserTransformTranslation;
            AnimProcessor::sUserTransformRotation.Identity();
            AnimProcessor::sUserTransformTranslation = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            if (ap->mFlags & 0x20) {
                qi->destTrans.x = -qi->destTrans.x;
                qi->destQuat.y = -qi->destQuat.y;
                qi->destQuat.z = -qi->destQuat.z;
            }
            break;
        default:
            break;
        }

        bool skipRootTrans = (mFirstSeg == 0);
        for (int s = 0; s < mSegCount; ++s) {
            qi->destScale.x -= qi->srcScale.x;
            qi->destScale.y -= qi->srcScale.y;
            qi->destScale.z -= qi->srcScale.z;
            if (skipRootTrans) {
                skipRootTrans = false;
            } else {
                qi->destTrans.x -= qi->srcTrans.x;
                qi->destTrans.y -= qi->srcTrans.y;
                qi->destTrans.z -= qi->srcTrans.z;
            }
            ++qi;
            if (--qiIdx == 0) {
                sb = sb->next;
                qi = &sb->quatInfo[0];
                qiIdx = 4;
            }
        }

        info->duration = duration;
        info->targetTime = targetTime;
        mElapsedTime = targetTime - duration;
        mStoredTime = targetTime - duration;
        if (mFlags & 2) {
            mLoopStartTime = 0.0f;
            mLoopEndTime = static_cast<float>(static_cast<int>(mKeylist->mKeyCount) * static_cast<int>(mKeylist->mTimePerKey));
        }
        mAlarmFlags &= ~3u;
        mFlags &= ~0x81u;
        mSwAlarmTable = nullptr;
        ap->mFlags |= 1;
        AdvanceOverAccumulatedTime();
        return;
    }

    SyncMirroredTransformFlag(ap);
    SwitchToKeylistAtTimeWithoutAdvancing(keylist, keylistID, targetTime, fxListHost, fxList);
    AdvanceOverAccumulatedTime();
    ap->Update(false, true);
}

// 0x004762b0 (PDB: 0x00477c30)
void AnimProcessor::BuildSegTransformNoControllers(cdc::Matrix* segMatrix, cdc::Matrix* parentMatrix, bool b0, bool b1, int segIndex)
{
    G2AnimSegValue* sv = &_segValues[segIndex];
    BuildSegLocalRotMatrix(sv, segMatrix);
    segMatrix->Scale(sv->scale);
    if (b0) {
        if (b1) {
            segMatrix->SetTrans(sv->trans);
            cdc::Mul4x4(parentMatrix, *parentMatrix, *segMatrix);
            *segMatrix = *parentMatrix;
        } else {
            segMatrix->SetTrans(cdc::Mul3x3(*parentMatrix, sv->trans));
            parentMatrix->col3 = parentMatrix->col3 + segMatrix->col3;
            segMatrix->col3 = parentMatrix->col3;
        }
    } else {
        segMatrix->SetTrans(sv->trans);
        cdc::Mul4x4(segMatrix, *parentMatrix, *segMatrix);
    }
}

// 0x00476390 (PDB: 0x00477d10)
void AnimProcessor::SwitchToKeylist(AnimFragment* keylist, int keylistID, void* fxListHost, AnimFxHeader* fxList)
{
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        sec.SwitchToKeylistAtTimeWithoutAdvancing(keylist, keylistID, 0.0f, fxListHost, fxList);
        sec.AdvanceOverAccumulatedTime();
    }
    SyncMirroredTransformFlag(this);
}

// 0x00476420 (PDB: 0x00477da0)
void AnimProcessor::SwitchToKeylistFrame(AnimFragment* keylist, int keylistID, int frame, void* fxListHost, AnimFxHeader* fxList)
{
    const float t = static_cast<float>(frame * static_cast<int>(keylist->mTimePerKey));
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        sec.SwitchToKeylistAtTimeWithoutAdvancing(keylist, keylistID, t, fxListHost, fxList);
        sec.AdvanceOverAccumulatedTime();
    }
    SyncMirroredTransformFlag(this);
}

// 0x004764c0 (PDB: 0x00477e40)
void AnimFragmentProcessor::InterpToKeylistFrame(AnimFragment* keylist, int keylistID, int frame, float duration, void* fxListHost, AnimFxHeader* fxList)
{
    if (debugRazielFlags1 & 0x80) {
        duration = 0.0f;
    }
    const float targetTime = static_cast<float>(frame * static_cast<int>(keylist->mTimePerKey));
    InterpToKeylistAtTime(keylist, keylistID, targetTime, duration, fxListHost, fxList);
}

// 0x00476510 (PDB: 0x00477e90)
void AnimProcessor::ApplyControllersToStoredFrame()
{
    if (mFlags & 8) {
        return;
    }
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].ApplyControllersToStoredFrame();
    }
    mFlags |= 8;
}

// 0x00476560 (PDB: 0x00477ee0)
void AnimProcessor::BuildSegTransformWithControllers(cdc::Matrix* segMatrix, cdc::Matrix* parentMatrix, AnimController* ac, bool b0, bool b1, int segIndex)
{
    (void)b1;
    G2AnimSegValue* sv = &_segValues[segIndex];
    BuildSegLocalRotMatrix(sv, segMatrix);

    uint32_t mask = 0x0f;
    while (ac > &_controllerPool.blockPool[0] && ac->mSegNumber == segIndex) {
        mask &= ac->ApplyToSegValue(sv, parentMatrix, segMatrix);
        ac = &_controllerPool.blockPool[ac->mNext];
    }

    if (!(mask & 8)) {
        if (b0) {
            parentMatrix->col3.x += segMatrix->col3.x;
            parentMatrix->col3.y += segMatrix->col3.y;
            parentMatrix->col3.z += segMatrix->col3.z;
            segMatrix->col3.x = 0.0f;
            segMatrix->col3.y = 0.0f;
            segMatrix->col3.z = 0.0f;
        }
        return;
    }

    segMatrix->Scale(sv->scale);
    if (mask & 1) {
        cdc::Mul3x3(segMatrix, *parentMatrix, *segMatrix);
    }
    if (mask & 4) {
        if (segIndex == 0) {
            segMatrix->SetTrans(cdc::Mul3x3(*segMatrix, sv->trans));
        } else {
            segMatrix->SetTrans(cdc::Mul3x3(*parentMatrix, sv->trans));
        }
    } else {
        segMatrix->SetTrans(sv->trans);
    }

    if (b0) {
        parentMatrix->col3.x += segMatrix->col3.x;
        parentMatrix->col3.y += segMatrix->col3.y;
        parentMatrix->col3.z += segMatrix->col3.z;
        segMatrix->col3.x = 0.0f;
        segMatrix->col3.y = 0.0f;
        segMatrix->col3.z = 0.0f;
    }
    segMatrix->col3.x += parentMatrix->col3.x;
    segMatrix->col3.y += parentMatrix->col3.y;
    segMatrix->col3.z += parentMatrix->col3.z;
}

// 0x004766d0 (PDB: 0x00478050)
void AnimProcessor::PlayHostedAnimation(Object* host, int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, BoneSet* boneSet, bool mirror)
{
    if (mirror) {
        ReleaseControl();
    }

    float baseKeyCount = 0.0f;
    int i = 0;
    for (; i < 9 && animFragment[i] != -1; ++i) {
        while (mSectionCount <= i) {
            AddSection(mSection[0].mFirstSeg, mSection[0].mSegCount);
        }

        AnimFragmentProcessor& sec = mSection[i];
        sec.mAnimProcessor = this;
        AnimFragment* kl = G2Instance_GetKeylistFromObject(host, animFragment[i]);
        AnimFxHeader* fx = host->animFXList[animFragment[i]];

        if (i == 0) {
            if ((sec.mFlags & 0x200) && sec.mAnimProcessor != nullptr) {
                sec.mAnimProcessor->mInterpInfo.alphaTable = nullptr;
            }
            float interpTime = (debugRazielFlags1 & 0x80) ? 0.0f : static_cast<float>(static_cast<int>(kl->mTimePerKey) * interpFrames);
            const float targetTime = static_cast<float>(static_cast<int>(kl->mTimePerKey) * startFrame);
            sec.InterpToKeylistAtTime(kl, animFragment[0], targetTime, interpTime, host, fx);
            baseKeyCount = static_cast<float>(kl->mKeyCount);
            sec.mSpeedAdjustment = 1.0f;
        } else {
            const float ratio = static_cast<float>(kl->mKeyCount) / baseKeyCount;
            const int frame = static_cast<int>(ratio * static_cast<float>(startFrame)) - interpFrames;
            sec.SwitchToKeylistAtTimeWithoutAdvancing(kl, animFragment[i], static_cast<float>(frame * static_cast<int>(kl->mTimePerKey)), host, fx);
            sec.AdvanceOverAccumulatedTime();
            sec.mSpeedAdjustment = ratio;
        }

        if (playMode == 0) {
            sec.mFlags |= 1;
        } else {
            sec.mFlags &= ~1u;
            if (playMode == 2) {
                sec.mAlarmFlags &= ~3u;
                sec.mFlags |= 2;
                sec.mLoopStartTime = 0.0f;
                sec.mLoopEndTime = static_cast<float>(static_cast<int>(kl->mKeyCount) * static_cast<int>(kl->mTimePerKey));
            } else {
                sec.mFlags &= ~2u;
            }
        }
        sec.mFlags &= ~0x20u;
    }

    mBlendType = blendType;
    mBoneSet = boneSet;
    while (mSectionCount < i) {
        AddSection(0, 0);
    }
    mSectionCount = static_cast<uint8_t>(i);
    SyncMirroredTransformFlag(this);
}

// 0x00476970 (PDB: 0x004782f0)
void AnimProcessor::PlayHostedAnimationWithAlpha(Object* host, int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, int alphaTable, BoneSet* boneSet, bool mirror)
{
    PlayHostedAnimation(host, animFragment, startFrame, interpFrames, playMode, blendType, boneSet, mirror);
    G2AnimAlphaTable* table = (alphaTable != 0) ? &G2AlphaTables[alphaTable] : nullptr;
    for (int i = 0; i < mSectionCount; ++i) {
        AnimFragmentProcessor& sec = mSection[i];
        if ((sec.mFlags & 0x200) && sec.mAnimProcessor != nullptr) {
            sec.mAnimProcessor->mInterpInfo.alphaTable = table;
        }
    }
}

// 0x00476a00 (PDB: 0x00478380)
void AnimProcessor::BuildTransformsNoControllers()
{
    const uint16_t f = mSection[0].mFlags;
    bool b0 = (f & 0x88) == 0x80;
    bool b1 = (f & 0x180) == 0x80;

    cdc::Matrix* segMat = mSegMatrices;
    Segment* segments = mModelData->segmentList;
    const int numSegs = mModelData->numSegments;
    for (int i = 0; i < numSegs; ++i) {
        BuildSegTransformNoControllers(segMat, &mSegMatrices[segments[i].parent], b0, b1, i);
        ++segMat;
        b0 = false;
        b1 = false;
    }
}

// 0x00476aa0 (PDB: 0x00478420)
void AnimProcessor::InterpToKeylistFrame(AnimFragment* keylist, int keylistID, int frame, float duration, void* fxListHost, AnimFxHeader* fxList)
{
    for (int i = 0; i < mSectionCount; ++i) {
        const float d = (debugRazielFlags1 & 0x80) ? 0.0f : duration;
        const float targetTime = static_cast<float>(frame * static_cast<int>(keylist->mTimePerKey));
        mSection[i].InterpToKeylistAtTime(keylist, keylistID, targetTime, d, fxListHost, fxList);
    }
    SyncMirroredTransformFlag(this);
}

// 0x00476b50 (PDB: 0x004784d0)
void AnimProcessor::GetControllerCurrentInterpVector(int seg, int type, cdc::Vector3* vector)
{
    AnimController* ac = FindControllerAny(this, seg, type);
    if (ac->mDuration != 0.0f) {
        if (ac->mFlags & 0x2000) {
            *vector = ac->mData.vector.base;
        } else {
            const float t = ac->mElapsedTime * (1.0f / 4096.0f);
            vector->x = ac->mData.vector.base.x + t * ac->mData.vector.offset.x;
            vector->y = ac->mData.vector.base.y + t * ac->mData.vector.offset.y;
            vector->z = ac->mData.vector.base.z + t * ac->mData.vector.offset.z;
        }
        return;
    }

    if (ac->mType == 0x12) {
        EvaluateStoredFrameWithControllers(this);
        CopySegValueBitmask(vector, &_segValues[ac->mSegNumber], 0x70);
    } else if (ac->mType == 0x22) {
        EvaluateStoredFrameWithControllers(this);
        CopySegValueBitmask(vector, &_segValues[ac->mSegNumber], 0x700);
    } else if (ac->mType == 0x20) {
        const cdc::Matrix& mat = mSegMatrices[ac->mSegNumber];
        vector->x = mat.col3.x;
        vector->y = mat.col3.y;
        vector->z = mat.col3.z;
    }
}

// 0x00476d20 (PDB: 0x004786a0)
void AnimProcessor::BuildTransformsWithControllers()
{
    const uint16_t f = mSection[0].mFlags;
    bool b0 = (f & 0x88) == 0x80;
    bool b1 = (f & 0x180) == 0x80;

    AnimController* ac = &_controllerPool.blockPool[mControllerList];
    cdc::Matrix* segMat = mSegMatrices;
    Segment* segments = mModelData->segmentList;
    const int numSegs = mModelData->numSegments;
    for (int i = 0; i < numSegs; ++i) {
        cdc::Matrix* parentMat = &mSegMatrices[segments[i].parent];
        if (ac->mSegNumber == i && ac->mType != 0) {
            BuildSegTransformWithControllers(segMat, parentMat, ac, b0, b1, i);
            while (ac > &_controllerPool.blockPool[0] && ac->mSegNumber == i && ac->mType != 0) {
                ac = &_controllerPool.blockPool[ac->mNext];
            }
        } else {
            BuildSegTransformNoControllers(segMat, parentMat, b0, b1, i);
        }
        ++segMat;
        b0 = false;
        b1 = false;
    }

    UpdateControllers();
}

// 0x00476e40 (PDB: 0x004787c0)
void AnimController::GetCurrentInterpQuat(AnimProcessor* ap, cdc::Quat* outQuat)
{
    if (mFlags & 0x4000) {
        if (mFlags & 0x2000) {
            *outQuat = mData.quat.dest;
        } else {
            const float alpha = mAlphaTable->GetValue(mElapsedTime / mDuration);
            G2Quat_Slerp(alpha, &mData.quat.src, &mData.quat.dest, outQuat, 0);
        }
        return;
    }

    if (mType == 8) {
        G2Quat_FromMatrixf(outQuat, &ap->mSegMatrices[mSegNumber]);
    } else if (mType == 0x4c) {
        *outQuat = cdc::Quat::Identity();
    } else if (mType == 0x0e) {
        _G2EulerAngles_Type euler;
        ap->CopyVectorWithOrder(&mData.vector.base, &euler, mFlags & 0xff);
        G2Quat_FromEulerf(outQuat, &euler);
    } else {
        cdc::Quat v;
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        v.w = 0.0f;
        EvaluateStoredFrameWithControllers(ap);
        CopySegValueBitmask(&v, &_segValues[mSegNumber], 7);
        *outQuat = QuatExp(v);
    }
}

// 0x00476fe0 (PDB: 0x00478960)
void AnimProcessor::PlayHostedAnimation(Object* host, AnimBlend* blend, bool mirror)
{
    if (blend == nullptr) {
        return;
    }

    const RootInterpMode mode = static_cast<RootInterpMode>(blend->rootInterpMode);
    for (int i = 0; i < mSectionCount; ++i) {
        mSection[i].mNewRootInterpMode = mode;
    }

    BoneSet* boneSet = (blend->boneSet >= 0) ? sBoneSet[blend->boneSet] : nullptr;
    PlayHostedAnimationWithAlpha(
        host,
        blend->animFragment,
        blend->startFrame,
        blend->interpFrames,
        blend->playMode,
        blend->blendType,
        blend->alphaTable,
        boneSet,
        mirror
    );
}

// 0x00477060 (PDB: 0x004789e0)
void AnimProcessor::PlayAnimation(int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, BoneSet* boneSet, bool mirror)
{
    PlayHostedAnimation(mAnimComponent->mInstance->object, animFragment, startFrame, interpFrames, playMode, blendType, boneSet, mirror);
}

// 0x004770a0 (PDB: 0x00478a20)
void AnimProcessor::PlayAnimationWithAlpha(int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, int alphaTable, BoneSet* boneSet, bool mirror)
{
    PlayHostedAnimationWithAlpha(mAnimComponent->mInstance->object, animFragment, startFrame, interpFrames, playMode, blendType, alphaTable, boneSet, mirror);
}

// 0x004770e0 (PDB: 0x00478a60)
void AnimProcessor::SetInterpController_Quat(int seg, int type, cdc::Quat* quat, float duration)
{
    AnimController* ac = FindControllerAny(this, seg, type);
    ac->GetCurrentInterpQuat(this, &ac->mData.quat.src);
    ac->mData.quat.dest = *quat;
    ac->mFlags = static_cast<uint16_t>((ac->mFlags & 0xffu) | 0x4000u);
    ac->mDuration = duration;
    ac->mElapsedTime = 0.0f;
}

// 0x00477150 (PDB: 0x00478ad0)
void AnimProcessor::PlayAnimation(AnimBlend* blend, bool mirror)
{
    PlayHostedAnimation(mAnimComponent->mInstance->object, blend, mirror);
}

// 0x00477180 (PDB: 0x00478b00)
void AnimProcessor::PlayNextAnim()
{
    while (mFrontIndex != mBackIndex) {
        const Anim anim = mAnimQueue[mFrontIndex];
        if ((anim.mFlags & 1) && static_cast<int16_t>(anim.mRepeatCount) > 1) {
            mAnimQueue[mFrontIndex].mRepeatCount--;
        } else {
            PopToNextAnim();
        }

        if (static_cast<int16_t>(anim.mAnimId) >= 0 && static_cast<int16_t>(anim.mRepeatCount) > 0) {
            const bool mirroredBlend = (anim.mFlags & 0x80) != 0;
            int16_t animIdx = static_cast<int16_t>(anim.mAnimId);
            if (mirroredBlend) {
                animIdx = gPlayer->interpoBlend->GetDesiredAnimation(animIdx);
            }

            Object* obj = mAnimComponent->mInstance->object;
            AnimBlend blend = obj->baseData->animBlend[animIdx];
            if (static_cast<int16_t>(anim.mTargetFrame) >= 0) {
                blend.startFrame = static_cast<uint8_t>(anim.mTargetFrame);
            }
            if (static_cast<int16_t>(anim.mInterpFrames) >= 0) {
                blend.interpFrames = static_cast<uint8_t>(anim.mInterpFrames);
            }

            bool mirror = ((((anim.mFlags >> 5) & 1u) ^ (blend.flags >> 7)) != 0);
            if (mirroredBlend && g_interpoBlendMirrorHack) {
                mirror = true;
            }
            g_interpoBlendMirrorHack = false;
            if (mirror) {
                mFlags |= 0x20u;
            } else {
                mFlags &= ~0x20u;
            }

            PlayHostedAnimation(obj, &blend, false);
            mAnimWasPlayed = true;
            SetSpeedAdjustment(anim.mSpeed);
            mCurrentBlendId = animIdx;
            if (mirroredBlend) {
                gPlayer->interpoBlend->BeginAnimationWithoutAnimBlend(static_cast<int16_t>(anim.mAnimId), mirror);
            }
            return;
        }

        if (anim.mFlags & 0x40) {
            mAnimTweaker.Start(anim.mTweakTranslation, anim.mTweakFrames);
        }
        PopToNextAnim();
    }

    if (mOldCallback != nullptr) {
        mOldCallback(this, 0, G2ANIM_MSG_ANIM_QUEUE_EMPTY, 0, 0, this);
    }
    ReleaseControl();
    mAnimTweaker.mIsRunning = false;
}

// 0x004773b0 (PDB: 0x00478d30)
void AnimProcessor::Process()
{
    if (mCurrentPatternIndex != -1) {
        if (mCurrentPatternIsNative) {
            sNativePattern[mCurrentPatternIndex]->Process(this);
        } else {
            mVirtualMachine->CallVirtualMethod(
                mAnimComponent->mInstance->object->animPatternList[mCurrentPatternIndex],
                1
            );
        }
    }

    if (mUseRateRamp) {
        AnimFragmentProcessor& sec0 = mSection[0];
        const float totalTime = static_cast<float>(static_cast<int>(sec0.mKeylist->mKeyCount) * static_cast<int>(sec0.mKeylist->mTimePerKey));
        sec0.mSpeedAdjustment = mStartRate + ((mEndRate - mStartRate) / totalTime) * sec0.mElapsedTime;
    }

    if (mReadyForNextAnim) {
        PlayNextAnim();
        mReadyForNextAnim = false;
    }
}

// 0x00477460 (PDB: 0x00478de0)
void AnimProcessor::SetInterpController_Vector(int seg, int type, cdc::Vector3* vector, float duration)
{
    AnimController* ac = FindControllerAny(this, seg, type);
    if ((ac->mType & 0x38) == 8) {
        _G2EulerAngles_Type euler;
        CopyVectorWithOrder(vector, &euler, ac->mFlags & 0xff);
        cdc::Quat q;
        G2Quat_FromEulerf(&q, &euler);
        SetInterpController_Quat(seg, type, &q, duration);
        return;
    }

    const float scale = 4096.0f / (duration + 1.0f);
    GetControllerCurrentInterpVector(seg, type, &ac->mData.vector.base);
    ac->mData.vector.offset.x = (vector->x - ac->mData.vector.base.x) * scale;
    ac->mData.vector.offset.y = (vector->y - ac->mData.vector.base.y) * scale;
    ac->mData.vector.offset.z = (vector->z - ac->mData.vector.base.z) * scale;
    ac->mFlags = static_cast<uint16_t>((ac->mFlags & 0xffu) | 0x4000u);
    ac->mDuration = duration;
    ac->mElapsedTime = 0.0f;
}

// 0x00477560 (PDB: 0x00478ee0)
void AnimProcessor::TakeControl()
{
    if (!mHasControl && mFrontIndex != mBackIndex) {
        mOldCallback = mSection[0].mCallback;
        mOldCallbackData = mSection[0].mCallbackData;
        mSection[0].mCallback = AnimProcessor::AnimCallback;
        mSection[0].mCallbackData = this;

        const uint32_t mode = mAnimQueue[mFrontIndex].mFlags & 3u;
        if (!mHasControl || mode == 2 || mode == 1) {
            PlayNextAnim();
        }
        mHasControl = true;
    }
}

// 0x004775f0 (PDB: 0x00478f70)
void AnimProcessor::InterpDisableController(int seg, int type, float duration)
{
    if (duration == 0.0f || type == 0x20) {
        AnimController* disabled = AnimController::RemoveFromList(seg, type, &mControllerList);
        if (disabled != nullptr) {
            disabled->InsertIntoList(&mDisabledControllerList);
        }
    }

    AnimController* ac = AnimController::FindInList(seg, type, &mControllerList);
    if (ac != nullptr) {
        if (ac->mType == 8) {
            cdc::Quat q;
            G2Quat_FromMatrixf(&q, &mSegMatrices[mModelData->segmentList[ac->mSegNumber].parent]);
            SetInterpController_Quat(seg, type, &q, duration);
        } else {
            cdc::Vector3 zero = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
            SetInterpController_Vector(seg, type, &zero, duration);
        }
        ac->mFlags |= 0x8000u;
    }
}

// 0x00477720 (PDB: 0x004790a0)
void AnimProcessor::PushBack(const Anim& anim)
{
    Anim queued = anim;
    const uint32_t mode = anim.mFlags & 3u;
    if (mode == 1) {
        ReleaseControl();
        AnimFragmentProcessor& masterSec = mSection[mMasterSection];
        AnimFragment* curKl = mSection[0].mKeylist;
        const float curTotal = static_cast<float>(static_cast<int>(curKl->mKeyCount) * static_cast<int>(curKl->mTimePerKey));
        const float curElapsed = masterSec.mElapsedTime;

        Instance* inst = mAnimComponent->mInstance;
        int16_t animIdx = static_cast<int16_t>(anim.mAnimId);
        if (anim.mFlags & 0x80) {
            animIdx = gPlayer->interpoBlend->GetDesiredAnimation(animIdx);
        }
        AnimBlend* newBlend = &inst->object->baseData->animBlend[animIdx];
        AnimFragment* newKl = G2Instance_GetKeylist(inst, newBlend->animFragment[0]);

        const int keyCount = newKl->mKeyCount;
        const int newTimePerKey = newKl->mTimePerKey;
        const float newTotal = static_cast<float>(keyCount * newTimePerKey);
        const float interpOffset = static_cast<float>(static_cast<int>(newBlend->interpFrames) * static_cast<int>(curKl->mTimePerKey));
        float targetTime = (interpOffset + curElapsed) * (newTotal / curTotal);
        if (targetTime < 0.0f) {
            targetTime = 0.0f;
        }
        int targetFrame = static_cast<int>(targetTime / static_cast<float>(newTimePerKey) + 0.5f);
        if (targetFrame > keyCount) {
            if ((anim.mFlags & 0x10) || (newBlend->flags & 2)) {
                targetFrame %= keyCount;
            } else {
                targetFrame = keyCount;
            }
        }
        queued.mTargetFrame = static_cast<uint16_t>(targetFrame);
    } else if (mode == 2) {
        ReleaseControl();
    }

    mAnimQueue[mBackIndex] = queued;
    ++mBackIndex;
    if (mBackIndex >= 16) {
        mBackIndex = 0;
    }
    TakeControl();
}

// 0x004778f0 (PDB: 0x00479270)
void AnimProcessor::CriticalOn()
{
    Anim a;
    a.mSpeed = 1.0f;
    a.mAnimId = 0xffff;
    a.mFlags = 4;
    a.mRepeatCount = 1;
    a.mTargetFrame = 0xffff;
    a.mInterpFrames = 0xffff;
    PushBack(a);
}

// 0x00477930 (PDB: 0x004792b0)
void AnimProcessor::CriticalOff()
{
    Anim a;
    a.mSpeed = 1.0f;
    a.mAnimId = 0xffff;
    a.mFlags = 8;
    a.mRepeatCount = 1;
    a.mTargetFrame = 0xffff;
    a.mInterpFrames = 0xffff;
    PushBack(a);
}

// 0x00477970 (PDB: 0x004792f0)
void AnimProcessor::PushBack(const AnimSequence& seq)
{
    for (int i = 0; i < seq.Size(); ++i) {
        PushBack(seq.GetAnim(i));
    }
}
