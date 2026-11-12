#include "game/sound/sound.h"
#include "game/archive/ArchiveFileSystem.h"
#include "game/pc/pclocals.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static const int MAX_SOUND_BANKS = 32;
static SoundBankEntry s_soundBanks[MAX_SOUND_BANKS] = {};
static int s_musicVolume = 100;
static int s_sfxVolume = 100;
static int s_speechVolume = 100;
static bool s_isUnderWater = false;
static bool s_isPaused = false;

// 0x00462220 (PDB: 0x00463be0)
int OBTABLE_GetObjectID(const char *name)
{
    if (name == nullptr) {
        return 0;
    }

    // Common standard asset bank IDs
    if (_stricmp(name, "generalbank") == 0) {
        return 1;
    }
    if (_stricmp(name, "globalsoundinfo") == 0) {
        return 2;
    }
    if (_stricmp(name, "shaders") == 0) {
        return 3;
    }

    // Simple hash-based fallback ID for game-specific sound banks
    uint32_t hash = 0;
    for (const char *p = name; *p != '\0'; p++) {
        hash = (hash * 31) + (unsigned char)(*p);
    }
    return (int)((hash & 0x7fff) + 10);
}

// 0x005d4b00 (PDB: 0x005cb910)
void SOUND_Init(void)
{
    for (int i = 0; i < MAX_SOUND_BANKS; i++) {
        s_soundBanks[i].bankID = 0;
        s_soundBanks[i].pData = nullptr;
        s_soundBanks[i].dataSize = 0;
        s_soundBanks[i].refCount = 0;
        s_soundBanks[i].name[0] = '\0';
    }
    s_isUnderWater = false;
    s_isPaused = false;
}

// 0x005cc070 in PDB
void SOUND_Free(void)
{
    for (int i = 0; i < MAX_SOUND_BANKS; i++) {
        if (s_soundBanks[i].pData != nullptr) {
            free(s_soundBanks[i].pData);
            s_soundBanks[i].pData = nullptr;
        }
        s_soundBanks[i].bankID = 0;
        s_soundBanks[i].refCount = 0;
    }
}

void SOUND_SetMusicVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    s_musicVolume = volume;
}

int SOUND_GetMusicVolume(void)
{
    return s_musicVolume;
}

void SOUND_SetSfxVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    s_sfxVolume = volume;
}

int SOUND_GetSfxVolume(void)
{
    return s_sfxVolume;
}

void SOUND_SetSpeechVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    s_speechVolume = volume;
}

int SOUND_GetSpeechVolume(void)
{
    return s_speechVolume;
}

void SOUND_PauseAllSound(void)
{
    s_isPaused = true;
}

void SOUND_ResumeAllSound(void)
{
    s_isPaused = false;
}

void SOUND_StopAllSound(void)
{
}

void SOUND_UpdateSound(float deltaTime)
{
    (void)deltaTime;
}

void SOUND_SetUnderWater(void)
{
    s_isUnderWater = true;
}

void SOUND_ResetUnderWater(void)
{
    s_isUnderWater = false;
}

// 0x005d3e00
int SOUND_LoadBank(const char *bankName, int param2)
{
    (void)param2;
    if (bankName == nullptr) {
        return 0;
    }

    int id = OBTABLE_GetObjectID(bankName);
    if (id == 0) {
        return 0;
    }

    // Check if bank is already loaded
    for (int i = 0; i < MAX_SOUND_BANKS; i++) {
        if (s_soundBanks[i].bankID == id) {
            s_soundBanks[i].refCount++;
            return (int)&s_soundBanks[i];
        }
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_SOUND_BANKS; i++) {
        if (s_soundBanks[i].bankID == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        return 0;
    }

    s_soundBanks[slot].bankID = id;
    s_soundBanks[slot].refCount = 1;
    strncpy(s_soundBanks[slot].name, bankName, sizeof(s_soundBanks[slot].name) - 1);

    char path[256];
    snprintf(path, sizeof(path), "%s%s.drm", g_buildDir, bankName);

    if (g_pArchiveFS != nullptr && g_pArchiveFS->FileExists(path)) {
        uint32_t sz = g_pArchiveFS->GetFileSize(path);
        if (sz > 0) {
            s_soundBanks[slot].pData = malloc(sz);
            s_soundBanks[slot].dataSize = sz;
        }
    }

    return (int)&s_soundBanks[slot];
}
