#include "game/signal.h"
#include "game/camera/camera.h"
#include "game/camera/cameraspline.h"
#include "game/camera/camerastack.h"
#include "game/instance.h"
#include "game/main.h"
#include "game/obtable.h"
#include "game/Stream/stream.h"

#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
static inline void Sleep(uint32_t ms)
{
    usleep(ms * 1000);
}
#endif

// Cross-subsystem externs
extern void DEBUG_Print(const char* fmt, ...);                                             // 0x004262f0
extern void TERRAIN_FSFXStack_Set_Primary(uint32_t dtpid, int streamUnitID, float dist);   // 0x0046b900 (PDB: 0x0046d260)
extern void SAVE_Instance(Instance* instance, Level* level);                               // 0x005c03a0 (PDB: 0x005c2b30)
extern void SOUND_SetSFXReverb(int reverbType, int reverbVolume);                          // 0x005c95a0 (PDB: 0x005cbc70)
extern void SRMUSIC_SetMusicVariable(int var, int value);                                  // 0x005ca3c0 (PDB: 0x005ccc00)
extern void STREAM_SignalTurnOnAttackWave(AttackWaveRuntime* wave);                        // 0x005d00d0 (PDB: 0x005d2b90)
extern void STREAM_SignalTurnOffAttackWave(AttackWaveRuntime* wave);                       // 0x005d00f0 (PDB: 0x005d2bb0)
extern void STREAM_SignalKillAttackWaveMonsters(AttackWaveRuntime* wave);                  // 0x005d1d50 (PDB: 0x005d4810)

struct TrackStream {
    static int16_t GetActiveUnitId();   // 0x005d5ce0 (PDB: 0x005d8800)
    static int16_t GetPreviousUnitId(); // 0x005d5cf0 (PDB: 0x005d8810)
};

// Module globals
SignalResetStruct ResetSignalArray[kMaxSignalsToReset]; // 0x010f0da8 (PDB: 0x010f1ba8)
int NumSignalsToReset = 0;                              // 0x010f0f28 (PDB: 0x010f1d28)

namespace {

constexpr uint32_t kSignalResetFSFXOutBit  = 0x0200; // byte[1] & 0x02
constexpr uint32_t kSignalResetFSFXInBit   = 0x0400; // byte[1] & 0x04
constexpr uint32_t kSignalObjectHitBit     = 0x2000; // byte[1] & 0x20
constexpr uint32_t kSignalResetSlideBit    = 0x4000; // byte[1] & 0x40
constexpr uint32_t kSignalTriggeredBit     = 0x8000; // byte[1] & 0x80

bool IsPlayerOrLinkedToPlayer(const Instance* instance)
{
    const auto* player = static_cast<const Instance*>(g_gameTracker.playerInstance);
    return instance == player || (player != nullptr && instance == player->LinkParent);
}

void ApplyDefaultUnitFSFX(const Level* level, int streamUnitID, uint16_t activeDist)
{
    if (level == nullptr || level->pUnitData == nullptr) {
        return;
    }

    const dtp::UnitData* unitData = level->pUnitData;
    if (unitData->m_numFSFX == 0 || unitData->m_pFSFX == nullptr) {
        return;
    }

    const dtp::UnitData::FSFXLink& defaultFSFX = unitData->m_pFSFX[0];
    if (defaultFSFX.bEnabled) {
        TERRAIN_FSFXStack_Set_Primary(defaultFSFX.ID, streamUnitID, static_cast<float>(activeDist));
    }
}

int SignalCameraStackSlot(const Signal* signal)
{
    return static_cast<int>(static_cast<int8_t>(signal->CameraStackFlags)) >> 4;
}

int SignalDegreesToCameraUnits(int degrees)
{
    int wrapped = (-degrees) % 360;
    if (wrapped < 0) {
        wrapped += 360;
    }
    return (wrapped << 12) / 360;
}

Signal* FindSignalWithWarning(Level* level, int id, const char* fmt)
{
    Signal* signal = SIGNAL_FindSignal(level, id);
    if (signal == nullptr) {
        StreamUnit* unit = STREAM_GetStreamUnitWithID(level->streamUnitID);
        DEBUG_Print(fmt, id, unit->baseAreaName);
    }
    return signal;
}

void ApplyWaterTransitionSignal(Instance* instance, bool enteringWater)
{
    StreamUnit* unit = STREAM_GetStreamUnitWithID(instance->currentStreamUnitID);
    Level* level = unit->level;
    Signal* signal = enteringWater ? level->startGoingIntoWaterSignal : level->startGoingOutOfWaterSignal;
    if (signal != nullptr) {
        SIGNAL_HandleSignal(instance, level, signal, 0);
    }
    if (instance == g_gameTracker.playerInstance) {
        unit->unitFlags |= enteringWater ? 0x0100 : 0x0200;
    }
}

} // namespace

// 0x00467250 (PDB: 0x00468c10)
void SIGNAL_HandleStreamLevelBaseCam(Instance* instance, Signal* signal)
{
    Level* currentLevel = STREAM_GetLevelWithID(instance->currentStreamUnitID);
    const int targetStreamID = currentLevel->terrain->streamUnitPortals[signal->streamPortalIndex].streamID;
    if (instance->currentStreamUnitID == targetStreamID) {
        return;
    }

    Level* targetLevel = STREAM_GetLevelWithID(targetStreamID);
    if (targetLevel != nullptr && targetLevel->pUnitData != nullptr) {
        dtp::UnitData::baseCamera& baseCam = targetLevel->pUnitData->BaseCamera;
        CameraStackSetBaseDTPID(
            static_cast<int>(baseCam.basecam),
            &baseCam.ledgeOverrides,
            baseCam.UseCameraStackSystem);
    } else {
        CameraStackSetBaseDTPID(0, nullptr, 0);
    }
}

// 0x004672d0 (PDB: 0x00468c90)
int SIGNAL_HandleStreamLevel(Instance* instance, Signal* signal)
{
    StreamUnit* currentUnit = STREAM_GetStreamUnitWithID(instance->currentStreamUnitID);
    int targetStreamID = currentUnit->level->terrain->streamUnitPortals[signal->streamPortalIndex].streamID;
    if (instance->currentStreamUnitID != targetStreamID && instance->LinkParent == nullptr) {
        Level* targetLevel = STREAM_GetLevelWithID(targetStreamID);
        if (targetLevel == nullptr) {
            const auto* player = static_cast<const Instance*>(g_gameTracker.playerInstance);
            if (instance != player && instance->LinkChild != player) {
                Level* sourceLevel = STREAM_GetLevelWithID(instance->currentStreamUnitID);
                for (Instance* cur = instance; cur != nullptr; cur = cur->LinkChild) {
                    if ((sourceLevel->unitFlags & 0x100000) == 0) {
                        SAVE_Instance(cur, sourceLevel);
                    }
                    cur->iflags |= 0x20;
                }
                return 1;
            }

            (void)TrackStream::GetActiveUnitId();
            (void)TrackStream::GetPreviousUnitId();

            StreamUnit* targetUnit = STREAM_GetStreamUnitWithName(signal->streamName);
            while (targetUnit == nullptr) {
                STREAM_PollLoadQueue();
                Sleep(5);
                targetUnit = STREAM_GetStreamUnitWithName(signal->streamName);
            }

            STREAM_WaitForUnit(targetUnit);
            targetStreamID = targetUnit->StreamUnitID;
            instance->cachedFace = nullptr;
            instance->cachedFaceTerrainGroup = nullptr;
            INSTANCE_SetCurrentStreamUnitID(instance, targetStreamID);
            targetLevel = STREAM_GetLevelWithID(targetStreamID);
        } else {
            instance->cachedFace = nullptr;
            instance->cachedFaceTerrainGroup = nullptr;
            INSTANCE_SetCurrentStreamUnitID(instance, targetStreamID);
        }

        if (instance == g_gameTracker.playerInstance && signal->FSFXInDTPID == 0) {
            ApplyDefaultUnitFSFX(targetLevel, targetStreamID, signal->FSFXInActiveDist);
        }
    }

    return 1;
}

// 0x00467490 (PDB: 0x00468e50)
void SIGNAL_HandleOutSignal(Instance* instance, Level* level, Signal* signal, int signalID)
{
    if (!IsPlayerOrLinkedToPlayer(instance)) {
        return;
    }

    // Retail checks bit 10 (byte[1] & 0x04) on exit
    if ((signal->flags & kSignalResetFSFXInBit) != 0) {
        ApplyDefaultUnitFSFX(level, level->streamUnitID, signal->FSFXInActiveDist);
    } else {
        TERRAIN_FSFXStack_Set_Primary(
            static_cast<uint32_t>(signal->FSFXExitDTPID),
            level->streamUnitID,
            static_cast<float>(signal->FSFXExitActiveDist));
    }

    if (signal->CameraStackType != 2 && signal->CameraStackType != 1) {
        const int slot = SignalCameraStackSlot(signal);
        CameraStackRemoveCameraDTPID(signal->DTPCameraID[slot], signalID, signal->CameraStackType);
        Signal* linked = signal->CameraLinkSignal;
        if (linked != nullptr) {
            const int linkedIdx = static_cast<int>(linked - level->SignalListStart);
            const auto* idList = static_cast<const int16_t*>(level->SignalIDList);
            const int linkedID = idList[linkedIdx];
            const int linkedSlot = SignalCameraStackSlot(linked);
            CameraStackRemoveCameraDTPID(linked->DTPCameraID[linkedSlot], linkedID, linked->CameraStackType);
        }
    }
}

// 0x004675a0 (PDB: 0x00468f60)
void SIGNAL_HandleSignal(Instance* instance, Level* level, Signal* signal, int signalID)
{
    if (IsPlayerOrLinkedToPlayer(instance)) {
        const int camModeSlot = static_cast<int>(theCamera.globalCameraMode) - 1;

        // Retail checks bit 9 (byte[1] & 0x02) on entry
        if ((signal->flags & kSignalResetFSFXOutBit) != 0) {
            ApplyDefaultUnitFSFX(level, level->streamUnitID, signal->FSFXInActiveDist);
        } else {
            TERRAIN_FSFXStack_Set_Primary(
                static_cast<uint32_t>(signal->FSFXInDTPID),
                level->streamUnitID,
                static_cast<float>(signal->FSFXInActiveDist));
        }

        if (signal->streamName[0] != '\0') {
            SIGNAL_HandleStreamLevelBaseCam(instance, signal);
        }

        if (signal->CameraStackType != 2 || (signal->CameraStackFlags & 0x0fu) == 0) {
            const int slot = SignalCameraStackSlot(signal);
            CameraStackAddCameraDTPID(
                signal->DTPCameraID[slot],
                signalID,
                signal->CameraStackType,
                signal->CameraStackSignalFlags[slot]);
            if (signal->CameraStackType == 2) {
                signal->CameraStackFlags = static_cast<uint8_t>((signal->CameraStackFlags & 0xf1u) | 0x01u);
            }
        }

        if (signal->CameraLock != 0) {
            CAMERA_Lock(&theCamera, signal->CameraLock);
        }
        if (signal->CameraRestore != 0) {
            CAMERA_Restore(&theCamera, signal->CameraRestore);
        }
        if (signal->CameraSave != 0) {
            CAMERA_Save(&theCamera, signal->CameraSave);
        }
        if (signal->CameraShakeScale != 0 || signal->CameraShakeTime != 0) {
            CAMERA_SetShake(&theCamera, signal->CameraShakeTime, signal->CameraShakeScale);
        }

        SplineCameraLevelData* splineData = signal->splineCamDataPtr[camModeSlot];
        if (splineData == nullptr) {
            if (camModeSlot == 0 || signal->splineCamDataPtr[0] == nullptr) {
                splineData = &signal->splineCamParams;
            } else {
                splineData = signal->splineCamDataPtr[0];
            }
        }

        int newCamMode = -1;
        if (CAMERA_ProcessSplineCamLevelData(&theCamera, splineData) != 0) {
            newCamMode = splineData->CameraMode;
        } else {
            emergencyLoadingCamDataPtr = splineData;
        }

        if (theCamera.nextCam.cutFlag != 0) {
            theCamera.nextCam.followCamSmooth = 0.0f;
            theCamera.smooth = 0.0f;
        } else if (splineData->CameraFollowSmooth != 0) {
            CAMERA_SetSmoothValue(&theCamera, splineData->CameraFollowSmooth);
        }

        if (splineData->CameraFollowRotation != 0) {
            CAMERA_Adjust_rotation(&theCamera, SignalDegreesToCameraUnits(splineData->CameraFollowRotation));
        }
        if (splineData->CameraZOffset != 0) {
            CAMERA_Adjust_ZOffset(&theCamera, splineData->CameraZOffset);
        }
        if (splineData->CameraFollowTilt != 0) {
            CAMERA_Adjust_tilt(&theCamera, SignalDegreesToCameraUnits(splineData->CameraFollowTilt));
        }
        if (signal->CameraUnlock != 0) {
            CAMERA_Unlock(&theCamera, signal->CameraUnlock);
        }

        if (newCamMode != -1) {
            auto* player = static_cast<Instance*>(g_gameTracker.playerInstance);
            if ((theCamera.flags & 0x10000) == 0 &&
                (theCamera.mode != 4 || INSTANCE_Query(player, 0x2c) == 0)) {
                emergencyLoadingCamDataPtr = nullptr;
                CAMERA_SetMode(&theCamera, newCamMode);
            } else {
                theCamera.nextCamDeferred = theCamera.nextCam;
                theCamera.deferredNextCamMode = static_cast<int8_t>(newCamMode);
                theCamera.nextCam.cutFlag = 1;
                CAMERA_ClearNextCamData(&theCamera.nextCam);
            }
        }

        if ((signal->flags & kSignalResetSlideBit) != 0) {
            INSTANCE_Post(instance, 0x4000006, 0);
        }
        if (signal->ReverbType != -1) {
            SOUND_SetSFXReverb(signal->ReverbType, signal->ReverbVolume);
        }
        if (signal->setMusicVar != 0) {
            SRMUSIC_SetMusicVariable(signal->setMusicVar, signal->setMusicVarValue);
        }
        if (signal->SetSlideAngle != 0) {
            INSTANCE_Post(instance, 0x4000005, signal->SetSlideAngle);
        }

        for (AttackWaveRuntime* wave = signal->enableAttackWaves; wave != nullptr; wave = wave->nextEnableAttackWave) {
            STREAM_SignalTurnOnAttackWave(wave);
        }
        for (AttackWaveRuntime* wave = signal->disableAttackWaves; wave != nullptr; wave = wave->nextDisableAttackWave) {
            STREAM_SignalTurnOffAttackWave(wave);
        }
        for (AttackWaveRuntime* wave = signal->killAttackWaves; wave != nullptr; wave = wave->nextKillAttackWave) {
            STREAM_SignalKillAttackWaveMonsters(wave);
        }
    }

    if (signal->streamName[0] != '\0') {
        SIGNAL_HandleStreamLevel(instance, signal);
    }
    if ((signal->flags & kSignalObjectHitBit) != 0) {
        INSTANCE_Post(instance, 0x200008, 0);
    }
    if (signal->fatalType != 0 && instance->object != nullptr && (instance->object->oflags2 & 0x100000) != 0) {
        INSTANCE_Post(instance, 0x200001a, 0);
    }
}

// 0x004679d0 (PDB: 0x00469390)
int SIGNAL_IsStreamSignal(Signal* signal)
{
    return signal->streamName[0] != '\0' ? 1 : 0;
}

// 0x004679e0 (PDB: 0x004693a0)
Signal* SIGNAL_FindSignal(Level* level, int id)
{
    const auto* idList = static_cast<const int16_t*>(level->SignalIDList);
    Signal* signal = level->SignalListStart;
    while (*idList != 0) {
        if (*idList == id) {
            return signal;
        }
        ++idList;
        ++signal;
    }
    return nullptr;
}

// 0x00467a20 (PDB: 0x004693e0)
Signal* SIGNAL_FindSignalStream(Level* level, int id)
{
    return FindSignalWithWarning(level, id, "Can't find signal %d for streaming, unit %s\n");
}

// 0x00467a60 (PDB: 0x00469420)
Signal* SIGNAL_FindSignalEvent(Level* level, int id)
{
    return FindSignalWithWarning(level, id, "Can't find signal %d for event code, unit %s\n");
}

// 0x00467aa0 (PDB: 0x00469460)
void SIGNAL_OutOfWater(Instance* instance)
{
    ApplyWaterTransitionSignal(instance, false);
}

// 0x00467af0 (PDB: 0x004694b0)
void SIGNAL_InWater(Instance* instance)
{
    ApplyWaterTransitionSignal(instance, true);
}

// 0x00467b40 (PDB: 0x00469500)
void SIGNAL_ClearResetSignals(void)
{
    std::memset(ResetSignalArray, 0, sizeof(ResetSignalArray));
    NumSignalsToReset = 0;
}

// 0x00467b60 (PDB: 0x00469520)
void SIGNAL_ResetSignals(void)
{
    for (int i = 0; i < kMaxSignalsToReset; ++i) {
        if (NumSignalsToReset <= 0) {
            break;
        }
        if (ResetSignalArray[i].timeLeft > 0) {
            --ResetSignalArray[i].timeLeft;
            if (ResetSignalArray[i].timeLeft == 0) {
                ResetSignalArray[i].mSignal->flags &= ~kSignalTriggeredBit;
                --NumSignalsToReset;
            }
        }
    }
}

// 0x00467ba0 (PDB: 0x00469560)
void SIGNAL_AddSignalToReset(Signal* signal)
{
    if (NumSignalsToReset >= kMaxSignalsToReset) {
        return;
    }

    for (int i = 0; i < kMaxSignalsToReset; ++i) {
        if (ResetSignalArray[i].timeLeft == 0) {
            ResetSignalArray[i].timeLeft = 1;
            ResetSignalArray[i].mSignal = signal;
            ++NumSignalsToReset;
            return;
        }
    }
}

// 0x00467be0 (PDB: 0x004695a0)
void SIGNAL_FixStreamSignal(Level* level)
{
    Terrain* terrain = level->terrain;
    for (int i = 0; i < terrain->numStreamUnitPortals; ++i) {
        Signal* signal = SIGNAL_FindSignalStream(level, terrain->streamUnitPortals[i].MSignalID);
        if (signal != nullptr) {
            signal->streamPortalIndex = i;
        }
    }
}
