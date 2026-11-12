#pragma once

#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include "common.h"
#include "cdc/cdcFile/FileSystem.h"

// Forward declaration of GameTracker
struct GameTracker {
    uint32_t flags;             // +0x14: 0x800000 = NOHINTS
    int gameMode;               // current mode (1=attract, 2=running, 3=mainmenu, 4=pause, 6=chapter)
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
