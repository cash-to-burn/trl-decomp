#ifndef GAME_MULTIBODY_SRC_MASSPROPERTIES_H
#define GAME_MULTIBODY_SRC_MASSPROPERTIES_H

#include "cdc/runtime/cdcMath/Math.h"

// PDB: MassProperties (size: 0x60)
struct alignas(16) MassProperties {
    float mass;             // 0x00
    uint32_t pad04[3];      // 0x04..0x0f
    cdc::Vector3 center;    // 0x10
    cdc::Matrix I;          // 0x20

    // 0x00515020 (PDB: 0x00516b10 / ?CheckValidity@MassProperties@@QBE_NXZ)
    bool CheckValidity() const;

    // 0x00515070 (PDB: 0x00516b60 / ?SetZero@MassProperties@@QAEXXZ)
    void SetZero();

    // 0x005150b0 (PDB: 0x00516ba0 / ?SetSphere@MassProperties@@QAEXMM@Z)
    void SetSphere(float massValue, float radius);

    // 0x00515110 (PDB: 0x00516c00 / ?SetBox@MassProperties@@QAEXMABVVector3@cdc@@@Z)
    void SetBox(float massValue, const cdc::Vector3& width);

    // 0x00515230 (PDB: 0x00516d20 / ?SetCapsule@MassProperties@@QAEXMMM@Z)
    void SetCapsule(float massValue, float radius, float length);

    // 0x00515360 (PDB: 0x00516e50 / ?Translate@MassProperties@@QAEXABVVector3@cdc@@@Z)
    void Translate(const cdc::Vector3& offset);

    // 0x00515470 (PDB: 0x00516f60 / ?Rotate@MassProperties@@QAEXABVMatrix@cdc@@@Z)
    void Rotate(const cdc::Matrix& R);

    // 0x00515510 (PDB: 0x00517000 / ?Add@MassProperties@@QAEXABU1@@Z)
    void Add(const MassProperties& other);
};
static_assert(sizeof(MassProperties) == 0x60, "MassProperties size mismatch");

#endif // GAME_MULTIBODY_SRC_MASSPROPERTIES_H
