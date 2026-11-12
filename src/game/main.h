#pragma once

#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include "common.h"
#include "cdc/cdcFile/FileSystem.h"

struct Level;
struct StreamUnit;

struct ScreenWipe {
    float current;
    float target;
    float rate;
};

// Extracted from tr7.pdb LF_STRUCTURE "GameTracker" (size 0x4b8 / 1208 bytes)
struct GameTracker {
    void *menu;                         // 0x000
    void *memcard;                      // 0x004
    Level *level;                       // 0x008
    void *playerInstance;               // 0x00c
    void *vertexPool;                   // 0x010
    uint32_t flags;                     // 0x014: debugFlags (0x800000 = NOHINTS)
    uint32_t debugFlags2;               // 0x018
    uint32_t debugFlags3;               // 0x01c
    uint32_t debugFlags4;               // 0x020
    ScreenWipe wipe[12];                // 0x024: screen wipe slots (144 bytes)
    uint32_t displayFrameCount;         // 0x0b4
    uint32_t frameCount;                // 0x0b8: main game simulation frame count
    int fpsFast;                        // 0x0bc
    int gameFlags;                      // 0x0c0
    int streamFlags;                    // 0x0c4
    StreamUnit *mainDrawUnit;           // 0x0c8
    char baseAreaName[20];              // 0x0cc: current active level area name
    uint8_t levelDone;                  // 0x0e0
    uint8_t levelChange;                // 0x0e1
    uint8_t loadingDisplayType;         // 0x0e2
    uint8_t gameMode;                   // 0x0e3 (1=attract, 2=running, 3=mainmenu, 4=pause, 6=chapter)
    uint8_t cheatMode;                  // 0x0e4
    uint8_t savingGame;                 // 0x0e5
    int16_t postSaveScreenID;           // 0x0e6
    int StreamUnitID;                   // 0x0e8
    uint8_t block_collide_override;     // 0x0ec
    uint8_t raziel_collide_override;    // 0x0ed
    uint8_t sound_collide_override;     // 0x0ee
    uint8_t attackwave_collide_override;// 0x0ef
    int decoupleGame;                   // 0x0f0
    uint32_t drawTime;                  // 0x0f4
    uint32_t currentTime;               // 0x0f8
    uint32_t currentMaterialTime;       // 0x0fc
    uint32_t lastLoopTime;              // 0x100
    float oldtimeMult;                  // 0x104
    int intoldtimeMult;                 // 0x108
    uint32_t currentTicks;              // 0x10c
    uint32_t totalTime;                 // 0x110
    uint32_t idleTime;                  // 0x114
    uint32_t CPUidleTime;               // 0x118
    int visibleInstances;               // 0x11c
    int gameFramePassed;                // 0x120
    int timeSinceLastGameFrame;         // 0x124
    float timeMult;                     // 0x128
    float unmodifiedActualTimeMult;     // 0x12c
    float actualTimeMult;               // 0x130
    float globalTimeMult;               // 0x134
    float scrollProgress;               // 0x138
    float timeDilation;                 // 0x13c
    int debugTimeMult;                  // 0x140
    float timeWarpDilation;             // 0x144
    float timeWarpDuration;             // 0x148
    float timeWarpRampTime;             // 0x14c
    float timeWarpRemainingTime;        // 0x150
    int frameRateLock;                  // 0x154
    uint8_t monster_collide_override;   // 0x158
    uint8_t monster_land_collide_override; // 0x159
    uint8_t physics_line_draw_override; // 0x15a
    uint8_t inSuperActionCine;          // 0x15b
    int noMonstersCount;                // 0x15c
    int nextGameMode;
    int birthStreamUnitID;
    int attachedID;
    void *face;

    void ResetOverrides();
    void ChangeMode(int mode);
};

// Global engine flags and options
extern int g_noHealth;
extern int g_mainMenu;
extern int g_startChapter;
extern int g_chapterVars;
extern int g_easyCheat;
extern int g_noVibration;
extern int g_noTrace;
extern char g_fontName[64];
extern char g_baseAreaName[64];
extern GameTracker g_gameTracker;

#ifdef __cplusplus
extern "C" {
#endif

// 0x0045c450 (PDB: 0x0045dba0 / ?FSHelper_ReadFile@@YAPAHPADEPAVFileSystem@cdc@@@Z)
// Reads an entire file into memory using the provided cdc::FileSystem instance.
void *FSHelper_ReadFile(const char *filename, unsigned char memTag, cdc::FileSystem *fs);

// 0x0045bb80 - Case-insensitive flag search
const char *Arg_FindFlag(const char *flag, const char *argStr);

// 0x0045bc40 - Argument parameter extractor
void Arg_GetValue(const char *flag, const char *argStr, char *outVal);

// 0x0045bbd0 - Identifier prefix extractor
void Arg_GetPrefix(const char *str, char *outPrefix);

// 0x0045bc00 - Digit suffix extractor
void Arg_GetDigits(const char *str, char *outDigits);

// 0x0045c530 (PDB: 0x0045dc90 / ?ProcessArgs@@YAHPADPAUGameTracker@@PAX@Z)
// Parses command line flags and \\TR7\\game\\pc\\TR7.arg arguments.
int ProcessArgs(char *argData, void *gameTracker, void *appData);

// 0x0045c940 (PDB: 0x0045e0d0 / ?MAIN_DoMainInit@@YAXXZ)
// Initializes core game assets, general sound bank, shaders, and UI.
void MAIN_DoMainInit(void);

// 0x0045cc10 (PDB: 0x0045e3a0 / ?MainG2@@YAHPAX@Z)
// The primary G2 engine game loop and top-level state machine.
int MainG2(void *appData);

#ifdef __cplusplus
}
#endif

#endif // GAME_MAIN_H
