#include "game/anim/anim.h"
#include "game/g2/instncg2.h"
#include "game/math3d.h"
#include "game/obtable.h"
#include "game/player/player.h"
#include "game/ragdoll.h"

#include <cmath>
#include <cstdint>

namespace {

constexpr uint32_t kScratchBufferBytes = 0x10000;
constexpr int kScratchSegValueBytes = 0x2d00; // 180 * sizeof(G2AnimSegValue)

alignas(16) uint8_t s_scratchMem[kScratchBufferBytes] = {};
int s_scratchOffset = 0;

// 0x004019c0
void* getScratchAddr(int size)
{
    void* ptr = s_scratchMem + s_scratchOffset;
    s_scratchOffset += size;
    return ptr;
}

// 0x004019f0
void popScratchAddr(int size)
{
    s_scratchOffset -= size;
}

// 0x0041ab10 (PDB: 0x00419a90)
uint32_t GetNextThreeBits(G2AnimBitStream* bs)
{
    uint32_t* stream = bs->stream;
    if (stream == nullptr) {
        return 0;
    }

    uint32_t value = bs->flags & 7u;
    bs->flags >>= 3;
    bs->bitCount -= 3;
    if (bs->bitCount <= 0) {
        ++stream;
        bs->stream = stream;
        const uint32_t nextWord = *stream;
        bs->flags = nextWord;
        if (bs->bitCount < 0) {
            value |= (nextWord << (bs->bitCount + 3)) & 7u;
            bs->flags = nextWord >> (-bs->bitCount);
        }
        bs->bitCount += 32;
    }
    return value;
}

// Decodes the section-0 root channel bitmask header and returns the initial channel stream pointer.
const uint8_t* SetupRootChannelMask(const AnimFragment* keylist, uint32_t* outMask)
{
    const uint32_t* headerWords = reinterpret_cast<const uint32_t*>(keylist->SectionDataOffsets());
    const uint8_t* channelData = reinterpret_cast<const uint8_t*>(headerWords) + headerWords[0];
    const uint32_t* bitWords = headerWords + 1;
    const uint8_t presentFlags = *reinterpret_cast<const uint8_t*>(bitWords);
    const int bitStride = (static_cast<int>(keylist->mSegmentCount) * 3 + 7) & ~7;

    G2AnimBitStream bsRot = {};
    G2AnimBitStream bsScale = {};
    G2AnimBitStream bsTrans = {};

    int bitOffset = 8;
    if ((presentFlags & 1) != 0) {
        bsRot.stream = const_cast<uint32_t*>(bitWords);
        bsRot.flags = bitWords[0] >> 8;
        bsRot.bitCount = 24;
        bitOffset = bitStride + 8;
    }
    auto initStream = [&](G2AnimBitStream& bs) {
        const int wordIndex = bitOffset >> 5;
        const int bitInWord = bitOffset & 31;
        bs.stream = const_cast<uint32_t*>(bitWords + wordIndex);
        bs.flags = bitWords[wordIndex] >> bitInWord;
        bs.bitCount = 32 - bitInWord;
    };
    if ((presentFlags & 2) != 0) {
        initStream(bsScale);
        bitOffset += bitStride;
    }
    if ((presentFlags & 4) != 0) {
        initStream(bsTrans);
    }

    const uint32_t transBits = GetNextThreeBits(&bsTrans);
    const uint32_t scaleBits = GetNextThreeBits(&bsScale);
    const uint32_t rotBits = GetNextThreeBits(&bsRot);
    *outMask = (transBits << 8) | (scaleBits << 4) | rotBits;
    return channelData;
}

// Steps through compressed/raw keyframe channels according to active bits in `channelMask`,
// sampling `frameA` into `outA` (when `writeA` is true) and `frameB` into `outB`.
void SampleChannelPair(
    const AnimFragment* keylist,
    uint32_t channelMask,
    const uint8_t* channelPtr,
    int frameA,
    int frameB,
    bool writeA,
    float* outA,
    float* outB)
{
    const int keyCount = static_cast<int>(keylist->mKeyCount);
    for (int slot = 0; channelMask != 0; channelMask >>= 1, ++slot) {
        if ((channelMask & 1u) == 0) {
            continue;
        }
        const int16_t* header = reinterpret_cast<const int16_t*>(channelPtr);
        const int16_t channelType = header[0];
        float valA = 0.0f;
        float valB = 0.0f;
        int strideFloats = 0;

        if (channelType == 0) {
            const float* raw = reinterpret_cast<const float*>(channelPtr + 4);
            valA = raw[frameA];
            valB = raw[frameB];
            strideFloats = keyCount;
        } else if (channelType == 1) {
            const float constantVal = *reinterpret_cast<const float*>(channelPtr + 4);
            valA = constantVal;
            valB = constantVal;
            channelPtr += 8;
        } else if (channelType == 2) {
            float* linearData = const_cast<float*>(reinterpret_cast<const float*>(channelPtr));
            valA = AnimFragmentProcessor::DecompressChannel_Linear(frameA, linearData);
            valB = AnimFragmentProcessor::DecompressChannel_Linear(frameB, linearData);
            const int wordCount = static_cast<int>(header[1]);
            strideFloats = ((wordCount + 2) >> 2) + wordCount;
        } else {
            valA = 0.0f;
            valB = 0.0f;
        }

        if (channelType != 1) {
            channelPtr += 4 + strideFloats * 4;
        }
        if (writeA) {
            outA[slot] = valA;
        }
        outB[slot] = valB;
    }
}

void ComputeFrameIndices(const AnimFragment* keylist, float time, bool clampUpper, int* outFrameA, int* outFrameB, float* outAlpha)
{
    const float ratio = time / static_cast<float>(keylist->mTimePerKey);
    const int frame = static_cast<int>(ratio);
    int frameA = frame - 1;
    int frameB = frame;
    if (frameA < 0) {
        frameA = frameB;
    }
    if (clampUpper && frameB >= static_cast<int>(keylist->mKeyCount)) {
        frameB = static_cast<int>(keylist->mKeyCount) - 1;
    }
    *outFrameA = frameA;
    *outFrameB = frameB;
    *outAlpha = ratio - static_cast<float>(frame);
}

cdc::Vector3 ToVec3(const cdc::Vector& v)
{
    return cdc::Vector3::Make(v.x, v.y, v.z, v.w);
}

cdc::Quat ToQuat(const cdc::Vector& v)
{
    cdc::Quat q;
    q.x = v.x;
    q.y = v.y;
    q.z = v.z;
    q.w = v.w;
    return q;
}

void CopySegEulerTwist(int dstSegIdx, int srcSegIdx)
{
    cdc::Matrix mat;
    const cdc::Quat srcLog = ToQuat(_segValues[srcSegIdx].rot);
    MATH3D_QuatLogToMatrix(&mat, &srcLog);
    cdc::Euler euler(mat);
    euler.x = 0.0f;
    cdc::Matrix rebuilt;
    rebuilt.Build(euler);
    cdc::Quat q;
    q.Build(rebuilt);
    // Matches retail 0x0041c8e2 / 0x0041c8f9 path (Quat(Euler) -> QuatLog, zeroing w).
    _segValues[dstSegIdx].rot = ToVec3(cdc::QuatLog(q));
    _segValues[dstSegIdx].rot.w = 0.0f;
}

} // namespace

// 0x0041a770 (PDB: 0x004196d0)
cdc::Vector3 AnimFragment::GetTrans() const
{
    return cdc::Vector3::Make(mTransX, mTransY, mTransZ, 0.0f);
}

// 0x0041a7c0 (PDB: 0x00419720)
cdc::Quat AnimFragment::GetRot() const
{
    cdc::Quat q;
    q.x = mRotX;
    q.y = mRotY;
    q.z = mRotZ;
    q.w = 0.0f;
    return q;
}

// 0x0041a810 (PDB: 0x00419770)
void AnimProcessor::SetMirrorMode(bool mirror)
{
    if (mirror) {
        mFlags |= 0x2000;
    } else {
        mFlags &= static_cast<uint16_t>(~0x2000u);
    }
}

// 0x0041bb90 (PDB: 0x0041ab60)
AnimSequence::AnimSequence()
{
    for (int i = 0; i < 4; ++i) {
        mAnimList[i].mAnimId = 0xffff;
        mAnimList[i].mTargetFrame = 0xffff;
        mAnimList[i].mInterpFrames = 0xffff;
        mAnimList[i].mSpeed = 1.0f;
        mAnimList[i].mFlags = 0;
        mAnimList[i].mRepeatCount = 1;
    }
    mCount = 0;
    mTranslation = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    mRotation.Identity();
}

// 0x0041a830 (PDB: 0x00419790)
AnimSequence& AnimSequence::operator<<(const Anim& anim)
{
    mAnimList[mCount++] = anim;
    return *this;
}

// 0x0041a860 (PDB: 0x004197c0)
Anim AnimSequence::GetAnim(int index) const
{
    return mAnimList[index];
}

// 0x0041a880 (PDB: 0x004197e0)
int AnimSequence::Size() const
{
    return mCount;
}

// 0x0041bc30 (PDB: 0x0041ac00)
bool AnimSequence::SolveTranslationRotation(BaseInstance* instance, const cdc::Vector3& targetPos, const cdc::Euler& /*targetRot*/)
{
    AnimProcessor& mainProc = instance->animComponent->mAnimProcessor[0];
    mTranslation = mainProc.GetTranslation();
    mRotation = mainProc.GetRotation();

    int rootAnimIdx = -1;
    if (!mainProc.mHasControl && mCount > 0 && ((mAnimList[0].mFlags >> 1) & 1) != 0) {
        AnimFragment* kl = mainProc.mSection->mKeylist;
        if (kl != nullptr) {
            const float endTime = static_cast<float>((static_cast<int>(kl->mKeyCount) - 1) * static_cast<int>(kl->mTimePerKey));
            const cdc::Vector3 endTrans = ANIM_GetTranslationAtTime(kl, 0, endTime);
            const cdc::Vector3 curTrans = ANIM_GetTranslationAtTime(kl, 0, mainProc.mSection->mElapsedTime);
            mTranslation += cdc::Mul3x4(mRotation, endTrans - curTrans);

            cdc::Matrix mEnd;
            mEnd.Build(ANIM_GetRotationAtTime(kl, 0, endTime));
            cdc::Matrix mCur;
            mCur.Build(ANIM_GetRotationAtTime(kl, 0, mainProc.mSection->mElapsedTime));
            cdc::Matrix mCurT;
            cdc::Transpose(&mCurT, mCur);
            mRotation = (mCurT * mEnd) * mRotation;
        }
    }

    for (int i = 0; i < mCount; ++i) {
        if ((mAnimList[i].mFlags & 1) != 0) {
            if (rootAnimIdx < 0) {
                rootAnimIdx = i;
            } else {
                mAnimList[i].mRepeatCount = 0;
            }
            continue;
        }
        AnimFragment* kl = G2Instance_GetKeylist(instance, static_cast<int16_t>(mAnimList[i].mAnimId));
        mAnimList[i].mRepeatCount = 1;
        mTranslation += cdc::Mul3x4(mRotation, kl->GetTrans());
        cdc::Matrix mRot;
        mRot.Build(cdc::QuatExp(kl->GetRot()));
        mRotation = mRot * mRotation;
    }

    if (rootAnimIdx >= 0) {
        Anim& rootAnim = mAnimList[rootAnimIdx];
        AnimFragment* kl = G2Instance_GetKeylist(instance, static_cast<int16_t>(rootAnim.mAnimId));
        cdc::Matrix invRot;
        cdc::Transpose(&invRot, mRotation);
        const cdc::Vector3 localDelta = cdc::Mul3x4(invRot, targetPos - mTranslation);
        const cdc::Vector3 animTrans = kl->GetTrans();
        const cdc::Vector3 unitTrans = animTrans.UnitVec3();
        float proj = cdc::Dot3(localDelta, unitTrans) / animTrans.Len3();
        if (proj < 0.0f) {
            proj = 0.0f;
        }
        const int repeats = static_cast<int>(proj + 0.5f);
        rootAnim.mRepeatCount = static_cast<uint16_t>(repeats);
        mTranslation += cdc::Mul3x4(mRotation, animTrans) * static_cast<float>(repeats);

        const cdc::Vector3 scaledLog = cdc::QuatLog(kl->GetRot()) * static_cast<float>(repeats);
        cdc::Matrix mRot;
        mRot.Build(cdc::QuatExp(scaledLog));
        mRotation = mRot * mRotation;
    }

    return true;
}

// 0x0041a890 (PDB: 0x004197f0)
AnimSequenceList::AnimSequenceList()
{
    mSize = 0;
}

// 0x0041a8a0 (PDB: 0x00419800)
void AnimSequenceList::Add(AnimSequence* seq)
{
    mAnimSequenceList[mSize++] = seq;
}

// PDB: 0x00419810
int AnimSequenceList::Size() const
{
    return mSize;
}

// PDB: 0x00419a80
AnimSequence* ANIM_MinimumErrorSequence(AnimSequenceList& list)
{
    return list.GetAnimSequence(0);
}

// 0x0041af70 (PDB: 0x00419f40)
AnimComponent::AnimComponent(BaseInstance* instance, int processorCount)
{
    mBlendProcessor.SetAnimComponent(this);
    mAnimProcessor = new AnimProcessor[processorCount];
    mAnimProcessorCount = processorCount;
    for (int i = 0; i < processorCount; ++i) {
        mAnimProcessor[i].SetAnimComponent(this);
    }
    mInstance = static_cast<Instance*>(instance);
    mModel = nullptr;
}

// 0x0041b020 (PDB: 0x00419ff0)
AnimComponent::~AnimComponent()
{
    delete[] mAnimProcessor;
    mAnimProcessor = nullptr;
    mAnimProcessorCount = 0;
}

// 0x0041a8b0 (PDB: 0x00419820)
void AnimComponent::PlayAnimation(AnimBlend* blend, int processorIdx, bool mirror)
{
    mAnimProcessor[processorIdx].PlayHostedAnimation(mInstance->object, blend, mirror);
}

// 0x0041a8e0 (PDB: 0x00419850)
void AnimComponent::PlayHostedAnimation(Object* host, AnimBlend* blend, int processorIdx, bool mirror)
{
    mAnimProcessor[processorIdx].PlayHostedAnimation(host, blend, mirror);
}

// 0x0041a910 (PDB: 0x00419880)
void AnimComponent::Stop()
{
    for (int i = 0; i < mAnimProcessorCount; ++i) {
        mAnimProcessor[i].Stop();
    }
}

// 0x0041a940 (PDB: 0x004198b0)
bool AnimComponent::IsAnimationPlaying(int animBlendId, int processorIdx)
{
    AnimProcessor& proc = mAnimProcessor[processorIdx];
    AnimBlend* blend = &proc.mAnimComponent->mInstance->object->baseData->animBlend[animBlendId];
    int i = 0;
    for (; i < 9; ++i) {
        if (blend->animFragment[i] < 0) {
            break;
        }
        if (i > static_cast<int>(proc.mSectionCount)) {
            return false;
        }
        if (animBlendId != proc.CurrentBlendId()) {
            return false;
        }
    }
    return i == static_cast<int>(proc.mSectionCount);
}

// 0x0041a9c0 (PDB: 0x00419930)
void BlendProcessor::UpdateStoredFrame()
{
    AnimProcessor* procs = mAnimComponent->mAnimProcessor;
    if ((procs->mFlags & 0x400) != 0 && procs == _segValuesAnim) {
        return;
    }
    for (int i = 0; i < mAnimComponent->mAnimProcessorCount; ++i) {
        mAnimComponent->mAnimProcessor[i].UpdateStoredFrame();
    }
    _segValuesAnim = mAnimComponent->mAnimProcessor;
}

// 0x0041aa10 (PDB: 0x00419980)
void BlendProcessor::NextKeyframe()
{
    for (int i = 0; i < mAnimComponent->mAnimProcessorCount; ++i) {
        _segValuesAnim = nullptr;
        _segValues = static_cast<G2AnimSegValue*>(getScratchAddr(kScratchSegValueBytes));
        mAnimComponent->mAnimProcessor[i].NextKeyframe();
        _segValuesAnim = nullptr;
        _segValues = _segValuesStatic;
        popScratchAddr(kScratchSegValueBytes);
    }
}

// 0x0041aa90 (PDB: 0x00419a00)
void BlendProcessor::NextKeyframeTimeDelta(float delta)
{
    for (int i = 0; i < mAnimComponent->mAnimProcessorCount; ++i) {
        _segValuesAnim = nullptr;
        _segValues = static_cast<G2AnimSegValue*>(getScratchAddr(kScratchSegValueBytes));
        mAnimComponent->mAnimProcessor[i].NextKeyframeTimeDelta(delta);
        _segValues = _segValuesStatic;
        _segValuesAnim = nullptr;
        popScratchAddr(kScratchSegValueBytes);
    }
}

// 0x0041c230 (PDB: 0x0041b200)
void BlendProcessor::ApplyRagdoll()
{
    Instance* inst = mAnimComponent->mInstance;
    RagdollClient* rc = inst->multibodyRagdoll;
    if (rc == nullptr) {
        return;
    }

    const int modelIdx = static_cast<int>(inst->currentRenderModel);
    Model* model = inst->object->modelList[modelIdx];
    Ragdoll* ragdoll = rc->m_ragdoll;

    for (int s = 0; s < model->numSegments; ++s) {
        _segValues[s].rot = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        _segValues[s].trans = model->segmentList[s].pivot;
        _segValues[s].scale = cdc::Vector3::Make(1.0f, 1.0f, 1.0f, 0.0f);
    }

    for (int b = 0; b < rc->m_boneCount; ++b) {
        RagdollBone* bone = ragdoll->GetBone(b);
        cdc::Quat q = bone->m_body->GetQuaternion();
        if (bone->m_parent != -1) {
            const cdc::Quat parentQ = ragdoll->GetBone(bone->m_parent)->m_body->GetQuaternion();
            q = (~parentQ) * q;
        }
        const int segIdx = rc->m_bones[b].segment;
        _segValues[segIdx].rot = ToVec3(cdc::QuatLog(q));
    }

    _segValues[0].trans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    _segValues[0].rot = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    const cdc::Quat rootQ = ragdoll->GetBone(0)->m_body->GetQuaternion();
    const cdc::Vector3 targetOffset = rc->m_rootOffset1 + rootQ.Rotate(rc->m_rootOffset2);
    const cdc::Vector3 body0Pos = ragdoll->GetBone(0)->m_body->GetPosition();
    const cdc::Vector3 rootPos = body0Pos - targetOffset;

    cdc::Matrix* prevMatrix = inst->matrix - 1;
    prevMatrix->col0.x = 1.0f;
    prevMatrix->col0.y = 0.0f;
    prevMatrix->col0.z = 0.0f;
    prevMatrix->col1.x = 0.0f;
    prevMatrix->col1.y = 1.0f;
    prevMatrix->col1.z = 0.0f;
    prevMatrix->col2.x = 0.0f;
    prevMatrix->col2.y = 0.0f;
    prevMatrix->col2.z = 1.0f;
    prevMatrix->col3.x = rootPos.x;
    prevMatrix->col3.y = rootPos.y;
    prevMatrix->col3.z = rootPos.z;

    inst->position = rootPos;
    inst->rotation.SetZero();

    if (inst == s_playerInstance) {
        CopySegEulerTwist(78, 59);
        CopySegEulerTwist(99, 80);
    }
}

// 0x0041cad0 (PDB: 0x0041baa0)
void BlendProcessor::BuildTransforms()
{
    if (mAnimComponent->mAnimProcessorCount == 0) {
        return;
    }

    UpdateStoredFrame();
    AnimProcessor* mainProc = mAnimComponent->mAnimProcessor;
    if (mainProc->mSectionCount == 0) {
        return;
    }

    AnimFragmentProcessor* fp0 = mainProc->mSection;
    if ((fp0->mFlags & 0x88) == 0x80) {
        mainProc->mRootTrans = fp0->mRootTrans;
        fp0->mRootTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        mainProc->mRootTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }

    if ((fp0->mFlags & 0x180) == 0x80) {
        mainProc->mRootRot = fp0->mRootRot;
        fp0->mRootRot = cdc::Quat::Identity();
    } else {
        mainProc->mRootRot = cdc::Quat::Identity();
    }

    const cdc::Vector3 rootTrans = mainProc->mRootTrans;
    if (!rootTrans.IsZero3()) {
        mainProc->mFlags |= 0x40;
    }
    _segValues[0].trans = rootTrans;

    const cdc::Quat rootRot = mainProc->mRootRot;
    if (rootRot != cdc::Quat::Identity()) {
        _segValues[0].rot = ToVec3(cdc::QuatLog(rootRot));
        mainProc->mFlags |= 0x80;
    } else {
        _segValues[0].rot = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }

    ApplyRagdoll();

    if (mainProc->GetControllerList() != nullptr) {
        mainProc->BuildTransformsWithControllers();
    } else {
        mainProc->BuildTransformsNoControllers();
    }

    mainProc->mRootTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < mAnimComponent->mAnimProcessorCount; ++i) {
        AnimProcessor& proc = mAnimComponent->mAnimProcessor[i];
        proc.mSection->mFlags &= static_cast<uint8_t>(~0x80u);
        if ((proc.mFlags & 0x1) != 0) {
            proc.mFlags |= 0x2;
        } else {
            proc.mFlags &= static_cast<uint16_t>(~0x2u);
        }
        proc.mFlags &= static_cast<uint16_t>(~0x1u);
    }
}

// 0x0041ab70 (PDB: 0x00419af0)
cdc::Euler ANIM_GetRotationAtTime(const AnimFragment* /*keylist*/, int /*seg*/, float /*time*/)
{
    cdc::Euler zero;
    zero.SetZero();
    return zero;
}

// 0x0041abc0 (PDB: 0x00419b40)
bool ANIM_HasRootMotion(const AnimFragment* keylist)
{
    uint32_t mask = 0;
    const uint8_t* ptr = SetupRootChannelMask(keylist, &mask);
    for (; mask != 0; mask >>= 1) {
        if ((mask & 1u) == 0) {
            continue;
        }
        const int16_t type = *reinterpret_cast<const int16_t*>(ptr);
        const float val = *reinterpret_cast<const float*>(ptr + 4);
        if (type != 1 || val != 0.0f) {
            return true;
        }
        ptr += 8;
    }
    return false;
}

// 0x0041acf0 (PDB: 0x00419c70)
AnimBlend* ANIM_AnimBlendFromIndex(BaseInstance* inst, int16_t index)
{
    if (inst == nullptr || index < 0) {
        return nullptr;
    }
    return &inst->object->baseData->animBlend[index];
}

// 0x0041ad20 (PDB: 0x00419ca0)
cdc::Vector3 ANIM_GetTranslationFromBlendId(Instance* inst, int16_t blendId)
{
    AnimBlend* blend = ANIM_AnimBlendFromIndex(inst, blendId);
    AnimFragment* kl = G2Instance_GetKeylistFromObject(inst->object, blend->animFragment[0]);
    return kl->GetTrans();
}

// 0x0041adb0 (PDB: 0x00419d30)
cdc::Quat ANIM_GetRotationFromBlendId(Instance* inst, int16_t blendId)
{
    AnimBlend* blend = ANIM_AnimBlendFromIndex(inst, blendId);
    AnimFragment* kl = G2Instance_GetKeylistFromObject(inst->object, blend->animFragment[0]);
    return cdc::QuatExp(kl->GetRot());
}

// 0x0041b090 (PDB: 0x0041a060)
cdc::Vector3 ANIM_GetTranslationAtTime(const AnimFragment* keylist, int /*seg*/, float time)
{
    if (time <= 0.0f) {
        return cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
    }

    int frameA = 0;
    int frameB = 0;
    float alpha = 0.0f;
    ComputeFrameIndices(keylist, time, false, &frameA, &frameB, &alpha);

    float vecA[12] = {};
    float vecB[12] = {};
    uint32_t mask = 0;
    const uint8_t* ptr = SetupRootChannelMask(keylist, &mask);
    if (mask != 0) {
        SampleChannelPair(keylist, mask, ptr, frameA, frameB, true, vecA, vecB);
    }

    const cdc::Vector3 transB = cdc::Vector3::Make(vecB[8], vecB[9], vecB[10], vecB[11]);
    if (frameB == 0) {
        return transB * alpha;
    }
    const cdc::Vector3 transA = cdc::Vector3::Make(vecA[8], vecA[9], vecA[10], vecA[11]);
    return transA * (1.0f - alpha) + transB * alpha;
}

// 0x0041b350 (PDB: 0x0041a320)
void ANIM_GetTransformAtTime(cdc::Quat* outRot, cdc::Vector3* outTrans, const AnimFragment* keylist, int /*segOrZero*/, float time)
{
    if (time <= 0.0f) {
        *outTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        *outRot = cdc::Quat::Identity();
        return;
    }

    int frameA = 0;
    int frameB = 0;
    float alpha = 0.0f;
    ComputeFrameIndices(keylist, time, true, &frameA, &frameB, &alpha);

    float vecA[12] = {};
    vecA[4] = 1.0f;
    vecA[5] = 1.0f;
    vecA[6] = 1.0f;
    float vecB[12] = {};

    uint32_t mask = 0;
    const uint8_t* ptr = SetupRootChannelMask(keylist, &mask);
    if (mask != 0) {
        SampleChannelPair(keylist, mask, ptr, frameA, frameB, frameB != 0, vecA, vecB);
    }

    cdc::Vector3 rot0 = cdc::Vector3::Make(vecA[0], vecA[1], vecA[2], vecA[3]);
    const cdc::Vector3 rot1 = cdc::Vector3::Make(vecB[0], vecB[1], vecB[2], vecB[3]);
    const cdc::Vector3 trans0 = cdc::Vector3::Make(vecA[8], vecA[9], vecA[10], vecA[11]);
    const cdc::Vector3 trans1 = cdc::Vector3::Make(vecB[8], vecB[9], vecB[10], vecB[11]);

    constexpr float kTwoPi = 6.28318548f;
    const cdc::Vector3 diff = (rot1 - rot0) / kTwoPi;
    const float diffLenSq = diff.Len3Squared();
    if (diffLenSq > 0.25f) {
        const int wrapCount = static_cast<int>(std::sqrt(diffLenSq) + 0.5f);
        const cdc::Vector3 dir = diff.UnitVec3();
        rot0 = rot0 - dir * (static_cast<float>(wrapCount) * kTwoPi);
    }

    const cdc::Vector3 interpRot = rot0 * (1.0f - alpha) + rot1 * alpha;
    *outRot = cdc::QuatExp(cdc::Vector3::Make(interpRot.x, interpRot.y, interpRot.z, 0.0f));
    *outTrans = trans0 * (1.0f - alpha) + trans1 * alpha;
}

// 0x0041b780 (PDB: 0x0041a750)
void ANIM_GetAnimBlendTransformAtTime(AnimProcessor* ap, const Anim& anim, cdc::Quat* outRot, cdc::Vector3* outTrans, int targetSeg, float time, bool applyMirror)
{
    const bool patternMirror = ap->GetPatternMirrorMode();
    const float duration = ap->GetAnimDuration(anim);

    AnimComponent tempComp(ap->mAnimComponent->mInstance, 1);
    AnimProcessor* tempProc = tempComp.mAnimProcessor;
    tempProc->SetModel(ap->mModelData);
    tempProc->mClearBeforeUpdate = true;
    tempProc->mLayerWeight = 1.0f;
    tempProc->mLayerBlendMode = 0;
    tempProc->mBlendValue1 = ap->mBlendValue1;
    tempProc->mBlendValue2 = ap->mBlendValue2;
    tempProc->mFlags &= static_cast<uint16_t>(~0x20u);

    Model* model = tempProc->mModelData;
    BoneSet boneSets[20] = {};
    int mapIdx = 19;
    boneSets[19].weight = 0.0f;
    boneSets[19].firstBone = 0xffff;
    boneSets[19].lastBone = 0xffff;

    for (int seg = targetSeg; seg != -1; ) {
        --mapIdx;
        boneSets[mapIdx].lastBone = static_cast<uint16_t>(seg);
        boneSets[mapIdx].firstBone = static_cast<uint16_t>(seg);
        boneSets[mapIdx].weight = 1.0f;
        int parent = model->segmentList[seg].parent;
        while (parent >= 0 && parent == seg - 1) {
            boneSets[mapIdx].firstBone = static_cast<uint16_t>(parent);
            seg = parent;
            parent = model->segmentList[seg].parent;
        }
        if (parent >= 0) {
            --mapIdx;
            boneSets[mapIdx].firstBone = static_cast<uint16_t>(parent + 1);
            boneSets[mapIdx].lastBone = static_cast<uint16_t>(seg - 1);
            boneSets[mapIdx].weight = 0.0f;
        }
        seg = parent;
    }

    AnimBlend* blend = ANIM_AnimBlendFromIndex(tempProc->mAnimComponent->mInstance, static_cast<int16_t>(anim.mAnimId));
    AnimBlend localBlend = *blend;
    localBlend.interpFrames = 0;
    tempProc->PlayHostedAnimation(tempProc->mAnimComponent->mInstance->object, &localBlend, true);
    tempProc->mBoneSet = &boneSets[mapIdx];

    if (((anim.mFlags >> 4) & 1) != 0 || blend->playMode == 2) {
        const int wrapCount = static_cast<int>(time / duration);
        time -= static_cast<float>(wrapCount) * duration;
    } else if (time > duration) {
        time = duration;
    }
    if (time < 100.0f) {
        time = 100.0f;
    }

    tempProc->JumpToTime(time);
    tempProc->Update(true, false);

    // Matches retail stack layout (0x0041ba4c..0x0041bb75):
    // Identity quat in [esp+0x80] is returned via *outRot, while accumRot in [esp+0x70] tracks chain.
    const cdc::Quat retRot = cdc::Quat::Identity();
    cdc::Quat accumRot = cdc::Quat::Identity();
    cdc::Vector3 totalTrans = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    const int minSeg = applyMirror ? 0 : 1;
    for (int seg = targetSeg; seg >= minSeg; seg = tempProc->mModelData->segmentList[seg].parent) {
        const cdc::Vector3 segRotLog = cdc::Vector3::Make(_segValues[seg].rot.x, _segValues[seg].rot.y, _segValues[seg].rot.z, 0.0f);
        const cdc::Quat segQuat = cdc::QuatExp(segRotLog);
        totalTrans = segQuat.Rotate(totalTrans) + _segValues[seg].trans;
        accumRot = segQuat * accumRot;
    }

    if (patternMirror) {
        totalTrans.x = -totalTrans.x;
    }
    if (outTrans != nullptr) {
        *outTrans = totalTrans;
    }
    if (outRot != nullptr) {
        *outRot = retRot;
    }
}
