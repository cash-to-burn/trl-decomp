#pragma once

#ifndef GAME_PC_SETUP_H
#define GAME_PC_SETUP_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Registry configuration values
extern int g_languageID;           // 0x00f11ca0 (0 = English, 1 = French, 2 = German, 3 = Italian, 4 = Spanish, 7 = Polish, 9 = Russian)
extern int g_subtitleLanguageID;   // 0x00f11ca4
extern int g_fullInstall;          // 0x00f11d44
extern int g_mouseSensitivity;     // 0x00f11c8c
extern int g_eaxSupport;           // 0x00f11c84
extern int g_useReverb;            // 0x00f11c88
extern int g_disableForceFeedback; // 0x00f11c78
extern int g_dontPauseOnFocusLoss; // 0x00f11c7c
extern int g_voVolume;             // 0x00f11c98
extern int g_musicVolume;          // 0x00f11c90
extern int g_sfxVolume;            // 0x00f11c94
extern int g_hints;                // 0x00f11c9c
extern int g_vibration;            // 0x00f11ca8
extern int g_widescreen;           // 0x00f11cac
extern int g_combatMode;           // 0x00f11cb0
extern int g_cameraXInverted;      // 0x00f11cb4
extern int g_cameraYInverted;      // 0x00f11cb8
extern int g_aimXInverted;         // 0x00f11cbc
extern int g_aimYInverted;         // 0x00f11cc0
extern char g_gamepadConfig[128];  // 0x00f11cc4

// 0x00404a40 - Reads registry settings from HKLM & HKCU
int SETUP_LoadRegistrySettings(void);

// 0x004044c0 - Reads user settings from HKCU
void SETUP_LoadUserPreferences(void);

// 0x00404e00 - Writes user settings back to HKCU
void SETUP_SaveRegistrySettings(void);

// 0x004056c0 - Displays setup / graphics configuration dialog
int SETUP_DoSetupDialog(int allowWindowSwitch, int forceDialog);

// 0x004057c0 - Initializes localization strings for setup dialog controls
void SETUP_LocalizeSetupDialog(void);

#ifdef __cplusplus
}
#endif

#endif // GAME_PC_SETUP_H
