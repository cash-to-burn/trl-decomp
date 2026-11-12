#pragma once

#ifndef GAME_PC_MAINVM_H
#define GAME_PC_MAINVM_H

#include "common.h"
#include "cdc/cdcSys/UTF8String.h"

extern HANDLE hMainThread;          // 0x00f2c3a4
extern DWORD iMainThreadId;         // 0x00f2c3a0
extern char g_mutexName[32];        // 0x00f10424 ("TRLMUTEX")
extern int quitdone;

#ifdef __cplusplus
extern "C" {
#endif

// 0x00401c60 - Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

// 0x00401a10 - Callback for EnumWindows to bring existing instance to foreground
BOOL CALLBACK FindGameEnumWindowsProc(HWND hwnd, LPARAM lParam);

// 0x00401b50 - Force terminate the game and clean up resources
void TerminateGame(void);

// 0x00401ab0 - Engine frame update tick
bool MainG2_UpdateLoop(void);

#ifdef __cplusplus
}
#endif

// 0x00401950 - Displays a wide-character message box from UTF-8 strings
void MessageBoxUTF8(HWND hWnd, const cdc::UTF8String &msg, const cdc::UTF8String &caption, UINT uType);

#endif // GAME_PC_MAINVM_H
