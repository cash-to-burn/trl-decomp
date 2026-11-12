#pragma once

#ifndef GAME_PC_PCLOCALS_H
#define GAME_PC_PCLOCALS_H

#include "common.h"

extern char g_buildDir[256];        // 0x01111df0 (e.g. "PCENGLISH\" or "PC-W\")
extern char g_languageName[256];    // 0x01111e10 (e.g. "PCENGLISH")

#ifdef __cplusplus
extern "C" {
#endif

// 0x0045c0e0 - Sets the active game asset directory and audio/video stream paths
void SetupBuildDir(const char *dir);

// 0x004e3fa0 - Verifies if locals.bin exists in the currently configured build directory
bool localstr_verifyexistence(void);

// 0x004e4010 - Sets active localization language index
void localstr_set_language(int langId);

// 0x004e4050 - Retrieves localized string pointer by text ID
const char *localstr_get(int stringId);

// 0x004e4050 / PDB PCLOCALS_GetFixedIDLocal
const char *PCLOCALS_GetFixedIDLocal(int localId);

#ifdef __cplusplus
}
#endif

#endif // GAME_PC_PCLOCALS_H
