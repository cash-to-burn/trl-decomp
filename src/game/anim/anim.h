#pragma once

#ifndef GAME_ANIM_ANIM_H
#define GAME_ANIM_ANIM_H

#include "cdc/cdcSys/Array.h"
#include "cdc/runtime/cdcMath/Math.h"
#include "game/instance.h"
#include <cstddef>
#include <cstdint>

class VirtualMachine;
struct AnimComponent;
struct AnimProcessor;
struct G2AnimAlphaTable;
struct Object;

// 0x5a1f in PDB
enum G2AnimCallbackMsg {
    G2ANIM_MSG_DONE = 1,
    G2ANIM_MSG_LOOPPOINT = 2,
    G2ANIM_MSG_SECTION_INTERPDONE = 3,
    G2ANIM_MSG_SEGCTRLR_INTERPDONE = 4,
    G2ANIM_MSG_SWALARMSET = 5,
    G2ANIM_MSG_PLAYEFFECT = 6,
    G2ANIM_MSG_PLAYEFFECT_HITREACTION = 7,
    G2ANIM_MSG_ANIM_QUEUE_EMPTY = 8,
    G2ANIM_MSG_SWITCH_KEYLIST = 9,
};

// 0x5a24 in PDB
enum RootInterpMode {
    kRootInterpMode_ConserveMotion = 0,
    kRootInterpMode_MorphSpeed = 1,
    kRootInterpMode_MorphVelocity = 2,
    kRootInterpMode_UserTransform = 3,
};

// Size: 0x10 (16 bytes)
struct _G2EulerAngles_Type {
    /* 0x00 */ float x;
    /* 0x04 */ float y;
    /* 0x08 */ float z;
    /* 0x0c */ int order;
};
static_assert(sizeof(_G2EulerAngles_Type) == 0x10, "_G2EulerAngles_Type size mismatch");

// Size: 0x40 (64 bytes, TI: 0x59e9)
struct G2AnimSegValue {
    /* 0x000 */ cdc::Vector3 rot;
    /* 0x010 */ cdc::Vector3 scale;
    /* 0x020 */ cdc::Vector3 trans;
    /* 0x030 */ cdc::Quat quat;
};
static_assert(sizeof(G2AnimSegValue) == 0x40, "G2AnimSegValue size mismatch");

// Size: 0x08 (8 bytes, TI: 0x59eb)
struct BoneSet {
    /* 0x00 */ float weight;
    /* 0x04 */ uint16_t firstBone;
    /* 0x06 */ uint16_t lastBone;
};
static_assert(sizeof(BoneSet) == 0x08, "BoneSet size mismatch");

// Size: 0x03 (3 bytes, TI: 0x5bac)
struct BoneMirrorData {
    /* 0x00 */ uint8_t bone1;
    /* 0x01 */ uint8_t bone2;
    /* 0x02 */ uint8_t count;
};
static_assert(sizeof(BoneMirrorData) == 0x03, "BoneMirrorData size mismatch");

// Size: 0x40 (64 bytes, TI: 0x5a8b)
struct Segment {
    /* 0x00 */ cdc::Vector3 min;
    /* 0x10 */ cdc::Vector3 max;
    /* 0x20 */ cdc::Vector3 pivot;
    /* 0x30 */ int flags;
    /* 0x34 */ int16_t firstVertex;
    /* 0x36 */ int16_t lastVertex;
    /* 0x38 */ int parent;
    /* 0x3c */ void* hInfo;
};
static_assert(sizeof(Segment) == 0x40, "Segment size mismatch");

// Size: 0xa0 (160 bytes, TI: 0x5bde)
struct Model {
    /* 0x00 */ int version;
    /* 0x04 */ int numSegments;
    /* 0x08 */ int numVirtSegments;
    /* 0x0c */ Segment* segmentList;
    /* 0x10 */ cdc::Vector3 modelScale;
    /* 0x20 */ int numVertices;
    /* 0x24 */ void* vertexList;
    /* 0x28 */ int numNormals;
    /* 0x2c */ void* normalList;
    /* 0x30 */ int numFaces;
    /* 0x34 */ void* faceList;
    /* 0x38 */ void* OBSOLETEaniTextures;
    /* 0x3c */ float maxRad;
    /* 0x40 */ float maxRadSq;
    /* 0x44 */ void* OBSOLETEstartTextures;
    /* 0x48 */ void* OBSOLETEendTextures;
    /* 0x4c */ void* animatedListInfo;
    /* 0x50 */ void* animatedInfo;
    /* 0x54 */ void* scrollInfo;
    /* 0x58 */ void* textureStripInfo;
    /* 0x5c */ void* envMappedVertices;
    /* 0x60 */ void* eyeRefEnvMappedVertices;
    /* 0x64 */ void* materialVertexColors;
    /* 0x68 */ void* spectralVertexColors;
    /* 0x6c */ void* pnShadowFaces;
    /* 0x70 */ void* pnShadowEdges;
    /* 0x74 */ BoneMirrorData* boneMirrorData;
    /* 0x78 */ void* drawgroupCenterList;
    /* 0x7c */ int numMarkUps;
    /* 0x80 */ void* markUpList;
    /* 0x84 */ int numTargets;
    /* 0x88 */ void* targetList;
    /* 0x8c */ uint32_t cdcRenderDataID;
    /* 0x90 */ void* cdcRenderModelData;
    /* 0x94 */ uint8_t pad094[0x0c];
};
static_assert(sizeof(Model) == 0xa0, "Model size mismatch");
static_assert(offsetof(Model, boneMirrorData) == 0x74, "Model::boneMirrorData offset mismatch");

// Size: 0x0c (12 bytes, TI: 0x5cc1)
struct G2AnimBitStream {
    /* 0x00 */ uint32_t* stream;
    /* 0x04 */ uint32_t flags;
    /* 0x08 */ int bitCount;
};
static_assert(sizeof(G2AnimBitStream) == 0x0c, "G2AnimBitStream size mismatch");

// Size: 0x1c (28 bytes, TI: 0x5aa3)
struct G2AnimAlphaTable {
    /* 0x00 */ int size;
    /* 0x04 */ float data[6];

    float GetValue(float alpha) const; // 0x004702f0 (PDB: 0x00471c70)
};
static_assert(sizeof(G2AnimAlphaTable) == 0x1c, "G2AnimAlphaTable size mismatch");

// Size: 0x60 (96 bytes, TI: 0x5a99)
struct G2AnimQuatInfo {
    /* 0x00 */ cdc::Quat srcQuat;
    /* 0x10 */ cdc::Quat destQuat;
    /* 0x20 */ cdc::Vector3 srcScale;
    /* 0x30 */ cdc::Vector3 destScale;
    /* 0x40 */ cdc::Vector3 srcTrans;
    /* 0x50 */ cdc::Vector3 destTrans;
};
static_assert(sizeof(G2AnimQuatInfo) == 0x60, "G2AnimQuatInfo size mismatch");

// Size: 0x190 (400 bytes, TI: 0x5cbf)
struct G2AnimInterpStateBlock {
    /* 0x000 */ G2AnimInterpStateBlock* next;
    /* 0x004 */ uint8_t pad004[0x0c];
    /* 0x010 */ G2AnimQuatInfo quatInfo[4];
};
static_assert(sizeof(G2AnimInterpStateBlock) == 0x190, "G2AnimInterpStateBlock size mismatch");

// Size: 0x10 (16 bytes, TI: 0x5a83)
struct G2AnimInterpInfo {
    /* 0x00 */ float duration;
    /* 0x04 */ float targetTime;
    /* 0x08 */ G2AnimAlphaTable* alphaTable;
    /* 0x0c */ G2AnimInterpStateBlock* stateBlockList;
};
static_assert(sizeof(G2AnimInterpInfo) == 0x10, "G2AnimInterpInfo size mismatch");

// Size: 0x02 (2 bytes, TI: 0x7111)
struct FxaInfo {
    uint16_t raw;
};
static_assert(sizeof(FxaInfo) == 0x02, "FxaInfo size mismatch");

// Size: 0x04 (4 bytes, TI: 0x7140)
struct ModeAndType {
    uint32_t raw;

    ModeAndType() = default;
    ModeAndType(FxaInfo& info, signed char unused, unsigned char fxId); // 0x00531090 (PDB: 0x00532d70)
};
static_assert(sizeof(ModeAndType) == 0x04, "ModeAndType size mismatch");

// Size: 0x08 (8 bytes, TI: 0x5cdf)
struct AnimFxHeader {
    /* 0x00 */ uint16_t info;
    /* 0x02 */ int16_t keyframeID;
    /* 0x04 */ uint8_t fxID;
    /* 0x05 */ uint8_t pad[3];
};
static_assert(sizeof(AnimFxHeader) == 0x08, "AnimFxHeader size mismatch");

// Size: 0x20 (32 bytes, TI: 0x5ac8; followed by mSectionDataOffset[])
struct AnimFragment {
    /* 0x00 */ float mTransX;
    /* 0x04 */ float mTransY;
    /* 0x08 */ float mTransZ;
    /* 0x0c */ float mRotX;
    /* 0x10 */ float mRotY;
    /* 0x14 */ float mRotZ;
    /* 0x18 */ uint16_t mAnimID;
    /* 0x1a */ uint16_t mKeyCount;
    /* 0x1c */ int16_t mTimePerKey;
    /* 0x1e */ uint8_t mSegmentCount;
    /* 0x1f */ uint8_t mSectionCount;

    int* SectionDataOffsets() { return reinterpret_cast<int*>(this + 1); }
    const int* SectionDataOffsets() const { return reinterpret_cast<const int*>(this + 1); }

    cdc::Vector3 GetTrans() const; // 0x0041a770 (PDB: 0x004196d0)
    cdc::Quat GetRot() const;      // 0x0041a7c0 (PDB: 0x00419720)
};
static_assert(sizeof(AnimFragment) == 0x20, "AnimFragment size mismatch");

// Size: 0x40 (64 bytes, TI: 0x5bc8)
struct Anim {
    /* 0x00 */ NodeType node;
    /* 0x08 */ uint8_t pad008[0x08];
    /* 0x10 */ cdc::Vector3 mTweakTranslation;
    /* 0x20 */ float mTweakFrames;
    /* 0x24 */ float mSpeed;
    /* 0x28 */ uint16_t mAnimId;
    /* 0x2a */ uint16_t mFlags;
    /* 0x2c */ uint16_t mRepeatCount;
    /* 0x2e */ uint16_t mTargetFrame;
    /* 0x30 */ uint16_t mInterpFrames;
    /* 0x32 */ uint8_t pad032[0x0e];
};
static_assert(sizeof(Anim) == 0x40, "Anim size mismatch");
static_assert(offsetof(Anim, mTweakTranslation) == 0x10, "Anim::mTweakTranslation offset mismatch");
static_assert(offsetof(Anim, mInterpFrames) == 0x30, "Anim::mInterpFrames offset mismatch");

// Size: 0x20 (32 bytes, TI: 0x64c8)
struct AnimBlend {
    /* 0x00 */ int16_t animFragment[9];
    /* 0x12 */ uint16_t playSpeed;
    /* 0x14 */ uint8_t playMode;
    /* 0x15 */ uint8_t blendType;
    /* 0x16 */ uint8_t rootInterpMode;
    /* 0x17 */ uint8_t interpFrames;
    /* 0x18 */ uint8_t startFrame;
    /* 0x19 */ uint8_t interpOut;
    /* 0x1a */ uint8_t alphaTable;
    /* 0x1b */ uint8_t alphaTableOut;
    /* 0x1c */ uint8_t clothSection;
    /* 0x1d */ int8_t boneSet;
    /* 0x1e */ uint8_t flags;
    /* 0x1f */ uint8_t pad;
};
static_assert(sizeof(AnimBlend) == 0x20, "AnimBlend size mismatch");

// Size: 0x160 (352 bytes, TI: 0x6995)
struct AnimSequence {
    /* 0x000 */ Anim mAnimList[4];
    /* 0x100 */ int mCount;
    /* 0x104 */ uint8_t pad104[0x0c];
    /* 0x110 */ cdc::Vector3 mTranslation;
    /* 0x120 */ cdc::Matrix mRotation;

    AnimSequence();                                                                              // 0x0041bb90 (PDB: 0x0041ab60)
    AnimSequence& operator<<(const Anim& anim);                                                  // 0x0041a830 (PDB: 0x00419790)
    Anim GetAnim(int index) const;                                                               // 0x0041a860 (PDB: 0x004197c0)
    int Size() const;                                                                            // 0x0041a880 (PDB: 0x004197e0)
    bool SolveTranslationRotation(BaseInstance* instance, const cdc::Vector3& targetPos, const cdc::Euler& targetRot); // 0x0041bc30 (PDB: 0x0041ac00)
};

// Size: 0x14 (20 bytes, TI: 0x664e)
struct AnimSequenceList {
    /* 0x000 */ AnimSequence* mAnimSequenceList[4];
    /* 0x010 */ int mSize;

    AnimSequenceList();                                             // 0x0041a890 (PDB: 0x004197f0)
    void Add(AnimSequence* seq);                                    // 0x0041a8a0 (PDB: 0x00419800)
    int Size() const;                                               // PDB: 0x00419810
    AnimSequence* GetAnimSequence(int index) const { return mAnimSequenceList[index]; }
};

static_assert(sizeof(AnimSequenceList) == 0x14, "AnimSequenceList size mismatch");
static_assert(offsetof(AnimSequenceList, mSize) == 0x10, "AnimSequenceList::mSize offset mismatch");
static_assert(sizeof(AnimSequence) == 0x160, "AnimSequence size mismatch");
static_assert(offsetof(AnimSequence, mCount) == 0x100, "AnimSequence::mCount offset mismatch");
static_assert(offsetof(AnimSequence, mTranslation) == 0x110, "AnimSequence::mTranslation offset mismatch");
static_assert(offsetof(AnimSequence, mRotation) == 0x120, "AnimSequence::mRotation offset mismatch");

// Size: 0xf0 (240 bytes, TI: 0x5cea)
struct AnimTweaker {
    struct QuatTrans {
        /* 0x00 */ cdc::Quat q;
        /* 0x10 */ cdc::Vector3 t;
    };
    static_assert(sizeof(QuatTrans) == 0x20, "AnimTweaker::QuatTrans size mismatch");

    /* 0x000 */ cdc::Matrix mOriginal;
    /* 0x040 */ cdc::Matrix mCurrentCorrection;
    /* 0x080 */ cdc::Matrix mModified;
    /* 0x0c0 */ QuatTrans mTotalCorrection;
    /* 0x0e0 */ float mFrames;
    /* 0x0e4 */ bool mIsRunning;
    /* 0x0e5 */ uint8_t pad0e5[0x0b];

    AnimTweaker();                                                 // 0x004747c0 (PDB: 0x00476140)
    void Start(const cdc::Vector3& translation, float frames);     // 0x00474850 (PDB: 0x004761d0)
};
static_assert(sizeof(AnimTweaker) == 0xf0, "AnimTweaker size mismatch");
static_assert(offsetof(AnimTweaker, mTotalCorrection) == 0xc0, "AnimTweaker::mTotalCorrection offset mismatch");
static_assert(offsetof(AnimTweaker, mIsRunning) == 0xe4, "AnimTweaker::mIsRunning offset mismatch");

struct AnimController;

typedef unsigned int (*AnimControllerCallback)(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix,
    void* voiddata);

typedef int (*AnimCallbackFn)(
    AnimProcessor* ap,
    int sectionID,
    G2AnimCallbackMsg msg,
    int messageDataA,
    int messageDataB,
    void* callbackData);

// Size: 0x70 (112 bytes, TI: 0x5b96)
struct AnimController {
    /* 0x000 */ uint16_t mNext;
    /* 0x002 */ uint8_t mType;
    /* 0x003 */ uint8_t mSegNumber;
    /* 0x004 */ void* mCallbackData;
    /* 0x008 */ uint16_t mFlags;
    /* 0x00a */ uint16_t pad00a;
    /* 0x00c */ float mDuration;
    /* 0x010 */ float mElapsedTime;
    /* 0x014 */ G2AnimAlphaTable* mAlphaTable;
    /* 0x018 */ uint8_t pad018[8];
    /* 0x020 */ union {
        struct {
            /* 0x00 */ AnimControllerCallback fn;
            /* 0x04 */ void* fnData;
        } callback;
        struct {
            /* 0x00 */ cdc::Matrix m;
        } matrix;
        struct {
            /* 0x00 */ uint8_t pad00[0x10];
            /* 0x10 */ cdc::Vector3 base;
            /* 0x20 */ cdc::Vector3 offset;
        } vector;
        struct {
            /* 0x00 */ uint8_t pad00[0x30];
            /* 0x30 */ cdc::Quat src;
            /* 0x40 */ cdc::Quat dest;
        } quat;
        uint8_t raw[0x50];
    } mData;

    void GetVector(cdc::Vector3* vector);                                                           // 0x0046fbb0 (PDB: 0x00471530)
    AnimController* Destroy();                                                                      // 0x0046fc10 (PDB: 0x00471590)
    void InsertIntoList(uint16_t* listHead);                                                        // 0x0046fc40 (PDB: 0x004715c0)
    static AnimController* FindInList(int seg, int type, uint16_t* listHead);                       // 0x0046fcd0 (PDB: 0x00471650)
    static AnimController* FindPtrInList(int seg, int type, uint16_t** listPtr);                    // 0x0046fd20 (PDB: 0x004716a0)
    static AnimController* RemoveFromList(int seg, int type, uint16_t* listHead);                   // 0x0046fd80 (PDB: 0x00471700)
    void GetMatrix(cdc::Matrix* matrix);                                                            // 0x00470f90 (PDB: 0x00472910)
    unsigned int ApplyWorldToParentMatrix(cdc::Matrix* parentMatrix);                               // 0x00474b20 (PDB: 0x004764a0)
    unsigned int ApplyToSegValue(G2AnimSegValue* segValue, cdc::Matrix* parentMatrix, cdc::Matrix* segMatrix); // 0x004755b0 (PDB: 0x00476f30)
    void GetCurrentInterpQuat(AnimProcessor* ap, cdc::Quat* outQuat);                               // 0x00476e40 (PDB: 0x004787c0)
};
static_assert(sizeof(AnimController) == 0x70, "AnimController size mismatch");
static_assert(offsetof(AnimController, mFlags) == 0x08, "AnimController::mFlags offset mismatch");
static_assert(offsetof(AnimController, mData) == 0x20, "AnimController::mData offset mismatch");
static_assert(offsetof(AnimController, mData.vector.base) == 0x30, "AnimController::mData.vector.base offset mismatch");
static_assert(offsetof(AnimController, mData.quat.src) == 0x50, "AnimController::mData.quat.src offset mismatch");
static_assert(offsetof(AnimController, mData.quat.dest) == 0x60, "AnimController::mData.quat.dest offset mismatch");

// Size: 0x10 (16 bytes, TI: 0xbbe1 / 0xbbe4 / 0x9cf0)
struct AnimControllerPool {
    /* 0x00 */ uint16_t blockSize;
    /* 0x02 */ uint16_t stackTop;
    /* 0x04 */ uint16_t stackSize;
    /* 0x06 */ uint16_t pad06;
    /* 0x08 */ uint16_t* stack;
    /* 0x0c */ AnimController* blockPool;
};
static_assert(sizeof(AnimControllerPool) == 0x10, "AnimControllerPool size mismatch");

struct G2AnimInterpStateBlockPool {
    /* 0x00 */ uint16_t blockSize;
    /* 0x02 */ uint16_t stackTop;
    /* 0x04 */ uint16_t stackSize;
    /* 0x06 */ uint16_t pad06;
    /* 0x08 */ uint16_t* stack;
    /* 0x0c */ G2AnimInterpStateBlock* blockPool;
};
static_assert(sizeof(G2AnimInterpStateBlockPool) == 0x10, "G2AnimInterpStateBlockPool size mismatch");

// Size: 0x70 (112 bytes, TI: 0x5a7f)
struct AnimFragmentProcessor {
    /* 0x00 */ cdc::Quat mRootRot;
    /* 0x10 */ cdc::Vector3 mRootTrans;
    /* 0x20 */ AnimProcessor* mAnimProcessor;
    /* 0x24 */ AnimFragment* mKeylist;
    /* 0x28 */ float mElapsedTime;
    /* 0x2c */ float mStoredTime;
    /* 0x30 */ void* mFxListHost;
    /* 0x34 */ AnimFxHeader* mFxList;
    /* 0x38 */ void* mSwAlarmTable;
    /* 0x3c */ float mLoopStartTime;
    /* 0x40 */ float mLoopEndTime;
    /* 0x44 */ AnimCallbackFn mCallback;
    /* 0x48 */ void* mCallbackData;
    /* 0x4c */ uint32_t mAlarmFlags;
    /* 0x50 */ float mSpeedAdjustment;
    /* 0x54 */ float mAccumulatedTime;
    /* 0x58 */ uint16_t mFlags;
    /* 0x5a */ uint16_t mKeylistID;
    /* 0x5c */ uint8_t mSectionID;
    /* 0x5d */ uint8_t mFirstSeg;
    /* 0x5e */ uint8_t mSegCount;
    /* 0x5f */ uint8_t pad05f;
    /* 0x60 */ RootInterpMode mRootInterpMode;
    /* 0x64 */ RootInterpMode mNewRootInterpMode;
    /* 0x68 */ uint8_t pad068[8];

    AnimFragmentProcessor() = default;                                                              // 0x00412630 (PDB: 0x00411a00)
    void JumpToTime(float targetTime);                                                              // 0x0046fef0 (PDB: 0x00471870)
    static void InitializeSegValue(Segment* segment, G2AnimSegValue* segValue);                     // 0x0046ffc0 (PDB: 0x00471940)
    void InitializeKeyflagInfo(G2AnimBitStream* bs1, G2AnimBitStream* bs2, G2AnimBitStream* bs3);    // 0x00470050 (PDB: 0x004719d0)
    static float DecompressChannel_Linear(int keyframe, float* channelData);                        // 0x00470140 (PDB: 0x00471ac0)
    void TriggerEffects(float startTime, float endTime, int masked);                                // 0x004701a0 (PDB: 0x00471b20)
    bool IsInInterpolation() const;                                                                 // 0x00470ad0 (PDB: 0x00472450)
    void SetAlphaTable(G2AnimAlphaTable* alphaTable);                                               // 0x00470b00 (PDB: 0x00472480)
    void SegValueToQuat(G2AnimSegValue* segValues, int dest);                                       // 0x00471120 (PDB: 0x00472aa0)
    void GetRootMotionFromTimeForDuration(float startTime, float duration, cdc::Vector3* trans, cdc::Quat* rot); // 0x004714e0 (PDB: 0x00472e60)
    void GetRootMotionOverInterval(float startTime, float endTime, cdc::Vector3* trans, cdc::Quat* rot);          // 0x00471d00 (PDB: 0x00473680)
    void SwitchToKeylistAtTimeWithoutAdvancing(AnimFragment* keylist, int keylistID, float time, void* fxListHost, AnimFxHeader* fxList); // 0x00471e00 (PDB: 0x00473780)
    void AdvanceOverAccumulatedTime();                                                              // 0x00471f40 (PDB: 0x004738c0)
    void DecompressFrameWithBoneSet(Model* model, int keyframe, float interpAlpha, float weight, float blendFrac, bool b0, bool b1, bool b2, const BoneSet* boneSet, bool b3); // 0x004725e0 (PDB: 0x00473f60)
    int GetKeyframeNumber() const;                                                                  // 0x00474740 (PDB: 0x004760c0)
    void SetLooping();                                                                              // 0x00474790 (PDB: 0x00476110)
    void UpdateStoredFrameFromQuat();                                                               // 0x00474c80 (PDB: 0x00476600)
    void ApplyControllersToStoredFrame();                                                           // 0x00475950 (PDB: 0x004772d0)
    void InterpToKeylistAtTime(AnimFragment* keylist, int keylistID, float targetTime, float duration, void* fxListHost, AnimFxHeader* fxList); // 0x00475b50 (PDB: 0x004774d0)
    void InterpToKeylistFrame(AnimFragment* keylist, int keylistID, int frame, float duration, void* fxListHost, AnimFxHeader* fxList);        // 0x004764c0 (PDB: 0x00477e40)
};
static_assert(sizeof(AnimFragmentProcessor) == 0x70, "AnimFragmentProcessor size mismatch");
static_assert(offsetof(AnimFragmentProcessor, mAnimProcessor) == 0x20, "AnimFragmentProcessor::mAnimProcessor offset mismatch");
static_assert(offsetof(AnimFragmentProcessor, mFlags) == 0x58, "AnimFragmentProcessor::mFlags offset mismatch");
static_assert(offsetof(AnimFragmentProcessor, mNewRootInterpMode) == 0x64, "AnimFragmentProcessor::mNewRootInterpMode offset mismatch");

// Size: 0x5f0 (1520 bytes, TI: 0x5ca1)
class AnimPattern {
public:
    virtual ~AnimPattern() = default;
    virtual bool Entry(AnimProcessor* ap) = 0;
    virtual bool Process(AnimProcessor* ap) { (void)ap; return false; } // 0x0049a3a0
    virtual bool Exit(AnimProcessor* ap) { (void)ap; return false; }    // 0x0049a3a0
};

struct AnimProcessor {
    /* 0x000 */ cdc::Vector3 mRootTrans;
    /* 0x010 */ cdc::Quat mRootRot;
    /* 0x020 */ G2AnimInterpInfo mInterpInfo;
    /* 0x030 */ AnimTweaker mAnimTweaker;
    /* 0x120 */ AnimFragmentProcessor* mSection;
    /* 0x124 */ float* mWeights;
    /* 0x128 */ Model* mModelData;
    /* 0x12c */ cdc::Matrix* mSegMatrices;
    /* 0x130 */ float mBlendValue1;
    /* 0x134 */ float mBlendValue2;
    /* 0x138 */ float mLastBlendValue1;
    /* 0x13c */ float mLastBlendValue2;
    /* 0x140 */ float mLayerWeight;
    /* 0x144 */ const BoneSet* mBoneSet;
    /* 0x148 */ bool mAnimWasPlayed;
    /* 0x149 */ uint8_t pad149;
    /* 0x14a */ uint16_t mControllerList;
    /* 0x14c */ uint16_t mDisabledControllerList;
    /* 0x14e */ uint16_t mFlags;
    /* 0x150 */ uint8_t mSectionCount;
    /* 0x151 */ uint8_t mSectionsAllocated;
    /* 0x152 */ uint8_t mMasterSection;
    /* 0x153 */ uint8_t mBlendType;
    /* 0x154 */ uint8_t mLayerBlendMode;
    /* 0x155 */ bool mClearBeforeUpdate;
    /* 0x156 */ uint8_t mPad[0x1a];
    /* 0x170 */ Anim mAnimQueue[16];
    /* 0x570 */ cdc::Vector3 mVectorVar[2];
    /* 0x590 */ float mFloatVar[4];
    /* 0x5a0 */ bool mBoolVar[4];
    /* 0x5a4 */ int mFrontIndex;
    /* 0x5a8 */ int mBackIndex;
    /* 0x5ac */ int mMarker;
    /* 0x5b0 */ bool mHasControl;
    /* 0x5b1 */ uint8_t pad5b1[3];
    /* 0x5b4 */ AnimCallbackFn mOldCallback;
    /* 0x5b8 */ void* mOldCallbackData;
    /* 0x5bc */ bool mReadyForNextAnim;
    /* 0x5bd */ uint8_t pad5bd[3];
    /* 0x5c0 */ VirtualMachine* mVirtualMachine;
    /* 0x5c4 */ bool mUseRateRamp;
    /* 0x5c5 */ uint8_t pad5c5[3];
    /* 0x5c8 */ float mStartRate;
    /* 0x5cc */ float mEndRate;
    /* 0x5d0 */ AnimComponent* mAnimComponent;
    /* 0x5d4 */ bool mCurrentPatternIsNative;
    /* 0x5d5 */ uint8_t pad5d5[3];
    /* 0x5d8 */ int mCurrentPatternIndex;
    /* 0x5dc */ int mCurrentPatternId;
    /* 0x5e0 */ int mCurrentBlendId;
    /* 0x5e4 */ bool mMirror;
    /* 0x5e5 */ uint8_t pad5e5[0x0b];

    static AnimPattern* sNativePattern[];          // 0x00f15b40 (PDB: 0x00f17e78)
    static cdc::Quat sUserTransformRotation;     // (PDB: 0x010fd140)
    static cdc::Vector3 sUserTransformTranslation; // (PDB: 0x010fd130)

    AnimProcessor();                                                                               // 0x004752a0 (PDB: 0x00476c20)
    ~AnimProcessor();                                                                              // 0x00474400 (PDB: 0x00475d80)

    void UpdateMirrorMode();                                                                       // 0x0046fb00 (PDB: 0x00471480)
    void CopyVectorWithOrder(cdc::Vector* vec, _G2EulerAngles_Type* euler, int order);             // 0x0046fb60 (PDB: 0x004714e0)
    static void FreeInterpStateBlockList(G2AnimInterpStateBlock* list);                            // 0x0046fe00 (PDB: 0x00471780)
    bool Install();                                                                                // 0x0046fe30 (PDB: 0x004717b0)
    void ResetInternalState();                                                                     // 0x0046fe70 (PDB: 0x004717f0)
    AnimFragmentProcessor* GetSectionWithSeg(int seg);                                             // 0x0046fea0 (PDB: 0x00471820)
    void ResetCache();                                                                             // 0x0046ff80 (PDB: 0x00471900)
    AnimController* GetControllerList();                                                           // 0x0046ffa0 (PDB: 0x00471920)
    void SwapBones();                                                                              // 0x00470340 (PDB: 0x00471cc0)
    void AllocateSections(int count);                                                              // 0x004704f0 (PDB: 0x00471e70)
    void SetMarker();                                                                              // 0x00470620 (PDB: 0x00471fa0)
    void PopToNextAnim();                                                                          // 0x00470630 (PDB: 0x00471fb0)
    cdc::Vector3 GetVector(int index);                                                             // 0x00470670 (PDB: 0x00471ff0)
    float GetFloat(int index);                                                                     // 0x004706a0 (PDB: 0x00472020)
    void ReleaseControl();                                                                         // 0x004706b0 (PDB: 0x00472030)
    cdc::Vector3 GetTranslation();                                                                 // 0x00470710 (PDB: 0x00472090)
    cdc::Matrix GetRotation();                                                                     // 0x004707a0 (PDB: 0x00472120)
    void SetAnimComponent(AnimComponent* comp);                                                    // 0x004707d0 (PDB: 0x00472150)
    int CurrentPatternId() const;                                                                  // 0x004707e0 (PDB: 0x00472160)
    int CurrentBlendId() const;                                                                    // 0x004707f0 (PDB: 0x00472170)
    void SetCurrentBlendId(int id);                                                                // 0x00470800 (PDB: 0x00472180)
    bool GetPatternMirrorMode() const;                                                             // 0x00470810 (PDB: 0x00472190)
    void SetMirrorMode(bool mirror);                                                               // 0x0041a810 (PDB: 0x00419770)
    void DisableRootTranslation();                                                                 // 0x00470820 (PDB: 0x004721a0)
    void EnableRootTranslation();                                                                  // 0x00470830 (PDB: 0x004721b0)
    float GetElapsedTime();                                                                        // 0x00470840 (PDB: 0x004721c0)
    AnimFragment* GetKeylist();                                                                    // 0x00470860 (PDB: 0x004721e0)
    int GetKeylistID();                                                                            // 0x00470880 (PDB: 0x00472200)
    bool IsAlarmSet(unsigned int mask);                                                            // 0x004708a0 (PDB: 0x00472220)
    void JumpToTime(float targetTime);                                                             // 0x004708d0 (PDB: 0x00472250)
    void SetCallback(AnimCallbackFn callback, void* data);                                         // 0x00470920 (PDB: 0x004722a0)
    void SetNoLooping();                                                                           // 0x00470970 (PDB: 0x004722f0)
    void SetPaused();                                                                              // 0x004709a0 (PDB: 0x00472320)
    void SetSpeedAdjustment(float speed);                                                          // 0x004709d0 (PDB: 0x00472350)
    void SetUnpaused();                                                                            // 0x00470a10 (PDB: 0x00472390)
    float GetAnimDuration(const Anim& anim);                                                       // 0x00470a40 (PDB: 0x004723c0)
    VirtualMachine* GetVirtualMachine();                                                           // 0x00470a90 (PDB: 0x00472410)
    void SetUserTransform(const cdc::Quat& rot, const cdc::Vector3& trans);                        // 0x00470b20 (PDB: 0x004724a0)
    void AttachControllerToSeg(int seg, int type);                                                 // 0x00470b60 (PDB: 0x004724e0)
    void DetachControllerFromSeg(int seg, int type);                                               // 0x00470bb0 (PDB: 0x00472530)
    void EnableController(int seg, int type);                                                      // 0x00470c30 (PDB: 0x004725b0)
    void DisableController(int seg, int type);                                                     // 0x00470dd0 (PDB: 0x00472750)
    bool IsControllerActive(int seg, int type);                                                    // 0x00470e10 (PDB: 0x00472790)
    bool IsControllerInterpolating(int seg, int type);                                             // 0x00470e40 (PDB: 0x004727c0)
    void UpdateControllers();                                                                      // 0x00470e80 (PDB: 0x00472800)
    static G2AnimInterpStateBlock* AllocateInterpStateBlockList(AnimFragmentProcessor* section);   // 0x00471200 (PDB: 0x00472b80)
    void SetModel(Model* model);                                                                   // 0x00471290 (PDB: 0x00472c10)
    AnimFragmentProcessor* AddSection(int firstSeg, int segCount);                                 // 0x00471380 (PDB: 0x00472d00)
    void Update(bool updateRoot, bool force);                                                      // 0x00472f50 (PDB: 0x004748d0)
    static int AnimCallback(AnimProcessor* ap, int sectionID, G2AnimCallbackMsg msg, int messageDataA, int messageDataB, void* callbackData); // 0x00474330 (PDB: 0x00475cb0)
    void PopBackToMarker();                                                                        // 0x00474520 (PDB: 0x00475ea0)
    bool StopPattern();                                                                            // 0x00474560 (PDB: 0x00475ee0)
    bool StartPattern(int patternId);                                                              // 0x004745d0 (PDB: 0x00475f50)
    void SetAlphaTable(G2AnimAlphaTable* alphaTable);                                              // 0x00474690 (PDB: 0x00476010)
    void SetInterpInfo();                                                                          // 0x004746e0 (PDB: 0x00476060)
    void SetSectionCount(int count);                                                               // 0x004748f0 (PDB: 0x00476270)
    bool DoesControllerExist(int seg, int type);                                                   // 0x00474930 (PDB: 0x004762b0)
    void SetControllerAlphaTable(int seg, int type, G2AnimAlphaTable* alphaTable);                 // 0x00474980 (PDB: 0x00476300)
    void SetControllerCallbackData(int seg, int type, void* data);                                 // 0x004749d0 (PDB: 0x00476350)
    void SetControllerAngleOrder(int seg, int type, int order);                                    // 0x00474a20 (PDB: 0x004763a0)
    void SetController_Vector(int seg, int type, cdc::Vector* vector);                             // 0x00474a70 (PDB: 0x004763f0)
    void SetController_Function(int seg, int type, AnimControllerCallback func, void* data);       // 0x00474ad0 (PDB: 0x00476450)
    void Stop();                                                                                   // 0x00474e30 (PDB: 0x004767b0)
    void UpdateStoredFrame();                                                                      // 0x00474eb0 (PDB: 0x00476830)
    static void BuildSegLocalRotMatrix(G2AnimSegValue* segValue, cdc::Matrix* matrix);             // 0x00475220 (PDB: 0x00476ba0)
    bool ExecutePattern(int patternId, bool mirror);                                               // 0x00475460 (PDB: 0x00476de0)
    int GetKeyframeNumber();                                                                       // 0x00475490 (PDB: 0x00476e10)
    void NextKeyframe();                                                                           // 0x004754b0 (PDB: 0x00476e30)
    void NextKeyframeTimeDelta(float delta);                                                       // 0x00475500 (PDB: 0x00476e80)
    void SetLooping();                                                                             // 0x00475550 (PDB: 0x00476ed0)
    void SwitchToKeylist(AnimFragment* keylist, int keylistID, void* fxListHost, AnimFxHeader* fxList); // 0x00476390 (PDB: 0x00477d10)
    void SwitchToKeylistFrame(AnimFragment* keylist, int keylistID, int frame, void* fxListHost, AnimFxHeader* fxList); // 0x00476420 (PDB: 0x00477da0)
    void ApplyControllersToStoredFrame();                                                          // 0x00476510 (PDB: 0x00477e90)
    void BuildSegTransformNoControllers(cdc::Matrix* segMatrix, cdc::Matrix* parentMatrix, bool b0, bool b1, int segIndex); // 0x004762b0 (PDB: 0x00477c30)
    void BuildSegTransformWithControllers(cdc::Matrix* segMatrix, cdc::Matrix* parentMatrix, AnimController* ac, bool b0, bool b1, int segIndex); // 0x00476560 (PDB: 0x00477ee0)
    void PlayHostedAnimation(Object* host, int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, BoneSet* boneSet, bool mirror); // 0x004766d0 (PDB: 0x00478050)
    void PlayHostedAnimationWithAlpha(Object* host, int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, int alphaTable, BoneSet* boneSet, bool mirror); // 0x00476970 (PDB: 0x004782f0)
    void BuildTransformsNoControllers();                                                           // 0x00476a00 (PDB: 0x00478380)
    void InterpToKeylistFrame(AnimFragment* keylist, int keylistID, int frame, float duration, void* fxListHost, AnimFxHeader* fxList); // 0x00476aa0 (PDB: 0x00478420)
    void GetControllerCurrentInterpVector(int seg, int type, cdc::Vector3* vector);                // 0x00476b50 (PDB: 0x004784d0)
    void BuildTransformsWithControllers();                                                         // 0x00476d20 (PDB: 0x004786a0)
    void PlayHostedAnimation(Object* host, AnimBlend* blend, bool mirror);                         // 0x00476fe0 (PDB: 0x00478960)
    void PlayAnimation(int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, BoneSet* boneSet, bool mirror); // 0x00477060 (PDB: 0x004789e0)
    void PlayAnimationWithAlpha(int16_t* animFragment, int startFrame, int interpFrames, int playMode, uint8_t blendType, int alphaTable, BoneSet* boneSet, bool mirror); // 0x004770a0 (PDB: 0x00478a20)
    void SetInterpController_Quat(int seg, int type, cdc::Quat* quat, float duration);             // 0x004770e0 (PDB: 0x00478a60)
    void PlayAnimation(AnimBlend* blend, bool mirror);                                             // 0x00477150 (PDB: 0x00478ad0)
    void PlayNextAnim();                                                                           // 0x00477180 (PDB: 0x00478b00)
    void Process();                                                                                // 0x004773b0 (PDB: 0x00478d30)
    void SetInterpController_Vector(int seg, int type, cdc::Vector3* vector, float duration);      // 0x00477460 (PDB: 0x00478de0)
    void TakeControl();                                                                            // 0x00477560 (PDB: 0x00478ee0)
    void InterpDisableController(int seg, int type, float duration);                               // 0x004775f0 (PDB: 0x00478f70)
    void PushBack(const Anim& anim);                                                               // 0x00477720 (PDB: 0x004790a0)
    void CriticalOn();                                                                             // 0x004778f0 (PDB: 0x00479270)
    void CriticalOff();                                                                            // 0x00477930 (PDB: 0x004792b0)
    void PushBack(const AnimSequence& seq);                                                        // 0x00477970 (PDB: 0x004792f0)
};
static_assert(sizeof(AnimProcessor) == 0x5f0, "AnimProcessor size mismatch");
static_assert(offsetof(AnimProcessor, mAnimTweaker) == 0x30, "AnimProcessor::mAnimTweaker offset mismatch");
static_assert(offsetof(AnimProcessor, mSection) == 0x120, "AnimProcessor::mSection offset mismatch");
static_assert(offsetof(AnimProcessor, mAnimQueue) == 0x170, "AnimProcessor::mAnimQueue offset mismatch");
static_assert(offsetof(AnimProcessor, mVectorVar) == 0x570, "AnimProcessor::mVectorVar offset mismatch");
static_assert(offsetof(AnimProcessor, mFloatVar) == 0x590, "AnimProcessor::mFloatVar offset mismatch");
static_assert(offsetof(AnimProcessor, mFrontIndex) == 0x5a4, "AnimProcessor::mFrontIndex offset mismatch");
static_assert(offsetof(AnimProcessor, mVirtualMachine) == 0x5c0, "AnimProcessor::mVirtualMachine offset mismatch");
static_assert(offsetof(AnimProcessor, mAnimComponent) == 0x5d0, "AnimProcessor::mAnimComponent offset mismatch");
static_assert(offsetof(AnimProcessor, mMirror) == 0x5e4, "AnimProcessor::mMirror offset mismatch");

// Size: 0x04 (4 bytes, TI: 0x6602)
struct BlendProcessor {
    /* 0x000 */ AnimComponent* mAnimComponent;

    void SetAnimComponent(AnimComponent* comp) { mAnimComponent = comp; }
    void UpdateStoredFrame();                                       // 0x0041a9c0 (PDB: 0x00419930)
    void NextKeyframe();                                            // 0x0041aa10 (PDB: 0x00419980)
    void NextKeyframeTimeDelta(float delta);                        // 0x0041aa90 (PDB: 0x00419a00)
    void ApplyRagdoll();                                            // 0x0041c230 (PDB: 0x0041b200)
    void BuildTransforms();                                         // 0x0041cad0 (PDB: 0x0041baa0)
};

static_assert(sizeof(BlendProcessor) == 0x04, "BlendProcessor size mismatch");

// Size: 0x14 (20 bytes, TI: 0x6677)
struct AnimComponent {
    /* 0x000 */ BlendProcessor mBlendProcessor;
    /* 0x004 */ AnimProcessor* mAnimProcessor;
    /* 0x008 */ int mAnimProcessorCount;
    /* 0x00c */ Instance* mInstance;
    /* 0x010 */ Model* mModel;

    AnimComponent(BaseInstance* instance, int processorCount);      // 0x0041af70 (PDB: 0x00419f40)
    ~AnimComponent();                                               // 0x0041b020 (PDB: 0x00419ff0)
    void PlayAnimation(AnimBlend* blend, int processorIdx, bool mirror);                                  // 0x0041a8b0 (PDB: 0x00419820)
    void PlayHostedAnimation(Object* host, AnimBlend* blend, int processorIdx, bool mirror);              // 0x0041a8e0 (PDB: 0x00419850)
    void Stop();                                                    // 0x0041a910 (PDB: 0x00419880)
    bool IsAnimationPlaying(int animBlendId, int processorIdx);     // 0x0041a940 (PDB: 0x004198b0)
};

// Size: 0x10 (16 bytes, TI: 0x87d6)
class AnimTarget {
public:
    virtual ~AnimTarget();                                          // 0x004779c0 (PDB: 0x00479340)
    virtual void PickSequence(AnimProcessor* animProcessor);        // 0x00613790 (PDB: 0x00450370)
    void AddSequence(AnimSequence* sequence);                       // 0x00477a10 (PDB: 0x005e7d10)

    /* 0x004 */ cdc::Array<AnimSequence*> mSequences;
};

static_assert(sizeof(AnimTarget) == 0x10, "AnimTarget size mismatch");
static_assert(sizeof(AnimComponent) == 0x14, "AnimComponent size mismatch");
static_assert(offsetof(AnimComponent, mAnimProcessor) == 0x04, "AnimComponent::mAnimProcessor offset mismatch");
static_assert(offsetof(AnimComponent, mInstance) == 0x0c, "AnimComponent::mInstance offset mismatch");

void MATH3D_InterpOnePlusAlpha(cdc::Vector3* dst, cdc::Vector3* a, cdc::Vector3* b, float alpha); // 0x0046fad0 (PDB: 0x00471450)
void ANIM_SetMorphVelocityDirection(const cdc::Vector3& dir);                                     // 0x0046fb30 (PDB: 0x004714b0)
void WrapQuatLog(cdc::Quat* out, cdc::Quat& in);                                                  // 0x00471050 (PDB: 0x004729d0)

extern G2AnimInterpStateBlockPool _interpStateBlockPool; // 0x010fa300 (PDB: 0x010fd100)
extern AnimControllerPool _controllerPool;               // 0x010fa310 (PDB: 0x010fd110)
extern cdc::Vector3 gAnimMorphVelocityDir;               // 0x010fa320 (PDB: 0x010fd120)
extern G2AnimAlphaTable G2AlphaTables[];                 // (PDB: 0x011019c0)
extern BoneSet sBoneSet[][4];                            // 0x00f15ac0 (PDB: 0x00f17df8)
extern G2AnimSegValue* _segValues;                       // 0x00f1658c (PDB: 0x00f188c4)
extern AnimProcessor* _segValuesAnim;                    // 0x010fe8a0 (PDB: 0x01101aa0)
extern G2AnimSegValue _segValuesStatic[];                // 0x010fe8c0 (PDB: 0x01101ac0)

AnimSequence* ANIM_MinimumErrorSequence(AnimSequenceList& list);                                // PDB: 0x00419a80
cdc::Euler ANIM_GetRotationAtTime(const AnimFragment* keylist, int seg, float time);            // 0x0041ab70 (PDB: 0x00419af0)
bool ANIM_HasRootMotion(const AnimFragment* keylist);                                           // 0x0041abc0 (PDB: 0x00419b40)
AnimBlend* ANIM_AnimBlendFromIndex(BaseInstance* inst, int16_t index);                          // 0x0041acf0 (PDB: 0x00419c70)
cdc::Vector3 ANIM_GetTranslationFromBlendId(Instance* inst, int16_t blendId);                   // 0x0041ad20 (PDB: 0x00419ca0)
cdc::Quat ANIM_GetRotationFromBlendId(Instance* inst, int16_t blendId);                         // 0x0041adb0 (PDB: 0x00419d30)
cdc::Vector3 ANIM_GetTranslationAtTime(const AnimFragment* keylist, int seg, float time);       // 0x0041b090 (PDB: 0x0041a060)
void ANIM_GetTransformAtTime(cdc::Quat* outRot, cdc::Vector3* outTrans, const AnimFragment* keylist, int segOrZero, float time); // 0x0041b350 (PDB: 0x0041a320)
void ANIM_GetAnimBlendTransformAtTime(AnimProcessor* ap, const Anim& anim, cdc::Quat* outRot, cdc::Vector3* outTrans, int targetSeg, float time, bool applyMirror); // 0x0041b780 (PDB: 0x0041a750)

#endif // GAME_ANIM_ANIM_H
