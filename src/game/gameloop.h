#pragma once

#ifndef GAME_GAMELOOP_H
#define GAME_GAMELOOP_H

#include "common.h"
#include "game/main.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x004536e0 (PDB: 0x004536e0 / ?GAMELOOP_MainLoop@@YAXPAUGameTracker@@@Z)
void GAMELOOP_MainLoop(void *gameTracker);

// 0x00450a50 (PDB: 0x00450a50 / ?GAMELOOP_SystemInit@@YAXPAUGameTracker@@@Z)
void GAMELOOP_SystemInit(void *gameTracker);

#ifdef __cplusplus
}
#endif

#endif // GAME_GAMELOOP_H
