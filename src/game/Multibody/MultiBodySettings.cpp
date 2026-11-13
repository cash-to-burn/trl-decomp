#include "game/Multibody/MultiBodySettings.h"

// 0x004702f0 / ?DEBUG_DrawLine@@YAXHHHABVVector3@cdc@@0@Z (no-op in release build at 0x004dee60)
extern void DEBUG_DrawLine(int r, int g, int b, const cdc::Vector3& start, const cdc::Vector3& end);

// 0x004f9160 (PDB: 0x004fac90 / ?MB_DrawLine@@YAXABVVector3@cdc@@00@Z)
void MB_DrawLine(
    const cdc::Vector3& start,
    const cdc::Vector3& end,
    const cdc::Vector3& color)
{
    DEBUG_DrawLine(
        static_cast<int>(color.x * 255.0f),
        static_cast<int>(color.y * 255.0f),
        static_cast<int>(color.z * 255.0f),
        start,
        end);
}
