#pragma once

#ifndef GAME_RESOLVE_ANIMATIONSECTION_H
#define GAME_RESOLVE_ANIMATIONSECTION_H

#include "game/resolve/resolvesection.h"

// TI 0x85c4 / size: 0x04 / vtable: 0x00f0614c (??_7AnimationSection@@6B@)
class AnimationSection : public ResolveSection {
public:
    unsigned int StartResource(int id, unsigned short type, unsigned int size, bool& loaded) override; // 0x005b97a0 (PDB: 0x005bc110)
    void ReleaseResource(unsigned int handle) override;                                               // 0x005b9800 (PDB: 0x005bc170)
    unsigned int HandleResourceData(unsigned int handle, const char* pData, unsigned int dataSize, unsigned int offset) override; // 0x005b97d0 (PDB: 0x005bc140)
    void HandleResourceEnd(unsigned int handle, const SectionInfo& info) override;                    // 0x004693a0 (PDB: 0x0055d060)
    void HandleResourceCancel(unsigned int handle) override;                                          // 0x005b9800 (PDB: 0x005bc170)
    void* GetBasePointer(unsigned int handle) override;                                               // 0x005b9810 (PDB: 0x005bc180)
    unsigned int GetSize(unsigned int handle) override;                                               // 0x005b9820 (PDB: 0x005bc190)
    unsigned int GetRefCount(unsigned int handle) override;                                           // 0x005b9840 (PDB: 0x005bc1b0)
    unsigned int FindResourceA(int id) override;                                                      // 0x005b9850 (PDB: 0x005bc1c0)
};
static_assert(sizeof(AnimationSection) == 0x04, "AnimationSection size must be 0x04");

#endif // GAME_RESOLVE_ANIMATIONSECTION_H
