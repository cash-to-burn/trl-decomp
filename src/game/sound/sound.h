#pragma once

#ifndef GAME_SOUND_SOUND_H
#define GAME_SOUND_SOUND_H

#include "common.h"

struct SoundBankEntry {
    int bankID;
    void *pData;
    uint32_t dataSize;
    int refCount;
    char name[32];
};

#ifdef __cplusplus
extern "C" {
#endif

// 0x005d4b00 (PDB: 0x005cb910 / ?SOUND_Init@@YAXXZ)
void SOUND_Init(void);

// 0x005cc070 in PDB
void SOUND_Free(void);

// Volume controls (0 - 100)
void SOUND_SetMusicVolume(int volume);
int SOUND_GetMusicVolume(void);
void SOUND_SetSfxVolume(int volume);
int SOUND_GetSfxVolume(void);
void SOUND_SetSpeechVolume(int volume);
int SOUND_GetSpeechVolume(void);

// Playback state controls
void SOUND_PauseAllSound(void);
void SOUND_ResumeAllSound(void);
void SOUND_StopAllSound(void);
void SOUND_UpdateSound(float deltaTime);

// Environmental audio modifiers
void SOUND_SetUnderWater(void);
void SOUND_ResetUnderWater(void);

// 0x005d3e00 - Asset bank loader
int SOUND_LoadBank(const char *bankName, int param2);

// 0x00462220 (PDB: 0x00463be0 / ?OBTABLE_GetObjectID@@YAHPAD@Z)
int OBTABLE_GetObjectID(const char *name);

#ifdef __cplusplus
}
#endif

#endif // GAME_SOUND_SOUND_H
