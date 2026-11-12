#pragma once

#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x0045cc10 (PDB: 0x0045e3a0 / ?MainG2@@YAHPAX@Z)
// The primary G2 engine game loop and top-level state machine.
int MainG2(void *appData);

// 0x0045c940 (PDB: 0x0045e0d0 / ?MAIN_DoMainInit@@YAXXZ)
// Initializes core game assets, general sound bank, shaders, and UI.
void MAIN_DoMainInit(void);

// 0x0045c530 (PDB: 0x0045dc90 / ?ProcessArgs@@YAHPADPAUGameTracker@@PAX@Z)
// Parses command line flags and \\TR7\\game\\pc\\TR7.arg arguments.
int ProcessArgs(char *argData, void *gameTracker, void *appData);

#ifdef __cplusplus
}
#endif

#endif // GAME_MAIN_H
