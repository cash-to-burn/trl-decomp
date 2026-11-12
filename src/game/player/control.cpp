#include "game/player/control.h"
#include <cstring>

// Subsystem globals
float g_fallMaxZSpeed = 0.0f;                           // 0x01118710
float g_fallMaxSideSpeed = 0.0f;                        // 0x01118714

// 0x005b2db0 (PDB: 0x005b56b0)
void SetFallMaxZSpeed(Instance* instance, float speed) {
    (void)instance;
    g_fallMaxZSpeed = speed;
}

// 0x005b2dc0 (PDB: 0x005b56c0)
void SetFallMaxSideSpeed(Instance* instance, float speed) {
    (void)instance;
    g_fallMaxSideSpeed = speed;
}

// 0x005b2dd0 (PDB: 0x005b56d0)
void SetImpulsePhysics(Instance* instance, Player* player) {
    if (!instance || !player) {
        return;
    }

    if (player->Mode & 0x00040000) {
        return;
    }

    // Set impulse velocity based on player momentum
}

// 0x005b2e80 (PDB: 0x005b5780)
void InitExternalForces(Force* forces, int count) {
    if (!forces || count <= 1) {
        return;
    }

    for (int i = 1; i < count; ++i) {
        std::memset(&forces[i], 0, sizeof(Force));
    }
}

// 0x005b2ec0 (PDB: 0x005b57c0)
void SetExternalForce(Force* force, float fx, float fy, float fz, unsigned char flags, float duration) {
    if (!force) {
        return;
    }

    force->LinearForce.x = fx;
    force->LinearForce.y = fy;
    force->LinearForce.z = fz;
    force->Type = static_cast<int>(flags);
    force->Friction = duration;
}

// 0x005b2ef0 (PDB: 0x005b57f0)
void SetPhysics(Instance* instance, float mass, float drag, float gravity, float maxVel) {
    if (!instance) {
        return;
    }
    (void)mass;
    (void)drag;
    (void)gravity;
    (void)maxVel;
}

// PDB: 0x005b5850
void ResetExternalForces(Instance* instance) {
    if (!instance) {
        return;
    }
}

// PDB: 0x005b5910
void ResetPhysics(Instance* instance, float mass) {
    if (!instance) {
        return;
    }
    (void)mass;
}

// PDB: 0x005b5980
void SetDropPhysics(Instance* instance, Player* player) {
    if (!instance || !player) {
        return;
    }
}

// PDB: 0x005b5b60
void ApplyExternalLocalForces(Player* player, Instance* instance, Force* forces, int count, cdc::Vector3* result) {
    (void)player;
    (void)instance;
    (void)forces;
    (void)count;
    if (result) {
        result->x = 0.0f;
        result->y = 0.0f;
        result->z = 0.0f;
    }
}

// PDB: 0x005b5d00
void ProcessPhysics(Player* player, CharacterState* charState, int param3, int param4) {
    (void)player;
    (void)charState;
    (void)param3;
    (void)param4;
}
