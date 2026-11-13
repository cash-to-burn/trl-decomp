#include "game/anim/anitracker.h"
#include <cstring>

namespace cdc {
struct HeapAllocator {
    void* Alloc(int size, int align); // vtable +0x34 (cdc::Allocator::Alloc)
    void Free(void* ptr);             // vtable +0x38 (cdc::Allocator::Free)
};
} // namespace cdc

namespace Resolve {
extern cdc::HeapAllocator* s_resolveHeap; // 0x0113880c (PDB seg 3:0x00229a6c)
} // namespace Resolve

int gFirstEmptyAnimation = 0;                          // 0x010fa350 (PDB seg 3:0x001eb150)
int gNumLoadedAnimations = 0;                          // 0x010fa354 (PDB seg 3:0x001eb154)
_AniTracker aniTracker[MAX_LOADED_ANIMATIONS] = {};    // 0x010fa358 (PDB seg 3:0x001eb158)

// 0x00477a50 (PDB: 0x00479390)
static int ANITRACKER_AllocateAnimation(void* animation, int id)
{
    int slot = gFirstEmptyAnimation;
    while (slot < MAX_LOADED_ANIMATIONS && aniTracker[slot].refCount != 0) {
        ++slot;
    }

    gFirstEmptyAnimation = slot + 1;
    aniTracker[slot].animID = static_cast<int16_t>(id);
    aniTracker[slot].refCount = 1;
    aniTracker[slot].animationData = static_cast<AnimFragment*>(animation);
    ++gNumLoadedAnimations;

    return slot;
}

// 0x00477ab0 (PDB: 0x004793f0)
void ANITRACKER_FreeAnimation(int animationHandle)
{
    --aniTracker[animationHandle].refCount;
    if (aniTracker[animationHandle].refCount == 0) {
        Resolve::s_resolveHeap->Free(aniTracker[animationHandle].animationData);
        if (gFirstEmptyAnimation > animationHandle) {
            gFirstEmptyAnimation = animationHandle;
        }
    }
}

// 0x00477af0 (PDB: 0x00479430)
int ANITRACKER_FindAnimation(int animID)
{
    for (int i = 0; i < MAX_LOADED_ANIMATIONS; ++i) {
        if (aniTracker[i].refCount != 0 && aniTracker[i].animID == animID) {
            return i;
        }
    }
    return -1;
}

// 0x00477b30 (PDB: 0x00479470)
void ANITRACKER_TranslateAnimationList(AnimListEntry* anim)
{
    while (anim->animationID != -1) {
        int foundIndex = -1;
        for (int i = 0; i < MAX_LOADED_ANIMATIONS; ++i) {
            if (aniTracker[i].refCount != 0 && aniTracker[i].animID == anim->animationID) {
                foundIndex = i;
                break;
            }
        }
        anim->animationNumber = static_cast<int16_t>(foundIndex);
        ++anim;
    }
}

// 0x00477b80 (PDB: 0x004794c0)
void ANITRACKER_Init(void)
{
    std::memset(aniTracker, 0, sizeof(aniTracker));
    gNumLoadedAnimations = 0;
    gFirstEmptyAnimation = 0;
}

// 0x00477ba0 (PDB: 0x004794e0)
void* ANITRACKER_ResolveAnimation(int id, int size)
{
    int index = ANITRACKER_FindAnimation(id);
    if (index == -1) {
        void* allocation = Resolve::s_resolveHeap->Alloc(size, 0);
        ANITRACKER_AllocateAnimation(allocation, id);
        return allocation;
    }

    ++aniTracker[index].refCount;
    return nullptr;
}
