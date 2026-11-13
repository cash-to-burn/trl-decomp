#pragma once

#ifndef GAME_ANIM_ANIMPATTERNVM_H
#define GAME_ANIM_ANIMPATTERNVM_H

#include <cstddef>
#include <cstdint>

class AnimProcessor;
class VirtualMachine;

// Size: 0x04 (TI: 0x5bae)
struct AnimScriptObject {
    typedef const uint32_t* Method;
    /* 0x00 */ Method* virtualMethodTable;
};
static_assert(sizeof(AnimScriptObject) == 0x04, "AnimScriptObject size mismatch");

// Size: 0x04 (TI: 0x5a10)
union AnimDataValue {
    float f;
    int32_t i;
    uint32_t u;
    bool b;
    void* p;
};
static_assert(sizeof(AnimDataValue) == 0x04, "AnimDataValue size mismatch");

// Size: 0x1c (TI: 0x882c)
class NativePlayer {
public:
    NativePlayer();  // 0x005b36e0 (PDB: 0x005b5fe0)
    ~NativePlayer(); // 0x005b36f0 (PDB: 0x005b5ff0)

    uint8_t pad[0x1c];
};
static_assert(sizeof(NativePlayer) == 0x1c, "NativePlayer size mismatch");

typedef void (*NativeMethodFn)(VirtualMachine* vm);
extern NativeMethodFn* gNativeClassTable[]; // 0x00f15c60 (PDB seg3:0x00005f98)

// Size: 0x28 (40 bytes, TI: 0x5a17)
class VirtualMachine {
public:
    // Size: 0x0c (12 bytes, TI: 0x5a19)
    struct MethodContext {
        /* 0x00 */ AnimScriptObject* mSelf;
        /* 0x04 */ const uint32_t* mPC;
        /* 0x08 */ AnimDataValue* mStack;
    };

    explicit VirtualMachine(AnimProcessor* animProcessor); // 0x0046f570 (PDB: 0x00470ee0)
    ~VirtualMachine();                                     // 0x0046fa60 (PDB: 0x004713e0)

    void CallVirtualMethod(AnimScriptObject* obj, int methodIdx); // 0x0046fab0 (PDB: 0x00471430)

    void SetUserData(void* data) { mUserData = data; }
    void* GetUserData() const { return mUserData; }
    NativePlayer* GetNativePlayer() const { return mNativePlayer; }
    AnimProcessor* GetAnimProcessor() { return mAnimProcessor; }

    float PopFloat();            // 0x0046f9e0 (PDB: 0x00471350)
    int PopInt();                // 0x0046f9f0 (PDB: 0x00471360)
    bool PopBool() { return mStack[mSP--].b; }
    void* PopObject() { return mStack[mSP--].p; }

    void PushFloat(float val);   // 0x0046fa00 (PDB: 0x00471370)
    void PushInt(int val);       // 0x0046fa20 (PDB: 0x00471390)
    void PushBool(bool val);     // 0x0046fa40 (PDB: 0x004713b0)
    void PushObject(void* val) { mStack[++mSP].p = val; }
    void PushNative(void* val) { mStack[++mSP].p = val; } // 0x0046fa20 (PDB: 0x00471390, COMDAT)

private:
    void PushMethodContext(AnimScriptObject* obj, const uint32_t* pc); // 0x0046f5f0 (PDB: 0x00470f60)
    void Execute();                                                    // 0x0046f650 (PDB: 0x00470fc0)

public:
    /* 0x00 */ AnimScriptObject* mSelf;
    /* 0x04 */ int mSP;
    /* 0x08 */ const uint32_t* mPC;
    /* 0x0c */ AnimDataValue* mStack;
    /* 0x10 */ AnimDataValue* mStackBase;
    /* 0x14 */ MethodContext* mMethodStack;
    /* 0x18 */ int mMethodStackDepth;
    /* 0x1c */ AnimProcessor* mAnimProcessor;
    /* 0x20 */ void* mUserData;
    /* 0x24 */ NativePlayer* mNativePlayer;
};
static_assert(sizeof(VirtualMachine::MethodContext) == 0x0c, "VirtualMachine::MethodContext size mismatch");
static_assert(sizeof(VirtualMachine) == 0x28, "VirtualMachine size mismatch");
static_assert(offsetof(VirtualMachine, mStackBase) == 0x10, "VirtualMachine::mStackBase offset mismatch");
static_assert(offsetof(VirtualMachine, mMethodStack) == 0x14, "VirtualMachine::mMethodStack offset mismatch");
static_assert(offsetof(VirtualMachine, mAnimProcessor) == 0x1c, "VirtualMachine::mAnimProcessor offset mismatch");
static_assert(offsetof(VirtualMachine, mNativePlayer) == 0x24, "VirtualMachine::mNativePlayer offset mismatch");

#endif // GAME_ANIM_ANIMPATTERNVM_H
