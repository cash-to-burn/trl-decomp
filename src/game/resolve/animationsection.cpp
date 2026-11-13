#include "game/resolve/animationsection.h"

#include "game/anim/anitracker.h"

#include <cstring>

namespace cdc {
struct HeapAllocator;
} // namespace cdc

namespace Resolve {
extern cdc::HeapAllocator* s_resolveHeap; // 0x0113880c (PDB seg 3:0x00229a6c)
} // namespace Resolve

// 0x005b97a0 (PDB: 0x005bc110)
unsigned int AnimationSection::StartResource(int id, unsigned short type, unsigned int size, bool& loaded) {
    (void)type;
    void* ptr = ANITRACKER_ResolveAnimation(id, static_cast<int>(size));
    loaded = (ptr == nullptr);
    return static_cast<unsigned int>(ANITRACKER_FindAnimation(id));
}

// 0x005b9800 (PDB: 0x005bc170)
void AnimationSection::ReleaseResource(unsigned int handle) {
    ANITRACKER_FreeAnimation(static_cast<int>(handle));
}

// 0x005b97d0 (PDB: 0x005bc140)
unsigned int AnimationSection::HandleResourceData(unsigned int handle, const char* pData, unsigned int dataSize, unsigned int offset) {
    uint8_t* dst = reinterpret_cast<uint8_t*>(aniTracker[handle].animationData) + offset;
    std::memcpy(dst, pData, dataSize);
    return dataSize;
}

// 0x004693a0 (PDB: 0x0055d060)
void AnimationSection::HandleResourceEnd(unsigned int handle, const SectionInfo& info) {
    (void)handle;
    (void)info;
}

// 0x005b9800 (PDB: 0x005bc170)
void AnimationSection::HandleResourceCancel(unsigned int handle) {
    ANITRACKER_FreeAnimation(static_cast<int>(handle));
}

// 0x005b9810 (PDB: 0x005bc180)
void* AnimationSection::GetBasePointer(unsigned int handle) {
    return aniTracker[handle].animationData;
}

// 0x005b9820 (PDB: 0x005bc190)
unsigned int AnimationSection::GetSize(unsigned int handle) {
    using PtrSizeFn = unsigned int(__thiscall*)(void*, const void*);
    void* const* vtable = nullptr;
    std::memcpy(&vtable, Resolve::s_resolveHeap, sizeof(vtable));
    PtrSizeFn ptrSize = reinterpret_cast<PtrSizeFn>(vtable[9]); // +0x24: cdc::HeapAllocator::PtrSize
    return ptrSize(Resolve::s_resolveHeap, aniTracker[handle].animationData);
}

// 0x005b9840 (PDB: 0x005bc1b0)
unsigned int AnimationSection::GetRefCount(unsigned int handle) {
    return static_cast<unsigned int>(static_cast<int>(aniTracker[handle].refCount));
}

// 0x005b9850 (PDB: 0x005bc1c0)
unsigned int AnimationSection::FindResourceA(int id) {
    return static_cast<unsigned int>(ANITRACKER_FindAnimation(id));
}
