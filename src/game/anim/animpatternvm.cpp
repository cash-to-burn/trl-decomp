#include "game/anim/animpatternvm.h"

namespace {

enum AnimVmOpcode {
    OP_POP          = 0x01,
    OP_PUSH_CONST   = 0x02,
    OP_NEG_FLOAT    = 0x47,
    OP_ADD_FLOAT    = 0x48,
    OP_SUB_FLOAT    = 0x49,
    OP_MUL_FLOAT    = 0x4a,
    OP_DIV_FLOAT    = 0x4b,
    OP_CMP_LT       = 0x4c,
    OP_CMP_LE       = 0x4d,
    OP_CMP_EQ       = 0x4e,
    OP_NOT_BOOL     = 0x50,
    OP_AND_BOOL     = 0x51,
    OP_OR_BOOL      = 0x52,
    OP_FLOAT_TO_INT = 0x5a,
    OP_INT_TO_FLOAT = 0x5c,
    OP_JMP          = 0x63,
    OP_JZ_BOOL      = 0x64,
    OP_CALL_NATIVE  = 0x65,
    OP_RET          = 0x7b,
};

} // namespace

// 0x0046f570 (PDB: 0x00470ee0)
VirtualMachine::VirtualMachine(AnimProcessor* animProcessor)
    : mSelf(nullptr)
    , mSP(0)
    , mPC(nullptr)
    , mStack(new AnimDataValue[128])
    , mStackBase(mStack)
    , mMethodStack(new MethodContext[16])
    , mMethodStackDepth(0)
    , mAnimProcessor(animProcessor)
    , mUserData(nullptr)
    , mNativePlayer(new NativePlayer())
{
}

// 0x0046f5f0 (PDB: 0x00470f60)
void VirtualMachine::PushMethodContext(AnimScriptObject* obj, const uint32_t* pc)
{
    MethodContext& ctx = mMethodStack[mMethodStackDepth];
    ctx.mSelf = mSelf;
    ctx.mPC = mPC;
    ctx.mStack = mStack;
    mMethodStackDepth++;

    mSelf = obj;
    mStack += mSP;
    mSP = 0;
    mPC = pc;
}

// 0x0046f650 (PDB: 0x00470fc0)
void VirtualMachine::Execute()
{
    while (true) {
        const uint32_t insn = *mPC++;
        const uint32_t opcode = insn >> 24;

        switch (opcode) {
        case OP_POP:
            mSP--;
            break;

        case OP_PUSH_CONST:
            mStack[++mSP].u = *mPC++;
            break;

        case OP_NEG_FLOAT:
            mStack[mSP].f = -mStack[mSP].f;
            break;

        case OP_ADD_FLOAT: {
            const float rhs = mStack[mSP--].f;
            mStack[mSP].f += rhs;
            break;
        }

        case OP_SUB_FLOAT: {
            const float rhs = mStack[mSP--].f;
            mStack[mSP].f -= rhs;
            break;
        }

        case OP_MUL_FLOAT: {
            const float rhs = mStack[mSP--].f;
            mStack[mSP].f *= rhs;
            break;
        }

        case OP_DIV_FLOAT: {
            const float rhs = mStack[mSP--].f;
            if (rhs != 0.0f) {
                mStack[mSP].f /= rhs;
            } else if (mStack[mSP].f < 0.0f) {
                mStack[mSP].u = 0xff7fffffU;
            } else if (mStack[mSP].f > 0.0f) {
                mStack[mSP].u = 0x7f7fffffU;
            } else {
                mStack[mSP].f = 0.0f;
            }
            break;
        }

        case OP_CMP_LT: {
            const float rhs = mStack[mSP--].f;
            mStack[mSP].b = (mStack[mSP].f < rhs);
            break;
        }

        case OP_CMP_LE: {
            const float rhs = mStack[mSP--].f;
            mStack[mSP].b = (mStack[mSP].f <= rhs);
            break;
        }

        case OP_CMP_EQ: {
            const float rhs = mStack[mSP--].f;
            mStack[mSP].b = (mStack[mSP].f == rhs);
            break;
        }

        case OP_NOT_BOOL:
            mStack[mSP].b = !mStack[mSP].b;
            break;

        case OP_AND_BOOL: {
            const bool rhs = mStack[mSP--].b;
            mStack[mSP].b = (mStack[mSP].b && rhs);
            break;
        }

        case OP_OR_BOOL: {
            const bool rhs = mStack[mSP--].b;
            mStack[mSP].b = (mStack[mSP].b || rhs);
            break;
        }

        case OP_JMP: {
            const int32_t offset = static_cast<int32_t>(insn << 8) >> 8;
            mPC += offset;
            break;
        }

        case OP_JZ_BOOL: {
            const bool cond = mStack[mSP--].b;
            if (!cond) {
                const int32_t offset = static_cast<int32_t>(insn << 8) >> 8;
                mPC += offset;
            }
            break;
        }

        case OP_FLOAT_TO_INT:
            mStack[mSP].i = static_cast<int32_t>(mStack[mSP].f);
            break;

        case OP_INT_TO_FLOAT:
            mStack[mSP].f = static_cast<float>(mStack[mSP].i);
            break;

        case OP_CALL_NATIVE: {
            const uint32_t methodIdx = mStack[mSP--].u;
            const uint32_t classIdx = mStack[mSP--].u;
            gNativeClassTable[classIdx][methodIdx](this);
            break;
        }

        case OP_RET: {
            mMethodStackDepth--;
            const MethodContext& ctx = mMethodStack[mMethodStackDepth];
            mSelf = ctx.mSelf;
            mPC = ctx.mPC;
            mStack = ctx.mStack;
            if (mMethodStackDepth == 0) {
                return;
            }
            break;
        }

        default:
            break;
        }
    }
}

// 0x0046f9e0 (PDB: 0x00471350)
float VirtualMachine::PopFloat()
{
    return mStack[mSP--].f;
}

// 0x0046f9f0 (PDB: 0x00471360)
int VirtualMachine::PopInt()
{
    return mStack[mSP--].i;
}

// 0x0046fa00 (PDB: 0x00471370)
void VirtualMachine::PushFloat(float val)
{
    mStack[++mSP].f = val;
}

// 0x0046fa20 (PDB: 0x00471390)
void VirtualMachine::PushInt(int val)
{
    mStack[++mSP].i = val;
}

// 0x0046fa40 (PDB: 0x004713b0)
void VirtualMachine::PushBool(bool val)
{
    mStack[++mSP].b = val;
}

// 0x0046fa60 (PDB: 0x004713e0)
VirtualMachine::~VirtualMachine()
{
    delete[] mStackBase;
    mStackBase = nullptr;
    delete[] mMethodStack;
    mMethodStack = nullptr;
    delete mNativePlayer;
    mNativePlayer = nullptr;
}

// 0x0046fab0 (PDB: 0x00471430)
void VirtualMachine::CallVirtualMethod(AnimScriptObject* obj, int methodIdx)
{
    PushMethodContext(obj, obj->virtualMethodTable[methodIdx]);
    Execute();
}
