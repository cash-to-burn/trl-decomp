#pragma once

#ifndef GAME_PC_D3D_RND_D3D_H
#define GAME_PC_D3D_RND_D3D_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x00413880 - Initializes D3D9 manager and enumerates display adapters
bool D3D_InitManager(void);

// 0x004139a0 - Creates or resets Direct3D9 device on the active window
bool D3D_InitDevice(bool param);

// 0x00413930 - Shuts down Direct3D device and releases adapter manager
void D3D_ShutdownDevice(void);

// 0x00413a20 - Flushes renderer resources and texture caches before device destroy
void RND_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // GAME_PC_D3D_RND_D3D_H
