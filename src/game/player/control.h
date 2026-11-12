#ifndef GAME_PLAYER_CONTROL_H
#define GAME_PLAYER_CONTROL_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/instance.h"
#include "game/player/player.h"

#pragma pack(push, 1)

// Size: 0x20 bytes (32 bytes, TI: 0x883f)
struct Force {
    /* 0x000 */ cdc::Vector3 LinearForce;
    /* 0x010 */ int Type;
    /* 0x014 */ float Friction;
    /* 0x018 */ uint8_t pad18[8];
};

#pragma pack(pop)

static_assert(sizeof(Force) == 0x20, "Force size mismatch");

// Subsystem globals
extern float g_fallMaxZSpeed;                           // 0x01118710
extern float g_fallMaxSideSpeed;                        // 0x01118714

// Control functions (Original: f:\tr7\game\player\control.c)
void SetFallMaxZSpeed(Instance* instance, float speed);                                 // 0x005b2db0 (PDB: 0x005b56b0)
void SetFallMaxSideSpeed(Instance* instance, float speed);                              // 0x005b2dc0 (PDB: 0x005b56c0)
void SetImpulsePhysics(Instance* instance, Player* player);                             // 0x005b2dd0 (PDB: 0x005b56d0)
void InitExternalForces(Force* forces, int count);                                      // 0x005b2e80 (PDB: 0x005b5780)
void SetExternalForce(Force* force, float fx, float fy, float fz, unsigned char flags, float duration); // 0x005b2ec0 (PDB: 0x005b57c0)
void SetPhysics(Instance* instance, float mass, float drag, float gravity, float maxVel); // 0x005b2ef0 (PDB: 0x005b57f0)
void ResetExternalForces(Instance* instance);                                           // PDB: 0x005b5850
void ResetPhysics(Instance* instance, float mass);                                      // PDB: 0x005b5910
void SetDropPhysics(Instance* instance, Player* player);                                // PDB: 0x005b5980
void ApplyExternalLocalForces(Player* player, Instance* instance, Force* forces, int count, cdc::Vector3* result); // PDB: 0x005b5b60
void ProcessPhysics(Player* player, CharacterState* charState, int param3, int param4); // PDB: 0x005b5d00

#endif // GAME_PLAYER_CONTROL_H
