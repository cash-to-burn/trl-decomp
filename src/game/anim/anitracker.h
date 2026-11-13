#pragma once

#ifndef GAME_ANIM_ANITRACKER_H
#define GAME_ANIM_ANITRACKER_H

#include "common.h"

struct AnimFragment;

// 0x5be0 in PDB / size: 0x04
struct AnimListEntry {
    int16_t animationID;     // 0x00
    int16_t animationNumber; // 0x02
};
static_assert(sizeof(AnimListEntry) == 4, "AnimListEntry size must be 4");

// 0x6d68 in PDB / size: 0x08
struct _AniTracker {
    int16_t refCount;            // 0x00
    int16_t animID;              // 0x02
    AnimFragment* animationData; // 0x04
};
static_assert(sizeof(_AniTracker) == 8, "_AniTracker size must be 8");

constexpr int MAX_LOADED_ANIMATIONS = 1024; // 0x400

extern int gFirstEmptyAnimation;                     // 0x010fa350 (PDB seg 3:0x001eb150)
extern int gNumLoadedAnimations;                     // 0x010fa354 (PDB seg 3:0x001eb154)
extern _AniTracker aniTracker[MAX_LOADED_ANIMATIONS]; // 0x010fa358 (PDB seg 3:0x001eb158)

void ANITRACKER_FreeAnimation(int animationHandle);          // 0x00477ab0 (PDB: 0x004793f0)
int ANITRACKER_FindAnimation(int animID);                    // 0x00477af0 (PDB: 0x00479430)
void ANITRACKER_TranslateAnimationList(AnimListEntry* anim); // 0x00477b30 (PDB: 0x00479470)
void ANITRACKER_Init(void);                                  // 0x00477b80 (PDB: 0x004794c0)
void* ANITRACKER_ResolveAnimation(int id, int size);         // 0x00477ba0 (PDB: 0x004794e0)

#endif // GAME_ANIM_ANITRACKER_H
