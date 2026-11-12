#pragma once

#ifndef GAME_GAMELOOP_H
#define GAME_GAMELOOP_H

#include "common.h"
#include "game/main.h"
#include "game/Stream/stream.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x0044f5d0 (PDB: 0x00450950)
void GAMELOOP_AllocStaticMemory(void);

// 0x0044f630 (PDB: 0x004509b0)
void GAMELOOP_InitGameTracker(void);

// 0x0044f6d0 (PDB: 0x00450a50 / ?GAMELOOP_SystemInit@@YAXPAUGameTracker@@@Z)
void GAMELOOP_SystemInit(GameTracker *gameTracker);

// 0x0044f710 (PDB: 0x00450a90 / ?GAMELOOP_ResetGameStates@@YAXXZ)
void GAMELOOP_ResetGameStates(void);

// 0x0044f740 (PDB: 0x00450ac0 / ?GAMELOOP_ClearGameTracker@@YAXXZ)
void GAMELOOP_ClearGameTracker(void);

// 0x0044f7e0 (PDB: 0x00450b60)
void GAMELOOP_IntroduceInstances(GameTracker *gameTracker);

// 0x0044f820 (PDB: 0x00450ba0)
void GAMELOOP_LoadingDisplayProcess(void *instance, GameTracker *gameTracker);

// 0x0044f830 (PDB: 0x00450bb0)
void GAMELOOP_InitRunning(char *baseAreaName, char *playerName);

// 0x0044fa30 (PDB: 0x00450db0)
void GAMELOOP_CloseRunning(void);

// 0x0044faa0 (PDB: 0x00450e30)
void GAMELOOP_DisplayRunning(StreamUnit *unit);

// 0x0044fba0 (PDB: 0x00450f30 / ?GAMELOOP_InitStaticDefault@@YAXPAD0@Z)
void GAMELOOP_InitStaticDefault(char *baseAreaname, char *playerName);

// 0x0044fc50 (PDB: 0x00450fe0 / ?GAMELOOP_InitStaticChapter@@YAXPAD0@Z)
void GAMELOOP_InitStaticChapter(char *baseAreaname, char *playerName, int chapterIndex);

// 0x0044fd40 (PDB: 0x004510d0 / ?GAMELOOP_InitStaticDeath@@YAXPAD0@Z)
void GAMELOOP_InitStaticDeath(char *baseAreaname, char *playerName);

// 0x0044fd90 (PDB: 0x00451120 / ?GAMELOOP_DisplayStatic@@YAXXZ)
void GAMELOOP_DisplayStatic(void);

// 0x0044fdc0 (PDB: 0x00451150 / ?GAMELOOP_CloseStatic@@YAXXZ)
void GAMELOOP_CloseStatic(void);

// 0x0044fd90 (PDB: 0x00451120 / ?GAMELOOP_DisplayStaticChapter@@YAXXZ)
void GAMELOOP_DisplayStaticChapter(void);

// 0x0044fde0 (PDB: 0x00451170 / ?GAMELOOP_SetLoadingDisplay@@YAXH@Z)
void GAMELOOP_SetLoadingDisplay(int display);

// 0x0044fdf0 (PDB: 0x00451180 / ?GAMELOOP_InitStandardObjects@@YAXXZ)
void GAMELOOP_InitStandardObjects(void);

// 0x0044fe10 (PDB: 0x004511a0 / ?GAMELOOP_IntroducePlayerInstance@@YAPAUInstance@@HMMMH@Z)
void *GAMELOOP_IntroducePlayerInstance(int unitID, int playerObjectID, void *intro);

// 0x0044ff60 (PDB: 0x004512e0 / ?GAMELOOP_ResetAllScreenWipes@@YAXXZ)
void GAMELOOP_ResetAllScreenWipes(void);

// 0x0044ff80 (PDB: 0x00451300 / ?GAMELOOP_IsWipeDone@@YAHH@Z)
int GAMELOOP_IsWipeDone(int type);

// 0x0044ffb0 (PDB: 0x00451330 / ?GAMELOOP_IsWipeStartingOrON@@YAHH@Z)
int GAMELOOP_IsWipeStartingOrON(int type);

// 0x00450010 (PDB: 0x00451390 / ?GAMELOOP_AreAnyScreenFadesOnOrStarting@@YAHXZ)
int GAMELOOP_AreAnyScreenFadesOnOrStarting(void);

// 0x00450040 (PDB: 0x004513c0 / ?GAMELOOP_SetScreenWipe@@YAXHHH@Z)
void GAMELOOP_SetScreenWipe(int type, int target, int time);

// 0x004500a0 (PDB: 0x00451420 / ?GAMELOOP_HandleScreenWipes@@YAXXZ)
void GAMELOOP_HandleScreenWipes(void);

// 0x004506f0 (PDB: 0x00451a70 / ?GAMELOOP_GetMainRenderUnit@@YAPAUStreamUnit@@XZ)
StreamUnit *GAMELOOP_GetMainRenderUnit(void);

// 0x00450770 (PDB: 0x00451af0 / ?GAMELOOP_DisplayGameElements@@YA_NPAUGameTracker@@@Z)
bool GAMELOOP_DisplayGameElements(GameTracker *gameTracker);

// 0x00450a30 (PDB: 0x00451db0 / ?GAMELOOP_DisplayFrame@@YAXPAUGameTracker@@@Z)
void GAMELOOP_DisplayFrame(GameTracker *gameTracker);

// 0x00450a90 (PDB: 0x00451e10)
float TimeWarpRamp(float current, float target, float rate);

// 0x00450ad0 (PDB: 0x00451e50 / ?GAMELOOP_DoTimeProcess@@YAXXZ)
void GAMELOOP_DoTimeProcess(void);

// 0x004511c0 (PDB: 0x00452540)
void GAMELOOP_ModeStartRunning(int wasSet);

// 0x00451270 (PDB: 0x004525f0 / ?GAMELOOP_ModeStartPause@@YAX_N@Z)
void GAMELOOP_ModeStartPause(bool systemPause);

// 0x00451360 (PDB: 0x004526e0)
void GAMELOOP_ChangeMode(int newMode);

// 0x00451680 (PDB: 0x00452a00 / ?GAMELOOP_ExitGame@@YAXH@Z)
void GAMELOOP_ExitGame(int doneType);

// 0x00451780 (PDB: 0x00452b00 / ?GAMELOOP_RequestLevelChangeByName@@YAXPADPAUGameTracker@@H@Z)
void GAMELOOP_RequestLevelChangeByName(char *name, GameTracker *gameTracker, int doneType);

// 0x004517e0 (PDB: 0x00452b60 / ?GAMELOOP_RequestLevelChange@@YAXPADHPAUGameTracker@@@Z)
void GAMELOOP_RequestLevelChange(char *levelName, int number, GameTracker *gameTracker);

// 0x00451820 (PDB: 0x00452ba0 / ?GAMELOOP_LoadingDisplay@@YAXXZ)
void GAMELOOP_LoadingDisplay(void);

// 0x00451910 (PDB: 0x00452c90 / ?GAMELOOP_LevelLoadAndInit@@YAHPADPAUGameTracker@@@Z)
int GAMELOOP_LevelLoadAndInit(char *baseAreaName, GameTracker *gameTracker);

// 0x00451f90 (PDB: 0x00453310 / ?GAMELOOP_Process@@YAXPAUGameTracker@@@Z)
void GAMELOOP_Process(GameTracker *gameTracker);

// 0x00452360 (PDB: 0x004536e0 / ?GAMELOOP_MainLoop@@YAXPAUGameTracker@@@Z)
void GAMELOOP_MainLoop(GameTracker *gameTracker);

#ifdef __cplusplus
}
#endif

#endif // GAME_GAMELOOP_H
