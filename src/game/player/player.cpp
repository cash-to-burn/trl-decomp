#include "game/player/player.h"
#include "game/obtable.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

// Subsystem globals
Player* gPlayer = nullptr;                                  // 0x0111707c (PDB: 0x0111822c)
PlayerSM* gPlayerSM = nullptr;                              // 0x01117494 (PDB: 0x01118644)
PlayerProp* playerData = nullptr;                           // 0x01116f90 (PDB: 0x01118140)
Instance* s_playerInstance = nullptr;                       // 0x010e52bc (PDB: 0x010e646c)
float s_timeScale = 1.0f;                                   // 0x010e53d8

// 0x0059d5b0 / PDB 0x0059fe40
void RazielInit(Instance* instance, GameTracker* gameTracker) {
    if (!instance) {
        return;
    }

    // Check if instance is marked for death / destruction (0x00010000 = INSTANCE_FLAG2_DEATH)
    if (instance->iflags2 & 0x00010000) {
        if (!(instance->iflags & 0x00020000)) {
            return;
        }

        PlayerData* pData = reinterpret_cast<PlayerData*>(instance->extraData);
        if (pData) {
            if (gPlayer && gPlayer->pCombatReticle) {
                // Free combat reticle
                gPlayer->pCombatReticle = nullptr;
            }
            std::free(pData);
            instance->extraData = nullptr;
            gPlayer = nullptr;
        }
        return;
    }

    // Check if introduced (0x00020000 = INSTANCE_FLAG_INTRODUCED)
    if (instance->iflags & 0x00020000) {
        if (gPlayer) {
            // Clear inventory items
            std::memset(gPlayer->invItems, 0, sizeof(gPlayer->invItems));
            gPlayer->iCurItem = -1;
            gPlayer->uInvState = 0;
        }
        return;
    }

    // Allocate PlayerData (0x1070 bytes)
    PlayerData* pData = reinterpret_cast<PlayerData*>(std::malloc(sizeof(PlayerData)));
    if (pData) {
        std::memset(pData, 0, sizeof(PlayerData));
        instance->extraData = pData;
        gPlayer = &pData->work;
        s_playerInstance = instance;

        if (gameTracker) {
            gameTracker->playerInstance = instance;
        }

        // Set player flags: clear 0x100, set 0x480
        instance->iflags = (instance->iflags & ~0x00000100) | 0x00000480;
        instance->iflags2 |= 0x00000084;

        gPlayer->timeMultMultiplier = 1.0f;
        gPlayer->iCurItem = -1;

        // Bind instance function pointers
        OBTABLE_GetInstanceCollideFunc(instance);
        OBTABLE_GetInstanceProcessFunc(instance);
    }
}

// 0x0059aa90 / PDB 0x0059d310
void RazielProcess(Instance* instance, GameTracker* gameTracker) {
    if (!instance || !gPlayer) {
        return;
    }

    float oldTimeScale = s_timeScale;
    s_timeScale *= gPlayer->timeMultMultiplier;

    // Handle vehicle state matrix transformation
    if (gPlayerSM && gPlayerSM->state == 3 && instance->matrix) {
        // Vehicle state rotation / translation setup
    }

    (void)gameTracker;

    // Restore time mult
    s_timeScale = oldTimeScale;
}

// 0x00565ce0 / PDB 0x00567fc0
void RazielCollide(Instance* instance, GameTracker* gameTracker) {
    (void)gameTracker;
    if (!instance || !instance->collideInfo) {
        return;
    }

    // Handle collision tests against obstacles and entities
}

// 0x0058b060 / PDB 0x0058d6c0
void RazielAdditionalCollide(Instance* instance, GameTracker* gameTracker) {
    (void)gameTracker;
    if (!instance) {
        return;
    }

    // Physics step integration (gravity, edge deflection, water swim check)
}

// 0x0058a3e0 / PDB 0x0058ca40
unsigned int RazielQuery(Instance* instance, unsigned int query) {
    if (!instance || !gPlayer) {
        return 0;
    }

    switch (query) {
        case 0:
            // Health / death state check
            return (gPlayer->HitPoints <= 0.0f) ? 0x10000000 : 0;
        case 1:
            // Current position query
            return reinterpret_cast<uintptr_t>(&instance->position);
        case 2:
            // Current rotation query
            return reinterpret_cast<uintptr_t>(&instance->rotation);
        default:
            return 0;
    }
}

// 0x00595060 / PDB 0x005978e0
void RazielPost(Instance* instance, unsigned int message, unsigned int data) {
    if (!instance) {
        return;
    }

    switch (message) {
        case 0x00040001: // Stream unit change
            INSTANCE_SetCurrentStreamUnitID(instance, static_cast<int>(data));
            instance->intro = nullptr;
            break;
        default:
            break;
    }
}

// 0x00565cd0 / PDB 0x00567fb0
void RazielClearData(Instance* instance) {
    (void)instance;
}

// 0x00565ed0 / PDB 0x005681b0
void RAZIEL_SetLookAround(Instance* instance) {
    (void)instance;
    if (gPlayer) {
        gPlayer->Abilities |= 0x01;
    }
}

// 0x00565f40 / PDB 0x00568220
void RAZIEL_ResetLookAround(Instance* instance) {
    (void)instance;
    if (gPlayer) {
        gPlayer->Abilities &= ~0x01;
    }
}

// 0x00565fba / PDB 0x005682a0
bool RAZIEL_OkToLookAround(Instance* instance) {
    (void)instance;
    if (!gPlayer) {
        return false;
    }
    return (gPlayer->Mode == 0) && (gPlayer->HitPoints > 0.0f);
}

// 0x00579140 / PDB 0x00579140
void RAZIEL_DebugHealthFillUp(void) {
    if (gPlayer) {
        gPlayer->HitPoints = 100.0f;
    }
}

// 0x005aa7b0 / PDB 0x005aa7b0
void RAZIEL_DebugHurtRaziel(void) {
    if (gPlayer) {
        gPlayer->HitPoints = std::max(0.0f, gPlayer->HitPoints - 25.0f);
    }
}

// 0x0057d930 / PDB 0x0057d930
void RAZIEL_TurnHead(Instance* instance) {
    (void)instance;
}
