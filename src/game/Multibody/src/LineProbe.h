#ifndef GAME_MULTIBODY_SRC_LINEPROBE_H
#define GAME_MULTIBODY_SRC_LINEPROBE_H

#include "cdc/runtime/cdcMath/Math.h"

class Geometry;

// PDB: LineProbeGeometryData (size: 0x08)
struct LineProbeGeometryData {
    bool (*fcn)(Geometry*, void*); // 0x00
    void* clientData;              // 0x04
};
static_assert(sizeof(LineProbeGeometryData) == 0x08, "LineProbeGeometryData size mismatch");

// 0x00514b20 (PDB: 0x00516610 / ?LineProbeNarrowPhase@@YA_NPAMPAVVector3@cdc@@ABV12@2PAX3@Z)
bool LineProbeNarrowPhase(
    float* t,
    cdc::Vector3* normal,
    const cdc::Vector3& start,
    const cdc::Vector3& dir,
    void* geomVoid,
    void* userData);

#endif // GAME_MULTIBODY_SRC_LINEPROBE_H
