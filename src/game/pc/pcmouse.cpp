#include "game/pc/pcmouse.h"
#include <algorithm>

// Global mouse coordinates and states
float g_mouseCursorX = 0.5f;
float g_mouseCursorY = 0.5f;
int g_mouseDeltaX = 0;
int g_mouseDeltaY = 0;
bool s_mouseActive = false;
bool s_mouseDeActivating = false;
int g_mouseWheelPos = 0;
uint32_t g_mouseButtonState[3] = {0, 0, 0};

static float s_cursorAlpha = 0.0f;
static uint32_t s_cursorImage = 0;

// Clamping limits from retail (0.0f to 0.98f)
static const float MOUSE_CURSOR_MIN = 0.0f;
static const float MOUSE_CURSOR_MAX = 0.98f;

// 0x004051b0 (PDB: 0x00405b00)
void PCMOUSE_SetCursorPos(float x, float y)
{
    float oldX = g_mouseCursorX;
    float oldY = g_mouseCursorY;

    if (x < MOUSE_CURSOR_MIN) {
        g_mouseCursorX = MOUSE_CURSOR_MIN;
    } else if (x > MOUSE_CURSOR_MAX) {
        g_mouseCursorX = MOUSE_CURSOR_MAX;
    } else {
        g_mouseCursorX = x;
    }

    if (y < MOUSE_CURSOR_MIN) {
        g_mouseCursorY = MOUSE_CURSOR_MIN;
    } else if (y > MOUSE_CURSOR_MAX) {
        g_mouseCursorY = MOUSE_CURSOR_MAX;
    } else {
        g_mouseCursorY = y;
    }

    if (oldX != g_mouseCursorX || oldY != g_mouseCursorY) {
        s_mouseActive = true;
        s_mouseDeActivating = false;
    }
}

// 0x00405280 (PDB: 0x00405bd0)
bool PCMOUSE_IsMouseActive(void)
{
    return s_mouseActive && !s_mouseDeActivating;
}

// 0x004052a0 (PDB: 0x00405bf0)
void PCMOUSE_DeActivateCursor(void)
{
    s_mouseDeActivating = true;
}

// 0x004052b0 (PDB: 0x00405c00)
void PCMOUSE_ActivateCursor(void)
{
    s_mouseActive = true;
    s_mouseDeActivating = false;
}

// 0x004052d0 (PDB: 0x00405c20)
uint32_t PCMOUSE_GetCursorPos(float *x, float *y)
{
    if (x != nullptr) {
        *x = g_mouseCursorX;
    }
    if (y != nullptr) {
        *y = g_mouseCursorY;
    }

    // Returns packed coordinate status and resets transient flags
    uint32_t status = (s_mouseActive ? 1 : 0);
    g_mouseDeltaX = 0;
    g_mouseDeltaY = 0;
    return status;
}

// 0x00405300 (PDB: 0x00405c50)
void PCMOUSE_UpdateDelta(float dx, float dy)
{
    if (!s_mouseActive) {
        s_mouseActive = true;
        s_mouseDeActivating = false;
        return;
    }

    g_mouseCursorX += dx;
    g_mouseCursorY += dy;

    if (dx != 0.0f || dy != 0.0f) {
        s_mouseActive = true;
        s_mouseDeActivating = false;

        if (g_mouseCursorX < MOUSE_CURSOR_MIN) {
            g_mouseCursorX = MOUSE_CURSOR_MIN;
        } else if (g_mouseCursorX > MOUSE_CURSOR_MAX) {
            g_mouseCursorX = MOUSE_CURSOR_MAX;
        }

        if (g_mouseCursorY < MOUSE_CURSOR_MIN) {
            g_mouseCursorY = MOUSE_CURSOR_MIN;
        } else if (g_mouseCursorY > MOUSE_CURSOR_MAX) {
            g_mouseCursorY = MOUSE_CURSOR_MAX;
        }
    }
}

// 0x004053f0 (PDB: 0x00405d40)
void PCMOUSE_UpdateButtonDown(int button)
{
    if (button >= 0 && button < 3) {
        g_mouseButtonState[button] = 10; // active held down code
    }
}

// 0x00405400 (PDB: 0x00405d50)
void PCMOUSE_UpdateButtonUp(int button)
{
    if (button >= 0 && button < 3) {
        g_mouseButtonState[button] = 0; // released
    }
}

// 0x00405410 (PDB: 0x00405d60)
void PCMOUSE_ScrollMouseWheel(int delta)
{
    g_mouseWheelPos += delta;
}

// 0x00405430 (PDB: 0x00405d80)
int PCMOUSE_GetMouseWheelPos(void)
{
    return g_mouseWheelPos;
}

// 0x00405440 (PDB: 0x00405d90)
void PCMOUSE_SetMouseWheelPos(int pos)
{
    g_mouseWheelPos = pos;
}

// 0x00405450 (PDB: 0x00405da0)
uint32_t PCMOUSE_GetButtonState(int button)
{
    if (button >= 0 && button < 3) {
        uint32_t state = g_mouseButtonState[button];
        g_mouseButtonState[button] &= ~2; // clear transient click bit
        return state;
    }
    return 0;
}

// 0x00405470 (PDB: 0x00405dc0)
void PCMOUSE_DisplayCursor(void)
{
    const float fadeSpeed = 4.0f * 0.016f; // approximate delta time step

    if (s_mouseDeActivating) {
        s_cursorAlpha -= fadeSpeed;
        if (s_cursorAlpha <= 0.0f) {
            s_cursorAlpha = 0.0f;
            s_mouseActive = false;
            s_mouseDeActivating = false;
            return;
        }
    }

    if (s_mouseActive) {
        if (!s_mouseDeActivating) {
            s_cursorAlpha += fadeSpeed;
            if (s_cursorAlpha > 1.0f) {
                s_cursorAlpha = 1.0f;
            }
        }

        g_mouseDeltaX = 0;
        g_mouseDeltaY = 0;

        // If cursor texture is uninitialized, it will be lazily loaded:
        // if (s_cursorImage == 0) s_cursorImage = IMAGE_LoadImage("cursor");
        // Render 2D cursor sprite at (g_mouseCursorX, g_mouseCursorY) with alpha s_cursorAlpha
    }
}
