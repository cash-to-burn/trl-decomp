#pragma once

#ifndef GAME_PC_PCMOUSE_H
#define GAME_PC_PCMOUSE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global mouse coordinates and states
extern float g_mouseCursorX;       // 0x00f11fc8 (normalized [0.0, 0.98])
extern float g_mouseCursorY;       // 0x00f11fcc (normalized [0.0, 0.98])
extern int g_mouseDeltaX;          // 0x00f11f40
extern int g_mouseDeltaY;          // 0x00f11f44
extern bool s_mouseActive;         // 0x00f40588
extern bool s_mouseDeActivating;   // 0x00f40589
extern int g_mouseWheelPos;        // 0x00f40580
extern uint32_t g_mouseButtonState[3]; // 0x00f40574 (0=LButton, 1=RButton, 2=MButton)

// 0x004051b0 (PDB: 0x00405b00) - Clamps and updates normalized cursor position
void PCMOUSE_SetCursorPos(float x, float y);

// 0x00405280 (PDB: 0x00405bd0) - Returns true if mouse is active and not deactivating
bool PCMOUSE_IsMouseActive(void);

// 0x004052a0 (PDB: 0x00405bf0) - Marks cursor as deactivating (begins fade out)
void PCMOUSE_DeActivateCursor(void);

// 0x004052b0 (PDB: 0x00405c00) - Marks cursor as active and visible
void PCMOUSE_ActivateCursor(void);

// 0x004052d0 (PDB: 0x00405c20) - Fetches current cursor coordinates
uint32_t PCMOUSE_GetCursorPos(float *x, float *y);

// 0x00405300 (PDB: 0x00405c50) - Adds delta and updates normalized cursor position
void PCMOUSE_UpdateDelta(float dx, float dy);

// 0x004053f0 (PDB: 0x00405d40) - Updates button pressed state (0=Left, 1=Right)
void PCMOUSE_UpdateButtonDown(int button);

// 0x00405400 (PDB: 0x00405d50) - Updates button released state (0=Left, 1=Right)
void PCMOUSE_UpdateButtonUp(int button);

// 0x00405410 (PDB: 0x00405d60) - Increments mouse wheel accumulator
void PCMOUSE_ScrollMouseWheel(int delta);

// 0x00405430 (PDB: 0x00405d80) - Gets current mouse wheel position
int PCMOUSE_GetMouseWheelPos(void);

// 0x00405440 (PDB: 0x00405d90) - Sets current mouse wheel position
void PCMOUSE_SetMouseWheelPos(int pos);

// 0x00405450 (PDB: 0x00405da0) - Gets button state and clears transient click bits
uint32_t PCMOUSE_GetButtonState(int button);

// 0x00405470 (PDB: 0x00405dc0) - Draws mouse cursor sprite on screen
void PCMOUSE_DisplayCursor(void);

#ifdef __cplusplus
}
#endif

#endif // GAME_PC_PCMOUSE_H
