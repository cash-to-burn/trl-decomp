#pragma once

#ifndef GAME_ANIM_ANIM_H
#define GAME_ANIM_ANIM_H

#include "cdc/runtime/cdcMath/Math.h"
#include <cstddef>
#include <cstdint>

struct G2AnimAlphaTable;

// Size: 0x40 (64 bytes, TI: 0x59e9)
struct G2AnimSegValue {
    /* 0x000 */ cdc::Vector3 rot;
    /* 0x010 */ cdc::Vector3 scale;
    /* 0x020 */ cdc::Vector3 trans;
    /* 0x030 */ cdc::Quat quat;
};
static_assert(sizeof(G2AnimSegValue) == 0x40, "G2AnimSegValue size mismatch");

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
    /* 0x020 */ uint8_t mData[0x50];
};
static_assert(sizeof(AnimController) == 0x70, "AnimController size mismatch");
static_assert(offsetof(AnimController, mFlags) == 0x08, "AnimController::mFlags offset mismatch");
static_assert(offsetof(AnimController, mData) == 0x20, "AnimController::mData offset mismatch");

typedef unsigned int (*AnimControllerCallback)(
    AnimController* ac,
    G2AnimSegValue* segValue,
    cdc::Matrix* parentMatrix,
    cdc::Matrix* segMatrix,
    void* voiddata);

// Size: 0x5f0 (1520 bytes, TI: 0x5ca1)
struct AnimProcessor {
    /* 0x000 */ uint8_t pad000[0x5f0];

    void EnableController(int seg, int type);                                                // 0x00470c30 (PDB: 0x004725b0)
    void DisableController(int seg, int type);                                               // 0x00470dd0 (PDB: 0x00472750)
    bool IsControllerActive(int seg, int type);                                              // 0x00470e10 (PDB: 0x00472790)
    void SetController_Function(int seg, int type, AnimControllerCallback func, void* data); // 0x00474ad0 (PDB: 0x00476450)
};
static_assert(sizeof(AnimProcessor) == 0x5f0, "AnimProcessor size mismatch");

// Size: 0x14 (20 bytes, TI: 0x6677)
struct AnimComponent {
    /* 0x000 */ void* vtable;
    /* 0x004 */ AnimProcessor* animProcessor;
    /* 0x008 */ uint8_t pad008[0x0c];
};
static_assert(sizeof(AnimComponent) == 0x14, "AnimComponent size mismatch");
static_assert(offsetof(AnimComponent, animProcessor) == 0x04, "AnimComponent::animProcessor offset mismatch");

#endif // GAME_ANIM_ANIM_H
