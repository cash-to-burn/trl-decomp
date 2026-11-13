#include "game/g2/timerg2.h"
#include "game/player/player.h"

// 0x004e02e0 (PDB: 0x004e1e10)
float G2Timer_GetFrameTime()
{
    return s_timeScale * 100.0f;
}
