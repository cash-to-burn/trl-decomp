#pragma once

#ifndef GAME_PC_GAMEWINDOW_H
#define GAME_PC_GAMEWINDOW_H

#include "common.h"

extern HWND g_hWnd;

#ifdef __cplusplus
extern "C" {
#endif

// 0x00403fd0 - Creates game window and registers "TOMBRAIDER" window class
void GAMEWINDOW_Init(void);

// 0x004040c0 - Destroys game window
void GAMEWINDOW_Deinit(void);

// 0x004040e0 - Message loop polling; returns 1 on WM_QUIT, 0 otherwise
int GAMEWINDOW_Poll(void);

// 0x00403c00 - Main game window procedure
LRESULT CALLBACK RegularWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 0x00404a40 - Reads HKLM/HKCU registry settings
int GAMEWINDOW_ReadSettings(void);

// 0x00404e00 - Writes registry settings back
void GAMEWINDOW_SaveSettings(void);

// 0x004044c0 - Reads user input/preferences
void GAMEWINDOW_ReadUserPreferences(void);

#ifdef __cplusplus
}
#endif

#endif // GAME_PC_GAMEWINDOW_H
