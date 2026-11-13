#pragma once

#ifndef GAME_RESOLVE_RESOLVESECTION_H
#define GAME_RESOLVE_RESOLVESECTION_H

#include "common.h"

// TI 0x6f83
enum SectionType {
    SECTION_GENERAL = 0,
    SECTION_EMPTY = 1,
    SECTION_ANIMATION = 2,
    SECTION_PUSHBUFFER_WC = 3,
    SECTION_PUSHBUFFER = 4,
    SECTION_TEXTURE = 5,
    SECTION_WAVE = 6,
    SECTION_DTPDATA = 7,
    SECTION_SCRIPT = 8,
    SECTION_SHADERLIB = 9,
    NUM_SECTION_TYPES = 10
};

// TI 0x7f14 / size: 0x14
struct SectionInfo {
    int size;           // 0x00
    uint8_t type;       // 0x04
    uint8_t flags;      // 0x05 (bit 0: skip, bit 1: __free)
    uint16_t versionID; // 0x06
    uint8_t pad08[0x0c];
};
static_assert(sizeof(SectionInfo) == 0x14, "SectionInfo size must be 0x14");

// TI 0x7f3e / size: 0x04
class ResolveSection {
public:
    virtual unsigned int StartResource(int id, unsigned short type, unsigned int size, bool& loaded) = 0; // +0x00
    virtual void UpdateResourceA(unsigned int handle, int, unsigned int) { (void)handle; } // +0x04: 0x00613780 (PDB: 0x00450390)
    virtual void ReleaseResource(unsigned int handle) = 0;                  // +0x08
    virtual unsigned int HandleResourceData(unsigned int handle, const char* pData, unsigned int dataSize, unsigned int offset) = 0; // +0x0c
    virtual void HandleResourceEnd(unsigned int handle, const SectionInfo& info) = 0; // +0x10
    virtual void HandleResourceCancel(unsigned int handle) = 0;             // +0x14
    virtual void HandleResourceReady(unsigned int handle, const SectionInfo& info) { (void)handle; (void)info; } // +0x18: 0x004693a0 (PDB: 0x0055d060)
    virtual bool CanUpdate() { return false; }                              // +0x1c: 0x0042ef30 (PDB: 0x00450380)
    virtual void* GetBasePointer(unsigned int handle) = 0;                  // +0x20
    virtual unsigned int GetSize(unsigned int handle) = 0;                  // +0x24
    virtual unsigned int GetRefCount(unsigned int handle) = 0;              // +0x28
    virtual unsigned int FindResourceA(int id) = 0;                         // +0x2c

    static void Init();                                                     // 0x005bd4c0
    static void Register(SectionType type, ResolveSection* pSection);
    static ResolveSection* Get(SectionType type);

    static ResolveSection* s_pSection[NUM_SECTION_TYPES]; // PDB seg 3:0x00229a70
};
static_assert(sizeof(ResolveSection) == 0x04, "ResolveSection size must be 0x04");

#endif // GAME_RESOLVE_RESOLVESECTION_H
