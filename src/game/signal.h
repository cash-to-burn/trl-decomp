#pragma once

#ifndef GAME_SIGNAL_H
#define GAME_SIGNAL_H

#include "common.h"
#include "game/camera/camerastack.h"
#include "game/collide.h"
#include <cstddef>
#include <cstdint>

struct Instance;
struct Level;
struct Signal;

namespace dtp {

// Size: 0x1dc bytes (TI: 0x5fe0)
struct UnitData {
    // Size: 0x0c bytes (TI: 0x5fff)
    struct FSFXLink {
        /* 0x000 */ uint32_t ID;
        /* 0x004 */ float alpha;
        /* 0x008 */ bool bEnabled;
        /* 0x009 */ uint8_t pad009[3];
    };

    // Size: 0x18 bytes (TI: 0x5ff1)
    struct baseCamera {
        /* 0x000 */ int UseCameraStackSystem;
        /* 0x004 */ uint32_t basecam;
        union {
            struct {
                /* 0x008 */ uint32_t ledgecam;
                /* 0x00c */ uint32_t crawlcam;
                /* 0x010 */ uint32_t vehiclecam;
                /* 0x014 */ uint32_t deathcam;
            };
            movementCameraOverrides ledgeOverrides;
        };
    };

    /* 0x000 */ uint32_t m_numFSFX;
    /* 0x004 */ FSFXLink* m_pFSFX;
    /* 0x008 */ uint32_t m_numCines;
    /* 0x00c */ void* m_pCines;
    /* 0x010 */ uint8_t DepthData[0x5c];
    /* 0x06c */ uint8_t BendData[0x120];
    /* 0x18c */ baseCamera BaseCamera;
    /* 0x1a4 */ uint32_t m_script;
    /* 0x1a8 */ uint32_t m_numEventVariableStorage;
    /* 0x1ac */ void* m_pEventVariableStorage;
    /* 0x1b0 */ uint8_t m_NextGenData[0x20];
    /* 0x1d0 */ uint8_t m_OverlayData[0x0c];
};

static_assert(sizeof(UnitData::FSFXLink) == 0x0c, "dtp::UnitData::FSFXLink size mismatch");
static_assert(sizeof(UnitData::baseCamera) == 0x18, "dtp::UnitData::baseCamera size mismatch");
static_assert(offsetof(UnitData, BaseCamera) == 0x18c, "dtp::UnitData::BaseCamera offset mismatch");
static_assert(sizeof(UnitData) == 0x1dc, "dtp::UnitData size mismatch");

} // namespace dtp

// Size: 0x290 bytes (TI: 0x6ea5)
struct AttackWaveRuntime {
    /* 0x000 */ float radiusCheckTimer;
    /* 0x004 */ uint8_t flags;
    /* 0x005 */ uint8_t rtFlags;
    /* 0x006 */ uint8_t probability;
    /* 0x007 */ uint8_t numAttackerSpawns;
    /* 0x008 */ uint8_t numSpawnsThisLoad;
    /* 0x009 */ uint8_t currentAttacker;
    /* 0x00a */ uint8_t triggerRemaining;
    /* 0x00b */ uint8_t numAttackersPerWave;
    /* 0x00c */ uint8_t numLinkedAttackWaves;
    /* 0x00d */ uint8_t numBlockedAttackWaves;
    /* 0x00e */ uint8_t numFinishedAttackWaves;
    /* 0x00f */ uint8_t numAttackers;
    /* 0x010 */ uint8_t musicRank;
    /* 0x011 */ int8_t pad;
    /* 0x012 */ uint16_t radiusCheckMarker;
    /* 0x014 */ int16_t radiusCheckRadius;
    /* 0x018 */ AttackWaveRuntime* nextEnableAttackWave;
    /* 0x01c */ AttackWaveRuntime* nextDisableAttackWave;
    /* 0x020 */ AttackWaveRuntime* nextKillAttackWave;
    /* 0x024 */ uint8_t movePosLimits[0x230];
    /* 0x254 */ uint8_t messages[0x34];
    /* 0x288 */ int endMarker;
    /* 0x28c */ void* data;
};
static_assert(offsetof(AttackWaveRuntime, nextEnableAttackWave) == 0x018, "AttackWaveRuntime::nextEnableAttackWave offset mismatch");
static_assert(offsetof(AttackWaveRuntime, nextDisableAttackWave) == 0x01c, "AttackWaveRuntime::nextDisableAttackWave offset mismatch");
static_assert(offsetof(AttackWaveRuntime, nextKillAttackWave) == 0x020, "AttackWaveRuntime::nextKillAttackWave offset mismatch");
static_assert(sizeof(AttackWaveRuntime) == 0x290, "AttackWaveRuntime size mismatch");

// Size: 0x08 bytes (TI: 0xbca1)
struct SignalResetStruct {
    /* 0x000 */ Signal* mSignal;
    /* 0x004 */ int timeLeft;
};
static_assert(sizeof(SignalResetStruct) == 0x08, "SignalResetStruct size mismatch");

static constexpr int kMaxSignalsToReset = 48;

extern SignalResetStruct ResetSignalArray[kMaxSignalsToReset]; // 0x010f0da8 (PDB: 0x010f1ba8)
extern int NumSignalsToReset;                                 // 0x010f0f28 (PDB: 0x010f1d28)

// Signal subsystem routines (Original: game/Signal.c / Signal.cpp)
void SIGNAL_HandleStreamLevelBaseCam(Instance* instance, Signal* signal);                   // 0x00467250 (PDB: 0x00468c10)
int  SIGNAL_HandleStreamLevel(Instance* instance, Signal* signal);                           // 0x004672d0 (PDB: 0x00468c90)
void SIGNAL_HandleOutSignal(Instance* instance, Level* level, Signal* signal, int signalID); // 0x00467490 (PDB: 0x00468e50)
void SIGNAL_HandleSignal(Instance* instance, Level* level, Signal* signal, int signalID);    // 0x004675a0 (PDB: 0x00468f60)
int  SIGNAL_IsStreamSignal(Signal* signal);                                                  // 0x004679d0 (PDB: 0x00469390)
Signal* SIGNAL_FindSignal(Level* level, int id);                                             // 0x004679e0 (PDB: 0x004693a0)
Signal* SIGNAL_FindSignalStream(Level* level, int id);                                       // 0x00467a20 (PDB: 0x004693e0)
Signal* SIGNAL_FindSignalEvent(Level* level, int id);                                        // 0x00467a60 (PDB: 0x00469420)
void SIGNAL_OutOfWater(Instance* instance);                                                  // 0x00467aa0 (PDB: 0x00469460)
void SIGNAL_InWater(Instance* instance);                                                     // 0x00467af0 (PDB: 0x004694b0)
void SIGNAL_ClearResetSignals(void);                                                         // 0x00467b40 (PDB: 0x00469500)
void SIGNAL_ResetSignals(void);                                                              // 0x00467b60 (PDB: 0x00469520)
void SIGNAL_AddSignalToReset(Signal* signal);                                                // 0x00467ba0 (PDB: 0x00469560)
void SIGNAL_FixStreamSignal(Level* level);                                                   // 0x00467be0 (PDB: 0x004695a0)

#endif // GAME_SIGNAL_H
