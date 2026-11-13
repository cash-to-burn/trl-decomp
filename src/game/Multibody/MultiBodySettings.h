#ifndef GAME_MULTIBODY_MULTIBODYSETTINGS_H
#define GAME_MULTIBODY_MULTIBODYSETTINGS_H

#include "cdc/runtime/cdcMath/Math.h"

// 0x004f9160 (PDB: 0x004fac90 / ?MB_DrawLine@@YAXABVVector3@cdc@@00@Z)
void MB_DrawLine(
    const cdc::Vector3& start,
    const cdc::Vector3& end,
    const cdc::Vector3& color);

#endif // GAME_MULTIBODY_MULTIBODYSETTINGS_H
