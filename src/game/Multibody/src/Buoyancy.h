#ifndef GAME_MULTIBODY_SRC_BUOYANCY_H
#define GAME_MULTIBODY_SRC_BUOYANCY_H

#include "cdc/runtime/cdcMath/Math.h"

// 0x00504820 (PDB: 0x00506460 / ?SphereBuoyancy@@YAMPAVVector3@cdc@@ABV12@1MM@Z)
float SphereBuoyancy(
    cdc::Vector3* outCentroid,
    const cdc::Vector3& sphereCenter,
    const cdc::Vector3& planeNormal,
    float planeDist,
    float radius);

// 0x00504ba0 (PDB: 0x005067e0 / ?BoxBuoyancy@@YAMPAVVector3@cdc@@ABV12@ABVQuat@2@11M@Z)
float BoxBuoyancy(
    cdc::Vector3* outCentroid,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxWidth,
    const cdc::Vector3& planeNormal,
    float planeDist);

#endif // GAME_MULTIBODY_SRC_BUOYANCY_H
